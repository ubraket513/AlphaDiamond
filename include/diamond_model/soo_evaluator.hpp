#pragma once

#include "diamond_model/soo_model.hpp"
#include "soo/evaluator.hpp"

namespace diamond_model {

class SooEvaluator final : public soo::Evaluator {
  public:
    explicit SooEvaluator(SooModel model, torch::Device device = torch::kCPU);

    soo::EvalOutcome evaluate(const soo::Encoded& encoded,
                              const std::vector<int32_t>& legal_actions) override;

  private:
    SooModel model_;
    torch::Device device_;
};

}  // namespace diamond_model
