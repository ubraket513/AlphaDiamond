// Geometric classifier and synchronous batch partition for Min specialists.
// Compare canonical camp-cell sets in seat order so relabeling/rotating a match
// preserves its specialist. Reversing the turn order may change the specialist.
// Classifier validation throws invalid_argument; evaluator failures propagate.
// Unsupported matches are never rerouted.
#include "soo/min_experts.hpp"

#include <array>
#include <bitset>
#include <stdexcept>
#include <utility>

#include "soo/board.hpp"

namespace soo {
namespace {

// Compare actual camp cells, rather than depending on camp numbering or player
// labels. Each signature starts at one seat and follows the full cyclic order.
using Geometry = std::array<std::bitset<kBoardSize>, 2 * kMaxPlayers>;

Geometry canonical_geometry(const Match& match, int first_seat) {
    const auto& topo = topology();
    const auto& rotation = topo.physical_to_canonical[match.players[first_seat].camp];
    Geometry geometry;
    for (int offset = 0; offset < match.count; ++offset) {
        const auto& player = match.players[(first_seat + offset) % match.count];
        for (const auto cell : topo.camp_positions[player.camp])
            geometry[2 * offset].set(rotation[cell]);
        for (const auto cell : topo.camp_positions[player.target_camp])
            geometry[2 * offset + 1].set(rotation[cell]);
    }
    return geometry;
}

MinTurnOrderGroup request_group(const BatchItem& item) {
    if (!item.match || !item.state || !item.encoded || item.value_width != 3)
        throw std::invalid_argument("Min expert evaluation requires a three-player request and match");
    return min_turn_order_group(*item.match);
}

} // namespace

// Finished players remain part of Match: classification describes the original
// game's topology/order, rather than changing expert as players finish. Validate
// IDs and camps before indexing topology tables used by canonical_geometry.
MinTurnOrderGroup min_turn_order_group(const Match& match) {
    if (match.count != 3)
        throw std::invalid_argument("Min turn-order groups require three players");
    std::bitset<256> ids;
    for (const auto& player : match.players) {
        if (player.id == 0 || ids.test(player.id))
            throw std::invalid_argument("Min turn-order groups require distinct nonzero player ids");
        ids.set(player.id);
        if (player.camp >= kCamps || player.target_camp >= kCamps)
            throw std::invalid_argument("Min turn-order groups require valid camps");
    }

    const auto actual = canonical_geometry(match, 0);
    auto reference = standard_min_match();
    for (const auto group : {MinTurnOrderGroup::cyclic, MinTurnOrderGroup::reverse}) {
        // A cyclic change of the initial seat preserves turn order. Compare
        // every anchor instead of assuming all actors have identical geometry.
        for (int first = 0; first < reference.count; ++first)
            if (actual == canonical_geometry(reference, first)) return group;
        std::swap(reference.players[1], reference.players[2]);
    }
    throw std::invalid_argument("Min turn-order groups require standard Min geometry or its rotation");
}

std::string_view min_turn_order_group_name(MinTurnOrderGroup group) {
    switch (group) {
        case MinTurnOrderGroup::cyclic: return "cyclic";
        case MinTurnOrderGroup::reverse: return "reverse";
    }
    throw std::invalid_argument("unknown Min turn-order group");
}

MinTurnOrderGroup min_turn_order_group_from_name(std::string_view name) {
    if (name == "cyclic") return MinTurnOrderGroup::cyclic;
    if (name == "reverse") return MinTurnOrderGroup::reverse;
    throw std::invalid_argument("unknown Min turn-order group: expected cyclic or reverse");
}

// prepare and evaluate must make the same geometric decision so buffers prepared
// by one model pool are consumed by that pool. ArenaModelRouter sits outside this
// router: root owner first, then that owner's cyclic/reverse expert.
void MinExpertRouter::prepare(BatchItem& item) {
    (request_group(item) == MinTurnOrderGroup::cyclic ? cyclic_ : reverse_).prepare(item);
}

void MinExpertRouter::evaluate(std::vector<BatchItem>& batch) {
    std::vector<BatchItem> cyclic;
    std::vector<BatchItem> reverse;
    cyclic.reserve(batch.size());
    reverse.reserve(batch.size());
    for (const auto& item : batch)
        (request_group(item) == MinTurnOrderGroup::cyclic ? cyclic : reverse).push_back(item);
    if (!cyclic.empty()) cyclic_.evaluate(cyclic);
    if (!reverse.empty()) reverse_.evaluate(reverse);
}

} // namespace soo
