// Standalone generation/diagnosis using the same actors as training. It never
// updates a model or ingests replay. Stored job seeds reproduce a training batch;
// those diagnostic games must not be presented as held-out evaluation evidence.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <cmath>
#include <string_view>
#include <vector>

#include <torch/torch.h>

#include "diamond_model/deployment_artifact.hpp"
#include "diamond_model/soo_model.hpp"
#include "diamond_orchestration/config.hpp"
#include "diamond_orchestration/training_wiring.hpp"
#include "diamond_pipeline/model_pool.hpp"
#include "diamond_pipeline/policy_diagnostics.hpp"
#include "diamond_support/build_provenance.hpp"
#include "diamond_support/json.hpp"
#include "diamond_training/checkpoint.hpp"
#include "diamond_training/device.hpp"
#include "soo/board.hpp"
#include "soo/min_experts.hpp"
#include "soo/search_target_metrics.hpp"
#include "soo/selfplay.hpp"

namespace {

struct Options {
    // Presence, not equality with a synthetic default, determines whether a
    // diagnostic CLI setting overrides the production configuration.
    std::set<std::string_view> explicit_options;
    std::filesystem::path artifact = "models/soo/2.0.0";
    bool artifact_explicit = false;
    std::optional<std::filesystem::path> checkpoint;
    std::optional<std::filesystem::path> config;
    std::string device = "cpu";
    std::string precision = "fp32";
    std::size_t repetitions = 1;
    std::size_t warmups = 1;
    std::size_t lanes = 2;
    std::size_t threads = 1;
    std::size_t max_batch = 2;
    std::size_t simulations = 4;
    std::optional<std::size_t> simulations_late;
    std::optional<std::size_t> repeat_window;
    std::optional<double> repetition_temperature;
    std::size_t max_moves = 8;
    std::optional<double> max_game_seconds;
    std::string bootstrap_prior = "config";
    std::size_t diagnostic_roots = 0;
    std::size_t diagnostic_batch = 32;
    uint64_t max_wait_us = 200;
    // Games are deliberately separate from lanes. With one game per lane the
    // job queue never engages: a lane cannot take fresh work when its game
    // ends, so the run finishes at the pace of its slowest game while the rest
    // idle, and the throughput that comes out is not the throughput production
    // sees. The production config rejects that shape outright; this benchmark
    // used to be built that way, which is why an earlier findings table had to
    // have its throughput column retracted. Without --config, zero keeps the
    // historical "one per lane" default. Configured runs inherit iteration jobs.
    std::size_t games = 0;
    std::optional<std::size_t> torch_threads;
    uint64_t seed = 17;
    // Exact persisted job seeds allow a failed production batch to be replayed
    // without replacing its RNG stream with the synthetic benchmark seed scheme.
    std::vector<uint64_t> game_seeds;
    // Production exploration. Without it lanes play near-identical games and
    // the request stream is not the shape the batcher sees in a real run.
    double temperature = 1.0;
    std::size_t temperature_moves = 20;
    double dirichlet_epsilon = 0.25;
    bool balance_turn_orders = false;
    std::string turn_order_group = "all";
    std::optional<std::filesystem::path> scratch;
};

std::size_t parse_count(std::string_view value, std::string_view option, bool allow_zero = false) {
    unsigned long long parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() ||
        (!allow_zero && parsed == 0) || parsed > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument(std::string(option) + (allow_zero
                                                               ? " must be a non-negative integer"
                                                               : " must be a positive integer"));
    }
    return static_cast<std::size_t>(parsed);
}

