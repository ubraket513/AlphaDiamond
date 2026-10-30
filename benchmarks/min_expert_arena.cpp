// Standalone Min comparison executable. It validates v3 checkpoint identities,
// freezes an invocation/schedule in a NEW output directory, then evaluates every
// scheduled seat/order. It never trains, promotes, retries or chooses checkpoints.
// Completed rows and pending counts are evidence; summaries alone are not a
// statistical strength claim. The external experiment analyzer handles that gate.
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "diamond_orchestration/config.hpp"
#include "diamond_orchestration/schedule.hpp"
#include "diamond_orchestration/training_wiring.hpp"
#include "diamond_pipeline/model_pool.hpp"
#include "diamond_support/json.hpp"
#include "diamond_training/checkpoint.hpp"
#include "soo/arena_router.hpp"
#include "soo/board.hpp"
#include "soo/min_experts.hpp"
#include "soo/rules.hpp"

namespace {
using Json = diamond_support::JsonValue;
using Object = Json::Object;
using Array = Json::Array;
using Path = std::filesystem::path;
using Config = diamond_orchestration::ProductionConfig;
constexpr std::string_view kClassifierVersion = "min-geometric-turn-order-v1";

std::string read(const Path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("cannot read " + path.string());
    return {std::istreambuf_iterator<char>(stream), {}};
}

void write_json(const Path& path, const Json& value) {
    const Path temporary = path.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        stream << diamond_support::canonical_json(value) << '\n';
        stream.close();
        if (!stream)
            throw std::runtime_error("cannot write " + path.string());
    }
#ifdef _WIN32
    if (!MoveFileExW(temporary.wstring().c_str(), path.wstring().c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("cannot replace " + path.string());
#else
    std::filesystem::rename(temporary, path);
#endif
}

const Object& object(const Json& value) {
    return std::get<Object>(value.value);
}
int64_t integer(const Object& value, const std::string& key) {
    return std::get<int64_t>(value.at(key).value);
}
Json integers(const auto& values) {
    Array result;
    for (const auto value : values)
        result.push_back(Json{static_cast<int64_t>(value)});
    return Json{std::move(result)};
}

struct Arguments {
    Path cyclic;
    Path reverse;
    std::optional<Path> shared;
    Path config;
    Path out;
    std::optional<Path> control;
    std::optional<Path> reference_cyclic;
    std::optional<Path> reference_reverse;
    std::string shared_group = "all";
    bool paired = false;
};

Arguments arguments(int argc, char** argv) {
    std::map<std::string, Path> values;
    for (int index = 1; index < argc; index += 2) {
        const std::string flag = argv[index];
        if (flag != "--cyclic-checkpoint" && flag != "--reverse-checkpoint" &&
            flag != "--shared-checkpoint" && flag != "--shared-control-checkpoint" &&
            flag != "--reference-cyclic-checkpoint" && flag != "--reference-reverse-checkpoint" &&
            flag != "--shared-turn-order-group" && flag != "--population-mode" &&
            flag != "--config" && flag != "--out")
            throw std::invalid_argument("unknown argument: " + flag);
        if (index + 1 >= argc || std::string_view(argv[index + 1]).starts_with("--") ||
            std::string_view(argv[index + 1]).empty())
            throw std::invalid_argument("missing value for " + flag);
        if (!values.emplace(flag, argv[index + 1]).second)
            throw std::invalid_argument("duplicate argument: " + flag);
    }
    for (const std::string flag :
         {"--cyclic-checkpoint", "--reverse-checkpoint", "--config", "--out"})
        if (!values.contains(flag))
            throw std::invalid_argument("required argument: " + flag);
    const bool shared = values.contains("--shared-checkpoint");
    const bool reference_cyclic = values.contains("--reference-cyclic-checkpoint");
    const bool reference_reverse = values.contains("--reference-reverse-checkpoint");
    if (shared ? (reference_cyclic || reference_reverse) : !(reference_cyclic && reference_reverse))
        throw std::invalid_argument(
            "reference mode requires exactly --shared-checkpoint or both "
            "--reference-cyclic-checkpoint and --reference-reverse-checkpoint");
    Arguments result;
    result.cyclic = values.at("--cyclic-checkpoint");
    result.reverse = values.at("--reverse-checkpoint");
    result.config = values.at("--config");
    result.out = values.at("--out");
    if (shared) {
        result.shared = values.at("--shared-checkpoint");
    } else {
        result.reference_cyclic = values.at("--reference-cyclic-checkpoint");
        result.reference_reverse = values.at("--reference-reverse-checkpoint");
    }
    if (values.contains("--shared-turn-order-group")) {
        if (!shared)
            throw std::invalid_argument("shared turn-order group requires a shared checkpoint");
        result.shared_group = values.at("--shared-turn-order-group").string();
        if (result.shared_group != "all" && result.shared_group != "cyclic" &&
            result.shared_group != "reverse")
            throw std::invalid_argument("shared turn-order group must be all, cyclic, or reverse");
    }
    if (values.contains("--population-mode")) {
        const auto mode = values.at("--population-mode").string();
        if (mode != "single" && mode != "paired")
            throw std::invalid_argument("population mode must be single or paired");
        result.paired = mode == "paired";
    }
    if (values.contains("--shared-control-checkpoint"))
        result.control = values.at("--shared-control-checkpoint");
    if (result.paired && result.control)
        throw std::invalid_argument("paired mode cannot include an additional shared control");
    if (std::filesystem::exists(result.out))
        throw std::invalid_argument("output directory must be new: " + result.out.string());
    return result;
}

diamond_training::Compatibility compatibility(const Config& config) {
    return diamond_training::Compatibility::min(
        config.model_version, {config.network.residual_blocks, config.network.width},
        soo::EncodingVersion::goals_v2);
}

Json compatibility_json(const diamond_training::Compatibility& value) {
    return Json{Object{{"action_space_version", Json{value.action_space_version}},
                       {"board_topology_version", Json{value.board_topology_version}},
                       {"encoder_version", Json{value.encoder_version}},
                       {"model_name", Json{value.model_name}},
                       {"model_version", Json{value.model_version}},
                       {"network_config",
                        Json{Object{{"residual_blocks", Json{value.network_config.residual_blocks}},
                                    {"width", Json{value.network_config.width}}}}},
                       {"player_count", Json{value.player_count}},
                       {"ruleset_fingerprint", Json{value.ruleset_fingerprint}},
                       {"ruleset_version", Json{value.ruleset_version}},
                       {"seat_layout_version", Json{value.seat_layout_version}},
                       {"value_semantics_version", Json{value.value_semantics_version}}}};
}

void require_min_goals(const Config& config) {
    if (config.model_name != "Min" ||
        config.encoder_version != soo::encoding_version_name(soo::EncodingVersion::goals_v2))
        throw std::invalid_argument("Min goals-v2 configuration is required");
}

diamond_pipeline::InferencePrecision precision(const Config& config) {
    if (config.runtime.precision == "fp16")
        return diamond_pipeline::InferencePrecision::fp16;
    if (config.runtime.precision == "bf16")
        return diamond_pipeline::InferencePrecision::bf16;
    return diamond_pipeline::InferencePrecision::fp32;
}

struct LoadedCheckpoint {
    std::unique_ptr<diamond_pipeline::ModelPool> pool;
    Json identity;
};

// Require the requested expert group and exact architecture before installing a
// detached actor snapshot. Weights-only load does not restore AdamW, but checkpoint
// inspection still verifies the optimizer file hash. State archive hash, runtime
// tensor digest and configuration digest identify different objects; record all.
// Recheck generation/manifest after load rather than accepting a changed CURRENT.
LoadedCheckpoint load_checkpoint(const Path& root, std::string_view expected_group,
                                 const Config& arena,
                                 const diamond_training::ResolvedDevice& device) {
    const auto info = diamond_training::inspect_checkpoint_v2(root);
    if (info.format_version != 3 || !info.provenance)
        throw std::invalid_argument(
            "checkpoint must be v3 with resolved configuration provenance: " + root.string());
    const auto source =
        Config::from_json(diamond_support::parse_json(info.provenance->resolved_config_bytes));
    require_min_goals(source);
    if (source.self_play.turn_order_group != expected_group)
        throw std::invalid_argument("checkpoint turn_order_group must be " +
                                    std::string(expected_group) + ": " + root.string());
    const auto manifest_bytes = read(info.generation / "manifest.json");
    const auto manifest = diamond_support::parse_json(manifest_bytes);
    const auto& provenance = object(object(manifest).at("provenance"));
    const auto& architecture = object(provenance.at("architecture"));
    const auto wire = compatibility(source);
    if (integer(architecture, "width") != source.network.width ||
        integer(architecture, "residual_blocks") != source.network.residual_blocks ||
        integer(architecture, "input_features") != wire.input_features() ||
        integer(architecture, "value_size") != 3 ||
        diamond_support::canonical_json(provenance.at("compatibility")) !=
            diamond_support::canonical_json(compatibility_json(wire)))
        throw std::invalid_argument(
            "checkpoint provenance disagrees with its resolved Min architecture: " + root.string());
    auto model = diamond_model::DiamondModel(source.network.width, source.network.residual_blocks,
                                             wire.input_features(), 3);
    const auto loaded = diamond_training::load_checkpoint_v2_weights(root, model, device);
    if (loaded.generation != info.generation || loaded.model_digest != info.model_digest ||
        read(info.generation / "manifest.json") != manifest_bytes)
        throw std::runtime_error("checkpoint changed while loading: " + root.string());
    if (!torch::equal(model->adjacency.to(torch::kCPU), diamond_model::topology_adjacency()))
        throw std::invalid_argument("checkpoint board adjacency is not canonical: " +
                                    root.string());
    auto pool = std::make_unique<diamond_pipeline::ModelPool>(1, device, precision(arena));
    const auto key = pool->install(wire, model);
    pool->activate(key);
    return {std::move(pool),
            Json{Object{{"checkpoint_root", Json{std::filesystem::canonical(root).string()}},
                        {"generation", Json{std::filesystem::canonical(info.generation).string()}},
                        {"manifest_sha256", Json{diamond_support::sha256(manifest_bytes)}},
                        {"optimizer_file_sha256", Json{info.optimizer_digest}},
                        {"state_file_sha256", Json{info.model_digest}},
                        {"runtime_model_sha256", Json{key.checkpoint_sha256}},
                        {"resolved_config_sha256",
                         Json{diamond_support::sha256(info.provenance->resolved_config_bytes)}},
                        {"resolved_config", source.to_json()},
                        {"source_git_commit", Json{info.provenance->source_git_commit}},
                        {"training_step", Json{static_cast<int64_t>(info.training_step)}},
                        {"turn_order_group", Json{std::string(expected_group)}}}}};
}

soo::Match ordered_match(const std::array<int, 3>& order) {
    auto match = soo::standard_min_match();
    const auto players = match.players;
    for (std::size_t index = 0; index < order.size(); ++index) {
        const auto player = std::find_if(
            players.begin(), players.begin() + match.count,
            [&](const soo::PlayerSpec& candidate) { return candidate.id == order[index]; });
        if (player == players.begin() + match.count)
            throw std::invalid_argument("arena schedule contains an unknown player");
        match.players[index] = *player;
    }
    return match;
}

// Materialize a descriptor with legal random moves using its seed and ply depth.
// The same descriptor is deliberately reused across balanced assignments. Its
// metadata hash alone is not a board-content hash or proof of held-out freshness.
soo::State opening(const soo::Match& match,
                   const diamond_orchestration::MaterializedOpening& descriptor) {
    descriptor.validate();
    soo::State state;
    for (std::size_t seat = 0; seat < match.count; ++seat)
        for (const auto position : soo::topology().camp_positions[match.players[seat].camp])
            state.occupancy[position] = match.players[seat].id;
    state.current_player = match.players[0].id;
    std::mt19937_64 rng(descriptor.seed);
    for (int64_t ply = 0; ply < descriptor.depth; ++ply) {
        std::vector<int32_t> legal;
        soo::legal_action_ids(state, legal);
        if (legal.empty())
            throw std::runtime_error("opening descriptor reached a terminal state early");
        std::uniform_int_distribution<std::size_t> choice(0, legal.size() - 1);
        state = soo::apply_action(state, match, legal[choice(rng)]);
    }
    return state;
}

struct ScheduledGame {
    diamond_orchestration::MaterializedOpening opening;
    diamond_orchestration::MinRatedMatch matchup;
    uint64_t seed;
    std::string group;
};

std::vector<ScheduledGame> schedule(const diamond_orchestration::MaterializedOpeningSuite& suite) {
    // IDs are protocol roles. Checkpoint identities must not enter match IDs or seeds.
    const auto blocks = diamond_orchestration::schedule_min_opening_blocks(
        suite, {"candidate", "reference-a", "reference-b"});
    std::vector<ScheduledGame> games;
    for (const auto& block : blocks) {
        for (const auto& matchup : block.matches) {
            const auto match = ordered_match(matchup.turn_order);
            const auto digest = diamond_support::sha256(diamond_support::canonical_json(
                Json{Object{{"match_id", Json{matchup.match_id}},
                            {"opening_seed", Json{std::to_string(block.opening.seed)}}}}));
            games.push_back(
                {block.opening, matchup, std::stoull(digest.substr(0, 16), nullptr, 16),
                 std::string(soo::min_turn_order_group_name(soo::min_turn_order_group(match)))});
        }
    }
    return games;
}

Json schedule_row(const ScheduledGame& game) {
    return Json{Object{{"group", Json{game.group}},
                       {"match_id", Json{game.matchup.match_id}},
                       {"opening_id", Json{game.opening.opening_id}},
                       {"opening_depth", Json{game.opening.depth}},
                       {"opening_seed", Json{std::to_string(game.opening.seed)}},
                       {"opening_state_sha256", Json{game.opening.state_sha256}},
                       {"seed", Json{std::to_string(game.seed)}},
                       {"seat_assignment", integers(game.matchup.seat_assignment)},
                       {"turn_order", integers(game.matchup.turn_order)}}};
}

Json outcome(const ScheduledGame& game, const soo::Episode& episode) {
    auto result = object(schedule_row(game));
    std::optional<int64_t> placement;
    if (episode.completed) {
        if (episode.finish_order.size() != 3)
            throw std::runtime_error("completed Min episode must contain all three placements");
        const auto candidate = std::find(episode.finish_order.begin(), episode.finish_order.end(),
                                         game.matchup.seat_assignment[0]);
        if (candidate == episode.finish_order.end())
            throw std::runtime_error("completed Min episode omitted candidate placement");
        placement = std::distance(episode.finish_order.begin(), candidate);
    }
    result.emplace("candidate_placement", placement ? Json{*placement} : Json{nullptr});
    const auto first = episode.diagnostics.first_finisher;
    if (first && std::find(game.matchup.seat_assignment.begin(), game.matchup.seat_assignment.end(),
                           *first) == game.matchup.seat_assignment.end())
        throw std::runtime_error("Min episode has an unknown first finisher");
    if (episode.completed && (!first || *first != episode.finish_order.front()))
        throw std::runtime_error("completed Min episode has inconsistent first finisher");
    result.emplace("first_finisher", first ? Json{int64_t{*first}} : Json{nullptr});
    result.emplace("candidate_won",
                   first ? Json{*first == game.matchup.seat_assignment[0]} : Json{nullptr});
    result.emplace("completed", Json{episode.completed});
    result.emplace("finish_order", integers(episode.finish_order));
    result.emplace("move_count", Json{static_cast<int64_t>(episode.move_count)});
    result.emplace("abort_reason", episode.completed
                                       ? Json{nullptr}
                                       : Json{episode.move_limit_exceeded         ? "move_limit"
                                              : episode.max_game_seconds_exceeded ? "game_deadline"
                                                                                  : "unfinished"});
    return Json{std::move(result)};
}

struct Counts {
    int64_t scheduled = 0;
    int64_t completed = 0;
    int64_t aborted = 0;
    std::array<int64_t, 3> placements{};
    void add(const std::optional<Json>& row) {
        ++scheduled;
        if (!row)
            return;
        const auto& value = object(*row);
        if (std::get<bool>(value.at("completed").value)) {
            ++completed;
            ++placements.at(static_cast<std::size_t>(integer(value, "candidate_placement")));
        } else
            ++aborted;
    }
    Json mean_score() const {
        return completed == 0
                   ? Json{nullptr}
                   : Json{(static_cast<double>(placements[0]) + 0.5 * placements[1]) / completed};
    }
    Json json() const {
        return Json{Object{{"scheduled_matches", Json{scheduled}},
                           {"completed_matches", Json{completed}},
                           {"aborted_matches", Json{aborted}},
                           {"pending_matches", Json{scheduled - completed - aborted}},
                           {"placements", integers(placements)},
                           {"mean_score_completed", mean_score()}}};
    }
};

// Rewrite snapshots after each whole turn-order group. A missing optional result
// is pending, not a loss or abort. Each JSON file is replaced separately, so readers
// may briefly see adjacent snapshots; invocation status becomes complete only once
// all groups return. No partial snapshot implies successful completion.
void save_arm(const Path& out, const std::vector<ScheduledGame>& games,
              const std::vector<std::optional<Json>>& results, double seconds) {
    Counts total;
    std::map<std::string, Counts> groups;
    std::map<std::string, Counts> orders;
    std::map<std::string, Counts> openings;
    Array rows;
    for (std::size_t index = 0; index < games.size(); ++index) {
        const auto& game = games[index];
        const auto& result = results[index];
        total.add(result);
        groups[game.group].add(result);
        orders[diamond_support::canonical_json(integers(game.matchup.turn_order))].add(result);
        openings[game.opening.opening_id].add(result);
        if (result)
            rows.push_back(*result);
    }
    auto summary = object(total.json());
    const auto as_object = [](const auto& values) {
        Object result;
        for (const auto& [key, value] : values)
            result.emplace(key, value.json());
        return Json{std::move(result)};
    };
    int64_t complete_blocks = 0;
    Array complete_scores;
    for (const auto& [_, counts] : openings)
        if (counts.completed == counts.scheduled) {
            ++complete_blocks;
            complete_scores.push_back(counts.mean_score());
        }
    summary.emplace("groups", as_object(groups));
    summary.emplace("turn_orders", as_object(orders));
    summary.emplace("opening_blocks", as_object(openings));
    summary.emplace("complete_blocks", Json{complete_blocks});
    summary.emplace("incomplete_blocks",
                    Json{static_cast<int64_t>(openings.size()) - complete_blocks});
    summary.emplace("complete_block_scores", Json{std::move(complete_scores)});
    summary.emplace("candidate_placement_index_base", Json{int64_t{0}});
    summary.emplace("elapsed_seconds", Json{seconds});
    write_json(out / "matches.json", Json{std::move(rows)});
    write_json(out / "summary.json", Json{std::move(summary)});
}

// One synchronous scheduler run per turn order keeps each Match immutable while
// lanes execute in parallel. Pools and borrowed router references live throughout
// the call. wire_arena_episode fixes evaluation exploration/repetition policy;
// it is intentionally different from training self-play policy. Any failure escapes
// without starting a replacement run; already published snapshots remain evidence.
void run_arm(const Path& out, const Config& config, const std::vector<ScheduledGame>& games,
             soo::BatchEvaluator& candidate, soo::BatchEvaluator& reference) {
    std::filesystem::create_directory(out);
    std::vector<std::optional<Json>> results(games.size());
    std::map<std::array<int, 3>, std::vector<std::size_t>> orders;
    for (std::size_t index = 0; index < games.size(); ++index)
        orders[games[index].matchup.turn_order].push_back(index);
    const auto start = std::chrono::steady_clock::now();
    const auto elapsed = [&] {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    };
    save_arm(out, games, results, elapsed());
    for (const auto& [order, indices] : orders) {
        const auto match = ordered_match(order);
        std::vector<soo::EpisodeJob> jobs;
        std::vector<int> owners;
        for (const auto index : indices) {
            const auto& game = games[index];
            jobs.push_back({opening(match, game.opening), game.seed});
            owners.push_back(game.matchup.seat_assignment[0]);
        }
        soo::ArenaModelRouter evaluator(candidate, reference, std::move(owners));
        soo::EpisodeMetrics metrics;
        auto played = soo::run_episodes(
            match, jobs, diamond_orchestration::wire_arena_episode(config, jobs.size()), evaluator,
            metrics);
        if (played.size() != indices.size())
            throw std::runtime_error("arena returned wrong episode count");
        for (std::size_t index = 0; index < indices.size(); ++index)
            results[indices[index]] = outcome(games[indices[index]], played[index]);
        save_arm(out, games, results, elapsed());
        std::cout << out.filename().string()
                  << " order=" << diamond_support::canonical_json(integers(order))
                  << " games=" << indices.size() << " elapsed_seconds=" << elapsed() << '\n';
        std::cout.flush();
    }
}

// Explicit reference modes are mutually exclusive: shared model or cyclic/reverse
// pair. Optional shared-control is another requested arm, not an error fallback.
// Load/validate before creating output; retain failed invocation evidence on error.
// Torch CPU threads are fixed to one here; scheduler parallelism comes from config.
void run(const Arguments& args) {
    const auto config_bytes = read(args.config);
    const auto config = Config::from_json(diamond_support::parse_json(config_bytes));
    require_min_goals(config);
    if (config.self_play.turn_order_group != "all")
        throw std::invalid_argument("arena configuration must cover all turn-order groups");
    soo::ensure_topology_configured();
    const auto suite = diamond_orchestration::materialize_opening_suite(config.opening_suite);
    const auto games = schedule(suite);
    if (static_cast<int64_t>(games.size()) != config.arena.games)
        throw std::invalid_argument(
            "arena.games must equal the complete Min opening schedule size");
    const auto device = diamond_training::resolve_device(config.runtime.device);
    torch::set_num_threads(1);
    auto cyclic = load_checkpoint(args.cyclic, "cyclic", config, device);
    auto reverse = load_checkpoint(args.reverse, "reverse", config, device);
    std::optional<LoadedCheckpoint> shared;
    std::optional<LoadedCheckpoint> reference_cyclic;
    std::optional<LoadedCheckpoint> reference_reverse;
    std::optional<soo::MinExpertRouter> reference_experts;
    if (args.shared) {
        shared = load_checkpoint(*args.shared, args.shared_group, config, device);
    } else {
        reference_cyclic = load_checkpoint(*args.reference_cyclic, "cyclic", config, device);
        reference_reverse = load_checkpoint(*args.reference_reverse, "reverse", config, device);
        reference_experts.emplace(*reference_cyclic->pool, *reference_reverse->pool);
    }
    soo::BatchEvaluator& reference =
        shared ? static_cast<soo::BatchEvaluator&>(*shared->pool) : *reference_experts;
    std::optional<LoadedCheckpoint> control;
    if (args.control)
        control = load_checkpoint(*args.control, "all", config, device);
    // All validation and weight loading precede creating the invocation directory.
    if (!args.out.parent_path().empty())
        std::filesystem::create_directories(args.out.parent_path());
    if (!std::filesystem::create_directory(args.out))
        throw std::invalid_argument("output directory must be new: " + args.out.string());
    Object sources{{"cyclic", cyclic.identity}, {"reverse", reverse.identity}};
    if (shared) {
        sources.emplace("shared", shared->identity);
    } else {
        sources.emplace("reference-cyclic", reference_cyclic->identity);
        sources.emplace("reference-reverse", reference_reverse->identity);
    }
    if (control)
        sources.emplace("shared-control", control->identity);
    Object invocation{
        {"schema_version", Json{int64_t{1}}},
        {"status", Json{"running"}},
        {"reference_mode", Json{shared ? "shared" : "expert-pair"}},
        {"population_mode", Json{args.paired ? "paired" : "single"}},
        {"classifier_version", Json{std::string(kClassifierVersion)}},
        {"config", config.to_json()},
        {"config_file_sha256", Json{diamond_support::sha256(config_bytes)}},
        {"resolved_config_sha256",
         Json{diamond_support::sha256(diamond_support::canonical_json(config.to_json()))}},
        {"resolved_device", Json{device.canonical_name}},
        {"inference_precision", Json{config.runtime.precision}},
        {"torch_threads", Json{int64_t{1}}},
        {"checkpoints", Json{std::move(sources)}},
        {"opening_suite_sha256", Json{suite.suite_sha256}},
        {"candidate_placement_index_base", Json{int64_t{0}}}};
    Array schedule_rows;
    for (const auto& game : games)
        schedule_rows.push_back(schedule_row(game));
    write_json(args.out / "schedule.json", Json{std::move(schedule_rows)});
    write_json(args.out / "invocation.json", Json{invocation});
    try {
        soo::MinExpertRouter experts(*cyclic.pool, *reverse.pool);
        run_arm(args.out / "experts", config, games, experts, reference);
        if (args.paired)
            run_arm(args.out / "reference", config, games, reference, experts);
        if (control)
            run_arm(args.out / "shared-control", config, games, *control->pool, reference);
        invocation.insert_or_assign("status", Json{"complete"});
        write_json(args.out / "invocation.json", Json{invocation});
    } catch (const std::exception& error) {
        invocation.insert_or_assign("status", Json{"failed"});
        invocation.emplace("error", Json{std::string(error.what())});
        write_json(args.out / "invocation.json", Json{invocation});
        throw;
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout << "min_expert_arena --cyclic-checkpoint DIR --reverse-checkpoint DIR "
                         "(--shared-checkpoint DIR | --reference-cyclic-checkpoint DIR "
                         "--reference-reverse-checkpoint DIR) --config FILE --out NEW_DIR "
                         "[--shared-control-checkpoint DIR] "
                         "[--shared-turn-order-group all|cyclic|reverse] "
                         "[--population-mode single|paired]\n";
            return 0;
        }
        run(arguments(argc, argv));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "min_expert_arena: " << error.what() << '\n';
        return 1;
    }
}
