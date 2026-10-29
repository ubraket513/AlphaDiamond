#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "diamond_model/deployment_artifact.hpp"
#include "diamond_model/soo_model.hpp"
#include "diamond_orchestration/config.hpp"
#include "diamond_pipeline/replay_store.hpp"
#include "diamond_support/json.hpp"
#include "diamond_training/checkpoint.hpp"
#undef CHECK
#include "check.hpp"

namespace {
std::string quote(const std::string& value) {
    std::string result = "'";
    for (const char character : value)
        result += character == '\'' ? "'\\''" : std::string(1, character);
    return result + "'";
}
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), {}};
}
}

int main(int argc, char** argv) {
    REQUIRE(argc == 3, "scratch path and benchmark required");
    torch::set_num_threads(1);
    const auto scratch = std::filesystem::path(std::string(argv[1]) + "-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(scratch);
    diamond_orchestration::ProductionConfig config;
    config.model_name = "Min";
    config.network = {.width = 8, .residual_blocks = 1};
    config.arena.games = 18;
    config.replay.capacity = 32;
    config.training.batch_size = 2;
    config.training.policy_loss_domain = "legal";
    const auto config_path = scratch / "config.json";
    std::ofstream(config_path) << diamond_support::canonical_json(config.to_json());
    const auto compatibility = diamond_training::Compatibility::min("2.0.0", {1, 8});
    auto model = diamond_model::DiamondModel(8, 1, 6, 3);
    model->set_adjacency(diamond_model::topology_adjacency());
    diamond_training::Trainer trainer(model, compatibility,
        {config.training.learning_rate, config.training.weight_decay,
         diamond_training::PolicyLossDomain::legal}, diamond_training::resolve_device("cpu"));
    diamond_pipeline::ReplayStore replay(scratch / "replay", compatibility, 32, config.replay.seed);
    diamond_pipeline::Episode episode;
    episode.game_id = "replay-benchmark";
    episode.retry_id = "attempt-0";
    episode.model_key = {.model_name = "Min", .model_version = "2.0.0",
                         .checkpoint_sha256 = std::string(64, 'a')};
    episode.compatibility = compatibility;
    episode.final_order = {0, 1, 2};
    for (int index = 0; index < 8; ++index) {
        diamond_training::TrainingSample sample;
        sample.compatibility = compatibility;
        sample.node_features.assign(73 * 6, 0.0F);
        sample.canonical_player_ids = {0, 1, 2};
        sample.sparse_policy = {{0, 0.6F}, {1, 0.4F}};
        sample.value_target = {1.0F, 0.0F, -1.0F};
        episode.samples.push_back(std::move(sample));
    }
    replay.ingest(std::span<const diamond_pipeline::Episode>(&episode, 1));
    (void)trainer.train(replay.sample(2, 7));
    diamond_training::CheckpointLineage lineage;
    lineage.run_id = "replay-benchmark-test";
    lineage.model_step = trainer.training_step();
    const auto checkpoint = diamond_training::save_checkpoint_v3(scratch / "checkpoint", trainer, lineage);
    const auto current_before = read(scratch / "checkpoint" / "CURRENT");
    const auto replay_before = replay.manifest_digest();
    const auto orphan = replay.manifest_path().parent_path() / "chunks" / "orphan-test.bin";
    std::ofstream(orphan) << "source-only orphan evidence";
    std::filesystem::copy(scratch / "replay", scratch / "snapshot",
                          std::filesystem::copy_options::recursive);
    const auto snapshot_orphan = scratch / "snapshot" /
        std::filesystem::relative(orphan, scratch / "replay");
    const std::string command = quote(argv[2]) + " --checkpoint " + quote((scratch / "checkpoint").string()) +
        " --config " + quote(config_path.string()) + " --replay " + quote((scratch / "snapshot").string()) +
        " --iteration 12 --device cpu --batch-size 2 --warmups 0 --repetitions 2 --threads 1";
    REQUIRE(std::system((command + " > " + quote((scratch / "report.json").string()) +
                         " 2> " + quote((scratch / "error.log").string())).c_str()) == 0,
            "Min replay benchmark must execute");
    const auto report = diamond_support::parse_json(read(scratch / "report.json"));
    const auto& object = std::get<diamond_support::JsonValue::Object>(report.value);
    const auto& domain = std::get<diamond_support::JsonValue::Object>(object.at("domain").value);
    CHECK_EQ(std::get<std::string>(domain.at("sample_kind").value), "replay");
    CHECK_EQ(std::get<std::string>(domain.at("model_name").value), "Min");
    CHECK_EQ(std::get<std::string>(domain.at("policy_loss_domain").value), "legal");
    CHECK_EQ(std::get<std::string>(domain.at("model_sha256").value), checkpoint.model_digest);
    CHECK_EQ(std::get<std::string>(domain.at("replay_manifest_sha256").value), replay_before);
    CHECK_EQ(std::get<int64_t>(domain.at("initial_training_step").value), int64_t{1});
    CHECK_EQ(std::get<int64_t>(domain.at("training_step").value), int64_t{3});
    CHECK_EQ(std::get<diamond_support::JsonValue::Array>(object.at("samples_seconds").value).size(), std::size_t{2});
    CHECK_EQ(read(scratch / "checkpoint" / "CURRENT"), current_before);
    CHECK_EQ(replay.manifest_digest(), replay_before);
    CHECK_EQ(read(orphan), "source-only orphan evidence");
    CHECK(!std::filesystem::exists(snapshot_orphan));
    CHECK(std::system((quote(argv[2]) + " --checkpoint " + quote((scratch / "checkpoint").string()) +
        " > /dev/null 2>&1").c_str()) != 0);
    CHECK(std::system((command + " --replay " + quote((scratch / "missing").string()) +
        " > /dev/null 2>&1").c_str()) != 0);
    CHECK(!std::filesystem::exists(scratch / "missing"));
    std::filesystem::remove_all(scratch);
    return soo_test::report("training_step_replay_benchmark_test");
}
