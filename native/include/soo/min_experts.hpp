#pragma once

#include <string_view>

#include "soo/selfplay.hpp"

namespace soo {

// Membership describes geometry and the full cyclic seat order, independent
// of player labels, the acting player, and which players have finished.
enum class MinTurnOrderGroup { cyclic, reverse };

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
    void evaluate(std::vector<BatchItem>& batch) override;

  private:
    BatchEvaluator& cyclic_;
    BatchEvaluator& reverse_;
};

} // namespace soo
