// Goal ownership must remain observable across supported turn orders and seats.
// These legal trajectories exposed identical legacy observations with different
// goal ownership. They are separate from the immutable legacy golden fixtures.
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "check.hpp"
#include "soo/board.hpp"
#include "soo/encoder.hpp"
#include "soo/mcts.hpp"
#include "soo/mcts3p.hpp"
#include "soo/rules.hpp"

namespace {
struct Position {
    soo::Match match;
    soo::State state;
};

soo::State opening(const soo::Match& match) {
    soo::State state;
    state.current_player = match.players[0].id;
    for (int seat = 0; seat < match.count; ++seat)
        for (const auto position : soo::topology().camp_positions[match.players[seat].camp])
            state.occupancy[position] = match.players[seat].id;
    return state;
}

Position load_position(const std::filesystem::path& path) {
    std::ifstream input(path);
    REQUIRE(input.good(), "cannot open reachable encoder fixture");
    Position result;
    int players = 0;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        std::string kind;
        row >> kind;
        if (kind == "players") {
            row >> players;
            REQUIRE(players == 2 || players == 3, "unsupported fixture player count");
            result.match = players == 2 ? soo::standard_soo_match() : soo::standard_min_match();
        } else if (kind == "order") {
            const auto standard = result.match;
            for (int seat = 0; seat < players; ++seat) {
                int id = 0;
                row >> id;
                result.match.players[seat] = standard.by_id(static_cast<uint8_t>(id));
            }
            result.state = opening(result.match);
        } else if (kind == "actions") {
            int count = 0;
            row >> count;
            for (int index = 0; index < count; ++index) {
                int32_t action = -1;
                row >> action;
                REQUIRE(row.good() || row.eof(), "truncated fixture actions");
                result.state = soo::apply_action(result.state, result.match, action);
                CHECK_EQ(result.state.finished_count, 0);
                for (int id = 1; id <= players; ++id)
                    CHECK_EQ(std::count(result.state.occupancy.begin(), result.state.occupancy.end(), id), 10);
            }
        } else if (kind == "current") {
            int actor = 0;
            row >> actor;
            CHECK_EQ(result.state.current_player, actor);
        } else if (kind == "turn") {
            int turn = 0;
            row >> turn;
            CHECK_EQ(result.state.turn_number, turn);
        } else if (kind == "occupancy") {
            std::string occupancy;
            row >> occupancy;
            REQUIRE(occupancy.size() == soo::kBoardSize, "bad fixture board length");
            for (int position = 0; position < soo::kBoardSize; ++position)
                CHECK_EQ(result.state.occupancy[position], occupancy[position] - '0');
        } else {
            REQUIRE(false, "unknown fixture field");
        }
    }
    return result;
}

void check_reachable_alias(const std::filesystem::path& fixtures, const std::string& family) {
    const auto a = load_position(fixtures / (family + "-a.txt"));
    const auto b = load_position(fixtures / (family + "-b.txt"));
    const auto legacy_a = soo::encode(a.state, a.match);
    const auto legacy_b = soo::encode(b.state, b.match);
    CHECK_EQ(legacy_a.node_features, legacy_b.node_features);
    CHECK_EQ(a.state.turn_number, b.state.turn_number);
    std::vector<int32_t> legal_a, legal_b;
    soo::canonical_legal_action_ids(a.state, a.match, legal_a);
    soo::canonical_legal_action_ids(b.state, b.match, legal_b);
    std::sort(legal_a.begin(), legal_a.end());
    std::sort(legal_b.begin(), legal_b.end());
    CHECK_EQ(legal_a, legal_b);

    const auto goals_a = soo::encode(a.state, a.match, soo::EncodingVersion::goals_v2);
    const auto goals_b = soo::encode(b.state, b.match, soo::EncodingVersion::goals_v2);
    CHECK_EQ(goals_a.feature_count, 3 * a.match.count);
    CHECK(goals_a.node_features != goals_b.node_features);
}

template <class Function> void expect_invalid_argument(Function&& operation) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    CHECK(rejected);
}

