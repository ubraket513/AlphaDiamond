// Native game production and explicit scheduler measurements over a fixed
// search-worker pool and one evaluator thread per run. The finite episode path
// consumes caller-owned jobs; the timed harness recycles games until its window
// ends. The caller retains evaluator/model ownership throughout the call.
//
// The architectural criterion this exists to satisfy: a CPU worker must never
// sleep alongside a game that is waiting on the evaluator. Synchronous MCTS
// allows one outstanding request per lane, so if a worker blocked on its own
// evaluation the achievable batch size would be capped by the thread count and
// "many logical games" would buy nothing. Workers therefore hand a suspended
// lane to the batcher and immediately look for another runnable one.
#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "soo/encoder.hpp"
#include "soo/evaluator.hpp"
#include "soo/mcts.hpp"
#include "soo/state.hpp"

namespace soo {

enum class MinTurnOrderGroup;

// One lane's pending request, as the evaluator sees it.
//
// Pointers into lane storage, not copies: the lane owns the encoded features
// and the legal set, and both stay alive for the whole dispatch.
struct BatchItem {
    const State* state = nullptr;
    const Encoded* encoded = nullptr;
    const std::vector<int32_t>* actions = nullptr;
    uint64_t salt = 0;
    EvalOutcome* outcome = nullptr;   // to be filled
    // How many value components the evaluator must return: 1 for Soo, 3 for
    // Min. An evaluator that ignores this fills `value` and leaves a Min lane
    // reading zeros for two of its seats.
    int value_width = 1;
    // Which job in this run the request belongs to, or -1 when the caller does
    // not track jobs. Lanes take the next unstarted job rather than owning one,
    // so an evaluator that must treat games differently -- the arena router,
    // which sends the candidate's turns to a different model in every game --
    // cannot key on the lane or on the outcome pointer.
    int job = -1;
    // The actual player choosing a move. Unlike state->current_player, this
    // stays fixed while a search evaluates descendants belonging to other seats.
    uint8_t root_player = 0;
    // The complete game geometry and turn order, including finished players.
    // Like the request's other pointers, this stays alive for the dispatch.
    const Match* match = nullptr;
};

// Evaluator interface borrowed by a run. evaluate() is serialized on that run's
// evaluator thread; prepare() is called concurrently on search workers. This
// does not make a shared evaluator instance safe across simultaneous runs or
// permit unsynchronized mutable state shared by prepare/evaluate.
class BatchEvaluator {
  public:
    virtual ~BatchEvaluator() = default;

    // Called on a SEARCH WORKER, before the lane is handed to the batcher.
    //
    // Request-local CPU preparation can run here without blocking the single
    // evaluator thread. Each worker owns its lane's pending request until it
    // submits it; shared evaluator state still needs its own synchronization.
    virtual void prepare(BatchItem& item) { (void)item; }

    // Called on the evaluator thread, one batch at a time.
    virtual void evaluate(std::vector<BatchItem>& batch) = 0;
};

// Gate C's evaluator: request-dependent, per-lane salted, artificial latency.
class DummyBatchEvaluator : public BatchEvaluator {
  public:
    explicit DummyBatchEvaluator(double latency_ms) : latency_ms_(latency_ms) {}
    void evaluate(std::vector<BatchItem>& batch) override;

  private:
    double latency_ms_;
};

struct SchedulerConfig {
    // Wall-clock throughput harness, not a finite production episode queue.
    int games = 64;             // logical games in flight
    int threads = 4;            // CPU search workers; not Torch intra-op threads
    int max_batch = 32;         // evaluator positions per call, at most
    int max_wait_us = 2000;     // collection wait in microseconds
    int simulations = 64;
    // Same positive finite PUCT coefficient as the episode path.
    double c_puct = 1.5;
    int max_moves = 400;
    double eval_latency_ms = 0.0;   // artificial per-batch evaluator cost
    double seconds = 5.0;           // measurement window
    bool trace_moves = false;       // record each lane's move sequence
    int stop_after_moves = 0;       // per lane; 0 = run for the full window

