#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "diamond_orchestration/config.hpp"
#include "diamond_training/checkpoint.hpp"
#include "diamond_support/json.hpp"
#undef CHECK
#include "check.hpp"

namespace {
using Json = diamond_support::JsonValue;
using Object = Json::Object;
using Array = Json::Array;
using Path = std::filesystem::path;

std::string read(const Path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("cannot read " + path.string());
    return {std::istreambuf_iterator<char>(stream), {}};
}

Json read_json(const Path& path) {
    return diamond_support::parse_json(read(path));
}
const Object& object(const Json& value) {
    return std::get<Object>(value.value);
}
int64_t integer(const Object& value, const std::string& key) {
    return std::get<int64_t>(value.at(key).value);
}

void write(const Path& path, const Json& value) {
    std::ofstream(path) << diamond_support::canonical_json(value) << '\n';
}

std::string quote(const std::string& value) {
#ifdef _WIN32
    if (value.find_first_of("\"%\r\n") != std::string::npos)
        throw std::runtime_error("unsupported Windows test argument");
    return "\"" + value + "\"";
#else
    std::string result = "'";
    for (const char c : value)
        result += c == '\'' ? "'\"'\"'" : std::string(1, c);
    return result + "'";
#endif
}

int invoke(const Path& binary, const Path& scratch, const Path& cyclic, const Path& reverse,
           const Path& shared, const Path& config, const Path& out, bool control = true,
           const std::vector<std::pair<std::string, Path>>& references = {}) {
    std::string command = quote(binary.string());
    for (const auto& [flag, path] :
         std::array<std::pair<std::string, Path>, 5>{{{"--cyclic-checkpoint", cyclic},
                                                      {"--reverse-checkpoint", reverse},
                                                      {"--shared-checkpoint", shared},
                                                      {"--config", config},
                                                      {"--out", out}}})
        if (!path.empty())
            command += " " + flag + " " + quote(path.string());
    for (const auto& [flag, path] : references)
        command += " " + flag + " " + quote(path.string());
    if (control)
        command += " --shared-control-checkpoint " + quote((scratch / "control").string());
    command += " > " + quote((scratch / "cli.log").string()) + " 2>&1";
    const int result = std::system(command.c_str());
    if (result != 0)
        std::cerr << read(scratch / "cli.log");
    return result;
}

std::map<std::string, std::string> files(const Path& root) {
    std::map<std::string, std::string> result;
    for (const auto& item : std::filesystem::recursive_directory_iterator(root))
        if (item.is_regular_file())
            result.emplace(item.path().lexically_relative(root).string(),
                           diamond_support::sha256(read(item.path())));
    return result;
}

diamond_orchestration::ProductionConfig config() {
    diamond_orchestration::ProductionConfig result;
    result.model_name = "Min";
    result.model_version = "2.1.0";
    result.encoder_version =
        std::string(soo::encoding_version_name(soo::EncodingVersion::goals_v2));
    result.network = {.width = 8, .residual_blocks = 1};
    result.mcts.simulations = 1;
    result.self_play.balance_turn_orders = true;
    result.self_play.bootstrap_prior =
        std::string(diamond_orchestration::kCanonicalTargetVacancyDistanceV2);
    result.workers.games_per_iteration = 6;
    result.inference.max_batch_size = 6;
    result.inference.request_queue_capacity = 6;
    result.arena.games = 36;
    result.arena.max_moves = 1;
    result.opening_suite = {
        .id = "min-expert-cli-heldout", .version = 1, .seed = 9317, .count = 1, .max_depth = 0};
    result.validate();
    return result;
}

void checkpoint(const Path& root, const diamond_orchestration::ProductionConfig& config,
                std::string group, int seed, bool legacy = false) {
    torch::manual_seed(seed);
    auto model =
        diamond_model::DiamondModel(config.network.width, config.network.residual_blocks, 9, 3);
    model->set_adjacency(diamond_model::topology_adjacency());
    diamond_training::zero_value_head(model);
    auto compatibility = diamond_training::Compatibility::min(
        "2.1.0", {config.network.residual_blocks, config.network.width},
        soo::EncodingVersion::goals_v2);
    diamond_training::Trainer trainer(model, compatibility, {0.001, 0.0001},
                                      diamond_training::resolve_device("cpu"));
    auto resolved = config.to_json();
    if (group != "all")
        std::get<Object>(std::get<Object>(resolved.value).at("self_play").value)
            .insert_or_assign("turn_order_group", Json{std::move(group)});
    diamond_training::CheckpointProvenance provenance;
    provenance.resolved_config_bytes = diamond_support::canonical_json(resolved);
    diamond_training::CheckpointLineage lineage;
    lineage.run_id = root.filename().string();
    if (legacy)
        (void)diamond_training::save_checkpoint_v2(root, trainer);
    else
        (void)diamond_training::save_checkpoint_v3(root, trainer, lineage, provenance);
}

void check_aborted_schedule(const Path& out) {
    const auto experts_json = read_json(out / "experts" / "matches.json");
    const auto control_json = read_json(out / "shared-control" / "matches.json");
    const auto& experts = std::get<Array>(experts_json.value);
    const auto& control = std::get<Array>(control_json.value);
    REQUIRE(experts.size() == 36 && control.size() == 36,
            "all completed and aborted rows are required");
    std::map<std::string, int> groups;
    std::map<std::string, int> orders;
    std::map<int64_t, int> seats;
    std::set<std::string> match_ids;
    std::set<std::string> matchups;
    const std::map<std::string, std::string> expected_groups{
        {"[1,2,3]", "cyclic"},  {"[2,3,1]", "cyclic"},  {"[3,1,2]", "cyclic"},
        {"[1,3,2]", "reverse"}, {"[2,1,3]", "reverse"}, {"[3,2,1]", "reverse"}};
    for (std::size_t i = 0; i < experts.size(); ++i) {
        const auto& row = object(experts[i]);
        const auto& other = object(control[i]);
        CHECK(match_ids.insert(std::get<std::string>(row.at("match_id").value)).second);
        CHECK(matchups
                  .insert(diamond_support::canonical_json(row.at("turn_order")) + ":" +
                          diamond_support::canonical_json(row.at("seat_assignment")))
                  .second);
        for (const std::string field :
             {"opening_id", "match_id", "turn_order", "seat_assignment", "seed", "group"})
            CHECK_EQ(diamond_support::canonical_json(row.at(field)),
                     diamond_support::canonical_json(other.at(field)));
        CHECK(!std::get<bool>(row.at("completed").value));
        CHECK(!std::get<bool>(other.at("completed").value));
        CHECK(std::holds_alternative<std::nullptr_t>(row.at("candidate_placement").value));
        CHECK_EQ(std::get<std::string>(row.at("abort_reason").value), "move_limit");
        const auto order = diamond_support::canonical_json(row.at("turn_order"));
        const auto group = std::get<std::string>(row.at("group").value);
        CHECK_EQ(group, expected_groups.at(order));
        ++groups[group];
        ++orders[order];
        ++seats[std::get<int64_t>(std::get<Array>(row.at("seat_assignment").value)[0].value)];
    }
    CHECK_EQ(groups["cyclic"], 18);
    CHECK_EQ(groups["reverse"], 18);
    CHECK_EQ(orders.size(), 6U);
    for (const auto& [_, count] : orders)
        CHECK_EQ(count, 6);
    for (const auto& [_, count] : seats)
        CHECK_EQ(count, 12);
    const auto summary_json = read_json(out / "experts" / "summary.json");
    const auto& summary = object(summary_json);
    CHECK_EQ(integer(summary, "completed_matches"), 0);
    CHECK_EQ(integer(summary, "aborted_matches"), 36);
    CHECK_EQ(integer(summary, "incomplete_blocks"), 1);
    CHECK_EQ(integer(summary, "complete_blocks"), 0);
    CHECK_EQ(object(summary.at("groups")).size(), 2U);
    CHECK_EQ(object(summary.at("turn_orders")).size(), 6U);
    const auto invocation = read_json(out / "invocation.json");
    CHECK_EQ(std::get<std::string>(object(invocation).at("reference_mode").value), "shared");
    const auto& identities = object(object(invocation).at("checkpoints"));
    CHECK_EQ(identities.size(), 4U);
    for (const auto& [_, identity] : identities) {
        CHECK_EQ(std::get<std::string>(object(identity).at("runtime_model_sha256").value).size(),
                 64U);
        CHECK_EQ(std::get<std::string>(object(identity).at("state_file_sha256").value).size(), 64U);
        CHECK_EQ(std::get<std::string>(object(identity).at("manifest_sha256").value).size(), 64U);
        CHECK_EQ(std::get<std::string>(object(identity).at("resolved_config_sha256").value).size(),
                 64U);
    }
    for (const auto& [role, directory] :
         std::array<std::pair<std::string, std::string>, 4>{{{"cyclic", "cyclic"},
                                                             {"reverse", "reverse"},
                                                             {"shared", "shared"},
                                                             {"shared-control", "control"}}}) {
        const auto checkpoint_root = out.parent_path() / directory;
        const auto checkpoint = diamond_training::inspect_checkpoint_v2(checkpoint_root);
        const auto& identity = object(identities.at(role));
        CHECK_EQ(std::get<std::string>(identity.at("state_file_sha256").value),
                 diamond_support::sha256(read(checkpoint.generation / "state.pt")));
        CHECK_EQ(std::get<std::string>(identity.at("optimizer_file_sha256").value),
                 diamond_support::sha256(read(checkpoint.generation / "optimizer.pt")));
        CHECK_EQ(std::get<std::string>(identity.at("manifest_sha256").value),
                 diamond_support::sha256(read(checkpoint.generation / "manifest.json")));
        CHECK_EQ(std::get<std::string>(identity.at("resolved_config_sha256").value),
                 diamond_support::sha256(checkpoint.provenance->resolved_config_bytes));
        auto model = diamond_model::DiamondModel(8, 1, 9, 3);
        (void)diamond_training::load_checkpoint_v2_weights(checkpoint_root, model,
                                                           diamond_training::resolve_device("cpu"));
        CHECK_EQ(std::get<std::string>(identity.at("runtime_model_sha256").value),
                 diamond_training::canonical_model_digest(model));
    }
    CHECK(diamond_support::canonical_json(
              object(identities.at("shared")).at("runtime_model_sha256")) !=
          diamond_support::canonical_json(
              object(identities.at("shared-control")).at("runtime_model_sha256")));
    CHECK_EQ(std::get<std::string>(object(invocation).at("status").value), "complete");
}
} // namespace

