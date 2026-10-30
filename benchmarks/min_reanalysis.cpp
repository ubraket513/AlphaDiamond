// Collect difficult positions and prepare bounded replay from native continuations.
// Only real terminal rankings produce targets; no checkpoint is selected here.
#include "diamond_orchestration/config.hpp"
#include "diamond_pipeline/pipeline.hpp"
#include "diamond_pipeline/replay_store.hpp"
#include "diamond_support/json.hpp"
#include "diamond_training/checkpoint.hpp"
#include "soo/board.hpp"
#include "soo/rules.hpp"
#include "weakness_positions.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using Json = diamond_support::JsonValue;
using Object = Json::Object;
using Array = Json::Array;
using Path = std::filesystem::path;

void require(bool condition, const char* message) {
    if (!condition)
        throw std::invalid_argument(message);
}
std::string read(const Path& path) {
    std::ifstream stream(path, std::ios::binary);
    require(stream.good(), "cannot open input");
    return {std::istreambuf_iterator<char>(stream), {}};
}
const Object& object(const Json& value) {
    return std::get<Object>(value.value);
}
const Array& array(const Json& value) {
    return std::get<Array>(value.value);
}
int64_t integer(const Json& value) {
    return std::get<int64_t>(value.value);
}
const std::string& string(const Json& value) {
    return std::get<std::string>(value.value);
}
void write(const Path& path, const Json& value) {
    std::ofstream stream(path);
    stream << diamond_support::canonical_json(value) << '\n';
    require(stream.good(), "cannot write experiment artifact");
}

struct Endpoint {
    soo::State state;
    soo::Match match;
    std::string identity;
};

std::vector<Endpoint> endpoints(const Json& input, std::string_view group) {
    require(group == "cyclic" || group == "reverse", "unknown order group");
    const auto& header = object(input);
    const auto schema = header.contains("schema_version") ? integer(header.at("schema_version")) : 2;
    require(schema == 1 || schema == 2, "unsupported endpoint schema");
    std::vector<Endpoint> result;
    std::set<std::string> seen;
    const auto base = soo::standard_min_match();
    for (const auto& row : array(object(input).at("endpoints"))) {
        const auto& entry = object(row);
        if (string(entry.at("group")) != group)
            continue;
        const auto& saved = object(entry.at("state"));
        Endpoint item;
        item.identity = string(entry.at("state_sha256"));
        auto serialized = diamond_support::canonical_json(entry.at("state"));
        if (schema == 1) {
            // The audited legacy archive used Python's default JSON separators.
            // Saved states contain only numeric values and fixed field names.
            std::string spaced;
            for (const char c : serialized) {
                spaced += c;
                if (c == ',' || c == ':') spaced += ' ';
            }
            serialized = std::move(spaced);
        }
        require(diamond_support::sha256(serialized) == item.identity,
                "saved-state SHA256 mismatch");
        const auto& cells = array(saved.at("occupancy"));
        require(cells.size() == soo::kBoardSize, "invalid occupancy size");
        for (std::size_t i = 0; i < cells.size(); ++i) {
            const auto held = integer(cells[i]);
            require(held >= 0 && held <= 3, "invalid piece id");
            item.state.occupancy[i] = static_cast<uint8_t>(held);
        }
        for (uint8_t id = 1; id <= 3; ++id)
            require(std::count(item.state.occupancy.begin(), item.state.occupancy.end(), id) == 10,
                    "invalid piece count");
        const auto current = integer(saved.at("current_player"));
        require(current >= 1 && current <= 3, "invalid current player");
        const auto turn = integer(saved.at("turn_number"));
        require(schema != 1 || turn == 501, "legacy cutoff must be turn501");
        require(turn > 0 && turn < std::numeric_limits<uint16_t>::max() &&
                    integer(saved.at("status")) == soo::kInProgress,
                "expected native in-progress restart state");
        item.state.current_player = static_cast<uint8_t>(current);
        item.state.turn_number = static_cast<uint16_t>(turn);
        const auto& placed = array(saved.at("finish_order"));
        require(placed.size() <= 1, "cutoff cannot have two placed players");
        for (const auto& value : placed) {
            const auto id = integer(value);
            require(id >= 1 && id <= 3 && id != current, "invalid placed player");
            item.state.place(static_cast<uint8_t>(id));
        }
        const auto& players = array(saved.at("players"));
        require(players.size() == 3, "invalid match size");
        item.match.count = 3;
        std::set<int64_t> ids;
        int inversions = 0;
        for (std::size_t seat = 0; seat < players.size(); ++seat) {
            const auto& player = object(players[seat]);
            const auto id = integer(player.at("id"));
            require(id >= 1 && id <= 3 && ids.insert(id).second, "invalid match players");
            const auto spec = base.by_id(static_cast<uint8_t>(id));
            require(integer(player.at("camp")) == spec.camp &&
                        integer(player.at("target_camp")) == spec.target_camp,
                    "nonstandard Min camps");
            item.match.players[seat] = spec;
            for (std::size_t prior = 0; prior < seat; ++prior)
                inversions += item.match.players[prior].id > id;
            const auto& target = soo::topology().camp_positions[spec.target_camp];
            const bool full = std::all_of(target.begin(), target.end(), [&](auto cell) {
                return item.state.occupancy[cell] == id;
            });
            require(full == item.state.has_placed(spec.id), "finish state disagrees with board");
        }
        require((inversions % 2 == 0) == (group == "cyclic"), "order group disagrees with match");
        const auto key = soo::dynamics_key(item.state);
        require(std::to_string(key) == string(entry.at("verified_dynamics_key")),
                "endpoint fingerprint mismatch");
        std::string identity = std::to_string(key);
        for (int seat = 0; seat < item.match.count; ++seat)
            identity += ':' + std::to_string(item.match.players[seat].id);
        require(seen.insert(identity).second, "duplicate endpoint");
        std::vector<int32_t> legal;
        soo::legal_action_ids(item.state, legal);
        require(!legal.empty(), "endpoint has no legal actions");
        result.push_back(std::move(item));
    }
    require(!result.empty(), "no endpoints in group");
    return result;
}

