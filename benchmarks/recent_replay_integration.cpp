#include "diamond_orchestration/config.hpp"
#include "diamond_orchestration/training_wiring.hpp"
#include "diamond_pipeline/pipeline.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <torch/torch.h>

int main(int argc, char** argv) {
    const auto require = [](bool ok) {
        if (!ok) throw std::runtime_error("recent replay integration invariant failed");
    };
    if (argc != 3 || std::filesystem::exists(argv[1]))
        throw std::invalid_argument("provide a new scratch directory and a production config");
    std::ifstream input(argv[2]);
    require(input.good());
    const auto production = diamond_orchestration::ProductionConfig::from_json(
        diamond_support::parse_json(std::string(std::istreambuf_iterator<char>(input), {})));
    auto mixed = production;
    mixed.training.recent_sample_fraction = .75;
    require(diamond_orchestration::validate_training_config_transition(production, mixed, {}) ==
            std::vector<std::string>{"training.recent_sample_fraction"});
    mixed.network.width += 8;
    bool incompatible = false;
    try { (void)diamond_orchestration::validate_training_config_transition(production, mixed, {}); }
    catch (const diamond_orchestration::ConfigError&) { incompatible = true; }
    require(incompatible);
    diamond_orchestration::TrainingConfig config;
    const auto original = config.to_json();
    require(!std::get<diamond_support::JsonValue::Object>(original.value).contains("recent_sample_fraction"));
    require(diamond_orchestration::TrainingConfig::from_json(original) == config);
    config.recent_sample_fraction = .75;
    require(diamond_orchestration::TrainingConfig::from_json(config.to_json()) == config);
    for (const double invalid : {-1.0, 1.0, std::numeric_limits<double>::quiet_NaN()}) {
        config.recent_sample_fraction = invalid;
        bool rejected = false;
        try { config.validate(); } catch (const diamond_orchestration::ConfigError&) { rejected = true; }
        require(rejected);
    }

    torch::manual_seed(7);
    torch::set_num_threads(1);
    torch::set_num_interop_threads(1);
    const auto compatibility = diamond_training::Compatibility::soo(
        "recent-replay-probe", {.residual_blocks = 1, .width = 8});
    diamond_pipeline::ReplayStore replay(argv[1], compatibility, 16, 7);
    std::vector<diamond_pipeline::Episode> episodes;
    for (int i = 0; i < 16; ++i) {
        diamond_pipeline::Episode episode;
        episode.game_id = "probe-" + std::to_string(i);
        episode.seed = i;
        episode.compatibility = compatibility;
        diamond_training::TrainingSample sample;
        sample.compatibility = compatibility;
        sample.node_features.assign(73 * compatibility.input_features(), static_cast<float>(i) / 16);
        sample.canonical_player_ids = {1, 2};
        sample.sparse_policy = {{0, 1.0F}};
        sample.value_target = {1.0F};
        episode.samples.push_back(std::move(sample));
        episodes.push_back(std::move(episode));
    }
    require(replay.ingest(episodes) == 16);
    const auto digest = replay.manifest_digest();
    const auto selected = replay.samples_at(std::array<std::size_t, 3>{15, 0, 15});
    require(selected[0].node_features[0] == 15.0F / 16 && selected[1].node_features[0] == 0);
    require(selected[2].node_features == selected[0].node_features);
    bool bad_index = false;
    try { (void)replay.samples_at(std::array<std::size_t, 1>{16}); }
    catch (const std::out_of_range&) { bad_index = true; }
    require(bad_index);

    auto model = diamond_model::DiamondModel(8, 1, compatibility.input_features(), 1);
    diamond_training::Trainer trainer(model, compatibility,
        {.learning_rate = 1e-3, .weight_decay = 1e-4}, diamond_training::resolve_device("cpu"));
    diamond_pipeline::IterationRequest request;
    request.operation_id = "probe-training";
    request.compatibility = compatibility;
    request.training_batch_size = 8;
    request.training_steps = 1;
    request.recent_sample_fraction = .75;
    request.recent_sample_count = 12;
    bool short_budget = false;
    try { (void)diamond_pipeline::train_replay(request, replay, trainer); }
    catch (const diamond_pipeline::PipelineError&) { short_budget = true; }
    require(short_budget && trainer.training_step() == 0);
    request.training_steps = 2;
    const auto trained = diamond_pipeline::train_replay(request, replay, trainer);
    require(trained.training_step == 2 && trained.recent_samples_drawn == 12);
    require(trained.training_batch_sizes == std::vector<std::size_t>({8, 8}));
    require(replay.manifest_digest() == digest);
    request.recent_sample_fraction = 0;
    const auto uniform = diamond_pipeline::train_replay(request, replay, trainer);
    require(uniform.training_step == 4 && uniform.recent_samples_drawn == 0);
    require(replay.manifest_digest() == digest);
    std::cout << "recent replay: config compatibility, real replay reads, budget gate and CPU training passed\n";
}
