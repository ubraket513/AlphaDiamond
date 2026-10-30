#include <filesystem>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

#include "diamond_model/soo_model.hpp"
#include "diamond_orchestration/config.hpp"
#include "diamond_pipeline/pipeline.hpp"
#include "learning_diagnostic.hpp"
#include "diamond_support/build_provenance.hpp"
#include "diamond_support/json.hpp"
#include "diamond_training/checkpoint.hpp"
#include "diamond_training/device.hpp"

namespace {
using Json = diamond_support::JsonValue;
using Object = Json::Object;
using Array = Json::Array;
struct Options {
    std::filesystem::path checkpoint, config, replay, evaluation_replay, checkpoint_out, out;
    std::filesystem::path episode_artifact;
    std::string episode_operation_id;
    std::optional<double> learning_rate;
    std::string device;
    uint64_t iteration = 0, seed = 0;
    std::size_t steps = 0, batch = 0, eval_samples = 0, eval_batch = 0, log_every = 0;
};
uint64_t count(std::string_view value, std::string_view name) {
    std::size_t used = 0;
    const auto parsed = std::stoull(std::string(value), &used);
    if (used != value.size())
        throw std::invalid_argument(std::string(name) + " must be an integer");
    return parsed;
}
Options parse(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        std::cout << "usage: min_learning_diagnostic --checkpoint DIR --config FILE --replay DIR "
                     "--device cpu|cuda|cuda:N --iteration N --steps N --batch-size N "
                     "--eval-samples N --eval-batch N --log-every N --seed N --out FILE "
                     "[--evaluation-replay DIR] [--learning-rate X] [--checkpoint-out DIR] "
                     "[--episode-artifact FILE --episode-operation-id ID]\n";
        std::exit(0);
    }
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (++i == argc)
            throw std::invalid_argument(key + " requires a value");
        const std::string value = argv[i];
        if (key == "--checkpoint")
            o.checkpoint = value;
        else if (key == "--config")
            o.config = value;
        else if (key == "--replay")
            o.replay = value;
        else if (key == "--evaluation-replay")
            o.evaluation_replay = value;
        else if (key == "--checkpoint-out")
            o.checkpoint_out = value;
        else if (key == "--episode-artifact")
            o.episode_artifact = value;
        else if (key == "--episode-operation-id")
            o.episode_operation_id = value;
        else if (key == "--learning-rate") {
            std::size_t used = 0;
            o.learning_rate = std::stod(value, &used);
            if (used != value.size() || !std::isfinite(*o.learning_rate) || *o.learning_rate <= 0)
                throw std::invalid_argument("learning-rate must be finite and positive");
        } else if (key == "--device")
            o.device = value;
        else if (key == "--iteration")
            o.iteration = count(value, key);
        else if (key == "--steps")
            o.steps = count(value, key);
        else if (key == "--batch-size")
            o.batch = count(value, key);
        else if (key == "--eval-samples")
            o.eval_samples = count(value, key);
        else if (key == "--eval-batch")
            o.eval_batch = count(value, key);
        else if (key == "--log-every")
            o.log_every = count(value, key);
        else if (key == "--seed")
            o.seed = count(value, key);
        else if (key == "--out")
            o.out = value;
        else
            throw std::invalid_argument("unknown argument: " + key);
    }
    if (o.checkpoint.empty() || o.config.empty() || o.replay.empty() || o.device.empty() ||
        o.out.empty())
        throw std::invalid_argument("checkpoint, config, replay, device, and out are required");
    if (o.episode_artifact.empty() != o.episode_operation_id.empty())
        throw std::invalid_argument("episode artifact and operation ID must be supplied together");
    return o;
}
diamond_orchestration::ProductionConfig read_config(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("cannot open config");
    const std::string text{std::istreambuf_iterator<char>(in), {}};
    return diamond_orchestration::ProductionConfig::from_json(diamond_support::parse_json(text));
}
std::filesystem::path replay_root(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path / "manifest.json"))
        return path;
    const auto family = path.parent_path();
    const auto schema = family.parent_path();
    if (schema.filename() != "persistent-replay-v2")
        throw std::invalid_argument("replay namespace is not persistent-replay-v2");
    return schema.parent_path();
}
Object metrics(const diamond_pipeline::HeldOutMetrics& m) {
    return {{"target_entropy", Json{m.target_entropy}},
            {"full_cross_entropy", Json{m.full_cross_entropy}},
            {"full_kl", Json{m.full_kl}},
            {"top1_agreement", Json{m.top1_agreement}},
            {"value_mse", Json{m.value_mse}},
            {"legal_cross_entropy", Json{m.legal_cross_entropy}},
            {"legal_kl", Json{m.legal_kl}}};
}