std::string position_key(const soo::State& state, const soo::Match& match) {
    std::string key = std::to_string(soo::dynamics_key(state));
    for (int seat = 0; seat < match.count; ++seat)
        key += ':' + std::to_string(match.players[seat].id);
    return key;
}

Json saved_state(const soo::State& state, const soo::Match& match) {
    Array cells, placed, players;
    for (const auto cell : state.occupancy) cells.emplace_back(Json{int64_t{cell}});
    for (int i = 0; i < state.finished_count; ++i)
        placed.emplace_back(Json{int64_t{state.finish_order[i]}});
    for (int i = 0; i < match.count; ++i) {
        const auto& player = match.players[i];
        players.emplace_back(Json{Object{{"id", Json{int64_t{player.id}}},
            {"camp", Json{int64_t{player.camp}}},
            {"target_camp", Json{int64_t{player.target_camp}}}}});
    }
    return Json{Object{{"occupancy", Json{cells}}, {"finish_order", Json{placed}},
        {"players", Json{players}}, {"current_player", Json{int64_t{state.current_player}}},
        {"turn_number", Json{int64_t{state.turn_number}}},
        {"status", Json{int64_t{state.status}}}}};
}

void collect(const Path& run, const Path& output) {
    require(!std::filesystem::exists(output), "collection output must be a new directory");
    const auto config_bytes = read(run / "resolved-config.json");
    const auto config = diamond_orchestration::ProductionConfig::from_json(
        diamond_support::parse_json(config_bytes));
    const auto group = config.self_play.turn_order_group;
    require(config.model_name == "Min" && config.encoder_version == "diamond-camp-relative-goals-v2" &&
            config.self_play.balance_turn_orders && (group == "cyclic" || group == "reverse"),
            "collection requires balanced Min goals-v2 source games");
    const auto wire = diamond_training::Compatibility::min(config.model_version,
        {config.network.residual_blocks, config.network.width}, soo::EncodingVersion::goals_v2);
    const auto match = soo::standard_min_match();
    soo::State start;
    for (int seat = 0; seat < match.count; ++seat)
        for (auto cell : soo::topology().camp_positions[match.players[seat].camp])
            start.occupancy[cell] = match.players[seat].id;
    start.current_player = match.players[0].id;
    std::vector<soo::EpisodeJob> jobs(static_cast<size_t>(config.workers.games_per_iteration),
                                     {start, 0, std::nullopt});
    soo::balance_episode_turn_orders(match, jobs, soo::min_turn_order_group_from_name(group),
                                     config.self_play.turn_order_weights);
    std::vector<Path> iterations;
    for (const auto& entry : std::filesystem::directory_iterator(run / "iterations"))
        if (entry.is_directory() && std::filesystem::exists(entry.path() / "self-play.json"))
            iterations.push_back(entry.path());
    std::sort(iterations.begin(), iterations.end(), [](const Path& a, const Path& b) {
        return std::stoull(a.filename().string()) < std::stoull(b.filename().string());
    });
    struct Position { Json row; std::string key, family, phase; bool validation; };
    std::vector<Position> positions;
    std::set<std::string> hard_families, validation_keys;
    Array sources;
    int64_t completed = 0, unresolved = 0;
    for (const auto& iteration : iterations) {
        const auto report = object(diamond_support::parse_json(read(iteration / "self-play.json")));
        const auto operation = string(report.at("operation_id"));
        const auto artifact = iteration / "selfplay.episodes";
        const auto artifact_sha = diamond_support::sha256(read(artifact));
        const auto episodes = diamond_pipeline::load_episode_artifact(artifact, operation, wire);
        require(episodes.size() == jobs.size(), "source episode count disagrees with config");
        const auto initialization = object(diamond_support::parse_json(read(iteration / "initialize.json")));
        sources.emplace_back(Json{Object{{"artifact", Json{std::filesystem::canonical(artifact).string()}},
            {"sha256", Json{artifact_sha}}, {"operation_id", Json{operation}},
            {"source_model_digest", initialization.at("champion_sha256")},
            {"aborted_games", report.at("aborted_games")}}});
        for (size_t game = 0; game < episodes.size(); ++game) {
            const auto& episode = episodes[game];
            if (!episode.completed) { ++unresolved; continue; }
            ++completed;
            require(episode.samples.size() == episode.move_count, "source trajectory is incomplete");
            const auto& ordered = *jobs[game].match;
            require(recover_min_state(episode.samples.front(), ordered, 1, episode.final_order) ==
                        jobs[game].initial_state, "source game does not begin at native opening");
            const bool validation = std::stoul(diamond_support::sha256(episode.game_id).substr(0, 8),
                                               nullptr, 16) % 5 == 0;
            for (const auto index : weakness_positions(episode, ordered)) {
                const auto state = recover_min_state(episode.samples[index], ordered,
                    static_cast<uint16_t>(index + 1), episode.final_order);
                const auto saved = saved_state(state, ordered);
                const auto key = position_key(state, ordered);
                const std::string phase = state.finished_count ? "completion" : "pre_first";
                Json row{Object{{"group", Json{group}}, {"state", saved},
                    {"state_sha256", Json{diamond_support::sha256(diamond_support::canonical_json(saved))}},
                    {"verified_dynamics_key", Json{std::to_string(soo::dynamics_key(state))}},
                    {"source_game_id", Json{episode.game_id}},
                    {"source_artifact_sha256", Json{artifact_sha}},
                    {"source_move", Json{static_cast<int64_t>(index)}},
                    {"source_game_moves", Json{static_cast<int64_t>(episode.move_count)}},
                    {"phase", Json{phase}}}};
                positions.push_back({std::move(row), key, episode.game_id, phase, validation});
                hard_families.insert(episode.game_id);
                if (validation) validation_keys.insert(key);
            }
        }
    }
    std::set<std::string> excluded, seen;
    for (const auto& position : positions)
        if (!position.validation && validation_keys.contains(position.key))
            excluded.insert(position.family);
    Array train, validation, completion, retained;
    for (const auto& position : positions) {
        if (excluded.contains(position.family) || !seen.insert(position.key).second) continue;
        if (position.phase == "completion") completion.push_back(position.row);
        else if (position.validation) validation.push_back(position.row);
        else train.push_back(position.row);
    }
    require(!train.empty() && !validation.empty(), "need both training and held-out weakness families");
    std::filesystem::create_directories(output);
    write(output / "train-endpoints.json", Json{Object{
        {"schema_version", Json{int64_t{2}}}, {"endpoints", Json{train}}}});
    write(output / "validation-endpoints.json", Json{Object{
        {"schema_version", Json{int64_t{2}}}, {"endpoints", Json{validation}}}});
    write(output / "completion-endpoints.json", Json{Object{
        {"schema_version", Json{int64_t{2}}}, {"endpoints", Json{completion}}}});
    // Keep normal replay for retention, excluding every weakness family and held-out state.
    diamond_pipeline::ReplayStore replay(output / "regular-replay", wire, config.replay.capacity,
                                         config.replay.seed);
    size_t normal_rows = 0;
    for (const auto& iteration : iterations) {
        if (normal_rows >= 100000) break;
        const auto report = object(diamond_support::parse_json(read(iteration / "self-play.json")));
        auto episodes = diamond_pipeline::load_episode_artifact(iteration / "selfplay.episodes",
            string(report.at("operation_id")), wire);
        std::vector<diamond_pipeline::Episode> normal;
        for (size_t game = 0; game < episodes.size(); ++game) {
            auto& episode = episodes[game];
            if (!episode.completed || hard_families.contains(episode.game_id)) continue;
            bool overlaps = false;
            for (size_t i = 0; i < episode.samples.size(); ++i) {
                const auto state = recover_min_state(episode.samples[i], *jobs[game].match,
                    static_cast<uint16_t>(i + 1), episode.final_order);
                if (validation_keys.contains(position_key(state, *jobs[game].match))) {
                    overlaps = true; break;
                }
            }
            if (overlaps) { excluded.insert(episode.game_id); continue; }
            retained.emplace_back(Json{episode.game_id});
            normal_rows += episode.samples.size();
            normal.push_back(std::move(episode));
            if (normal_rows >= 100000) break;
        }
        if (!normal.empty()) (void)replay.ingest_iteration(normal);
    }
    Array excluded_ids;
    for (const auto& family : excluded) excluded_ids.emplace_back(Json{family});
    write(output / "manifest.json", Json{Object{{"schema_version", Json{int64_t{1}}},
        {"source_run", Json{std::filesystem::canonical(run).string()}},
        {"source_config_sha256", Json{diamond_support::sha256(config_bytes)}},
        {"sources", Json{sources}}, {"completed_source_games", Json{completed}},
        {"unresolved_source_games", Json{unresolved}},
        {"training_positions", Json{static_cast<int64_t>(train.size())}},
        {"validation_positions", Json{static_cast<int64_t>(validation.size())}},
        {"completion_positions", Json{static_cast<int64_t>(completion.size())}},
        {"normal_samples", Json{static_cast<int64_t>(normal_rows)}},
        {"normal_game_ids", Json{retained}}, {"excluded_game_ids", Json{excluded_ids}},
        {"split", Json{std::string("sha256(original_game_id)[0:8] modulo5; zero validation-state overlap")}},
        {"replay_manifest_sha256", Json{replay.manifest_digest()}}}});
    std::cout << train.size() << " training, " << validation.size() << " held-out, "
              << completion.size() << " completion positions; " << normal_rows << " retention rows\n";
}

