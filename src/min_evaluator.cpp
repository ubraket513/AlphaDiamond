#include "diamond_model/min_evaluator.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace diamond_model {

MinEvaluator::MinEvaluator(DiamondModel model, torch::Device device)
    : model_(std::move(model)), device_(device) {
    if (!model_ || (model_->input_features() != 6 && model_->input_features() != 9) ||
        model_->value_size() != 3)
        throw std::invalid_argument("MinEvaluator requires a supported Min architecture");
    model_->to(device_);
    model_->eval();
}

void MinEvaluator::evaluate(std::vector<soo::BatchItem>& batch) {
    torch::NoGradGuard no_grad;
    for (const auto& item : batch) {
        if (!item.encoded || !item.actions || !item.outcome || item.value_width != 3)
            throw std::invalid_argument("MinEvaluator requires a complete three-value request");
        const auto& encoded = *item.encoded;
        const auto& actions = *item.actions;
        const auto features_per_node = model_->input_features();
        if (encoded.feature_count != features_per_node ||
            encoded.node_features.size() != 73U * static_cast<size_t>(features_per_node))
            throw std::invalid_argument("Min encoding does not match model input features");
        if (actions.empty()) throw std::invalid_argument("Min evaluation requires legal actions");
        for (const auto action : actions)
            if (action < 0 || action >= 73 * 73)
                throw std::invalid_argument("Min action is outside the policy space");
        const auto features = torch::from_blob(
            const_cast<float*>(encoded.node_features.data()), {1, 73, features_per_node},
            torch::kFloat32).to(device_);
        const auto [logits, values] = model_->forward(features);
        const auto indices = torch::tensor(actions, torch::TensorOptions().dtype(torch::kLong).device(device_));
        // Blocking CPU copies finish CUDA inference before search consumes outputs and records timing.
        const auto probabilities = torch::softmax(logits.index_select(1, indices), 1).to(torch::kCPU).contiguous();
        const auto cpu_values = values.to(torch::kCPU).contiguous();
        auto& outcome = *item.outcome;
        outcome.priors.clear();
        outcome.priors.reserve(actions.size());
        for (size_t index = 0; index < actions.size(); ++index) {
            const auto prior = probabilities.data_ptr<float>()[index];
            if (!std::isfinite(prior)) throw std::runtime_error("Min produced a non-finite prior");
            outcome.priors.push_back(prior);
        }
        for (size_t index = 0; index < outcome.values.size(); ++index) {
            outcome.values[index] = cpu_values[0][static_cast<int64_t>(index)].item<float>();
            if (!std::isfinite(outcome.values[index]))
                throw std::runtime_error("Min produced a non-finite value");
        }
        outcome.value = outcome.values.front();
    }
}

} // namespace diamond_model
