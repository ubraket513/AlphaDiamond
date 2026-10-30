#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace diamond_model {

// A validated deployment bundle, described by its own metadata rather than by
// constants compiled into this binary. Format 3 declares the family and the
// architecture; the loader checks the weights against what was declared, which
// is what lets a second model family ship without a format change.
struct DeploymentArtifact {
    std::filesystem::path root;
    std::filesystem::path weights;
    std::string model_family;       // "soo" or "min"
    std::string model_version;
    std::string model_sha256;
    std::string runtime_sha256;
    int64_t width = 0;
    int64_t residual_blocks = 0;
    int64_t input_features = 0;     // per hole
    int64_t value_size = 0;         // value-head outputs
    std::string encoder_version;   // exact game_contract.encoder
    std::string turn_order_group;  // optional Min training provenance
    std::vector<DeploymentArtifact> experts; // empty for a leaf; [cyclic, reverse] for Min MoE
};

// Computes the existing format-v3 runtime manifest digest from topology and weights.
std::string deployment_runtime_sha256(const std::filesystem::path& root,
    int64_t width, int64_t blocks, int64_t input_features, int64_t value_size);

// Strictly validates the versioned deployment bundle before
// any raw tensor is loaded by the production runtime.
DeploymentArtifact validate_deployment_artifact(const std::filesystem::path& root);

// Same, and additionally requires the declared family.
DeploymentArtifact validate_deployment_artifact(const std::filesystem::path& root,
                                                const std::string& expected_family);

// Writes the release representation of a validated format-v3 artifact.  The
// destination must not already exist.  Only metadata.json, the topology
// tables, and weights/ are copied for leaves; MoE roots retain both runtime
// expert bundles. The destination is validated before it is returned.
DeploymentArtifact write_runtime_deployment_artifact(
    const std::filesystem::path& source_root,
    const std::filesystem::path& destination_root);

// Composes two provenance-checked Min leaves without changing their identities.
// The destination must not exist; only runtime files are copied.
DeploymentArtifact write_min_moe_deployment_artifact(
    const std::filesystem::path& cyclic_root,
    const std::filesystem::path& reverse_root,
    const std::filesystem::path& destination_root,
    const std::string& version);

// Soo-specific spelling kept for the existing probes and the Qt runtime.
inline DeploymentArtifact validate_soo_deployment_artifact(
    const std::filesystem::path& root) {
    return validate_deployment_artifact(root, "soo");
}

}  // namespace diamond_model