    // Root noise and move sampling are independent of the PUCT coefficient.
    // Their zero settings keep the deterministic baseline, including RNG use.
    double temperature = 0.0;       // move sampling, for the first N moves
    int temperature_moves = 0;
    double dirichlet_alpha = 0.3;
    double dirichlet_epsilon = 0.0;
    uint64_t seed = 0;              // base seed; each lane derives its own
};

// --------------------------------------------------------------------------
// Episode production: the same scheduler, driven to produce training data.
//
// run_scheduler is a measurement harness -- it runs for a wall-clock window and
// recycles a lane the moment its game ends.  Training needs the opposite: a
// fixed set of games, each played to its own terminal state, with every move
// recorded.  Same batching architecture, different stop condition and an
// output that is samples rather than timings.
// --------------------------------------------------------------------------

struct EpisodeJob {
    State initial_state;
    uint64_t seed = 0;      // caller-derived per-game seed, belonging to the job, not its lane
    std::optional<Match> match; // Per-game turn order; absent keeps the batch default.
};

// Assign every supported permutation equally, preserving camps and seeds.
// A selected Min group has three orders; otherwise use all orders. Opening jobs only.
void balance_episode_turn_orders(const Match& match, std::vector<EpisodeJob>& jobs,
                                std::optional<MinTurnOrderGroup> group = std::nullopt);

// One recorded move.  The features are the *request* the evaluator answered, so
// a sample is exactly what the network saw, not a re-encoding of it.
struct EpisodeMove {
    Encoded features;
    std::vector<int32_t> root_actions;    // canonical ids, authoritative order
    std::vector<uint32_t> visit_counts;   // aligned; the policy target
    int32_t selected_action = 0;          // canonical
};

// Per-seat state of a target camp at the moment a game stopped.
struct CampDiagnostics {
    uint8_t player_id = 0;
    uint8_t target_camp = 0;
    uint8_t own_in_target = 0;      // cells of the camp holding the owner's piece
    uint8_t foreign_in_target = 0;  // cells holding somebody else's piece
    uint8_t empty_in_target = 0;    // cells holding nothing
    // Board cells of the camp occupied by a foreign piece, and how many legal
    // moves each of those pieces had. A blocker with zero legal moves is stuck;
    // a blocker with legal moves that stayed put is being retained.
    std::vector<uint8_t> blocker_cells;
    std::vector<uint8_t> blocker_owners;
    std::vector<uint16_t> blocker_legal_moves;
    // Plies since any cell of this camp last changed hands. A large value with
    // a mobile blocker is the signature of a held block.
    uint32_t plies_since_camp_changed = 0;
};

struct EpisodeDiagnostics {
    uint8_t current_player = 0;
    std::vector<uint8_t> occupancy;
    std::vector<CampDiagnostics> camps;  // indexed by seat
    // Position repetition over the whole game, keyed on dynamics_key: the
    // physical state, deliberately excluding turn_number.
    uint32_t unique_positions = 0;
    uint32_t max_revisits = 0;
    // How many positions were observed, which is `move_count + 1`: the opening
    // is recorded when the lane is seated, then once after every move. Carried
    // explicitly because it is the denominator `unique_positions` belongs over,
    // and dividing by `move_count` instead reports 1 + 1/move_count for a game
    // that never repeated anything -- a ratio above 1.0, which reads like an
    // error and hides how much headroom is left.
    uint32_t observations = 0;
    // Observations whose position had already occurred within the previous 8
    // plies. `max_revisits` says one position recurred often; it cannot tell a
    // tight A-B-A cycle from a slow return, and the tight cycle is what the
    // aborted tail is made of.
    uint32_t repeat_within_8 = 0;
    // The tail of the game, newest last, so a short cycle is visible directly.
    std::vector<uint64_t> recent_keys;
};

struct Episode {
    std::vector<EpisodeMove> moves;
    std::vector<uint8_t> finish_order;
    int move_count = 0;
    bool completed = false;
    // Set on a move-limit abort. Moves remain available for diagnostics;
    // production replay ingestion is responsible for rejecting incomplete
    // episodes rather than treating this partial trajectory as training data.
    bool move_limit_exceeded = false;
    // Set when the game's own monotonic deadline elapsed. This is deliberately
    // distinct from the move limit and caller cancellation.
    bool max_game_seconds_exceeded = false;
    // Why a game did not terminate.
    //
    // A seat finishes only when every cell of its target camp holds its own
    // piece, so a foreign piece resting there blocks that camp. That alone
    // proves nothing -- a blocker is free to leave -- so what has to be
    // distinguished is a transient occupation from a *retained* one: a blocker
    // that can move, does not, while the position cycles. These fields carry
    // exactly what that classification needs, and are filled for completed
    // games too, since the blocked counts only mean something against that
    // baseline.
    EpisodeDiagnostics diagnostics;
};

struct EpisodeConfig {
    // Runtime inputs shared by training, arena and explicit benchmarks. Jobs
    // beyond this concurrency bound wait for a lane; finishing a game refills
    // that lane from the queue. Zero keeps the existing automatic choice
    // min(jobs, 2 * max_batch). The actual count never exceeds available jobs.
    int lanes = 0;
    // CPU search workers, independent of logical games and Torch intra-op
    // threads. Evaluation runs on a separate thread shared by all lanes.
    int threads = 4;
    // Maximum evaluator positions per dispatch and collection wait in us.
    // These do not control the learner's samples per optimizer update.
    int max_batch = 32;
    int max_wait_us = 2000;
    int simulations = 64;
    // Positive finite PUCT coefficient forwarded to the 2P/3P search sessions.
    // Keeping 1.5 as the default preserves existing callers' search behavior.
    double c_puct = 1.5;
    int max_moves = 2000;
    // Per-game monotonic budget. Absent or zero disables it; expiry is recorded
    // separately from a move-limit abort and from an execution failure.
    std::optional<std::chrono::steady_clock::duration> max_game_duration;
    double temperature = 1.0;
    int temperature_moves = 20;
    double dirichlet_alpha = 0.3;
    double dirichlet_epsilon = 0.25;