double parse_number(std::string_view value, std::string_view option) {
    const std::string text(value);
    try {
        std::size_t consumed = 0;
        const double parsed = std::stod(text, &consumed);
        if (consumed != text.size() || !std::isfinite(parsed))
            throw std::invalid_argument("bad");
        return parsed;
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string(option) + " must be a finite number");
    }
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string_view option = argv[index];
        if (option == "--help") {
            std::cout
                << "usage: selfplay_benchmark [--artifact DIR | --checkpoint DIR --config FILE] "
                   "[--device cpu|cuda|cuda:N] "
                   "[--precision fp32|fp16|bf16] "
                   "[--lanes N] [--threads N] [--torch-threads N] [--max-batch N] [--max-wait-us "
                   "N] "
                   "[--simulations N] [--max-moves N] [--games N] [--seed N | --game-seeds FILE] "
                   "[--bootstrap-prior config|vacancy|none] [--simulations-late N] "
                   "[--repeat-window N] [--repetition-temperature F] [--max-game-seconds F] "
                   "[--diagnostic-roots N] [--diagnostic-batch N] "
                   "[--temperature F] [--temperature-moves N] "
                   "[--dirichlet-epsilon F] [--warmups N] [--repetitions N] "
                   "[--scratch PATH]\n"
                   "With --config, production workload/runtime settings apply unless explicitly "
                   "overridden. Without it, the small synthetic defaults apply.\n";
            std::exit(0);
        }
        if (++index == argc)
            throw std::invalid_argument(std::string(option) + " requires a value");
        const std::string_view value = argv[index];
        options.explicit_options.insert(option);
        if (option == "--artifact") {
            options.artifact = value;
            options.artifact_explicit = true;
        } else if (option == "--checkpoint")
            options.checkpoint = value;
        else if (option == "--config")
            options.config = value;
        else if (option == "--device")
            options.device = value;
        else if (option == "--precision")
            options.precision = value;
        else if (option == "--lanes")
            options.lanes = parse_count(value, option);
        else if (option == "--threads")
            options.threads = parse_count(value, option);
        else if (option == "--torch-threads")
            options.torch_threads = parse_count(value, option);
        else if (option == "--max-batch")
            options.max_batch = parse_count(value, option);
        else if (option == "--max-wait-us")
            options.max_wait_us = parse_count(value, option, true);
        else if (option == "--simulations")
            options.simulations = parse_count(value, option);
        else if (option == "--simulations-late")
            options.simulations_late = parse_count(value, option, true);
        else if (option == "--repeat-window")
            options.repeat_window = parse_count(value, option, true);
        else if (option == "--repetition-temperature")
            options.repetition_temperature = parse_number(value, option);
        else if (option == "--max-moves")
            options.max_moves = parse_count(value, option);
        else if (option == "--max-game-seconds")
            options.max_game_seconds = parse_number(value, option);
        else if (option == "--bootstrap-prior")
            options.bootstrap_prior = value;
        else if (option == "--diagnostic-roots")
            options.diagnostic_roots = parse_count(value, option, true);
        else if (option == "--diagnostic-batch")
            options.diagnostic_batch = parse_count(value, option);
        else if (option == "--warmups")
            options.warmups = parse_count(value, option, true);
        else if (option == "--repetitions")
            options.repetitions = parse_count(value, option);
        else if (option == "--games")
            options.games = parse_count(value, option);
        else if (option == "--seed")
            options.seed = parse_count(value, option, true);
        else if (option == "--game-seeds") {
            std::ifstream input{std::filesystem::path(value)};
            if (!input) throw std::invalid_argument("cannot open --game-seeds file");
            uint64_t seed = 0;
            while (input >> seed) options.game_seeds.push_back(seed);
            if (!input.eof() || options.game_seeds.empty())
                throw std::invalid_argument("--game-seeds requires a nonempty integer list");
        } else if (option == "--temperature")
            options.temperature = parse_number(value, option);
        else if (option == "--temperature-moves")
            options.temperature_moves = parse_count(value, option, true);
        else if (option == "--dirichlet-epsilon")
            options.dirichlet_epsilon = parse_number(value, option);
        else if (option == "--scratch")
            options.scratch = value;
        else
            throw std::invalid_argument("unknown argument: " + std::string(option));
    }
    if (options.precision != "fp32" && options.precision != "fp16" &&
        options.precision != "bf16")
        throw std::invalid_argument("--precision must be fp32, fp16, or bf16");
    if (options.checkpoint.has_value() != options.config.has_value())
        throw std::invalid_argument("--checkpoint and --config must be provided together");
    if (options.checkpoint && options.artifact_explicit)
        throw std::invalid_argument("--artifact and --checkpoint are mutually exclusive");
    if (!options.game_seeds.empty() && options.explicit_options.contains("--seed"))
        throw std::invalid_argument("--seed and --game-seeds are mutually exclusive");
    if (options.bootstrap_prior != "config" && options.bootstrap_prior != "vacancy" &&
        options.bootstrap_prior != "none")
        throw std::invalid_argument("--bootstrap-prior must be config, vacancy, or none");
    if (options.max_game_seconds && *options.max_game_seconds < 0.0)
        throw std::invalid_argument("--max-game-seconds must be non-negative");
    if (options.repetition_temperature && *options.repetition_temperature < 0.0)
        throw std::invalid_argument("--repetition-temperature must be non-negative");
    if (!(options.temperature >= 0.0) || !(options.dirichlet_epsilon >= 0.0) ||
        options.dirichlet_epsilon > 1.0) {
        throw std::invalid_argument(
            "--temperature must be non-negative and --dirichlet-epsilon in [0, 1]");
    }
    const auto int_max = static_cast<std::size_t>(std::numeric_limits<int>::max());
    if (options.lanes > int_max || options.threads > int_max ||
        options.torch_threads.value_or(0) > int_max || options.max_batch > int_max ||
        options.max_wait_us > static_cast<uint64_t>(std::numeric_limits<int>::max()) ||
        options.simulations > int_max || options.simulations_late.value_or(0) > int_max ||
        options.repeat_window.value_or(0) > int_max || options.max_moves > int_max ||
        options.temperature_moves > int_max) {
        throw std::invalid_argument("self-play workload counts must fit in a signed integer");
    }
    return options;
}

soo::Match make_match(bool min_model) {
    return min_model ? soo::standard_min_match() : soo::standard_soo_match();
}

