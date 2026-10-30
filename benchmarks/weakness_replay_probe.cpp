#include "weakness_positions.hpp"
#include "soo/encoder.hpp"
#include "soo/rules.hpp"
#include "soo/board.hpp"

#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace {
soo::State opening(const soo::Match& match) {
    soo::State state;
    for (int seat = 0; seat < match.count; ++seat)
        for (auto cell : soo::topology().camp_positions[match.players[seat].camp])
            state.occupancy[cell] = match.players[seat].id;
    state.current_player = match.players[0].id;
    return state;
}
void require(bool condition, std::source_location where = std::source_location::current()) {
    if (!condition) throw std::runtime_error("weakness replay probe failed at line " +
                                            std::to_string(where.line()));
}
template <class F> void rejects(F function) {
    try { function(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("invalid weakness state was accepted");
}
diamond_training::TrainingSample sample(const soo::State& state, const soo::Match& match) {
    const auto encoded = soo::encode(state, match, soo::EncodingVersion::goals_v2);
    diamond_training::TrainingSample result;
    result.node_features = encoded.node_features;
    result.canonical_player_ids.assign(encoded.canonical_player_ids.begin(),
                                      encoded.canonical_player_ids.begin() + 3);
    for (const auto id : result.canonical_player_ids) result.value_target.push_back(2.0F - id);
    return result;
}
}

int main() {
    soo::ensure_topology_configured();
    const auto base = soo::standard_min_match();
    std::array<uint8_t, 3> order{1, 2, 3};
    do {
        auto match = base;
        for (size_t i = 0; i < order.size(); ++i) match.players[i] = base.by_id(order[i]);
        auto state = opening(match);
        state.turn_number = 141;
        auto row = sample(state, match);
        require(recover_min_state(row, match, 141, {1, 2, 3}) == state);
        auto bad = row;
        bad.node_features[3] = 0.5F;
        rejects([&] { (void)recover_min_state(bad, match, 141, {1, 2, 3}); });
        bad = row;
        bad.node_features[6] = 1.0F - bad.node_features[6];
        rejects([&] { (void)recover_min_state(bad, match, 141, {1, 2, 3}); });
        rejects([&] { (void)recover_min_state(row, match, 0, {1, 2, 3}); });
        rejects([&] { (void)recover_min_state(row, match, 141, {1, 1, 3}); });
        bad = row;
        bad.value_target[0] = 42;
        rejects([&] { (void)recover_min_state(bad, match, 141, {1, 2, 3}); });
    } while (std::next_permutation(order.begin(), order.end()));

    auto placed = opening(base);
    placed.occupancy.fill(0);
    for (auto cell : soo::topology().camp_positions[base.by_id(1).target_camp]) {
        placed.occupancy[cell] = 1;
    }
    for (uint8_t id : {2, 3}) {
        int pieces = 0;
        const auto empty_goal = soo::topology().camp_positions[base.by_id(id).target_camp][0];
        for (int cell = 0; cell < soo::kBoardSize && pieces < 10; ++cell)
            if (cell != empty_goal && placed.occupancy[cell] == 0) {
                placed.occupancy[cell] = id;
                ++pieces;
            }
        require(pieces == 10);
    }
    placed.place(1);
    placed.current_player = 2;
    placed.turn_number = 151;
    require(recover_min_state(sample(placed, base), base, 151, {1, 2, 3}) == placed);

    diamond_pipeline::Episode episode;
    episode.completed = true;
    episode.final_order = {1, 2, 3};
    episode.samples.assign(200, sample(opening(base), base));
    episode.move_count = 200;
    const auto selected = weakness_positions(episode, base);
    require(selected.size() == 1 && selected.front() < 128);
    episode.completed = false;
    require(weakness_positions(episode, base).empty());
    std::cout << "weakness state recovery and selection checks passed\n";
}