void prepare(const Path& archive, const Path& training_run, const Path& validation_run,
             const Path& output) {
    require(!std::filesystem::exists(output), "dataset output must be a new directory");
    const auto manifest_bytes = read(archive / "manifest.json");
    const auto manifest = object(diamond_support::parse_json(manifest_bytes));
    const auto config_bytes = read(Path(string(manifest.at("source_run"))) / "resolved-config.json");
    require(diamond_support::sha256(config_bytes) == string(manifest.at("source_config_sha256")),
            "source config changed after collection");
    const auto config = diamond_orchestration::ProductionConfig::from_json(
        diamond_support::parse_json(config_bytes));
    const auto wire = diamond_training::Compatibility::min(config.model_version,
        {config.network.residual_blocks, config.network.width}, soo::EncodingVersion::goals_v2);
    const auto group = config.self_play.turn_order_group;
    struct Root { Endpoint endpoint; std::string family; bool validation; };
    std::map<std::string, Root> roots;
    std::set<std::string> train_families;
    for (const auto name : {"train-endpoints.json", "validation-endpoints.json"}) {
        const auto json = diamond_support::parse_json(read(archive / name));
        const auto states = endpoints(json, group);
        const auto& rows = array(object(json).at("endpoints"));
        require(states.size() == rows.size(), "mixed groups in weakness archive");
        for (size_t i = 0; i < states.size(); ++i) {
            const auto& row = object(rows[i]);
            require(string(row.at("phase")) == "pre_first", "training root is post-first");
            const bool validation = std::string_view(name) == "validation-endpoints.json";
            const auto family = string(row.at("source_game_id"));
            if (validation) require(!train_families.contains(family), "source family crosses split");
            else train_families.insert(family);
            require(roots.emplace(states[i].identity,
                Root{states[i], family, validation}).second,
                "training and validation share a root");
        }
    }
    std::set<std::string> validation_keys, training_keys, excluded_families;
    std::map<std::string, size_t> family_rows;
    const auto train_invocation = object(diamond_support::parse_json(read(training_run / "invocation.json")));
    const auto validation_invocation = object(diamond_support::parse_json(read(validation_run / "invocation.json")));
    require(string(train_invocation.at("model_digest")) ==
                string(validation_invocation.at("model_digest")), "reanalysis teacher changed across split");
    const auto filtered = [&](const Path& run, bool validation) {
        const auto invocation = object(diamond_support::parse_json(read(run / "invocation.json")));
        const auto input = archive / (validation ? "validation-endpoints.json" : "train-endpoints.json");
        require(string(invocation.at("endpoints_sha256")) == diamond_support::sha256(read(input)) &&
                string(invocation.at("group")) == group, "continuation split does not match archive");
        const auto episodes = diamond_pipeline::load_episode_artifact(run / "continuations.episodes",
            string(invocation.at("operation_id")), wire);
        const auto result = object(diamond_support::parse_json(read(run / "result.json")));
        const auto& rows = array(result.at("episodes"));
        require(rows.size() == episodes.size(), "continuation report/artifact mismatch");
        std::vector<diamond_pipeline::Episode> selected;
        for (size_t i = 0; i < episodes.size(); ++i) {
            const auto& row = object(rows[i]);
            const auto& source = episodes[i];
            require(source.game_id == string(row.at("game_id")), "continuation identity mismatch");
            const auto& root = roots.at(string(row.at("endpoint")));
            require(root.validation == validation, "continuation root crosses split");
            if (!source.completed) { require(source.samples.empty(), "unresolved outcome labels"); continue; }
            require(!source.samples.empty(), "completed continuation is empty");
            const auto first = recover_min_state(source.samples.front(), root.endpoint.match,
                root.endpoint.state.turn_number, source.final_order);
            require(first.occupancy == root.endpoint.state.occupancy &&
                        first.current_player == root.endpoint.state.current_player &&
                        first.finished_count == root.endpoint.state.finished_count &&
                        first.finish_order == root.endpoint.state.finish_order,
                    "continuation first sample does not match root");
            auto retained = source;
            retained.samples.clear();
            std::vector<std::string> keys;
            const size_t limit = validation ? 64 : 16;
            bool overlap = false;
            for (size_t step = 0; step < source.samples.size(); ++step) {
                require(step + root.endpoint.state.turn_number < std::numeric_limits<uint16_t>::max(),
                        "continuation turn range exceeded");
                const auto state = recover_min_state(source.samples[step], root.endpoint.match,
                    static_cast<uint16_t>(step + root.endpoint.state.turn_number), source.final_order);
                if (state.finished_count) break;
                const auto key = position_key(state, root.endpoint.match);
                if (!validation && validation_keys.contains(key)) { overlap = true; break; }
                auto& seen_keys = validation ? validation_keys : training_keys;
                if (retained.samples.size() < limit && family_rows[root.family] < 4 * limit &&
                    seen_keys.insert(key).second) {
                    retained.samples.push_back(source.samples[step]);
                    keys.push_back(key);
                    ++family_rows[root.family];
                }
            }
            if (overlap) {
                excluded_families.insert(root.family);
                for (const auto& key : keys) training_keys.erase(key);
            } else if (!retained.samples.empty()) selected.push_back(std::move(retained));
        }
        return selected;
    };
    auto validation = filtered(validation_run, true);
    auto training = filtered(training_run, false);
    // Drop all branches of a family if any branch touched a held-out state.
    std::set<std::string> excluded_game_ids;
    const auto train_result = object(diamond_support::parse_json(read(training_run / "result.json")));
    for (const auto& value : array(train_result.at("episodes"))) {
        const auto& row = object(value);
        if (excluded_families.contains(roots.at(string(row.at("endpoint"))).family))
            excluded_game_ids.insert(string(row.at("game_id")));
    }
    std::erase_if(training, [&](const auto& episode) { return excluded_game_ids.contains(episode.game_id); });
    require(!training.empty() && !validation.empty(), "no disjoint completed pre-first targets");
    std::filesystem::create_directories(output);
    const std::string operation = "weakness-targets-" + diamond_support::sha256(
        manifest_bytes + string(train_invocation.at("operation_id")));
    diamond_pipeline::save_episode_artifact(output / "training.episodes", operation, training);
    const auto restored = diamond_pipeline::load_episode_artifact(output / "training.episodes", operation, wire);
    require(restored.size() == training.size(), "filtered episode roundtrip failed");
    diamond_pipeline::ReplayStore heldout(output / "validation-replay", wire, config.replay.capacity,
                                          config.replay.seed);
    const auto validation_ingest = heldout.ingest_iteration(validation);
    std::set<std::string> normal_ids;
    for (const auto& value : array(manifest.at("normal_game_ids"))) normal_ids.insert(string(value));
    auto match = soo::standard_min_match();
    soo::State start;
    start.current_player = match.players[0].id;
    std::vector<soo::EpisodeJob> jobs(static_cast<size_t>(config.workers.games_per_iteration),
                                     {start, 0, std::nullopt});
    soo::balance_episode_turn_orders(match, jobs, soo::min_turn_order_group_from_name(group),
                                     config.self_play.turn_order_weights);
    diamond_pipeline::ReplayStore normal(output / "regular-replay", wire, config.replay.capacity,
                                         config.replay.seed);
    size_t normal_overlap_games = 0, hard_samples = 0;
    for (const auto& episode : training) hard_samples += episode.samples.size();
    for (const auto& value : array(manifest.at("sources"))) {
        const auto& source = object(value);
        const Path path = string(source.at("artifact"));
        require(diamond_support::sha256(read(path)) == string(source.at("sha256")),
                "source trajectory changed after collection");
        auto episodes = diamond_pipeline::load_episode_artifact(path, string(source.at("operation_id")), wire);
        std::vector<diamond_pipeline::Episode> selected;
        for (size_t game = 0; game < episodes.size(); ++game) {
            auto& episode = episodes[game];
            if (!normal_ids.contains(episode.game_id)) continue;
            bool overlap = false;
            for (size_t step = 0; step < episode.samples.size(); ++step) {
                const auto state = recover_min_state(episode.samples[step], *jobs[game].match,
                    static_cast<uint16_t>(step + 1), episode.final_order);
                if (validation_keys.contains(position_key(state, *jobs[game].match))) {
                    overlap = true; break;
                }
            }
            if (overlap) ++normal_overlap_games;
            else selected.push_back(std::move(episode));
        }
        if (!selected.empty()) (void)normal.ingest_iteration(selected);
    }
    require(normal.size() >= 512 && validation_ingest.accepted_samples > 0 && hard_samples > 0,
            "weakness dataset is too small");
    write(output / "manifest.json", Json{Object{{"operation_id", Json{operation}},
        {"archive_sha256", Json{diamond_support::sha256(manifest_bytes)}},
        {"teacher_digest", train_invocation.at("model_digest")},
        {"training_games", Json{static_cast<int64_t>(training.size())}},
        {"hard_samples", Json{static_cast<int64_t>(hard_samples)}},
        {"validation_games", Json{static_cast<int64_t>(validation.size())}},
        {"validation_samples", Json{static_cast<int64_t>(validation_ingest.accepted_samples)}},
        {"normal_samples", Json{static_cast<int64_t>(normal.size())}},
        {"normal_overlap_games_removed", Json{static_cast<int64_t>(normal_overlap_games)}},
        {"overlap_families_removed", Json{static_cast<int64_t>(excluded_families.size())}},
        {"max_training_rows_per_family", Json{int64_t{64}}},
        {"training_episode_sha256", Json{diamond_support::sha256(read(output / "training.episodes"))}}}});
    std::cout << hard_samples << " unique hard rows, " << heldout.size()
              << " held-out rows, " << normal.size() << " retention rows\n";
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 6 && std::string_view(argv[1]) == "--prepare") {
            soo::ensure_topology_configured();
            prepare(argv[2], argv[3], argv[4], argv[5]);
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--collect") {
            soo::ensure_topology_configured();
            collect(argv[2], argv[3]);
            return 0;
        }
        const bool validate = argc == 4 && std::string_view(argv[1]) == "--validate";
        require(validate || argc == 6,
                "usage: min_reanalysis ENDPOINTS CHECKPOINT GROUP control|reanalyse NEW_OUTPUT; or "
                "--validate ENDPOINTS GROUP; --collect RUN NEW_OUTPUT; "
                "--prepare ARCHIVE TRAIN_RUN VALIDATION_RUN NEW_OUTPUT");
        soo::ensure_topology_configured();
        const auto input_bytes = read(argv[validate ? 2 : 1]);
        const auto positions = endpoints(diamond_support::parse_json(input_bytes), argv[3]);
        if (validate) {
            std::cout << positions.size() << " validated endpoints\n";
            return 0;
        }
        const std::string mode = argv[4];
        require(mode == "control" || mode == "reanalyse", "unknown experiment arm");
        const Path output = argv[5];
        require(!std::filesystem::exists(output), "output must be a new directory");
        const auto info = diamond_training::inspect_checkpoint_v2(argv[2]);
        require(info.format_version == 3 && info.provenance.has_value(),
                "need checkpoint provenance");
        const auto config = diamond_orchestration::ProductionConfig::from_json(
            diamond_support::parse_json(info.provenance->resolved_config_bytes));
        require(config.model_name == "Min" &&
                    config.encoder_version == "diamond-camp-relative-goals-v2" &&
                    config.self_play.turn_order_group == argv[3],
                "checkpoint does not match Min group");
        const auto wire = diamond_training::Compatibility::min(
            config.model_version, {config.network.residual_blocks, config.network.width},
            soo::EncodingVersion::goals_v2);
        torch::set_num_threads(2);
        torch::set_num_interop_threads(1);
        const auto device = diamond_training::resolve_device("cuda:0");
        auto model = diamond_model::DiamondModel(
            config.network.width, config.network.residual_blocks, wire.input_features(), 3);
        const auto loaded = diamond_training::load_checkpoint_v2_weights(argv[2], model, device);
        require(loaded.model_digest == info.model_digest && loaded.generation == info.generation,
                "checkpoint changed during load");
        require(torch::equal(model->adjacency.to(torch::kCPU), diamond_model::topology_adjacency()),
                "noncanonical board adjacency");
        diamond_pipeline::ModelPool models(1, device);
        const auto key = models.install(wire, model);
        models.activate(key);
        diamond_pipeline::IterationRequest request;
        request.operation_id =
            "reanalysis-" + diamond_support::sha256(input_bytes + info.model_digest + mode);
        request.model_key = key;
        request.compatibility = wire;
        request.match = soo::standard_min_match();
        request.selfplay.lanes = 32;
        request.selfplay.threads = 8;
        request.selfplay.max_batch = 32;
        request.selfplay.max_wait_us = 100;
        request.selfplay.simulations = mode == "control" ? 512 : 2048;
        request.selfplay.max_moves = 512;
        request.selfplay.temperature = 0;
        request.selfplay.dirichlet_epsilon = 0;
        request.selfplay.repeat_window = mode == "control" ? 0 : 8;
        request.selfplay.repetition_temperature = mode == "control" ? 0 : 1;
        request.selfplay.encoding = soo::EncodingVersion::goals_v2;
        for (std::size_t i = 0; i < positions.size(); ++i)
            for (uint64_t copy = 0; copy < 4; ++copy)
                request.jobs.push_back(
                    {positions[i].state, 918300096 + i * 4 + copy, positions[i].match});
        std::filesystem::create_directories(output);
        write(output / "invocation.json",
              Json{Object{{"checkpoint", Json{std::filesystem::canonical(argv[2]).string()}},
                          {"model_digest", Json{info.model_digest}},
                          {"endpoints_sha256", Json{diamond_support::sha256(input_bytes)}},
                          {"endpoints_file", Json{std::filesystem::canonical(argv[1]).string()}},
                          {"operation_id", Json{request.operation_id}},
                          {"mode", Json{mode}},
                          {"group", Json{std::string(argv[3])}},
                          {"jobs", Json{static_cast<int64_t>(request.jobs.size())}}}});
        const auto result =
            diamond_pipeline::run_self_play(request, models, {}, [&](std::size_t done) {
                write(output / "progress.json",
                      Json{Object{{"completed", Json{static_cast<int64_t>(done)}},
                                  {"total", Json{static_cast<int64_t>(request.jobs.size())}}}});
            });
        Array rows;
        for (std::size_t i = 0; i < result.episodes.size(); ++i) {
            const auto& episode = result.episodes[i];
            const auto& initial = positions[i / 4];
            if (episode.completed) {
                require(episode.final_order.size() == 3, "missing terminal ranking");
                if (initial.state.finished_count)
                    require(episode.final_order.front() == initial.state.finish_order.front(),
                            "known first finisher changed");
                require(episode.samples.size() == episode.move_count,
                        "missing continuation samples");
                for (const auto& sample : episode.samples) {
                    for (std::size_t seat = 0; seat < sample.canonical_player_ids.size(); ++seat) {
                        const auto rank =
                            std::find(episode.final_order.begin(), episode.final_order.end(),
                                      sample.canonical_player_ids[seat]);
                        require(rank != episode.final_order.end() &&
                                    sample.value_target.at(seat) ==
                                        1.0F - std::distance(episode.final_order.begin(), rank),
                                "invalid rank target");
                    }
                }
            } else
                require(episode.samples.empty(), "unresolved game acquired outcome labels");
            Array ranking;
            for (const auto id : episode.final_order)
                ranking.push_back(Json{static_cast<int64_t>(id)});
            rows.push_back(
                Json{Object{{"endpoint", Json{initial.identity}},
                            {"copy", Json{static_cast<int64_t>(i % 4)}},
                            {"game_id", Json{episode.game_id}},
                            {"completed", Json{episode.completed}},
                            {"moves", Json{static_cast<int64_t>(episode.move_count)}},
                            {"samples", Json{static_cast<int64_t>(episode.samples.size())}},
                            {"final_order", Json{ranking}}}});
        }
        diamond_pipeline::save_episode_artifact(output / "continuations.episodes",
                                                request.operation_id, result.episodes);
        const auto restored = diamond_pipeline::load_episode_artifact(
            output / "continuations.episodes", request.operation_id, wire);
        require(restored.size() == result.episodes.size(), "episode roundtrip count mismatch");
        for (std::size_t i = 0; i < restored.size(); ++i)
            require(restored[i].completed == result.episodes[i].completed &&
                        restored[i].samples.size() == result.episodes[i].samples.size() &&
                        restored[i].final_order == result.episodes[i].final_order,
                    "episode roundtrip mismatch");
        write(output / "result.json",
              Json{Object{{"completed", Json{static_cast<int64_t>(result.completed_games)}},
                          {"aborted", Json{static_cast<int64_t>(result.aborted_games)}},
                          {"samples", Json{static_cast<int64_t>(result.new_samples)}},
                          {"wall_seconds", Json{result.metrics.wall_seconds}},
                          {"episodes", Json{rows}}}});
        std::cout << result.completed_games << '/' << result.episodes.size() << " completed; "
                  << result.new_samples << " valid continuation samples\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