void check_version_contract() {
    using Version = soo::EncodingVersion;
    CHECK_EQ(soo::encoding_version_from_name("diamond-camp-relative-v1"), Version::legacy_v1);
    CHECK_EQ(soo::encoding_version_from_name("diamond-camp-relative-goals-v2"), Version::goals_v2);
    for (const auto version : {Version::legacy_v1, Version::goals_v2})
        CHECK_EQ(soo::encoding_version_from_name(soo::encoding_version_name(version)), version);
    CHECK_EQ(soo::encoded_feature_count(2, Version::legacy_v1), 4);
    CHECK_EQ(soo::encoded_feature_count(3, Version::legacy_v1), 6);
    CHECK_EQ(soo::encoded_feature_count(2, Version::goals_v2), 6);
    CHECK_EQ(soo::encoded_feature_count(3, Version::goals_v2), 9);
    for (const std::string_view unknown : {"", "diamond-camp-relative-v2", "goals_v2"})
        expect_invalid_argument([&] { (void)soo::encoding_version_from_name(unknown); });
    for (const int count : {0, 1, 4})
        expect_invalid_argument([&] { (void)soo::encoded_feature_count(count, Version::goals_v2); });
    expect_invalid_argument([] { (void)soo::encoding_version_name(static_cast<Version>(99)); });
    expect_invalid_argument([] { (void)soo::encoded_feature_count(2, static_cast<Version>(99)); });
}

// These camp indices are derived from the three fixed seats and the sixfold
// board geometry, independently of the encoder's per-player feature loop.
int expected_goal_camp(int players, uint8_t actor, uint8_t player) {
    constexpr std::array<std::array<int, 3>, 3> min_goals{{{5, 4, 3}, {3, 5, 4}, {4, 3, 5}}};
    constexpr std::array<std::array<int, 2>, 2> soo_goals{{{5, 3}, {4, 5}}};
    return players == 3 ? min_goals[actor - 1][player - 1]
                        : soo_goals[actor - 1][player - 1];
}

void check_goal_planes(const soo::State& state, const soo::Match& match) {
    const auto old = soo::encode(state, match);
    const auto explicit_old = soo::encode(state, match, soo::EncodingVersion::legacy_v1);
    const auto goals = soo::encode(state, match, soo::EncodingVersion::goals_v2);
    CHECK_EQ(old.node_features, explicit_old.node_features);
    CHECK_EQ(old.canonical_player_ids, goals.canonical_player_ids);
    CHECK_EQ(goals.feature_count, 3 * match.count);
    REQUIRE(goals.node_features.size() == static_cast<size_t>(soo::kBoardSize * goals.feature_count),
            "goal encoding has wrong shape");
    for (int position = 0; position < soo::kBoardSize; ++position) {
        for (int channel = 0; channel < 2 * match.count; ++channel)
            CHECK_EQ(goals.node_features[position * goals.feature_count + channel],
                     old.node_features[position * old.feature_count + channel]);
        for (int channel = 0; channel < match.count; ++channel) {
            const auto player = goals.canonical_player_ids[channel];
            const auto& camp = soo::topology().camp_positions[
                expected_goal_camp(match.count, state.current_player, player)];
            const bool expected = std::find(camp.begin(), camp.end(), position) != camp.end();
            CHECK_EQ(goals.node_features[position * goals.feature_count + 2 * match.count + channel],
                     expected ? 1.0f : 0.0f);
            CHECK_EQ(goals.node_features[position * goals.feature_count + match.count + channel],
                     state.has_placed(player) ? 1.0f : 0.0f);
        }
    }
}

std::vector<int32_t> sorted_actions(const soo::State& state, const soo::Match& match) {
    std::vector<int32_t> result;
    soo::canonical_legal_action_ids(state, match, result);
    std::sort(result.begin(), result.end());
    return result;
}