std::set<std::string> game_ids(const diamond_pipeline::ReplayStore& replay) {
    std::ifstream in(replay.manifest_path());
    const std::string text{std::istreambuf_iterator<char>(in), {}};
    const auto manifest = diamond_support::parse_json(text);
    const auto& ids = std::get<Array>(std::get<Object>(manifest.value).at("game_ids").value);
    std::set<std::string> result;
    for (const auto& id : ids)
        result.insert(std::get<std::string>(id.value));
    return result;
}
} // namespace
int main(int argc, char** argv) {
    try {
        const auto o = parse(argc, argv);
        if (std::filesystem::exists(o.out) ||
            (!o.checkpoint_out.empty() && std::filesystem::exists(o.checkpoint_out)))
            throw std::invalid_argument("diagnostic outputs must be new");
        torch::set_num_threads(2);
        torch::set_num_interop_threads(1);
        torch::manual_seed(o.seed);
        auto config = read_config(o.config);
        if (!o.episode_artifact.empty() &&
            std::filesystem::canonical(replay_root(o.replay).parent_path()) !=
                std::filesystem::canonical(o.out.parent_path()))
            throw std::invalid_argument(
                "episode ingestion requires a replay copy inside the output directory");
        if (config.training.recent_sample_fraction > 0 && o.episode_artifact.empty())
            throw std::invalid_argument("recent replay requires an episode artifact");
        if (config.model_name != "Min")
            throw std::invalid_argument("config must be Min");
        const auto device = diamond_training::resolve_device(o.device);
        const auto compatibility = diamond_training::Compatibility::min(
            config.model_version,
            {.residual_blocks = config.network.residual_blocks, .width = config.network.width},
            soo::encoding_version_from_name(config.encoder_version));
        auto model =
            diamond_model::DiamondModel(config.network.width, config.network.residual_blocks,
                                        compatibility.input_features(), 3);
        diamond_training::Trainer trainer(
            model, compatibility,
            {.learning_rate = config.training.learning_rate,
             .weight_decay = config.training.weight_decay,
             .policy_loss_domain = config.training.policy_loss_domain == "legal"
                                       ? diamond_training::PolicyLossDomain::legal
                                       : diamond_training::PolicyLossDomain::full},
            device);
        const auto checkpoint = diamond_training::load_checkpoint_v3(
            o.checkpoint, trainer, device, diamond_training::CheckpointLoadIntent::exact_resume);
        Array loaded_learning_rates, effective_learning_rates;
        for (auto& group : trainer.optimizer().param_groups()) {
            loaded_learning_rates.emplace_back(Json{group.options().get_lr()});
            if (o.learning_rate)
                group.options().set_lr(*o.learning_rate);
            effective_learning_rates.emplace_back(Json{group.options().get_lr()});
        }
        if (trainer.optimizer().param_groups().size() != 1)
            throw std::invalid_argument("diagnostic expects one optimizer parameter group");
        config.training.learning_rate =
            trainer.optimizer().param_groups().front().options().get_lr();
        trainer.optimizer().defaults().set_lr(config.training.learning_rate);
        auto training_config = trainer.config();
        training_config.learning_rate = config.training.learning_rate;
        trainer.restore_checkpoint_state(training_config, trainer.training_step());
        diamond_pipeline::ReplayStore replay(
            replay_root(o.replay), compatibility, config.replay.capacity, config.replay.seed,
            diamond_pipeline::ReplayContents::full, diamond_pipeline::ReplayOpenMode::must_exist);
        diamond_pipeline::ReplayIngestReport ingest;
        if (!o.episode_artifact.empty()) {
            const auto episodes = diamond_pipeline::load_episode_artifact(
                o.episode_artifact, o.episode_operation_id, compatibility);
            for (const auto& episode : episodes)
                if (!episode.completed || episode.samples.empty())
                    throw std::invalid_argument(
                        "diagnostic ingestion requires completed episodes with targets");
            ingest = replay.ingest_iteration(episodes);
            if (ingest.duplicate_games != 0 || ingest.accepted_samples == 0 ||
                ingest.accepted_samples > replay.size())
                throw std::invalid_argument(
                    "diagnostic ingestion must add a new retained sample tail");
        }
        std::unique_ptr<diamond_pipeline::ReplayStore> validation;
        if (!o.evaluation_replay.empty()) {
            validation = std::make_unique<diamond_pipeline::ReplayStore>(
                replay_root(o.evaluation_replay), compatibility, config.replay.capacity,
                config.replay.seed, diamond_pipeline::ReplayContents::full,
                diamond_pipeline::ReplayOpenMode::must_exist);
            const auto training_ids = game_ids(replay);
            for (const auto& id : game_ids(*validation))
                if (training_ids.contains(id))
                    throw std::invalid_argument("training and evaluation replay share game IDs");
        }
        const auto& evaluation_replay = validation ? *validation : replay;
        const auto result = diamond_pipeline::run_min_learning_diagnostic(
            trainer, replay, evaluation_replay,
            {.iteration = o.iteration,
             .steps = o.steps,
             .batch_size = o.batch,
             .evaluation_samples = o.eval_samples,
             .evaluation_batch = o.eval_batch,
             .log_every = o.log_every,
             .seed = o.seed,
             .recent_samples = ingest.accepted_samples,
             .recent_sample_fraction = config.training.recent_sample_fraction});
        std::string saved_digest;
        if (!o.checkpoint_out.empty()) {
            const auto saved = diamond_training::save_checkpoint_v3(
                o.checkpoint_out, trainer,
                {.initialization_mode = diamond_training::CheckpointInitializationMode::resume,
                 .run_id = "learning-diagnostic",
                 .iteration = o.iteration,
                 .model_step = trainer.training_step(),
                 .parent_digest = checkpoint.model_digest,
                 .source_digest = checkpoint.model_digest,
                 .source_training_step = checkpoint.training_step,
                 .optimizer_restored = true},
                {.resolved_config_bytes = diamond_support::canonical_json(config.to_json()),
                 .replay_manifest_sha256 = replay.manifest_digest()});
            saved_digest = saved.model_digest;
        }
        Array steps;
        for (const auto& row : result.steps) {
            Object groups;
            for (const auto& [group, n] : row.groups)
                groups.emplace(diamond_training::parameter_group_name(group),
                               Json{Object{{"parameter_l2", Json{n.parameter_l2}},
                                           {"gradient_l2", Json{n.gradient_l2}},
                                           {"update_l2", Json{n.update_l2}},
                                           {"relative_update", Json{n.relative_update}}}});
            steps.emplace_back(
                Json{Object{{"local_step", Json{static_cast<int64_t>(row.local_step)}},
                            {"training_step", Json{static_cast<int64_t>(row.losses.training_step)}},
                            {"policy_loss", Json{row.losses.policy_loss}},
                            {"value_loss", Json{row.losses.value_loss}},
                            {"groups", Json{std::move(groups)}}}});
        }
        const Json report{Object{
            {"schema_version", Json{int64_t{1}}},
            {"checkpoint_training_step", Json{static_cast<int64_t>(checkpoint.training_step)}},
            {"model_sha256", Json{checkpoint.model_digest}},
            {"replay_size", Json{static_cast<int64_t>(replay.size())}},
            {"ingested_games", Json{static_cast<int64_t>(ingest.accepted_games)}},
            {"ingested_samples", Json{static_cast<int64_t>(ingest.accepted_samples)}},
            {"episode_artifact", Json{o.episode_artifact.string()}},
            {"episode_operation_id", Json{o.episode_operation_id}},
            {"recent_sample_fraction", Json{config.training.recent_sample_fraction}},
            {"recent_samples_drawn", Json{static_cast<int64_t>(result.recent_samples_drawn)}},
            {"historical_samples_drawn",
             Json{static_cast<int64_t>(result.historical_samples_drawn)}},
            {"replay_manifest_sha256", Json{replay.manifest_digest()}},
            {"evaluation_replay_manifest_sha256", Json{evaluation_replay.manifest_digest()}},
            {"evaluation_scope",
             Json{std::string(validation ? "disjoint_games" : "training_pool_reference")}},
            {"loaded_learning_rates", Json{std::move(loaded_learning_rates)}},
            {"effective_learning_rates", Json{std::move(effective_learning_rates)}},
            {"saved_checkpoint", Json{o.checkpoint_out.string()}},
            {"saved_model_sha256", Json{saved_digest}},
            {"config_path", Json{o.config.string()}},
            {"device", Json{device.canonical_name}},
            {"iteration", Json{static_cast<int64_t>(o.iteration)}},
            {"steps_requested", Json{static_cast<int64_t>(o.steps)}},
            {"batch_size", Json{static_cast<int64_t>(o.batch)}},
            {"eval_samples", Json{static_cast<int64_t>(o.eval_samples)}},
            {"eval_batch", Json{static_cast<int64_t>(o.eval_batch)}},
            {"log_every", Json{static_cast<int64_t>(o.log_every)}},
            {"seed", Json{static_cast<int64_t>(o.seed)}},
            {"provenance", diamond_support::parse_json(diamond_support::build_provenance_json())},
            {"initial", Json{metrics(result.initial)}},
            {"final", Json{metrics(result.final)}},
            {"drift",
             Json{Object{{"policy_kl", Json{result.drift.policy_kl}},
                         {"logit_rms_delta", Json{result.drift.logit_rms_delta}},
                         {"value_rms_delta", Json{result.drift.value_rms_delta}},
                         {"top1_agreement_delta", Json{result.drift.top1_agreement_delta}},
                         {"full_cross_entropy_delta", Json{result.drift.full_cross_entropy_delta}},
                         {"value_mse_delta", Json{result.drift.value_mse_delta}}}}},
            {"step_diagnostics", Json{std::move(steps)}}}};
        std::ofstream out(o.out, std::ios::binary | std::ios::trunc);
        if (!out)
            throw std::runtime_error("cannot open output");
        out << diamond_support::canonical_json(report) << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "min_learning_diagnostic: " << e.what() << '\n';
        return 2;
    }
}
