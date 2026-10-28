#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include <diamond_model/deployment_artifact.hpp>
#include <diamond_model/soo_model.hpp>
#include <diamond_orchestration/rating.hpp>
#include <diamond_support/json.hpp>
#include <soo/board.hpp>

#if !defined(_MSC_VER) && __cplusplus <= 202302L
#error "Runtime SDK did not propagate the C++26 dialect"
#endif

int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::runtime_error("expected SDK data directory");
        const std::filesystem::path data(argv[1]);
        if (!soo::load_topology_from_dir((data / "topology").string()))
            throw std::runtime_error("SDK topology did not load");
        if (!soo::topology().configured)
            throw std::runtime_error("topology not configured");
        if (std::abs(diamond_orchestration::expected_elo_score(1500, 1500, {}) - 0.5) > 1e-12)
            throw std::runtime_error("rating API result changed");
        if (diamond_support::canonical_json(diamond_support::parse_json("{\"sdk\":true}")) !=
            "{\"sdk\":true}")
            throw std::runtime_error("support API result changed");

        torch::NoGradGuard guard;
        torch::set_num_threads(2);
        for (const std::string family : {"soo", "min"}) {
            const auto artifact =
                diamond_model::validate_deployment_artifact(data / "fixtures" / family, family);
            diamond_model::DiamondModel model(artifact.width, artifact.residual_blocks,
                                              artifact.input_features, artifact.value_size);
            model->load_weights(artifact.weights);
            model->eval();
            const auto [policy, value] = model->forward(torch::zeros(
                {1, 73, artifact.input_features}, torch::TensorOptions().dtype(torch::kFloat32)));
            if (policy.numel() != 5329 || value.numel() != artifact.value_size ||
                !torch::isfinite(policy).all().item<bool>() ||
                !torch::isfinite(value).all().item<bool>() ||
                torch::count_nonzero(value).item<int64_t>() != 0)
                throw std::runtime_error(family + " SDK fixture inference failed");
        }
        std::cout << "SDK core, rating, artifact and Soo/Min CPU inference passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
