// Goal-plane warm starts preserve the old function while allowing new inputs to learn.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "diamond_model/soo_evaluator.hpp"
#include "diamond_pipeline/replay_segment.hpp"
#include "diamond_training/checkpoint.hpp"
#include "diamond_training/checkpoint_export.hpp"
#include "soo/rules.hpp"
#undef CHECK
#include "check.hpp"

namespace {
using diamond_training::Compatibility;
using diamond_training::Trainer;
using soo::EncodingVersion;

template<class Operation> bool rejects(Operation&& operation) {
    try { operation(); } catch (const std::exception&) { return true; }
    return false;
}

soo::State initial(const soo::Match& match) {
    soo::State state;
    state.current_player = match.players[0].id;
    for (int seat = 0; seat < match.count; ++seat)
        for (const auto hole : soo::topology().camp_positions[match.players[seat].camp])
            state.occupancy[hole] = match.players[seat].id;
    return state;
}

torch::Tensor features(soo::Encoded encoded) {
    return torch::from_blob(encoded.node_features.data(), {1, 73, encoded.feature_count}).clone();
}

diamond_training::TrainingSample sample(const soo::State& state, const soo::Match& match,
                                         const Compatibility& compatibility) {
    const auto encoded = soo::encode(state, match,
        soo::encoding_version_from_name(compatibility.encoder_version));
    diamond_training::TrainingSample result;
    result.compatibility = compatibility;
    result.node_features = encoded.node_features;
    result.canonical_player_ids.assign(encoded.canonical_player_ids.begin(),
                                       encoded.canonical_player_ids.end());
    std::vector<int32_t> legal;
    soo::canonical_legal_action_ids(state, match, legal);
    REQUIRE(!legal.empty(), "fixture has legal actions");
    for (const auto action : legal)
        result.sparse_policy.emplace_back(action, action == legal.front() ? 1.0F : 0.0F);
    result.value_target = match.count == 2 ? std::vector<float>{1.0F}
                                          : std::vector<float>{1.0F, 0.0F, -1.0F};
    return result;
}

void family_case(bool min, const std::filesystem::path& scratch) {
    const auto device = diamond_training::resolve_device("cpu");
    const auto legacy = min ? Compatibility::min("test", {1, 8})
                             : Compatibility::soo("test", {1, 8});
    const auto goals = min ? Compatibility::min("test", {1, 8}, EncodingVersion::goals_v2)
                            : Compatibility::soo("test", {1, 8}, EncodingVersion::goals_v2);
    CHECK_EQ(legacy.input_features(), min ? 6 : 4);
    CHECK_EQ(goals.input_features(), min ? 9 : 6);
    auto invalid = goals;
    invalid.encoder_version = "unknown-encoder";
    CHECK(rejects([&] { invalid.validate(); }));

    torch::manual_seed(min ? 731 : 732);
    auto source = diamond_model::DiamondModel(8, 1, legacy.input_features(), min ? 3 : 1);
    source->set_adjacency(diamond_model::topology_adjacency());
    source->eval();
    const auto source_digest = diamond_training::canonical_model_digest(source);
    auto migrated = diamond_training::migrate_goal_inputs(source, goals, device);
    CHECK(migrated->is_training());
    const auto migrated_parameters = migrated->named_parameters();
    for (const auto& entry : source->named_parameters()) {
        const auto& other = migrated_parameters[entry.key()];
        if (entry.key() == "input_projection.weight") {
            CHECK(torch::equal(entry.value(), other.narrow(1, 0, legacy.input_features())));
            CHECK_EQ(other.narrow(1, legacy.input_features(), goals.player_count).abs().max().item<float>(), 0.0F);
        } else CHECK(torch::equal(entry.value(), other));
        CHECK(entry.value().data_ptr() != other.data_ptr());
        CHECK(other.requires_grad());
    }
    const auto migrated_buffers = migrated->named_buffers();
    for (const auto& entry : source->named_buffers())
        CHECK(torch::equal(entry.value(), migrated_buffers[entry.key()]));
    CHECK(!source->is_training());

    auto empty_model = diamond_model::DiamondModel(8, 1, goals.input_features(), min ? 3 : 1);
    Trainer trainer(empty_model, goals, {1e-3, 0.0, diamond_training::PolicyLossDomain::legal}, device);
    const auto before_storage = trainer.learner()->input_projection->weight.data_ptr();
    diamond_training::migrate_goal_inputs(source, trainer);
    CHECK(before_storage == trainer.learner()->input_projection->weight.data_ptr());
    CHECK_EQ(diamond_training::canonical_model_digest(migrated),
             diamond_training::canonical_model_digest(trainer.learner()));
    CHECK_EQ(diamond_training::canonical_model_digest(source), source_digest);
    CHECK(rejects([&] { diamond_training::migrate_goal_inputs(source, legacy, device); }));
    CHECK(rejects([&] { diamond_training::migrate_goal_inputs(migrated, goals, device); }));

    const auto standard = min ? soo::standard_min_match() : soo::standard_soo_match();
    std::vector<int> seats;
    for (int seat = 0; seat < standard.count; ++seat) seats.push_back(seat);
    std::vector<diamond_training::TrainingSample> batch;
    do {
        auto match = standard;
        for (int seat = 0; seat < match.count; ++seat) match.players[seat] = standard.players[seats[seat]];
        auto state = initial(match);
        for (int ply = 0; ply < 6; ++ply) {
            const auto old_encoded = soo::encode(state, match);
            const auto new_encoded = soo::encode(state, match, EncodingVersion::goals_v2);
            {
                torch::NoGradGuard guard;
                const auto [old_policy, old_value] = source->forward(features(old_encoded));
                const auto [new_policy, new_value] = trainer.learner()->forward(features(new_encoded));
                // Expanded GEMMs may use a different floating-point reduction order.
                CHECK(torch::allclose(old_policy, new_policy, 1e-6, 1e-6));
                CHECK(torch::allclose(old_value, new_value, 1e-6, 1e-6));
            }
            if (!min) {
                std::vector<int32_t> legal;
                soo::canonical_legal_action_ids(state, match, legal);
                diamond_model::SooEvaluator old_evaluator(source), new_evaluator(migrated);
                const auto old_result = old_evaluator.evaluate(old_encoded, legal);
                const auto new_result = new_evaluator.evaluate(new_encoded, legal);
                CHECK_EQ(old_result.priors.size(), new_result.priors.size());
                for (size_t index = 0; index < old_result.priors.size(); ++index)
                    CHECK(std::abs(old_result.priors[index] - new_result.priors[index]) < 1e-6);
                CHECK(std::abs(old_result.value - new_result.value) < 1e-6);
                CHECK(rejects([&] { new_evaluator.evaluate(old_encoded, legal); }));
                CHECK(rejects([&] { old_evaluator.evaluate(new_encoded, legal); }));
            }
            batch.push_back(sample(state, match, goals));
            std::vector<int32_t> actions;
            soo::legal_action_ids(state, actions);
            state = soo::apply_action(state, match, actions.at((ply * 7) % actions.size()));
        }
    } while (std::next_permutation(seats.begin(), seats.end()));
    CHECK_EQ(batch.size(), min ? size_t{36} : size_t{12});
    CHECK_EQ(trainer.training_step(), uint64_t{0});
    CHECK(trainer.optimizer().state().empty());
    trainer.train(batch);
    const auto input = trainer.learner()->input_projection->weight;
    const auto goal_grad = input.grad().narrow(1, legacy.input_features(), goals.player_count);
    CHECK(torch::isfinite(goal_grad).all().item<bool>());
    CHECK(goal_grad.abs().max().item<float>() > 0.0F);
    CHECK(input.narrow(1, legacy.input_features(), goals.player_count).abs().max().item<float>() > 0.0F);
    CHECK(rejects([&] { diamond_training::migrate_goal_inputs(source, trainer); }));
    CHECK_EQ(diamond_training::canonical_model_digest(source), source_digest);

    auto old_sample = sample(initial(standard), standard, legacy);
    CHECK(rejects([&] { trainer.train(std::vector{old_sample}); }));
    Trainer old_trainer(source, legacy, {1e-3, 0.0}, device);
    CHECK(rejects([&] { old_trainer.train(batch); }));
    diamond_pipeline::Episode episode;
    episode.game_id = "goal-gate";
    episode.compatibility = goals;
    episode.samples = {batch.front()};
    auto segment = diamond_pipeline::encode_replay_segment(episode, goals);
    CHECK_EQ(diamond_pipeline::decode_replay_segment(segment, goals).samples.front().node_features,
             batch.front().node_features);
    CHECK(rejects([&] { diamond_pipeline::decode_replay_segment(segment, legacy); }));
    episode.compatibility = legacy;
    episode.samples = {old_sample};
    segment = diamond_pipeline::encode_replay_segment(episode, legacy);
    CHECK(rejects([&] { diamond_pipeline::decode_replay_segment(segment, goals); }));

    diamond_training::CheckpointLineage lineage;
    lineage.run_id = "goal-migration-test";
    lineage.model_step = trainer.training_step();
    diamond_training::save_checkpoint_v3(scratch / "goals-checkpoint", trainer, lineage);
    lineage.model_step = old_trainer.training_step();
    diamond_training::save_checkpoint_v3(scratch / "legacy-checkpoint", old_trainer, lineage);
    CHECK(rejects([&] { diamond_training::load_checkpoint_v3(scratch / "goals-checkpoint", old_trainer,
        device, diamond_training::CheckpointLoadIntent::exact_resume); }));
    CHECK(rejects([&] { diamond_training::load_checkpoint_v3(scratch / "legacy-checkpoint", trainer,
        device, diamond_training::CheckpointLoadIntent::exact_resume); }));
    Trainer restored(empty_model, goals, {1e-3, 0.0}, device);
    diamond_training::load_checkpoint_v3(scratch / "goals-checkpoint", restored, device,
                                        diamond_training::CheckpointLoadIntent::exact_resume);
    CHECK_EQ(diamond_training::canonical_model_digest(restored.learner()),
             diamond_training::canonical_model_digest(trainer.learner()));
    CHECK_EQ(restored.training_step(), trainer.training_step());
    for (const auto goal_aware : {false, true}) {
        const auto name = goal_aware ? "goals" : "legacy";
        const auto& compatibility = goal_aware ? goals : legacy;
        const auto& model = goal_aware ? trainer.learner() : old_trainer.learner();
        const auto artifact = diamond_training::export_checkpoint(scratch / (std::string(name) + "-checkpoint"),
            scratch / (std::string(name) + "-artifact"), "test");
        CHECK_EQ(artifact.input_features, compatibility.input_features());
        CHECK_EQ(artifact.encoder_version, compatibility.encoder_version);
        auto reloaded = diamond_model::DiamondModel(8, 1, artifact.input_features, min ? 3 : 1);
        reloaded->load_weights(artifact.weights);
        CHECK_EQ(diamond_training::canonical_model_digest(reloaded),
                 diamond_training::canonical_model_digest(model));
        const auto metadata_path = artifact.root / "metadata.json";
        std::ifstream stream(metadata_path);
        auto metadata = std::string(std::istreambuf_iterator<char>(stream), {});
        const auto where = metadata.find(compatibility.encoder_version);
        REQUIRE(where != std::string::npos, "encoder version in deployment contract");
        metadata.replace(where, compatibility.encoder_version.size(), "unknown-encoder");
        std::ofstream(metadata_path) << metadata;
        CHECK(rejects([&] { diamond_model::validate_deployment_artifact(artifact.root); }));
    }
}
} // namespace

int main(int argc, char** argv) {
    REQUIRE(argc == 2, "scratch path required");
    torch::set_num_threads(2);
    const std::filesystem::path scratch = argv[1];
    std::filesystem::remove_all(scratch);
    std::filesystem::create_directories(scratch);
    family_case(false, scratch / "soo");
    family_case(true, scratch / "min");
    std::filesystem::remove_all(scratch);
    return soo_test::report("goal_input_migration_test");
}
