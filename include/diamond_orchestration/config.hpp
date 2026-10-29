#pragma once

// Serializable production settings consumed by the orchestration adapters.
// Parsing/validation and canonical serialization live in config.cpp; worker
// creation, live batching and model ownership belong to their runtime modules.
// A stored resolved config is checkpoint evidence, so legacy field presence
// and supplied values must survive loading even when no runtime consumes them.

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "diamond_support/json.hpp"

namespace diamond_orchestration {

class ConfigError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

inline constexpr std::string_view kBootstrapPriorNone = "none";
inline constexpr std::string_view kCanonicalTargetDistanceV1 = "canonical-target-distance-v1";
inline constexpr std::string_view kCanonicalTargetVacancyDistanceV2 =
    "canonical-target-vacancy-distance-v2";
inline constexpr std::string_view kOpeningBlockBootstrapV1 = "opening-block-bootstrap-v1";
inline constexpr std::string_view kOpeningBlockResamplingUnit = "opening_block";

struct NetworkConfig final {
    int64_t width = 128;
    int64_t residual_blocks = 6;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static NetworkConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const NetworkConfig&) const = default;
};

struct RuntimeConfig final {
    std::string device = "cpu";
    // Actor inference precision; this is not a CPU search-thread setting or a
    // complete description of process-wide LibTorch/TF32 environment settings.
    std::string precision = "fp32";

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static RuntimeConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const RuntimeConfig&) const = default;
};

struct MCTSConfig final {
    int64_t simulations = 200;
    // Positive finite PUCT coefficient, forwarded to both training and arena
    // search. Distinct from root noise and move temperature; 1.5 is historical.
    double c_puct = 1.5;
    double dirichlet_alpha = 0.3;
    double dirichlet_epsilon = 0.25;
    // Historical JSON "seed" never controlled production search. Retain only
    // its supplied value/presence so old resolved-config hashes still match;
    // new configs omit it. Per-game search RNG comes from RunState::derive_seed.
    std::optional<uint64_t> legacy_seed;

    // Training-only adaptive depth: replace the base simulation budget when a
    // dynamics position recurs within this many plies. Both controls must be
    // positive, or both zero to disable the trigger. This does not configure
    // arena's independent eight-ply, temperature-1 repetition escape policy.
    int64_t simulations_late = 0;
    int64_t repeat_window = 0;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static MCTSConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const MCTSConfig&) const = default;
};

struct SelfPlayConfig final {
    int64_t max_moves = 2000;
    int64_t temperature_moves = 20;
    double temperature = 1.0;
    // Override move sampling only when a dynamics position repeats within
    // mcts.repeat_window plies. Zero preserves the normal temperature schedule.
    double repetition_temperature = 0.0;
    // Historical JSON "seed" is compatibility metadata, not a second training
    // RNG. Production derives game seeds from run_seed and job identifiers.
    // The standalone benchmark now uses run_seed, or an explicit --seed.
    std::optional<uint64_t> legacy_seed;
    std::string bootstrap_prior = std::string(kBootstrapPriorNone);
    double bootstrap_prior_weight = 1.0;
    std::optional<double> max_game_seconds;
    // Balance jobs over all turn orders or one declared Min specialist group;
    // these settings select training data, not evaluator model ownership.
    bool balance_turn_orders = false;
    std::string turn_order_group = "all";
    // Positive integer ratios in lexicographic player-ID order after group
    // filtering. Empty keeps equal allocation and old configuration hashes.
    // Min reverse: [1-3-2, 2-1-3, 3-2-1]; [1,2,1] requests 24/48/24 of 96 games.
    std::vector<int64_t> turn_order_weights;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static SelfPlayConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const SelfPlayConfig&) const = default;
};

struct WorkerConfig final {
    // Concurrent game states, not threads. Pending jobs refill retired lanes.
    int64_t logical_lanes = 1;
    // CPU search workers, independent of LibTorch intra-op threads. The
    // scheduler also has one evaluator thread; this does not promise full CPU
    // utilization or bind workers to physical cores.
    int64_t search_threads = 1;
    // Two, not one: games must exceed lanes or the job queue never engages, and
    // a default configuration has to be a valid one.
    int64_t games_per_iteration = 2;
    std::string retry_id = "attempt-0";

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static WorkerConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const WorkerConfig&) const = default;
};

