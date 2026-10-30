#pragma once

#include "diamond_pipeline/replay.hpp"
#include "soo/encoder.hpp"
#include "soo/min_experts.hpp"
#include "soo/selfplay.hpp"
#include "soo/board.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>

inline soo::State recover_min_state(const diamond_training::TrainingSample& sample,
                                   const soo::Match& match, uint16_t turn_number,
                                   const std::vector<int32_t>& final_order) {
    const auto require = [](bool condition) {
        if (!condition) throw std::invalid_argument("invalid saved Min weakness state");
    };
    require(match.count == 3 && turn_number > 0 &&
            sample.node_features.size() == soo::kBoardSize * 9 &&
            sample.canonical_player_ids.size() == 3 && sample.value_target.size() == 3 &&
            final_order.size() == 3);
    const std::set<int32_t> expected{1, 2, 3};
    require(std::set<int32_t>(final_order.begin(), final_order.end()) == expected &&
            std::set<int32_t>(sample.canonical_player_ids.begin(),
                              sample.canonical_player_ids.end()) == expected);
    const auto standard = soo::standard_min_match();
    for (int seat = 0; seat < 3; ++seat) {
        const auto& player = match.players[seat];
        require(expected.contains(player.id));
        const auto spec = standard.by_id(player.id);
        require(player.camp == spec.camp && player.target_camp == spec.target_camp);
    }
    (void)soo::min_turn_order_group(match);
    soo::State state;
    state.current_player = static_cast<uint8_t>(sample.canonical_player_ids.front());
    state.turn_number = turn_number;
    const auto& mapping = soo::topology().physical_to_canonical[
        match.by_id(state.current_player).camp];
    for (int physical = 0; physical < soo::kBoardSize; ++physical) {
        const auto offset = static_cast<size_t>(mapping[physical]) * 9;
        for (size_t channel = 0; channel < 3; ++channel) {
            const float occupied = sample.node_features[offset + channel];
            require(occupied == 0 || occupied == 1);
            if (occupied == 1) {
                require(state.occupancy[physical] == 0);
                state.occupancy[physical] =
                    static_cast<uint8_t>(sample.canonical_player_ids[channel]);
            }
        }
    }
    std::set<int32_t> placed;
    for (size_t channel = 0; channel < 3; ++channel) {
        const auto id = sample.canonical_player_ids[channel];
        const float flag = sample.node_features[3 + channel];
        require(flag == 0 || flag == 1);
        if (flag == 1) placed.insert(id);
        require(std::count(state.occupancy.begin(), state.occupancy.end(), id) == 10);
        const auto& target = soo::topology().camp_positions[match.by_id(id).target_camp];
        require(std::all_of(target.begin(), target.end(), [&](auto cell) {
            return state.occupancy[cell] == id;
        }) == (flag == 1));
        const auto rank = std::find(final_order.begin(), final_order.end(), id);
        require(sample.value_target[channel] ==
                1.0F - std::distance(final_order.begin(), rank));
    }
    require(placed.size() <= 1 && !placed.contains(state.current_player));
    for (size_t rank = 0; rank < placed.size(); ++rank) {
        require(placed.contains(final_order[rank]));
        state.place(static_cast<uint8_t>(final_order[rank]));
    }
    const auto encoded = soo::encode(state, match, soo::EncodingVersion::goals_v2);
    require(encoded.node_features == sample.node_features &&
            std::equal(sample.canonical_player_ids.begin(), sample.canonical_player_ids.end(),
                       encoded.canonical_player_ids.begin()));
    return state;
}

inline std::vector<size_t> weakness_positions(const diamond_pipeline::Episode& episode,
                                             const soo::Match& match) {
    if (!episode.completed || episode.samples.empty()) return {};
    if (episode.samples.size() >= std::numeric_limits<uint16_t>::max())
        throw std::invalid_argument("weakness trajectory exceeds native turn range");
    size_t first_finish = episode.samples.size();
    std::optional<size_t> first_repeat;
    std::set<uint64_t> seen;
    for (size_t i = 0; i < episode.samples.size(); ++i) {
        const auto state = recover_min_state(episode.samples[i], match,
                                             static_cast<uint16_t>(i + 1), episode.final_order);
        if (state.finished_count) { first_finish = i; break; }
        if (!seen.insert(soo::dynamics_key(state)).second && i >= 96 && !first_repeat)
            first_repeat = i;
    }
    std::vector<size_t> result;
    if (first_repeat || first_finish >= 128) {
        const size_t problem = first_repeat.value_or(128);
        result.push_back(problem - 16);
    }
    if (episode.samples.size() - first_finish >= 64)
        result.push_back(first_finish + 16);
    return result;
}
