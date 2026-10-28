// Synthetic format-v3 runtime fixtures. No training/checkpoint/exporter needed.
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <diamond_model/deployment_artifact.hpp>
#include <diamond_support/json.hpp>
#include <soo/board.hpp>
#include <soo/encoder.hpp>

namespace {
void write_tensor(const std::filesystem::path& root, const std::string& name,
                  const std::vector<float>& values) {
    std::ofstream output(root / "weights" / (name + ".f32"), std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    for (const float value : values) {
        const auto bits = std::bit_cast<uint32_t>(value);
        for (unsigned shift = 0; shift < 32; shift += 8)
            output.put(static_cast<char>((bits >> shift) & 0xffU));
    }
}

void make_fixture(const std::filesystem::path& root, const std::filesystem::path& topology,
                  const std::string& family) {
    constexpr int width = 4;
    const int features =
        soo::encoded_feature_count(family == "soo" ? 2 : 3, soo::EncodingVersion::goals_v2);
    const int value_size = family == "soo" ? 1 : 3;
    std::filesystem::create_directories(root / "weights");
    for (const auto& table : std::filesystem::directory_iterator(topology))
        std::filesystem::copy_file(table.path(), root / table.path().filename(),
                                   std::filesystem::copy_options::overwrite_existing);
    const auto tensor = [&](const std::string& name, int count, float value = 0.0F) {
        write_tensor(root, name, std::vector<float>(static_cast<size_t>(count), value));
    };
    tensor("trunk__input_projection__weight", width * features);
    tensor("trunk__input_projection__bias", width);
    tensor("trunk__blocks__0__self_projection__weight", width * width);
    tensor("trunk__blocks__0__self_projection__bias", width);
    for (int direction = 0; direction < 6; ++direction)
        tensor("trunk__blocks__0__direction_projections__" + std::to_string(direction) + "__weight",
               width * width);
    tensor("trunk__blocks__0__norm__weight", width, 1.0F);
    tensor("trunk__blocks__0__norm__bias", width);
    tensor("trunk__output_norm__weight", width, 1.0F);
    tensor("trunk__output_norm__bias", width);
    for (const std::string head : {"policy_head__source", "policy_head__destination"}) {
        tensor(head + "__weight", width * width);
        tensor(head + "__bias", width);
    }
    tensor("value_head__0__weight", width * width);
    tensor("value_head__0__bias", width);
    tensor("value_head__2__weight", width * value_size);
    tensor("value_head__2__bias", value_size);
    std::vector<float> adjacency(6 * 73 * 73, 0.0F);
    for (int node = 0; node < 73; ++node)
        for (int direction = 0; direction < 6; ++direction) {
            const int neighbour = soo::topology().neighbour[node][direction];
            if (neighbour >= 0)
                adjacency[static_cast<size_t>((direction * 73 + node) * 73 + neighbour)] = 1.0F;
        }
    write_tensor(root, "trunk__adjacency", adjacency);
    const auto digest =
        diamond_model::deployment_runtime_sha256(root, width, 1, features, value_size);
    // No model.ts is shipped. This digest identifies the deterministic generator
    // recipe, rather than falsely claiming a training checkpoint or graph exists.
    const auto model_digest = diamond_support::sha256("sdk-fixture-v1:" + family);
    std::ofstream metadata(root / "metadata.json");
    metadata.exceptions(std::ios::badbit | std::ios::failbit);
    metadata
        << "{\"format_version\":3,\"dtype\":\"float32\",\"model_family\":\"" << family
        << "\",\"model_version\":\"2.0.0\",\"model_sha256\":\"" << model_digest
        << "\",\"runtime_sha256\":\"" << digest
        << "\",\"architecture\":{\"type\":\"directional_residual\",\"width\":4,\"residual_blocks\":"
           "1},"
           "\"game_contract\":{\"topology\":\"diamond73-v1\",\"action_space\":\"diamond73-srcdst-"
           "v1\","
           "\"encoder\":\"diamond-camp-relative-goals-v2\"},\"tensor_shapes\":{\"input\":[1,73,"
        << features << "],\"policy\":[1,5329],\"value\":[1," << value_size
        << "]},\"source\":{\"checkpoint_sha256\":null,\"training_commit\":null,\"training_step\":"
           "null},"
           "\"corpus_seed\":0}\n";
    metadata.close();
    (void)diamond_model::validate_deployment_artifact(root, family);
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3)
            throw std::runtime_error("usage: sdk-fixtures TOPOLOGY OUTPUT");
        const std::filesystem::path topology(argv[1]);
        const std::filesystem::path output(argv[2]);
        if (!soo::load_topology_from_dir(topology.string()))
            throw std::runtime_error("canonical topology did not load");
        for (const std::string family : {"soo", "min"})
            make_fixture(output / family, topology, family);
        std::ofstream provenance(output / "PROVENANCE.txt");
        provenance << "Synthetic SDK smoke fixtures; never trained. Recipe sdk-fixture-v1 in "
                      "native/tests/sdk_consumer/generate_fixtures.cpp. width=4 blocks=1, "
                      "goals-v2, zero linear tensors, unit layer-norm scale, canonical adjacency. "
                      "Topology comes verbatim from tests/golden/topology (diamond73-v1). "
                      "metadata runtime_sha256 and SDK manifest cover every payload.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