struct InferenceConfig final {
    // Positions per evaluator call and batch collection wait in microseconds.
    // The batch size is a cap, not a promise of a full batch or a learner batch.
    int64_t max_batch_size = 1;
    int64_t max_wait_us = 1;
    // Legacy serialized metadata only: the current in-process scheduler does
    // not use these as a live queue limit or response deadline. Preserve each
    // supplied value and its presence for old resolved-config identity; omit
    // absent fields in new configs. Resume transitions cannot edit either.
    // Optional means an absent JSON key, not permission to accept JSON null.
    std::optional<int64_t> request_queue_capacity;
    std::optional<double> response_timeout_s;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static InferenceConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const InferenceConfig&) const = default;
};

struct ReplayConfig final {
    int64_t capacity = 100000;
    uint64_t seed = 0;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static ReplayConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const ReplayConfig&) const = default;
};

struct TrainingConfig final {
    // Samples per optimizer update, independent of inference.max_batch_size.
    int64_t batch_size = 128;
    int64_t train_steps_per_iteration = 1;
    double learning_rate = 1e-3;
    double weight_decay = 1e-4;
    uint64_t seed = 0;
    std::string policy_loss_domain = "full";

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static TrainingConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const TrainingConfig&) const = default;
};

struct RunBudgetConfig final {
    std::optional<int64_t> max_iterations = 1;
    std::optional<double> max_wall_clock_seconds;
    // Every training iteration publishes a checkpoint for the next pipeline
    // stages and restart recovery, so only 1 is supported. Keep this serialized
    // field to preserve canonical configuration bytes for supported configs.
    int64_t checkpoint_every_iterations = 1;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static RunBudgetConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const RunBudgetConfig&) const = default;
};

struct ArenaConfig final {
    // Promotion scheduling and decision inputs. Effective episode search is
    // assembled by wire_arena_episode: noise-free/greedy except for its fixed
    // seeded repetition escape. It does not inherit self-play temperature.
    bool enabled = true;
    int64_t games = 36;
    uint64_t seed = 0;
    int64_t max_moves = 2000;
    double promotion_threshold = 0.55;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static ArenaConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const ArenaConfig&) const = default;
};

struct OpeningSuiteConfig final {
    std::string id = "production-openings-v1";
    int64_t version = 1;
    uint64_t seed = 0;
    int64_t count = 1;
    int64_t max_depth = 0;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static OpeningSuiteConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const OpeningSuiteConfig&) const = default;
};

struct PromotionStatisticsConfig final {
    std::string method = std::string(kOpeningBlockBootstrapV1);
    std::string resampling_unit = std::string(kOpeningBlockResamplingUnit);
    double confidence_level = 0.95;
    int64_t bootstrap_replicates = 10000;
    uint64_t seed = 0;

    void validate() const;
    diamond_support::JsonValue to_json() const;
    static PromotionStatisticsConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const PromotionStatisticsConfig&) const = default;
};

struct ProductionConfig final {
    int64_t schema_version = 2;
    std::string model_name = "Soo";
    std::string model_version = "2.0.0";
    NetworkConfig network;
    RuntimeConfig runtime;
    MCTSConfig mcts;
    SelfPlayConfig self_play;
    WorkerConfig workers;
    InferenceConfig inference;
    ReplayConfig replay;
    TrainingConfig training;
    RunBudgetConfig run_budget;
    ArenaConfig arena{.games = 40};
    OpeningSuiteConfig opening_suite;
    PromotionStatisticsConfig promotion_statistics;
    uint64_t run_seed = 0;
    // Absent in historical resolved configs; preserve their serialized identity.
    std::string encoder_version = "diamond-camp-relative-v1";

    // Cross-section checks supplement each group's local range/type checks.
    // Equality includes optional legacy presence; no runtime normalization may
    // erase that distinction when comparing checkpoint-boundary transitions.
    void validate() const;
    diamond_support::JsonValue to_json() const;
    static ProductionConfig from_json(const diamond_support::JsonValue& value);
    bool operator==(const ProductionConfig&) const = default;
};

} // namespace diamond_orchestration
