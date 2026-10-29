#pragma once

// Stateless adapters from validated production settings to existing runtime
// inputs. They own no model, start no threads and write no run/checkpoint data.
// Keeping this boundary explicit separates algorithm policy from resource
// counts without introducing another scheduler or hardware-selection layer.

#include <cstddef>
#include <string>
#include <vector>

#include "diamond_orchestration/config.hpp"
#include "soo/selfplay.hpp"

namespace diamond_orchestration {

struct TrainingIterationWiring final {
    soo::EpisodeConfig selfplay;
    // Total jobs for the iteration, not the concurrently active lane count.
    std::size_t games_per_iteration = 0;
    // Learner samples/update and number of updates, not evaluator batching.
    std::size_t training_batch_size = 0;
    std::size_t training_steps = 0;
    std::string policy_loss_domain = "full";
};

struct EvaluationPipelineWiring final {
    bool run_arena = true;
    bool record_rating = true;
    bool activate_candidate = false;
};

// Validate and translate declared training controls without hardware tuning.
// Legacy inference queue/timeout metadata has no effective runtime mapping.
TrainingIterationWiring wire_training_iteration(const ProductionConfig& config);
// Arena-disabled training still activates its candidate, without rating it.
EvaluationPipelineWiring wire_evaluation_pipeline(const ProductionConfig& config);

// The arena's episode config for a group of `lanes` games played together.
//
// Separate from the iteration wiring because its lane count is not a setting:
// it is the size of the group of games sharing a turn order that the promotion
// arena is about to play, and the search threads and batch are bounded by it.
// Normal moves are greedy and take no root noise. A matching dynamics position
// within eight plies activates seeded temperature-1 move sampling; the training
// repetition-depth trigger is not inherited. Throws on an empty game group.
soo::EpisodeConfig wire_arena_episode(const ProductionConfig& config, std::size_t lanes);

// Validate an explicit checkpoint-boundary transition. The implementation's
// allowlist covers precision, worker/batching counts, training step count and
// policy-loss domain, bootstrap phase, self-play order group and arena enablement. Other fields,
// including architecture, replay identity, MCTS semantics and legacy queue/
// timeout metadata (presence and values), remain immutable. Returns changed
// field names in stable audit order; rejects no-op transitions. Increasing a
// bootstrap weight also requires the caller's named failed gate for rollback.
std::vector<std::string>
validate_training_config_transition(const ProductionConfig& from, const ProductionConfig& to,
                                    std::string_view rollback_failed_gate = {});

} // namespace diamond_orchestration