int main(int argc, char** argv) {
    REQUIRE(argc == 3, "usage: min_expert_arena_cli_test <scratch-prefix> <min_expert_arena>");
    torch::set_num_threads(1);
    const Path scratch =
        std::string(argv[1]) + "-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(scratch);
    auto arena = config();
    const Path config_path = scratch / "arena.json";
    write(config_path, arena.to_json());
    checkpoint(scratch / "cyclic", arena, "cyclic", 11);
    checkpoint(scratch / "reverse", arena, "reverse", 13);
    checkpoint(scratch / "shared", arena, "all", 17);
    checkpoint(scratch / "control", arena, "all", 19);
    checkpoint(scratch / "reference-cyclic", arena, "cyclic", 31);
    checkpoint(scratch / "reference-reverse", arena, "reverse", 37);
    const auto before = files(scratch / "cyclic");
    const auto reverse_before = files(scratch / "reverse");
    const auto shared_before = files(scratch / "shared");
    const auto control_before = files(scratch / "control");
    const auto reference_cyclic_before = files(scratch / "reference-cyclic");
    const auto reference_reverse_before = files(scratch / "reference-reverse");
    const auto call = [&](const Path& out, const Path& cyclic, bool control = true) {
        return invoke(argv[2], scratch, cyclic, scratch / "reverse", scratch / "shared",
                      config_path, out, control);
    };
    const auto aborted = scratch / "aborted";
    REQUIRE(call(aborted, scratch / "cyclic") == 0, "CPU expert/control arena must execute");
    check_aborted_schedule(aborted);
    const auto result_before = files(aborted);
    CHECK(call(aborted, scratch / "cyclic") != 0);
    CHECK_EQ(files(aborted), result_before);
    const auto invalid = scratch / "invalid";
    const auto reference_call = [&](const auto& references, const Path& shared = Path{}) {
        return invoke(argv[2], scratch, scratch / "cyclic", scratch / "reverse", shared,
                      config_path, invalid, false, references);
    };
    using Flags = std::vector<std::pair<std::string, Path>>;
    const Flags pair{{"--reference-cyclic-checkpoint", scratch / "reference-cyclic"},
                     {"--reference-reverse-checkpoint", scratch / "reference-reverse"}};
    const auto reject_reference = [&](const Flags& flags, const Path& shared,
                                      std::string_view reason) {
        CHECK(reference_call(flags, shared) != 0);
        CHECK(read(scratch / "cli.log").find(reason) != std::string::npos);
        CHECK(!std::filesystem::exists(invalid));
    };
    reject_reference({}, {}, "reference mode");
    reject_reference({pair[0]}, {}, "reference mode");
    reject_reference({pair[1]}, {}, "reference mode");
    reject_reference(pair, scratch / "shared", "reference mode");
    reject_reference({pair[0]}, scratch / "shared", "reference mode");
    reject_reference({pair[1]}, scratch / "shared", "reference mode");
    reject_reference({pair[0], pair[1], pair[0]}, {}, "duplicate argument");
    reject_reference({pair[0], pair[1], pair[1]}, {}, "duplicate argument");
    reject_reference({{"--reference-cyclic-checkpoint", {}}, pair[1]}, {}, "missing value");
    reject_reference({pair[0], {"--reference-reverse-checkpoint", {}}}, {}, "missing value");
    reject_reference({{"--reference-cyclic-checkpoint", scratch / "shared"}, pair[1]}, {},
                     "turn_order_group must be cyclic");
    reject_reference({pair[0], {"--reference-reverse-checkpoint", scratch / "cyclic"}}, {},
                     "turn_order_group must be reverse");
    const auto pair_out = scratch / "expert-pair";
    REQUIRE(invoke(argv[2], scratch, scratch / "cyclic", scratch / "reverse", {}, config_path,
                   pair_out, true, pair) == 0, "CPU expert-reference arena must execute");
    const auto pair_invocation = read_json(pair_out / "invocation.json");
    CHECK_EQ(std::get<std::string>(object(pair_invocation).at("reference_mode").value),
             "expert-pair");
    CHECK_EQ(std::get<std::string>(object(pair_invocation).at("status").value), "complete");
    const auto& pair_sources = object(object(pair_invocation).at("checkpoints"));
    CHECK_EQ(pair_sources.size(), 5U);
    CHECK(!pair_sources.contains("shared"));
    for (const std::string group : {"cyclic", "reverse"}) {
        const auto& identity = object(pair_sources.at("reference-" + group));
        CHECK_EQ(std::get<std::string>(identity.at("turn_order_group").value), group);
        const auto root = scratch / ("reference-" + group);
        const auto info = diamond_training::inspect_checkpoint_v2(root);
        CHECK_EQ(std::get<std::string>(identity.at("checkpoint_root").value),
                 std::filesystem::canonical(root).string());
        CHECK_EQ(std::get<std::string>(identity.at("generation").value),
                 std::filesystem::canonical(info.generation).string());
        CHECK_EQ(std::get<std::string>(identity.at("state_file_sha256").value), info.model_digest);
        CHECK_EQ(std::get<std::string>(identity.at("manifest_sha256").value),
                 diamond_support::sha256(read(info.generation / "manifest.json")));
        CHECK(diamond_support::canonical_json(identity.at("runtime_model_sha256")) !=
              diamond_support::canonical_json(object(pair_sources.at(group)).at("runtime_model_sha256")));
    }
    for (const std::string arm : {"experts", "shared-control"}) {
        const auto rows = read_json(pair_out / arm / "matches.json");
        CHECK_EQ(std::get<Array>(rows.value).size(), 36U);
        const auto summary = read_json(pair_out / arm / "summary.json");
        CHECK_EQ(integer(object(summary), "scheduled_matches"), 36);
        CHECK_EQ(integer(object(summary), "pending_matches"), 0);
        CHECK_EQ(integer(object(summary), "aborted_matches"), 36);
        const auto& summaries = object(object(summary).at("groups"));
        CHECK_EQ(integer(object(summaries.at("cyclic")), "aborted_matches"), 18);
        CHECK_EQ(integer(object(summaries.at("reverse")), "aborted_matches"), 18);
    }
    CHECK_EQ(read(pair_out / "schedule.json"), read(aborted / "schedule.json"));
    const auto pair_only_out = scratch / "expert-pair-only";
    REQUIRE(invoke(argv[2], scratch, scratch / "cyclic", scratch / "reverse", {}, config_path,
                   pair_only_out, false, pair) == 0,
            "CPU expert-reference arena without control must execute");
    CHECK(!std::filesystem::exists(pair_only_out / "shared-control"));
    const auto pair_only_invocation = read_json(pair_only_out / "invocation.json");
    CHECK_EQ(object(object(pair_only_invocation).at("checkpoints")).size(), 4U);
    CHECK(call(invalid, scratch / "shared") != 0);
    CHECK(!std::filesystem::exists(invalid));
    CHECK(call(invalid, scratch / "reverse") != 0);
    CHECK(!std::filesystem::exists(invalid));
    auto wide = arena;
    wide.network.width = 16;
    checkpoint(scratch / "wide", wide, "cyclic", 23);
    reject_reference({{"--reference-cyclic-checkpoint", scratch / "wide"}, pair[1]}, {},
                     "architecture differs");
    CHECK(call(invalid, scratch / "wide") != 0);
    CHECK(!std::filesystem::exists(invalid));
    checkpoint(scratch / "legacy", arena, "cyclic", 29, true);
    reject_reference({{"--reference-cyclic-checkpoint", scratch / "legacy"}, pair[1]}, {},
                     "checkpoint must be v3");
    CHECK(call(invalid, scratch / "legacy") != 0);
    CHECK(!std::filesystem::exists(invalid));
    arena.arena.games = 72;
    write(config_path, arena.to_json());
    CHECK(call(invalid, scratch / "cyclic") != 0);
    CHECK(!std::filesystem::exists(invalid));
    arena.arena.games = 36;
    arena.arena.max_moves = 1000;
    // Repetition sampling needs more than one visited root action to escape cycles.
    arena.mcts.simulations = 16;
    write(config_path, arena.to_json());
    const auto completed = scratch / "completed";
    REQUIRE(call(completed, scratch / "cyclic", false) == 0, "CPU completion arena must execute");
    const auto completed_summary = read_json(completed / "experts" / "summary.json");
    CHECK(integer(object(completed_summary), "completed_matches") > 0);
    const auto completed_rows = read_json(completed / "experts" / "matches.json");
    CHECK_EQ(std::get<Array>(completed_rows.value).size(), 36U);
    int64_t completed_count = 0;
    for (const auto& row_json : std::get<Array>(completed_rows.value)) {
        const auto& row = object(row_json);
        if (std::get<bool>(row.at("completed").value)) {
            ++completed_count;
            const auto placement = integer(row, "candidate_placement");
            CHECK(placement >= 0 && placement <= 2);
            const auto& finish = std::get<Array>(row.at("finish_order").value);
            const auto& seats = std::get<Array>(row.at("seat_assignment").value);
            CHECK_EQ(finish.size(), 3U);
            std::set<int64_t> ranked_players;
            for (const auto& player : finish)
                ranked_players.insert(std::get<int64_t>(player.value));
            CHECK((ranked_players == std::set<int64_t>{1, 2, 3}));
            if (placement >= 0 && placement <= 2 && finish.size() == 3U)
                CHECK_EQ(std::get<int64_t>(finish[static_cast<std::size_t>(placement)].value),
                         std::get<int64_t>(seats[0].value));
        }
    }
    CHECK(completed_count > 0);
    CHECK_EQ(completed_count, integer(object(completed_summary), "completed_matches"));
    CHECK_EQ(36 - completed_count, integer(object(completed_summary), "aborted_matches"));
    CHECK_EQ(files(scratch / "cyclic"), before);
    CHECK_EQ(files(scratch / "reverse"), reverse_before);
    CHECK_EQ(files(scratch / "shared"), shared_before);
    CHECK_EQ(files(scratch / "control"), control_before);
    CHECK_EQ(files(scratch / "reference-cyclic"), reference_cyclic_before);
    CHECK_EQ(files(scratch / "reference-reverse"), reference_reverse_before);
    if (soo_test::failures() == 0)
        std::filesystem::remove_all(scratch);
    else
        std::cerr << "retained scratch: " << scratch << '\n';
    return soo_test::report("min_expert_arena_cli_test");
}