int rotated_camp(int camp, const std::array<uint8_t, soo::kBoardSize>& rotation) {
    std::set<uint8_t> transformed;
    for (const auto position : soo::topology().camp_positions[camp])
        transformed.insert(rotation[position]);
    for (int candidate = 0; candidate < soo::kCamps; ++candidate) {
        const auto& positions = soo::topology().camp_positions[candidate];
        if (transformed == std::set<uint8_t>(positions.begin(), positions.end())) return candidate;
    }
    REQUIRE(false, "rotation does not preserve camps");
    return -1;
}

void check_rotations(const soo::State& state, const soo::Match& match) {
    const auto original = soo::encode(state, match, soo::EncodingVersion::goals_v2);
    const auto actions = sorted_actions(state, match);
    for (const auto& rotation : soo::topology().physical_to_canonical) {
        auto rotated_match = match;
        for (int seat = 0; seat < match.count; ++seat) {
            const auto original_player = match.players.at(static_cast<size_t>(seat));
            auto& rotated_player = rotated_match.players.at(static_cast<size_t>(seat));
            rotated_player.camp = static_cast<uint8_t>(rotated_camp(original_player.camp, rotation));
            rotated_player.target_camp =
                static_cast<uint8_t>(rotated_camp(original_player.target_camp, rotation));
        }
        auto rotated = state;
        for (int position = 0; position < soo::kBoardSize; ++position)
            rotated.occupancy[rotation[position]] = state.occupancy[position];
        const auto encoded = soo::encode(rotated, rotated_match, soo::EncodingVersion::goals_v2);
        CHECK_EQ(original.node_features, encoded.node_features);
        CHECK_EQ(original.canonical_player_ids, encoded.canonical_player_ids);
        CHECK_EQ(actions, sorted_actions(rotated, rotated_match));
        for (const auto canonical : actions) {
            const auto physical = soo::to_physical_action(canonical, match, state.current_player);
            const auto transformed = soo::to_physical_action(canonical, rotated_match, state.current_player);
            CHECK_EQ(transformed / soo::kBoardSize, rotation[physical / soo::kBoardSize]);
            CHECK_EQ(transformed % soo::kBoardSize, rotation[physical % soo::kBoardSize]);
            CHECK_EQ(soo::to_canonical_action(transformed, rotated_match, state.current_player), canonical);
            const auto next = soo::apply_action(state, match, physical);
            const auto rotated_next = soo::apply_action(rotated, rotated_match, transformed);
            CHECK_EQ(soo::encode(next, match, soo::EncodingVersion::goals_v2).node_features,
                     soo::encode(rotated_next, rotated_match, soo::EncodingVersion::goals_v2).node_features);
        }
    }
}

void check_search(const soo::State& state, const soo::Match& match) {
    const std::array<double, 3> player_values{0.125, -0.5, 0.875};
    std::vector<int32_t> previous_actions;
    std::vector<uint32_t> previous_visits;
    int32_t previous_selected = -1;
    for (const auto version : {soo::EncodingVersion::legacy_v1, soo::EncodingVersion::goals_v2}) {
        soo::MCTSConfig config;
        config.simulations = 3;
        config.encoding = version;
        if (match.count == 3) {
            soo::SearchSession3P session(match, config);
            session.begin(state, 0.0);
            while (session.advance() == soo::SearchSession3P::Status::NeedsEvaluation) {
                const auto& features = session.pending_features();
                CHECK_EQ(features.node_features, soo::encode(session.pending_state(), match, version).node_features);
                soo::EvalOutcome3P response;
                response.priors.assign(session.pending_actions().size(), 1.0 / session.pending_actions().size());
                for (size_t channel = 0; channel < features.canonical_player_ids.size(); ++channel)
                    response.value[channel] = player_values[features.canonical_player_ids[channel] - 1];
                session.supply(response);
            }
            const auto& result = session.result();
            for (size_t action = 0; action < result.root_actions.size(); ++action) {
                if (!result.visit_counts[action]) continue;
                for (int seat = 0; seat < match.count; ++seat)
                    CHECK_EQ(result.q_vectors[action][seat], player_values[match.players[seat].id - 1]);
            }
            if (version == soo::EncodingVersion::legacy_v1) {
                previous_actions = result.root_actions;
                previous_visits = result.visit_counts;
                previous_selected = result.selected_action;
            } else {
                CHECK_EQ(result.root_actions, previous_actions);
                CHECK_EQ(result.visit_counts, previous_visits);
                CHECK_EQ(result.selected_action, previous_selected);
            }
        } else {
            soo::SearchSession session(match, config);
            session.begin(state, 0.0, false);
            while (session.advance() == soo::SearchSession::Status::NeedsEvaluation) {
                CHECK_EQ(session.pending_features().node_features,
                         soo::encode(session.pending_state(), match, version).node_features);
                soo::EvalOutcome response;
                response.priors.assign(session.pending_actions().size(), 1.0 / session.pending_actions().size());
                response.value = session.pending_state().current_player == 1 ? 0.25 : -0.25;
                session.supply(response);
            }
            const auto& result = session.result();
            for (size_t action = 0; action < result.root_actions.size(); ++action)
                if (result.visit_counts[action])
                    CHECK_EQ(result.q_values[action], state.current_player == 1 ? 0.25 : -0.25);
            if (version == soo::EncodingVersion::legacy_v1) {
                previous_actions = result.root_actions;
                previous_visits = result.visit_counts;
                previous_selected = result.selected_action;
            } else {
                CHECK_EQ(result.root_actions, previous_actions);
                CHECK_EQ(result.visit_counts, previous_visits);
                CHECK_EQ(result.selected_action, previous_selected);
            }
        }
    }
}