    // Alternate search budget, not a move-sampling temperature. Zero disables
    // adaptation. With a positive repeat_window the recent-position trigger
    // takes precedence; otherwise late_move_threshold is the low-level trigger.
    // Production training validates its paired window/budget fields separately.
    int simulations_late = 0;
    int late_move_threshold = 0;

    // History of dynamics positions: occupancy, side to move and finish state,
    // excluding the bookkeeping turn counter. Budget adaptation and move
    // sampling can independently consult it. In particular arena deliberately
    // uses window 8 and simulations_late 0, so a window alone is not a request
    // for deeper search.
    int repeat_window = 0;

    // On a repeated position only, replace the normal move-temperature schedule
    // with this value. Zero retains that schedule without disabling an adaptive
    // search budget. Arena uses 1.0 here; training defaults to zero and can
    // explicitly enable it through self_play.repetition_temperature.
    // Sampling is seeded by game and move, not by worker scheduling order.
    double repetition_temperature = 0.0;

    // Blend the canonical vacancy heuristic into legal network priors while
    // retaining network values. Weight 1 replaces the network policy; weight 0
    // leaves it unchanged. This is a declared bootstrap phase, not a fallback
    // after a model failure. Geometry is canonicalized for the acting seat.
    bool bootstrap_prior = false;
    double bootstrap_prior_weight = 1.0;
    EncodingVersion encoding = EncodingVersion::legacy_v1;
};

std::vector<double> blend_legal_priors(const std::vector<double>& vacancy,
                                       const std::vector<double>& network, double weight);

struct EpisodeMetrics {
    uint64_t evaluations = 0;
    uint64_t batches = 0;
    uint64_t moves = 0;
    // How often the boosted budget fired. The trigger only earns its complexity
    // if this stays small, so it is reported rather than inferred.
    uint64_t boosted_moves = 0;
    // How often repetition changed the move-selection temperature.
    uint64_t repetition_moves = 0;
    double wall_seconds = 0.0;
    // Host wall time spent inside evaluator calls, not sampled GPU utilization.
    double evaluator_seconds = 0.0;
    double worker_busy_seconds = 0.0; // summed across search workers, not wall time
    std::vector<uint32_t> batch_sizes;
};

// Consume finite jobs and return one episode per input in job order, including
// explicit move/deadline aborts. The evaluator is borrowed until all threads
// join; individual request pointers are borrowed only for their dispatch.
// Worker/evaluator failures stop the run and propagate after joining; they are
// not converted into completed games. Empty jobs immediately return no episodes.
std::vector<Episode> run_episodes(const Match& match, const std::vector<EpisodeJob>& jobs,
                                  const EpisodeConfig& config, BatchEvaluator& batch_evaluator,
                                  EpisodeMetrics& metrics);

struct SchedulerMetrics {
    uint64_t evaluations = 0;
    uint64_t batches = 0;
    uint64_t moves = 0;
    uint64_t games_finished = 0;
    uint64_t batcher_wakeups = 0;
    double wall_seconds = 0.0;
    double worker_busy_seconds = 0.0;   // summed across workers
    double evaluator_seconds = 0.0;

    std::vector<uint32_t> batch_sizes;
    std::vector<uint32_t> ready_depth;   // runnable games, sampled per dispatch
    std::vector<uint32_t> waiting;       // games waiting on the evaluator
    std::vector<uint64_t> wait_ns;       // per-lane submit -> result latency

    // Per-lane move sequences, when config.trace_moves is set.
    //
    // A lane's evaluations depend only on its own request and its own salt, so
    // its trajectory must not depend on how many workers ran it, how batches
    // happened to form, or in what order lanes were scheduled. Comparing these
    // across thread counts is how the scheduler is proven free of cross-lane
    // contamination.
    std::vector<std::vector<int32_t>> lane_moves;
};

// Run the timed recycling harness for config.seconds (or its existing trace
// stop condition). This returns workload measurements, not training episodes;
// evaluator/worker exceptions propagate after the run's threads are joined.
SchedulerMetrics run_scheduler(const Match& match, const State& opening,
                               const SchedulerConfig& config, BatchEvaluator& evaluator);

}  // namespace soo