diamond_orchestration::ProductionConfig read_config(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::invalid_argument("cannot open config: " + path.string());
    return diamond_orchestration::ProductionConfig::from_json(
        diamond_support::parse_json(std::string(std::istreambuf_iterator<char>(input), {})));
}

soo::State opening(const soo::Match& match) {
    soo::State state;
    for (std::size_t seat = 0; seat < match.count; ++seat) {
        for (const uint8_t position : soo::topology().camp_positions[match.players[seat].camp]) {
            state.occupancy[position] = match.players[seat].id;
        }
    }
    state.current_player = match.players[0].id;
    return state;
}

struct Totals {
    std::size_t attempted = 0;
    std::size_t completed = 0;
    std::size_t aborted = 0;
    std::size_t completed_samples = 0;
    std::size_t max_move_aborts = 0;
    std::size_t deadline_aborts = 0;
    std::size_t other_aborts = 0;
    uint64_t evaluations = 0;
    uint64_t batches = 0;
    uint64_t moves = 0;
    uint64_t boosted_moves = 0;
    uint64_t repetition_moves = 0;
    double wall_seconds = 0.0;
    double evaluator_seconds = 0.0;
    double collation_seconds = 0.0;
    double h2d_seconds = 0.0;
    double forward_seconds = 0.0;
    double postprocess_seconds = 0.0;
    double d2h_seconds = 0.0;
    double scatter_seconds = 0.0;
    double worker_busy_seconds = 0.0;
    std::vector<uint32_t> batch_sizes;
    std::vector<uint32_t> move_counts;
    std::vector<soo::VisitTargetObservation> search_targets;
    std::vector<soo::EpisodeMove> diagnostic_moves;
    double revisit_fraction_total = 0.0;
    double repeat_within_8_fraction_total = 0.0;
    double max_revisits_total = 0.0;
    uint64_t cycling_games = 0;
    std::array<uint64_t, 4> first_finisher_counts{}; // indexed by player id
    // Requested/completed/aborted by actual job order, including override matches.
    std::map<std::string, std::array<uint64_t, 3>> turn_orders;
};

int dominant_cycle_period(const std::vector<uint64_t>& tail, int longest = 32) {
    const auto size = static_cast<int>(tail.size());
    if (size < 4)
        return 0;
    for (int period = 1; period <= longest && period * 4 <= size; ++period) {
        bool periodic = true;
        for (int back = 0; back < period * 3 && periodic; ++back) {
            const int here = size - 1 - back;
            const int previous = here - period;
            if (previous < 0 ||
                tail[static_cast<std::size_t>(here)] != tail[static_cast<std::size_t>(previous)])
                periodic = false;
        }
        if (periodic)
            return period;
    }
    return 0;
}

soo::EpisodeConfig episode_config(const Options& options, soo::EpisodeConfig config) {
    // Configured runs start with the exact production wiring. Synthetic runs
    // keep their historical small defaults; only explicit flags may replace a
    // production value, including an explicit zero that disables a feature.
    const auto should_override = [&](std::string_view flag) {
        return !options.config || options.explicit_options.contains(flag);
    };
    if (should_override("--lanes"))
        config.lanes = static_cast<int>(options.lanes);
    if (should_override("--threads"))
        config.threads = static_cast<int>(options.threads);
    if (should_override("--max-batch"))
        config.max_batch = static_cast<int>(options.max_batch);
    if (should_override("--max-wait-us"))
        config.max_wait_us = static_cast<int>(options.max_wait_us);
    if (should_override("--simulations"))
        config.simulations = static_cast<int>(options.simulations);
    if (should_override("--max-moves"))
        config.max_moves = static_cast<int>(options.max_moves);
    if (should_override("--temperature"))
        config.temperature = options.temperature;
    if (should_override("--temperature-moves"))
        config.temperature_moves = static_cast<int>(options.temperature_moves);
    if (should_override("--dirichlet-epsilon"))
        config.dirichlet_epsilon = options.dirichlet_epsilon;
    if (should_override("--simulations-late"))
        config.simulations_late = static_cast<int>(options.simulations_late.value_or(0));
    if (should_override("--repeat-window"))
        config.repeat_window = static_cast<int>(options.repeat_window.value_or(0));
    if (should_override("--repetition-temperature"))
        config.repetition_temperature = options.repetition_temperature.value_or(0.0);
    // Training applies the deadline outside wire_training_iteration. The
    // resolved option already contains its configured value or explicit override.
    config.max_game_duration =
        options.max_game_seconds && *options.max_game_seconds > 0.0
            ? std::optional{std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                  std::chrono::duration<double>(*options.max_game_seconds))}
            : std::nullopt;
    if (options.bootstrap_prior == "vacancy") {
        config.bootstrap_prior = true;
        config.bootstrap_prior_weight = 1.0;
    } else if (options.bootstrap_prior == "none") {
        config.bootstrap_prior = false;
        config.bootstrap_prior_weight = 0.0;
    }
    if (config.threads > config.lanes)
        throw std::invalid_argument("threads must not exceed lanes");
    if (options.games != 0 && options.games <= static_cast<std::size_t>(config.lanes))
        throw std::invalid_argument(
            "games must exceed lanes, otherwise the job queue never engages");
    if (config.repeat_window > 0 && config.simulations_late == 0 &&
        config.repetition_temperature == 0.0)
        throw std::invalid_argument(
            "--repeat-window requires a search budget or repetition temperature");
    return config;
}

