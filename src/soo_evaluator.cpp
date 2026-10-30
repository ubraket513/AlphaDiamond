#include "diamond_model/soo_evaluator.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace diamond_model {

SooEvaluator::SooEvaluator(SooModel model, torch::Device device)
    : model_(std::move(model)), device_(device) {
    if (!model_) throw std::invalid_argument("SooEvaluator requires a model");
    if ((model_->input_features() != 4 && model_->input_features() != 6) ||
        model_->value_size() != 1)
        throw std::invalid_argument("SooEvaluator requires a supported Soo architecture");
    model_->to(device_);
    model_->eval();
}

soo::EvalOutcome SooEvaluator::evaluate(const soo::Encoded& encoded,
                                        const std::vector<int32_t>& legal_actions) {
    const auto features_per_node = model_->input_features();
    if (encoded.feature_count != features_per_node ||
        encoded.node_features.size() != 73U * static_cast<size_t>(features_per_node)) {
        throw std::invalid_argument("Soo encoding does not match model input features");
    }
    if (legal_actions.empty()) throw std::invalid_argument("Soo evaluator requires legal actions");
    for (const auto action : legal_actions) {
        if (action < 0 || action >= 73 * 73) {
            throw std::invalid_argument("legal action is outside the Soo policy space");
        }
    }

    torch::NoGradGuard no_grad;
    auto features = torch::from_blob(
                         const_cast<float*>(encoded.node_features.data()),
                         {1, 73, features_per_node}, torch::kFloat32)
                         .clone().to(device_);
    const auto [policy, value] = model_->forward(features);
    const auto action_index = torch::tensor(legal_actions, torch::TensorOptions().dtype(torch::kLong).device(device_));
    const auto legal_logits = policy.index_select(1, action_index);
    const auto probabilities = torch::softmax(legal_logits, 1).to(torch::kCPU).contiguous().view(-1);
    const auto cpu_value = value.to(torch::kCPU);
    const auto values = probabilities.data_ptr<float>();
    soo::EvalOutcome outcome;
    outcome.priors.reserve(legal_actions.size());
    for (size_t i = 0; i < legal_actions.size(); ++i) {
        if (!std::isfinite(values[i])) throw std::runtime_error("Soo produced a non-finite prior");
        outcome.priors.push_back(static_cast<double>(values[i]));
    }
    outcome.value = static_cast<double>(cpu_value.item<float>());
    if (!std::isfinite(outcome.value)) throw std::runtime_error("Soo produced a non-finite value");
    return outcome;
}

}  // namespace diamond_model
