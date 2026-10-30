// Min's two specialists are selected by camp geometry and full turn order.
// Classification uses the match, never a learned score, failure, or current turn.
// These routers borrow evaluator references; their owners must keep both expert
// pools alive and serialize evaluation as required by ModelPool.
#pragma once

#include <string_view>

#include "soo/selfplay.hpp"

namespace soo {

// Membership describes geometry and the full cyclic seat order, independent
// of player labels, the acting player, and which players have finished.
enum class MinTurnOrderGroup { cyclic, reverse };

// Accepts standard three-player geometry and its rotations/cyclic seat anchors.
// Player labels do not define the group. Invalid IDs/camps or unsupported geometry
// throw; there is no shared-expert fallback for an unclassifiable match.
MinTurnOrderGroup min_turn_order_group(const Match& match);
std::string_view min_turn_order_group_name(MinTurnOrderGroup group);
MinTurnOrderGroup min_turn_order_group_from_name(std::string_view name);

// Compose inside ArenaModelRouter so the actual root mover first selects
// candidate versus champion, then its game's geometry selects the expert.
class MinExpertRouter final : public BatchEvaluator {
  public:
    MinExpertRouter(BatchEvaluator& cyclic, BatchEvaluator& reverse)
        : cyclic_(cyclic), reverse_(reverse) {}

    void prepare(BatchItem& item) override;
    // Partitions descriptors while preserving borrowed scheduler output buffers.
    // Called synchronously by the evaluator thread; owns neither model weights
    // nor request lifetimes and creates no background execution path.
    void evaluate(std::vector<BatchItem>& batch) override;

  private:
    BatchEvaluator& cyclic_;
    BatchEvaluator& reverse_;
};

} // namespace soo