Totals run_once(const Options& options, const soo::Match& match, const soo::State& initial,
                diamond_pipeline::ModelPool& evaluator, const soo::EpisodeConfig& config) {
    const std::size_t job_count = options.games != 0 ? options.games : config.lanes;
    std::vector<soo::EpisodeJob> jobs;
    jobs.reserve(job_count);
    if (!options.game_seeds.empty() && options.game_seeds.size() != job_count)
        throw std::invalid_argument("--game-seeds count must equal the requested game count");
    // Benchmark seeds stay reproducible independently of persisted training
    // iteration/retry state; the configured base seed is still honored.
    for (std::size_t game = 0; game < job_count; ++game)
        jobs.push_back({initial, options.game_seeds.empty() ? options.seed + game * 12
                                                          : options.game_seeds[game]});
    if (options.balance_turn_orders) {
        const auto group =
            options.turn_order_group == "all"
                ? std::nullopt
                : std::optional{soo::min_turn_order_group_from_name(options.turn_order_group)};
        soo::balance_episode_turn_orders(match, jobs, group);
    }

    soo::EpisodeMetrics metrics;
    evaluator.reset_evaluation_stats();
    const auto episodes = soo::run_episodes(match, jobs, config, evaluator, metrics);
    const auto stage = evaluator.accumulated_evaluation_stats();

    Totals totals;
    totals.attempted = episodes.size();
    totals.evaluations = metrics.evaluations;
    totals.batches = metrics.batches;
    totals.moves = metrics.moves;
    totals.boosted_moves = metrics.boosted_moves;
    totals.repetition_moves = metrics.repetition_moves;
    totals.wall_seconds = metrics.wall_seconds;
    totals.evaluator_seconds = metrics.evaluator_seconds;
    totals.collation_seconds = stage.collation_seconds;
    totals.h2d_seconds = stage.h2d_seconds;
    totals.forward_seconds = stage.forward_seconds;
    totals.postprocess_seconds = stage.policy_postprocess_seconds;
    totals.d2h_seconds = stage.d2h_seconds;
    totals.scatter_seconds = stage.scatter_seconds;
    totals.worker_busy_seconds = metrics.worker_busy_seconds;
    totals.batch_sizes = std::move(metrics.batch_sizes);
    for (std::size_t index = 0; index < episodes.size(); ++index) {
        const auto& episode = episodes[index];
        const auto& game_match = jobs[index].match ? *jobs[index].match : match;
        std::string order;
        for (int seat = 0; seat < game_match.count; ++seat) {
            if (seat) order += '-';
            order += std::to_string(game_match.players[seat].id);
        }
        auto& counts = totals.turn_orders[order];
        ++counts[0];
        ++counts[episode.completed ? 1 : 2];
        totals.completed += episode.completed ? 1 : 0;
        if (episode.completed && !episode.finish_order.empty() &&
            episode.finish_order.front() < totals.first_finisher_counts.size())
            ++totals.first_finisher_counts[episode.finish_order.front()];
        totals.aborted += episode.completed ? 0 : 1;
        totals.completed_samples += episode.completed ? episode.moves.size() : 0;
        totals.move_counts.push_back(static_cast<uint32_t>(episode.move_count));
        const double observations = static_cast<double>(episode.diagnostics.observations);
        if (observations > 0.0) {
            totals.revisit_fraction_total +=
                1.0 - static_cast<double>(episode.diagnostics.unique_positions) / observations;
            totals.repeat_within_8_fraction_total +=
                static_cast<double>(episode.diagnostics.repeat_within_8) / observations;
        }
        totals.max_revisits_total += episode.diagnostics.max_revisits;
        if (dominant_cycle_period(episode.diagnostics.recent_keys) > 0)
            ++totals.cycling_games;
        for (const auto& move : episode.moves) {
            totals.search_targets.push_back(soo::inspect_visit_target(move.visit_counts));
            if (options.diagnostic_roots > 0)
                totals.diagnostic_moves.push_back(move);
        }
        if (!episode.completed) {
            if (episode.move_limit_exceeded)
                ++totals.max_move_aborts;
            else if (episode.max_game_seconds_exceeded)
                ++totals.deadline_aborts;
            else
                ++totals.other_aborts;
        }
    }
    return totals;
}