soo::State finished_position(const soo::Match& match, uint8_t finisher, uint8_t actor) {
    soo::State result;
    result.current_player = actor;
    for (const auto position : soo::topology().camp_positions[match.by_id(finisher).target_camp])
        result.occupancy[position] = finisher;
    for (int seat = 0; seat < match.count; ++seat) {
        const auto player = match.players[seat];
        if (player.id == finisher) continue;
        int placed = 0;
        for (const auto position : soo::topology().camp_positions[player.camp])
            if (result.occupancy[position] == soo::kEmpty) {
                result.occupancy[position] = player.id;
                ++placed;
            }
        for (int position = 0; placed < soo::kCampSize && position < soo::kBoardSize; ++position)
            if (result.occupancy[position] == soo::kEmpty) {
                result.occupancy[position] = player.id;
                ++placed;
            }
    }
    result.place(finisher);
    CHECK(soo::has_finished(result, match.by_id(finisher)));
    return result;
}

void check_supported_combinations() {
    for (int count = 2; count <= 3; ++count) {
        const auto standard = count == 2 ? soo::standard_soo_match() : soo::standard_min_match();
        std::vector<uint8_t> order;
        for (int player = 1; player <= count; ++player) order.push_back(static_cast<uint8_t>(player));
        int cases = 0;
        do {
            auto match = standard;
            for (int seat = 0; seat < count; ++seat) match.players[seat] = standard.by_id(order[seat]);
            for (int actor = 1; actor <= count; ++actor) {
                auto state = opening(match);
                state.current_player = static_cast<uint8_t>(actor);
                check_goal_planes(state, match);
                check_rotations(state, match);
                check_search(state, match);
                if (count == 3)
                    for (int finisher = 1; finisher <= count; ++finisher)
                        if (finisher != actor) {
                            const auto finished = finished_position(match, static_cast<uint8_t>(finisher), state.current_player);
                            check_goal_planes(finished, match);
                            check_rotations(finished, match);
                        }
                ++cases;
            }
        } while (std::next_permutation(order.begin(), order.end()));
        CHECK_EQ(cases, count == 2 ? 4 : 18);
    }
}
} // namespace

int main(int argc, char** argv) {
    REQUIRE(argc == 2, "usage: encoder_version_test <reachable-fixture-directory>");
    soo::ensure_topology_configured();
    check_reachable_alias(argv[1], "soo");
    check_reachable_alias(argv[1], "min");
    check_version_contract();
    check_supported_combinations();
    return soo_test::report("encoder_version_test");
}
