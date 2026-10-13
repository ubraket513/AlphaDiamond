// Versioned camp-relative encoding; v1 preserves the frozen Python oracle.
#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "soo/state.hpp"

namespace soo {

enum class EncodingVersion { legacy_v1, goals_v2 };

std::string_view encoding_version_name(EncodingVersion encoding);
EncodingVersion encoding_version_from_name(std::string_view name);
int encoded_feature_count(int players, EncodingVersion encoding);

struct Encoded {
    // Row-major [73][feature_count]; rows are indexed by canonical position id.
    std::vector<float> node_features;
    std::vector<uint8_t> canonical_player_ids;
    int feature_count = 0;
};

// V1: occupancy, then finished flags. V2 appends each canonical player's
// target-camp membership, using the same actor-relative spatial rotation.
Encoded encode(const State& state, const Match& match,
               EncodingVersion encoding = EncodingVersion::legacy_v1);

// Turn-order seat ids rotated to start at ``state.current_player``.
void canonical_player_ids(const State& state, const Match& match, std::vector<uint8_t>& out);

int32_t to_canonical_action(int32_t physical_action, const Match& match, uint8_t current_player);
int32_t to_physical_action(int32_t canonical_action, const Match& match, uint8_t current_player);

// Legal actions in canonical space, in the same order as legal_action_ids.
void canonical_legal_action_ids(const State& state, const Match& match, std::vector<int32_t>& out);

}  // namespace soo
