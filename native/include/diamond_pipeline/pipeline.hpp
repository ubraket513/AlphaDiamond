// Native operation contracts, not another coordinator. Production train_main
// calls self-play, ingest and train separately; the benchmark can use run_iteration.
// Pool/store/trainer arguments are borrowed for the synchronous call. Callers must
// serialize mutations of those objects and keep them alive until the call returns.
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "diamond_pipeline/model_pool.hpp"
#include "diamond_pipeline/replay_store.hpp"
#include "diamond_training/trainer.hpp"
#include "soo/search_target_metrics.hpp"
#include "soo/selfplay.hpp"

namespace diamond_pipeline {

// Carries one operation's explicit inputs. jobs own initial states/seeds; model_key
// names the actor snapshot already installed in the borrowed pool. Batch size is
// samples per learner step, training_steps is update count, iteration fixes replay
// sampling seeds. checkpoint_root is used only by the convenience run_iteration.
struct IterationRequest {
    std::string operation_id;
    ModelKey model_key;
    Compatibility compatibility;
    soo::Match match;
    std::vector<soo::EpisodeJob> jobs;
    soo::EpisodeConfig selfplay;
    std::size_t training_batch_size = 1;
    std::size_t training_steps = 0;
    // Iteration index, which together with the store's replay seed and the
    // local step fixes every minibatch this iteration will draw.
    uint64_t iteration = 0;
    std::optional<std::filesystem::path> checkpoint_root;
};

struct IterationResult {
    std::string operation_id;
    std::size_t completed_games = 0;
    std::size_t aborted_games = 0;
    std::size_t new_samples = 0;
    std::size_t requested_training_steps = 0;
    std::size_t completed_training_steps = 0;
    std::vector<std::size_t> training_batch_sizes;
    std::vector<diamond_training::TrainingMetrics> training_metrics;
    std::size_t replay_size = 0;
    uint64_t training_step = 0;
};

// Engine-side counters for one self-play stage. These live only for the
// duration of the run -- the stage report is rebuilt from the reloaded episode
// artifact so that it survives `resume`, and episodes carry no scheduler state
// -- so they are written to a sidecar at the moment self-play executes. Without
// that, `boosted_moves` in particular is computed and dropped, leaving no way to
// tell a repetition trigger that was configured and never needed from one that
// was configured and silently did nothing.
struct SelfPlayMetrics {
    uint64_t evaluations = 0;
    uint64_t batches = 0;
    uint64_t moves = 0;
    uint64_t boosted_moves = 0;
    double boosted_fraction = 0.0;
    double wall_seconds = 0.0;
    double evaluator_seconds = 0.0;
    double worker_busy_seconds = 0.0;
    double evaluator_busy_fraction = 0.0;
    double batch_mean = 0.0;
    uint32_t batch_p50 = 0;
    uint32_t batch_p90 = 0;
    uint32_t batch_max = 0;
    // Move counts of the *completed* games only. Completion rate and game
    // length degrade together when the network itself is degrading, so the two
    // read side by side distinguish that from a healthy network with an
    // inflated pathological tail -- which look identical in the abort rate
    // alone. Aborted games are excluded because they are all exactly max_moves
    // and would only dilute the percentile.
    // Non-terminating tail diagnosis: how many aborted games ended with at
    // least one foreign piece in some seat's target camp, which makes that camp
    // unfillable and the game unwinnable for its owner. Compared against the
    // same count over completed games, which is the control.
    uint64_t aborted_with_blocked_camp = 0;
    uint64_t completed_with_blocked_camp = 0;
    uint64_t aborted_blocked_cells_total = 0;

    uint64_t completed_moves_p50 = 0;
    uint64_t completed_moves_p90 = 0;
    uint64_t completed_moves_p99 = 0;
    uint64_t completed_moves_max = 0;

    soo::VisitTargetSummary all_targets;
    soo::VisitTargetSummary completed_targets;
    soo::VisitTargetSummary aborted_targets;
};

// One aborted game, with what it takes to tell a retained block from a
// transient one. Written to a diagnostic sidecar, never to the replay.
struct AbortedGameDiagnostics {
    std::string game_id;
    uint64_t seed = 0;
    uint64_t move_count = 0;
    std::string abort_reason;
    soo::EpisodeDiagnostics state;
};

// Owns completed and aborted episode records. Only completed episodes contain
// learning samples; abort diagnostics are kept separately and never relabeled as
// terminal targets. Metrics are observations of this invocation, not durable state.
struct SelfPlayResult {
    std::string operation_id;
    std::size_t completed_games = 0;
    std::size_t aborted_games = 0;
    std::size_t new_samples = 0;
    SelfPlayMetrics metrics;
    std::vector<AbortedGameDiagnostics> aborted_diagnostics;
    std::vector<Episode> episodes;
};

struct TrainingResult {
    std::string operation_id;
    std::size_t requested_training_steps = 0;
    std::size_t completed_training_steps = 0;
    std::vector<std::size_t> training_batch_sizes;
    std::vector<diamond_training::TrainingMetrics> training_metrics;
    std::size_t replay_size = 0;
    uint64_t training_step = 0;
    // Time spent inside ReplayStore::sample() across the stage, and the worst
    // single draw. Sampling is pure memory work now, so this is the number that
    // says whether it has stayed that way as the pool grows -- a replay whose
    // sampling cost rose with capacity would show up here first.
    double replay_sample_seconds = 0.0;
    double replay_sample_max_seconds = 0.0;
};

// Durable self-play hand-off used between the coordinator's SELF_PLAY and
// REPLAY_INGEST callbacks.  It contains only native pipeline records.
void save_episode_artifact(const std::filesystem::path& path, std::string_view operation_id,
                           std::span<const Episode> episodes);
std::vector<Episode> load_episode_artifact(const std::filesystem::path& path,
                                           std::string_view operation_id,
                                           const Compatibility& compatibility);

// Enforces actor identity; returns owned samples without ingesting them. The stop
// token is not an immediate interrupt of run_episodes; scheduler limits govern the
// in-flight games. CLI/Panel immediate Stop terminates the process externally.
SelfPlayResult run_self_play(const IterationRequest& request, ModelPool& models,
                             std::stop_token stop = {});
// Writes replay chunks/manifest through ReplayStore's existing ingestion contract.
// Kept separate so the production coordinator can persist the self-play artifact
// before ingestion and distinguish that side effect from learner updates.
ReplayIngestReport ingest_self_play(ReplayStore& replay, std::span<const Episode> episodes);
TrainingResult train_replay(const IterationRequest& request, ReplayStore& replay,
                            diamond_training::Trainer& trainer, std::stop_token stop = {});

// Sequential convenience for benchmark callers. It is not transactional: ingested
// replay and completed optimizer updates survive a later failure. Optional output
// uses the v2 checkpoint writer; production publishes v3 provenance explicitly.
IterationResult run_iteration(const IterationRequest& request, ModelPool& models,
                              ReplayStore& replay, diamond_training::Trainer& trainer,
                              std::stop_token stop);

}  // namespace diamond_pipeline
