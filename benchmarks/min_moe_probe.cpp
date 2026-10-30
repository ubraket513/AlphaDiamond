#include "diamond_model/deployment_artifact.hpp"
#include "diamond_model/min_evaluator.hpp"
#include "diamond_support/json.hpp"
#include "soo/min_experts.hpp"
#include "soo/mcts3p.hpp"
#include "soo/rules.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
diamond_model::DiamondModel load(const diamond_model::DeploymentArtifact& artifact) {
    diamond_model::DiamondModel model(artifact.width, artifact.residual_blocks,
                                      artifact.input_features, artifact.value_size);
    model->load_weights(artifact.weights);
    model->eval();
    return model;
}
template<class Operation> void reject(Operation operation) {
    try { operation(); }
    catch (const std::exception&) { return; }
    throw std::runtime_error("corrupt MoE accepted");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 4 || argc == 5,
                "usage: min_moe_probe CYCLIC_ARTIFACT REVERSE_ARTIFACT NEW_OUTPUT [cpu|cuda:0]");
        const torch::Device device(argc == 5 ? argv[4] : "cpu");
        torch::set_num_threads(2);
        torch::set_num_interop_threads(1);
        soo::ensure_topology_configured();
        const auto cyclic = diamond_model::validate_deployment_artifact(argv[1], "min");
        const auto reverse = diamond_model::validate_deployment_artifact(argv[2], "min");
        const auto swapped = std::filesystem::path(std::string(argv[3]) + ".swapped");
        require(!std::filesystem::exists(swapped), "swapped-role probe destination already exists");
        reject([&] { (void)diamond_model::write_min_moe_deployment_artifact(
            argv[2], argv[1], swapped, "1.0.0"); });
        require(!std::filesystem::exists(swapped), "swapped roles created an output bundle");
        const auto bundle = diamond_model::write_min_moe_deployment_artifact(
            argv[1], argv[2], argv[3], "1.0.0");
        require(bundle.experts.size() == 2 && bundle.model_family == "min" &&
                bundle.model_version == "1.0.0", "bad MoE identity");
        require(bundle.experts[0].runtime_sha256 == cyclic.runtime_sha256 &&
                bundle.experts[1].runtime_sha256 == reverse.runtime_sha256,
                "expert identities changed");
        diamond_model::MinEvaluator expected_cyclic(load(cyclic)), expected_reverse(load(reverse));
        diamond_model::MinEvaluator deployed_cyclic(load(bundle.experts[0]), device),
                                    deployed_reverse(load(bundle.experts[1]), device);
        soo::MinExpertRouter router(deployed_cyclic, deployed_reverse);
        const auto base = soo::standard_min_match();
        std::array<int, 3> order{0, 1, 2};
        int checked = 0;
        int evaluations = 0;
        double max_prior_difference = 0, max_value_difference = 0;
        do {
            auto match = base;
            for (int i = 0; i < 3; ++i) match.players[i] = base.players[order[i]];
            soo::State state;
            for (int i = 0; i < 3; ++i)
                for (const auto cell : soo::topology().camp_positions[base.players[i].camp])
                    state.occupancy[cell] = base.players[i].id;
            for (int actor = 0; actor < 3; ++actor) {
                state.current_player = base.players[actor].id;
                soo::MCTSConfig config;
                config.simulations = 16;
                config.encoding = soo::EncodingVersion::goals_v2;
                soo::SearchSession3P session(match, config);
                session.begin(state, 0.0);
                while (session.advance() == soo::SearchSession3P::Status::NeedsEvaluation) {
                    soo::EvalOutcome actual, expected;
                    soo::BatchItem item{.state=&session.pending_state(), .encoded=&session.pending_features(),
                        .actions=&session.pending_actions(), .outcome=&actual, .value_width=3,
                        .root_player=state.current_player, .match=&match};
                    std::vector<soo::BatchItem> batch{item};
                    router.evaluate(batch);
                    batch[0].outcome = &expected;
                    auto& evaluator = soo::min_turn_order_group(match) == soo::MinTurnOrderGroup::cyclic
                        ? expected_cyclic : expected_reverse;
                    evaluator.evaluate(batch);
                    require(actual.priors.size() == expected.priors.size(), "deployed prior shape differs");
                    for (size_t i = 0; i < actual.priors.size(); ++i)
                        max_prior_difference = std::max(max_prior_difference,
                                                       std::abs(actual.priors[i] - expected.priors[i]));
                    for (size_t i = 0; i < actual.values.size(); ++i)
                        max_value_difference = std::max(max_value_difference,
                                                       std::abs(actual.values[i] - expected.values[i]));
                    const double tolerance = device.is_cpu() ? 0 : 1e-5;
                    if (max_prior_difference > tolerance || max_value_difference > tolerance) {
                        std::cerr << "parity differences: prior=" << max_prior_difference
                                  << " value=" << max_value_difference << '\n';
                        throw std::runtime_error("deployed router differs from original CPU expert");
                    }
                    soo::EvalOutcome3P outcome;
                    outcome.priors = std::move(actual.priors);
                    outcome.value = actual.values;
                    session.supply(outcome);
                    ++evaluations;
                }
                const auto& result = session.result();
                require(result.simulations_run == 16, "Min search did not complete 16 simulations");
                const auto physical = soo::to_physical_action(result.selected_action, match, state.current_player);
                std::vector<int32_t> legal;
                soo::legal_action_ids(state, legal);
                require(std::find(legal.begin(), legal.end(), physical) != legal.end(),
                        "Min search produced an illegal move");
                ++checked;
            }
        } while (std::next_permutation(order.begin(), order.end()));
        const auto child = bundle.root / "experts/reverse/metadata.json";
        const auto hidden = bundle.root / "experts/reverse/metadata.hidden";
        std::filesystem::rename(child, hidden);
        reject([&] { (void)diamond_model::validate_deployment_artifact(bundle.root); });
        std::filesystem::rename(hidden, child);
        const auto metadata_size = std::filesystem::file_size(child);
        {
            std::ofstream output(child, std::ios::binary | std::ios::app);
            output.put(' ');
            require(static_cast<bool>(output), "cannot tamper with expert metadata");
        }
        reject([&] { (void)diamond_model::validate_deployment_artifact(bundle.root); });
        std::filesystem::resize_file(child, metadata_size);
        const auto weights = bundle.experts[1].weights;
        const auto tensor = std::filesystem::directory_iterator(weights)->path();
        const auto moved = tensor.string() + ".hidden";
        std::filesystem::rename(tensor, moved);
        reject([&] { (void)diamond_model::validate_deployment_artifact(bundle.root); });
        std::filesystem::rename(moved, tensor);
        const auto external = std::filesystem::absolute(std::string(argv[3]) + ".external-weight");
        std::filesystem::copy_file(tensor, external);
        std::filesystem::rename(tensor, moved);
        std::filesystem::create_symlink(external, tensor);
        reject([&] { (void)diamond_model::validate_deployment_artifact(bundle.root); });
        std::filesystem::remove(tensor);
        std::filesystem::rename(moved, tensor);
        std::filesystem::remove(external);
        (void)diamond_model::validate_deployment_artifact(bundle.root);
        reject([&] { (void)diamond_model::write_min_moe_deployment_artifact(
            argv[1], argv[2], argv[3], "1.0.0"); });
        std::cout << "MoE: " << checked
                  << " order/actor parity cases at 16 simulations on " << device
                  << "; evaluations=" << evaluations << " max_prior_difference=" << max_prior_difference
                  << " max_value_difference=" << max_value_difference
                  << "; missing/corrupt expert, swapped roles, metadata tamper, symlink rejected\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