void accumulate(Totals& destination, Totals source) {
    destination.attempted += source.attempted;
    destination.completed += source.completed;
    destination.aborted += source.aborted;
    destination.completed_samples += source.completed_samples;
    destination.max_move_aborts += source.max_move_aborts;
    destination.deadline_aborts += source.deadline_aborts;
    destination.other_aborts += source.other_aborts;
    destination.evaluations += source.evaluations;
    destination.batches += source.batches;
    destination.moves += source.moves;
    destination.boosted_moves += source.boosted_moves;
    destination.repetition_moves += source.repetition_moves;
    destination.wall_seconds += source.wall_seconds;
    destination.evaluator_seconds += source.evaluator_seconds;
    destination.collation_seconds += source.collation_seconds;
    destination.h2d_seconds += source.h2d_seconds;
    destination.forward_seconds += source.forward_seconds;
    destination.postprocess_seconds += source.postprocess_seconds;
    destination.d2h_seconds += source.d2h_seconds;
    destination.scatter_seconds += source.scatter_seconds;
    destination.worker_busy_seconds += source.worker_busy_seconds;
    destination.batch_sizes.insert(destination.batch_sizes.end(), source.batch_sizes.begin(),
                                   source.batch_sizes.end());
    destination.move_counts.insert(destination.move_counts.end(), source.move_counts.begin(),
                                   source.move_counts.end());
    destination.search_targets.insert(destination.search_targets.end(),
                                      source.search_targets.begin(), source.search_targets.end());
    destination.diagnostic_moves.insert(destination.diagnostic_moves.end(),
                                        source.diagnostic_moves.begin(),
                                        source.diagnostic_moves.end());
    destination.revisit_fraction_total += source.revisit_fraction_total;
    destination.repeat_within_8_fraction_total += source.repeat_within_8_fraction_total;
    destination.max_revisits_total += source.max_revisits_total;
    destination.cycling_games += source.cycling_games;
    for (const auto& [order, counts] : source.turn_orders)
        for (std::size_t index = 0; index < counts.size(); ++index)
            destination.turn_orders[order][index] += counts[index];
    for (std::size_t player = 1; player < destination.first_finisher_counts.size(); ++player)
        destination.first_finisher_counts[player] += source.first_finisher_counts[player];
}

struct PolicyTotals {
    uint64_t rows = 0;
    double target_entropy = 0.0;
    double full_cross_entropy = 0.0;
    double legal_cross_entropy = 0.0;
    double full_kl = 0.0;
    double legal_kl = 0.0;
    double legal_mass = 0.0;
    uint64_t top1_agrees = 0;
};

PolicyTotals diagnose_roots(std::vector<soo::EpisodeMove> moves, std::size_t limit,
                            std::size_t batch_size, uint64_t seed,
                            diamond_model::DiamondModel& model,
                            const diamond_training::ResolvedDevice& device) {
    if (limit == 0 || moves.empty())
        return {};
    std::mt19937_64 random(seed);
    for (std::size_t index = limit; index < moves.size(); ++index) {
        std::uniform_int_distribution<std::size_t> choose(0, index);
        const std::size_t selected = choose(random);
        if (selected < limit)
            moves[selected] = std::move(moves[index]);
    }
    moves.resize(std::min(limit, moves.size()));
    model->to(device.torch_device, torch::kFloat32);
    model->eval();
    torch::NoGradGuard no_grad;
    PolicyTotals totals;
    for (std::size_t begin = 0; begin < moves.size(); begin += batch_size) {
        const std::size_t count = std::min(batch_size, moves.size() - begin);
        const std::size_t feature_count = moves[begin].features.feature_count;
        std::vector<float> flat;
        flat.reserve(count * soo::kBoardSize * feature_count);
        for (std::size_t row = 0; row < count; ++row) {
            const auto& features = moves[begin + row].features;
            if (features.feature_count != feature_count)
                throw std::runtime_error("diagnostic roots have mixed feature widths");
            flat.insert(flat.end(), features.node_features.begin(), features.node_features.end());
        }
        auto input = torch::from_blob(flat.data(),
                                      {static_cast<int64_t>(count), soo::kBoardSize,
                                       static_cast<int64_t>(feature_count)},
                                      torch::kFloat32)
                         .clone()
                         .to(device.torch_device);
        auto [policy, value] = model->forward(input);
        (void)value;
        policy = policy.to(torch::kCPU, torch::kFloat32).contiguous();
        const float* logits = policy.data_ptr<float>();
        for (std::size_t row = 0; row < count; ++row) {
            const auto& move = moves[begin + row];
            const auto diagnostic = diamond_pipeline::diagnose_policy_row(
                std::span<const float>(logits + row * soo::kActionSize, soo::kActionSize),
                move.root_actions, move.visit_counts);
            ++totals.rows;
            totals.target_entropy += diagnostic.target_entropy;
            totals.full_cross_entropy += diagnostic.full_cross_entropy;
            totals.legal_cross_entropy += diagnostic.legal_cross_entropy;
            totals.full_kl += diagnostic.full_kl;
            totals.legal_kl += diagnostic.legal_kl;
            totals.legal_mass += diagnostic.legal_probability_mass;
            totals.top1_agrees += diagnostic.top1_agrees ? 1 : 0;
        }
    }
    return totals;
}

uint32_t percentile(std::vector<uint32_t> values, double fraction) {
    if (values.empty())
        return 0;
    std::sort(values.begin(), values.end());
    const auto index = static_cast<std::size_t>(fraction * static_cast<double>(values.size() - 1));
    return values[index];
}

