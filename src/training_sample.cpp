#include "diamond_training/training_sample.hpp"

#include <stdexcept>

namespace diamond_training {
namespace {
constexpr const char* kRuleset = "diamond-authoritative-rules-v1";
constexpr const char* kTopology = "diamond73-v1";
constexpr const char* kFingerprint = "sha256:02fff0c9c9436f247c4a2b5fb6b01903f658aae1c752377073011d0d150ba7a1";
constexpr const char* kActions = "diamond73-srcdst-v1";
constexpr const char* kSeats = "diamond-seat-layout-v1";
constexpr const char* kSooValue = "current-player-scalar-winloss-v1";
constexpr const char* kMinValue = "canonical-placement-utility-1-0-minus1-v1";

Compatibility make(std::string name, std::string version, int64_t players,
                   std::string value, NetworkConfig network, soo::EncodingVersion encoding) {
    return Compatibility{std::move(name), std::move(version), players,
                         kRuleset, kTopology, kFingerprint,
                         std::string(soo::encoding_version_name(encoding)), kActions,
                         kSeats, std::move(value), network};
}
}  // namespace

Compatibility Compatibility::soo(std::string version, NetworkConfig network,
                                 soo::EncodingVersion encoding) {
    return make("Soo", std::move(version), 2, kSooValue, network, encoding);
}

Compatibility Compatibility::min(std::string version, NetworkConfig network,
                                 soo::EncodingVersion encoding) {
    return make("Min", std::move(version), 3, kMinValue, network, encoding);
}

int64_t Compatibility::input_features() const {
    validate();
    return soo::encoded_feature_count(static_cast<int>(player_count),
                                     soo::encoding_version_from_name(encoder_version));
}

std::string Compatibility::family() const {
    if (model_name == "Soo") return "soo";
    if (model_name == "Min") return "min";
    throw std::invalid_argument("unknown compatibility model_name");
}

void Compatibility::validate() const {
    (void)family();
    (void)soo::encoding_version_from_name(encoder_version);
    if (model_version.empty() || ruleset_version.empty() || board_topology_version.empty() ||
        ruleset_fingerprint.empty() || encoder_version.empty() || action_space_version.empty() ||
        seat_layout_version.empty() || value_semantics_version.empty() ||
        network_config.residual_blocks <= 0 || network_config.width <= 0)
        throw std::invalid_argument("compatibility contains an empty or invalid gate");
    if ((model_name == "Soo" && (player_count != 2 || value_semantics_version != kSooValue)) ||
        (model_name == "Min" && (player_count != 3 || value_semantics_version != kMinValue)))
        throw std::invalid_argument("compatibility model identity is inconsistent");
}

}  // namespace diamond_training
