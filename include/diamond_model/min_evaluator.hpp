#pragma once

#include "diamond_model/soo_model.hpp"
#include "soo/selfplay.hpp"

namespace diamond_model {

// Inference adapter for interactive Min search and the fixed expert router.
class MinEvaluator final : public soo::BatchEvaluator {
  public:
    explicit MinEvaluator(DiamondModel model, torch::Device device = torch::kCPU);
    void evaluate(std::vector<soo::BatchItem>& batch) override;

  private:
    DiamondModel model_;
    torch::Device device_;
};

} // namespace diamond_model