double ratio(double numerator, double denominator) {
    return denominator > 0.0 ? numerator / denominator : 0.0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Options options = parse_options(argc, argv);
        soo::ensure_topology_configured();
        std::optional<diamond_orchestration::ProductionConfig> production_config;
        soo::EpisodeConfig selfplay;
        selfplay.bootstrap_prior_weight = 0.0;
        if (options.config) {
            production_config = read_config(*options.config);
            const auto& config = *production_config;
            const auto wiring = diamond_orchestration::wire_training_iteration(config);
            selfplay = wiring.selfplay;
            if (!options.explicit_options.contains("--device"))
                options.device = config.runtime.device;
            if (!options.explicit_options.contains("--precision"))
                options.precision = config.runtime.precision;
            if (!options.explicit_options.contains("--games"))
                options.games = wiring.games_per_iteration;
            if (!options.explicit_options.contains("--seed"))
                // Use the production root seed, not the obsolete self_play.seed
                // metadata. --seed remains an explicit standalone experiment
                // override; benchmark job IDs differ from a durable train run.
                options.seed = config.run_seed;
            if (!options.max_game_seconds)
                options.max_game_seconds = config.self_play.max_game_seconds;
            options.balance_turn_orders = config.self_play.balance_turn_orders;
            options.turn_order_group = config.self_play.turn_order_group;
        }
        selfplay = episode_config(options, selfplay);
        torch::set_num_threads(static_cast<int>(options.torch_threads.value_or(selfplay.threads)));
        torch::set_num_interop_threads(1);

        const auto device = diamond_training::resolve_device(options.device);
        diamond_training::Compatibility compatibility;
        diamond_model::DiamondModel model;
        std::string model_sha256;
        std::string runtime_sha256;
        soo::EncodingVersion encoding = soo::EncodingVersion::legacy_v1;
        bool min_model = false;
        if (options.checkpoint) {
            const auto& config = *production_config;
            encoding = soo::encoding_version_from_name(config.encoder_version);
            min_model = config.model_name == "Min";
            compatibility = min_model
                                ? diamond_training::Compatibility::min(
                                      config.model_version,
                                      {.residual_blocks = config.network.residual_blocks,
                                       .width = config.network.width}, encoding)
                                : diamond_training::Compatibility::soo(
                                      config.model_version,
                                      {.residual_blocks = config.network.residual_blocks,
                                       .width = config.network.width}, encoding);
            model = diamond_model::DiamondModel(config.network.width,
                                                config.network.residual_blocks,
                                                compatibility.input_features(), min_model ? 3 : 1);
            // Match production construction before the checkpoint loader
            // validates and restores the model's parameters and buffers.
            model->set_adjacency(diamond_model::topology_adjacency());
            const auto checkpoint =
                diamond_training::load_checkpoint_v2_weights(*options.checkpoint, model, device);
            model_sha256 = checkpoint.model_digest;
            // A native checkpoint has no separately packaged runtime tree. The
            // executable provenance identifies the runtime; keep this SHA field
            // stable for schema consumers and identify the source below.
            runtime_sha256 = checkpoint.model_digest;
        } else {
            const auto artifact =
                diamond_model::validate_deployment_artifact(options.artifact, "soo");
            encoding = soo::encoding_version_from_name(artifact.encoder_version);
            compatibility = diamond_training::Compatibility::soo(
                artifact.model_version,
                {.residual_blocks = artifact.residual_blocks, .width = artifact.width}, encoding);
            model = diamond_model::DiamondModel(artifact.width, artifact.residual_blocks,
                                                artifact.input_features, artifact.value_size);
            model->load_weights(artifact.weights);
            model_sha256 = artifact.model_sha256;
            runtime_sha256 = artifact.runtime_sha256;
        }
        selfplay.encoding = encoding;
        const auto precision = options.precision == "fp16"
                                   ? diamond_pipeline::InferencePrecision::fp16
                                   : options.precision == "bf16"
                                         ? diamond_pipeline::InferencePrecision::bf16
                                         : diamond_pipeline::InferencePrecision::fp32;
        diamond_pipeline::ModelPool evaluator(1, device, precision);
        const auto model_key = evaluator.install(compatibility, model);
        evaluator.activate(model_key);

        const soo::Match match = make_match(min_model);
        const soo::State initial = opening(match);
        for (std::size_t warmup = 0; warmup < options.warmups; ++warmup)
            (void)run_once(options, match, initial, evaluator, selfplay);

        Totals totals;
        std::vector<double> seconds;
        seconds.reserve(options.repetitions);
        for (std::size_t repetition = 0; repetition < options.repetitions; ++repetition) {
            const auto started = std::chrono::steady_clock::now();
            Totals current = run_once(options, match, initial, evaluator, selfplay);
            seconds.push_back(
                std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
            accumulate(totals, std::move(current));
        }

        auto sorted_seconds = seconds;
        std::sort(sorted_seconds.begin(), sorted_seconds.end());
        const auto max_batch =
            totals.batch_sizes.empty()
                ? 0U
                : *std::max_element(totals.batch_sizes.begin(), totals.batch_sizes.end());
        uint64_t total_batch_rows = 0;
        for (const uint32_t size : totals.batch_sizes)
            total_batch_rows += size;
        double measured_seconds = 0.0;
        for (const double value : seconds)
            measured_seconds += value;
        const auto search_targets = soo::summarize_visit_targets(totals.search_targets);
        const auto policy =
            diagnose_roots(std::move(totals.diagnostic_moves), options.diagnostic_roots,
                           options.diagnostic_batch, options.seed, model, device);
        const double policy_rows = static_cast<double>(policy.rows);
        const double episode_count = static_cast<double>(totals.attempted);

        std::cout
            << "{\"schema_version\":1,\"benchmark\":\"selfplay\",\"workload\":{\"repetitions\":"
            << options.repetitions << ",\"warmups\":" << options.warmups
            << ",\"configured\":" << (options.config ? "true" : "false")
            << ",\"games\":" << (options.games != 0 ? options.games : selfplay.lanes)
            << ",\"queued\":"
            << ((options.games > static_cast<std::size_t>(selfplay.lanes)) ? "true" : "false")
            << ",\"temperature\":" << selfplay.temperature
            << ",\"temperature_moves\":" << selfplay.temperature_moves
            << ",\"c_puct\":" << selfplay.c_puct
            << ",\"dirichlet_alpha\":" << selfplay.dirichlet_alpha
            << ",\"dirichlet_epsilon\":" << selfplay.dirichlet_epsilon
            << ",\"seed\":"
            << (options.game_seeds.empty() ? std::to_string(options.seed) : "null")
            << ",\"seed_scheme\":\""
            << (options.game_seeds.empty() ? "base-plus-game-times-12" : "stored-game-seeds") << '"'
            << ",\"lanes\":" << selfplay.lanes << ",\"threads\":" << selfplay.threads
            << ",\"max_batch\":" << selfplay.max_batch
            << ",\"max_wait_us\":" << selfplay.max_wait_us
            << ",\"simulations\":" << selfplay.simulations
            << ",\"simulations_late\":" << selfplay.simulations_late
            << ",\"repeat_window\":" << selfplay.repeat_window
            << ",\"repetition_temperature\":" << selfplay.repetition_temperature
            << ",\"max_game_seconds\":"
            << (selfplay.max_game_duration
                    ? std::chrono::duration<double>(*selfplay.max_game_duration).count()
                    : 0.0)
            << ",\"bootstrap_prior\":\"" << (selfplay.bootstrap_prior ? "vacancy" : "none")
            << "\",\"bootstrap_prior_request\":\"" << options.bootstrap_prior
            << "\",\"bootstrap_prior_weight\":" << selfplay.bootstrap_prior_weight
            << ",\"balance_turn_orders\":" << (options.balance_turn_orders ? "true" : "false")
            << ",\"turn_order_group\":\"" << options.turn_order_group << '"'
            << ",\"diagnostic_roots\":" << options.diagnostic_roots
            << ",\"diagnostic_batch\":" << options.diagnostic_batch
            << ",\"max_moves\":" << selfplay.max_moves
            << "},\"environment\":{\"requested_device\":\"" << device.requested_name
            << "\",\"canonical_device\":\"" << device.canonical_name << "\",\"precision\":\""
            << options.precision << "\",\"torch_threads\":" << torch::get_num_threads()
            << "},\"provenance\":" << diamond_support::build_provenance_json()
            << ",\"samples_seconds\":[";
        for (std::size_t index = 0; index < seconds.size(); ++index)
            std::cout << (index ? "," : "") << seconds[index];
        std::cout << "],\"turn_orders\":{";
        bool first_order = true;
        for (const auto& [order, counts] : totals.turn_orders) {
            std::cout << (first_order ? "" : ",") << '"' << order
                      << "\":{\"requested\":" << counts[0]
                      << ",\"completed\":" << counts[1]
                      << ",\"aborted\":" << counts[2] << '}';
            first_order = false;
        }
        std::cout
            << "},\"summary_seconds\":{\"min\":" << sorted_seconds.front()
            << ",\"median\":" << sorted_seconds[sorted_seconds.size() / 2]
            << ",\"max\":" << sorted_seconds.back()
            << ",\"range\":" << sorted_seconds.back() - sorted_seconds.front()
            << "},\"domain\":{\"model_source\":\""
            << (options.checkpoint ? "checkpoint" : "artifact") << "\",\"model_family\":\""
            << (min_model ? "Min" : "Soo") << "\",\"model_sha256\":\"" << model_sha256
            << "\",\"runtime_sha256\":\"" << runtime_sha256 << "\",\"encoder_version\":\""
            << compatibility.encoder_version
            << "\",\"network_width\":" << compatibility.network_config.width
            << ",\"residual_blocks\":" << compatibility.network_config.residual_blocks
            << ",\"input_features\":" << compatibility.input_features()
            << ",\"value_size\":" << (min_model ? 3 : 1)
            << ",\"attempted_episodes\":" << totals.attempted
            << ",\"completed_episodes\":" << totals.completed
            << ",\"aborted_episodes\":" << totals.aborted
            << ",\"abort_reasons\":{\"max_moves\":" << totals.max_move_aborts
            << ",\"max_game_seconds\":" << totals.deadline_aborts
            << ",\"other\":" << totals.other_aborts
            << "},\"completed_samples\":" << totals.completed_samples
            << ",\"first_finisher_counts\":[" << totals.first_finisher_counts[1] << ','
            << totals.first_finisher_counts[2] << ',' << totals.first_finisher_counts[3] << ']'
            << ",\"search_targets\":{\"rows\":" << search_targets.rows
            << ",\"legal_actions_mean\":" << search_targets.legal_actions_mean
            << ",\"entropy_mean\":" << search_targets.entropy_mean
            << ",\"entropy_p50\":" << search_targets.entropy_p50
            << ",\"entropy_p90\":" << search_targets.entropy_p90
            << ",\"normalized_entropy_mean\":" << search_targets.normalized_entropy_mean
            << ",\"max_probability_mean\":" << search_targets.max_probability_mean
            << ",\"top3_mass_mean\":" << search_targets.top3_mass_mean
            << ",\"effective_actions_mean\":" << search_targets.effective_actions_mean
            << ",\"zero_visit_fraction_mean\":" << search_targets.zero_visit_fraction_mean
            << "},\"policy_fit\":{\"sampled_roots\":" << policy.rows
            << ",\"target_entropy_mean\":" << ratio(policy.target_entropy, policy_rows)
            << ",\"full_cross_entropy_mean\":" << ratio(policy.full_cross_entropy, policy_rows)
            << ",\"legal_cross_entropy_mean\":" << ratio(policy.legal_cross_entropy, policy_rows)
            << ",\"full_kl_mean\":" << ratio(policy.full_kl, policy_rows)
            << ",\"legal_kl_mean\":" << ratio(policy.legal_kl, policy_rows)
            << ",\"legal_probability_mass_mean\":" << ratio(policy.legal_mass, policy_rows)
            << ",\"top1_agreement\":" << ratio(static_cast<double>(policy.top1_agrees), policy_rows)
            << "},\"repetition\":{\"revisit_fraction_mean\":"
            << ratio(totals.revisit_fraction_total, episode_count)
            << ",\"repeat_within_8_fraction_mean\":"
            << ratio(totals.repeat_within_8_fraction_total, episode_count)
            << ",\"max_revisits_mean\":" << ratio(totals.max_revisits_total, episode_count)
            << ",\"cycling_games\":" << totals.cycling_games << "}" << ",\"moves\":" << totals.moves
            << ",\"moves_p50\":" << percentile(totals.move_counts, 0.50)
            << ",\"moves_p90\":" << percentile(totals.move_counts, 0.90)
            << ",\"moves_p99\":" << percentile(totals.move_counts, 0.99)
            << ",\"evaluations\":" << totals.evaluations
            << ",\"repetition_moves\":" << totals.repetition_moves
            << ",\"boosted_moves\":" << totals.boosted_moves << ",\"boosted_fraction\":"
            << ratio(static_cast<double>(totals.boosted_moves), static_cast<double>(totals.moves))
            << ",\"batches\":" << totals.batches << ",\"evaluations_per_second\":"
            << ratio(static_cast<double>(totals.evaluations), totals.wall_seconds)
            << ",\"batches_per_second\":"
            << ratio(static_cast<double>(totals.batches), totals.wall_seconds) << ",\"batch_mean\":"
            << ratio(static_cast<double>(total_batch_rows),
                     static_cast<double>(totals.batch_sizes.size()))
            << ",\"batch_p50\":" << percentile(totals.batch_sizes, 0.50)
            << ",\"batch_p90\":" << percentile(totals.batch_sizes, 0.90)
            << ",\"batch_max\":" << max_batch << ",\"evaluator_busy_fraction\":"
            << ratio(totals.evaluator_seconds, totals.wall_seconds)
            << ",\"search_worker_busy_fraction\":"
            << ratio(totals.worker_busy_seconds,
                     totals.wall_seconds * static_cast<double>(selfplay.threads))
            << ",\"samples_per_hour\":"
            << ratio(static_cast<double>(totals.completed_samples) * 3600.0, measured_seconds)
            // Where the evaluator's time actually went. evaluator_busy
            // above is the sum of all six, so a high value does not by
            // itself mean the GPU is saturated.
            << ",\"evaluator_stage_seconds\":{" << "\"collation\":" << totals.collation_seconds
            << ",\"h2d\":" << totals.h2d_seconds << ",\"forward\":" << totals.forward_seconds
            << ",\"policy_postprocess\":" << totals.postprocess_seconds
            << ",\"d2h\":" << totals.d2h_seconds << ",\"scatter\":" << totals.scatter_seconds << "}"
            << "}}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "selfplay_benchmark: " << error.what() << '\n';
        return 2;
    }
}
