#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "check.hpp"
#include "soo/arena_router.hpp"
#include "soo/board.hpp"
#include "soo/encoder.hpp"
#include "soo/mcts3p.hpp"
#include "soo/min_experts.hpp"

namespace {

using Group = soo::MinTurnOrderGroup;
constexpr std::array<std::array<uint8_t, 3>, 6> orders{{
    {1, 2, 3}, {1, 3, 2}, {2, 1, 3}, {2, 3, 1}, {3, 1, 2}, {3, 2, 1}}};
constexpr std::array<Group, 6> groups{
    Group::cyclic, Group::reverse, Group::reverse,
    Group::cyclic, Group::cyclic, Group::reverse};

soo::Match ordered_match(size_t index) {
    const auto base = soo::standard_min_match();
    auto match = base;
    for (size_t seat = 0; seat < orders[index].size(); ++seat)
        match.players[seat] = base.by_id(orders[index][seat]);
    return match;
}

soo::State opening(const soo::Match& match, uint8_t actor) {
    soo::State state;
    state.current_player = actor;
    for (int seat = 0; seat < match.count; ++seat)
        for (const auto cell : soo::topology().camp_positions[match.players[seat].camp])
            state.occupancy[cell] = match.players[seat].id;
    return state;
}

bool same_match(const soo::Match& left, const soo::Match& right) {
    if (left.count != right.count) return false;
    for (int seat = 0; seat < left.count; ++seat) {
        const auto& a = left.players[seat];
        const auto& b = right.players[seat];
        if (a.id != b.id || a.camp != b.camp || a.target_camp != b.target_camp) return false;
    }
    return true;
}

uint8_t rotated_camp(uint8_t camp, const std::array<uint8_t, soo::kBoardSize>& rotation) {
    std::set<uint8_t> transformed;
    for (const auto cell : soo::topology().camp_positions[camp]) transformed.insert(rotation[cell]);
    for (uint8_t candidate = 0; candidate < soo::kCamps; ++candidate) {
        const auto& cells = soo::topology().camp_positions[candidate];
        if (transformed == std::set<uint8_t>(cells.begin(), cells.end())) return candidate;
    }
    throw std::runtime_error("test rotation does not preserve camps");
}

soo::Match rotate_match(const soo::Match& original,
                        const std::array<uint8_t, soo::kBoardSize>& rotation) {
    auto match = original;
    for (int seat = 0; seat < match.count; ++seat) {
        match.players[seat].camp = rotated_camp(original.players[seat].camp, rotation);
        match.players[seat].target_camp = rotated_camp(original.players[seat].target_camp, rotation);
    }
    return match;
}

template <typename Function>
void expect_invalid(Function&& function) {
    bool rejected = false;
    try { function(); } catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}

// Catches classifiers based on numeric IDs, absolute camps, or only one actor's geometry.
void check_geometric_groups() {
    for (size_t index = 0; index < orders.size(); ++index) {
        const auto match = ordered_match(index);
        CHECK_EQ(soo::min_turn_order_group(match), groups[index]);
        for (const auto& rotation : soo::topology().physical_to_canonical) {
            auto rotated = rotate_match(match, rotation);
            CHECK_EQ(soo::min_turn_order_group(rotated), groups[index]);
            constexpr std::array<uint8_t, 4> renamed{0, 42, 7, 19};
            for (auto& player : rotated.players) player.id = renamed[player.id];
            CHECK_EQ(soo::min_turn_order_group(rotated), groups[index]);
        }
    }
    CHECK_EQ(soo::min_turn_order_group_name(Group::cyclic), std::string_view("cyclic"));
    CHECK_EQ(soo::min_turn_order_group_name(Group::reverse), std::string_view("reverse"));
    CHECK_EQ(soo::min_turn_order_group_from_name("cyclic"), Group::cyclic);
    CHECK_EQ(soo::min_turn_order_group_from_name("reverse"), Group::reverse);
    for (const auto name : {"", "all", "123", "Cyclic"})
        expect_invalid([&] { (void)soo::min_turn_order_group_from_name(name); });
    expect_invalid([] { (void)soo::min_turn_order_group_name(static_cast<Group>(42)); });
    expect_invalid([] { (void)soo::min_turn_order_group(soo::standard_soo_match()); });
    for (int fault = 0; fault < 5; ++fault) {
        auto bad = soo::standard_min_match();
        if (fault == 0) bad.players[1].id = bad.players[0].id;
        if (fault == 1) bad.players[0].id = 0;
        if (fault == 2) bad.players[0].camp = soo::kCamps;
        if (fault == 3) bad.players[0].target_camp = bad.players[0].camp;
        if (fault == 4) bad.players[1].camp = bad.players[0].camp;
        expect_invalid([&] { (void)soo::min_turn_order_group(bad); });
    }
}

// A deterministic evaluator keeps actual scheduler/search/routing code under test.
class ProbeModel final : public soo::BatchEvaluator {
  public:
    explicit ProbeModel(double score, const std::vector<soo::Match>* matches = nullptr)
        : score_(score), matches_(matches) {}

    void prepare(soo::BatchItem& item) override {
        validate_match(item);
        ++prepared;
        item.outcome->value = score_;
    }

    void evaluate(std::vector<soo::BatchItem>& batch) override {
        for (auto& item : batch) {
            validate_match(item);
            if (item.outcome->value != score_)
                throw std::runtime_error("prepare and evaluate selected different models");
            ++evaluated;
            if (item.state->current_player != item.root_player) ++foreign_leaves;
            if (item.job >= 0) jobs.insert(item.job);
            item.outcome->priors.assign(item.actions->size(), 1.0 / item.actions->size());
            item.outcome->values = {score_, score_, score_};
        }
    }

    std::atomic<int> prepared{0};
    int evaluated = 0;
    int foreign_leaves = 0;
    std::set<int> jobs;

  private:
    void validate_match(const soo::BatchItem& item) const {
        if (!item.match) throw std::runtime_error("request lost its match");
        if (matches_) {
            const size_t index = item.job < 0 ? 0 : static_cast<size_t>(item.job);
            if (index >= matches_->size() || !same_match(*item.match, matches_->at(index)))
                throw std::runtime_error("request carries a different job's match");
        }
    }

    double score_;
    const std::vector<soo::Match>* matches_;
};

// State-dependent routing would switch experts as actors cycle or players finish.
void check_mixed_batch_and_finish_stability() {
    const std::array<soo::Match, 6> matches{
        ordered_match(0), ordered_match(1), ordered_match(2),
        ordered_match(3), ordered_match(4), ordered_match(5)};
    constexpr size_t count = 6 * 3 * 8;
    std::array<soo::State, count> states;
    std::array<soo::Encoded, count> encoded;
    std::array<soo::EvalOutcome, count> outcomes;
    const std::vector<int32_t> actions{0};
    std::vector<soo::BatchItem> batch;
    ProbeModel cyclic(0.25), reverse(0.5);
    soo::MinExpertRouter router(cyclic, reverse);
    for (size_t index = 0; index < matches.size(); ++index)
        for (uint8_t actor = 1; actor <= 3; ++actor)
            for (int mask = 0; mask < 8; ++mask) {
                const size_t slot = batch.size();
                states[slot] = opening(matches[index], actor);
                for (uint8_t id = 1; id <= 3; ++id)
                    if (mask & (1 << (id - 1))) states[slot].place(id);
                encoded[slot] = soo::encode(states[slot], matches[index], soo::EncodingVersion::goals_v2);
                batch.push_back({&states[slot], &encoded[slot], &actions, 0, &outcomes[slot],
                                 3, static_cast<int>(index), actor, &matches[index]});
                router.prepare(batch.back());
            }
    router.evaluate(batch);
    for (size_t slot = 0; slot < batch.size(); ++slot) {
        const double expected = groups[static_cast<size_t>(batch[slot].job)] == Group::cyclic ? 0.25 : 0.5;
        CHECK_EQ(outcomes[slot].values[0], expected);
    }
    CHECK_EQ(cyclic.evaluated, 72);
    CHECK_EQ(reverse.evaluated, 72);
    CHECK_EQ(cyclic.prepared.load(), cyclic.evaluated);
    CHECK_EQ(reverse.prepared.load(), reverse.evaluated);
    auto invalid = batch.front();
    invalid.match = nullptr;
    expect_invalid([&] { router.prepare(invalid); });
    std::vector<soo::BatchItem> invalid_batch{invalid};
    expect_invalid([&] { router.evaluate(invalid_batch); });
    invalid.match = &matches.front();
    invalid.value_width = 1;
    expect_invalid([&] { router.prepare(invalid); });
}

// Search results must use the root owner's expert even when expanding another actor.
void check_search_ownership() {
    for (size_t index = 0; index < orders.size(); ++index) {
        const auto match = ordered_match(index);
        for (uint8_t actor = 1; actor <= 3; ++actor)
            for (const bool reference_pair : {false, true})
                for (const bool candidate_owns_root : {false, true}) {
                    ProbeModel cyclic(0.25), reverse(0.5), champion(-0.75);
                    ProbeModel reference_cyclic(-0.25), reference_reverse(-0.5);
                    soo::MinExpertRouter experts(cyclic, reverse);
                    soo::MinExpertRouter references(reference_cyclic, reference_reverse);
                    soo::BatchEvaluator& reference =
                        reference_pair ? static_cast<soo::BatchEvaluator&>(references) : champion;
                    soo::ArenaModelRouter router(experts, reference,
                                                 {candidate_owns_root ? actor : actor % 3 + 1});
                    soo::MCTSConfig config;
                    config.simulations = 1;
                    soo::SearchSession3P session(match, config);
                    session.begin(opening(match, actor), 0.0);
                    while (session.advance() == soo::SearchSession3P::Status::NeedsEvaluation) {
                        soo::EvalOutcome outcome;
                        std::vector<soo::BatchItem> batch{
                            {&session.pending_state(), &session.pending_features(),
                             &session.pending_actions(), 0, &outcome, 3, 0, actor, &match}};
                        router.prepare(batch.front());
                        router.evaluate(batch);
                        session.supply({outcome.priors, outcome.values});
                    }
                    const double reference_score =
                        !reference_pair ? -0.75 : (groups[index] == Group::cyclic ? -0.25 : -0.5);
                    const double expected = !candidate_owns_root
                                                ? reference_score
                                                : (groups[index] == Group::cyclic ? 0.25 : 0.5);
                    const auto& result = session.result();
                    for (size_t action = 0; action < result.visit_counts.size(); ++action)
                        if (result.visit_counts[action])
                            CHECK_EQ(result.q_vectors[action][match.seat_of(actor)], expected);
                    CHECK_EQ(cyclic.evaluated,
                             candidate_owns_root && groups[index] == Group::cyclic ? 2 : 0);
                    CHECK_EQ(reverse.evaluated,
                             candidate_owns_root && groups[index] == Group::reverse ? 2 : 0);
                    CHECK_EQ(champion.evaluated, !candidate_owns_root && !reference_pair ? 2 : 0);
                    CHECK_EQ(reference_cyclic.evaluated, !candidate_owns_root && reference_pair &&
                                                                 groups[index] == Group::cyclic
                                                             ? 2
                                                             : 0);
                    CHECK_EQ(reference_reverse.evaluated, !candidate_owns_root && reference_pair &&
                                                                  groups[index] == Group::reverse
                                                              ? 2
                                                              : 0);
                    for (const auto* model :
                         {&cyclic, &reverse, &champion, &reference_cyclic, &reference_reverse})
                        CHECK_EQ(model->prepared.load(), model->evaluated);
                    CHECK_EQ(cyclic.foreign_leaves + reverse.foreign_leaves +
                                 champion.foreign_leaves + reference_cyclic.foreign_leaves +
                                 reference_reverse.foreign_leaves,
                             1);
                }
    }
}

// A reused lane must carry its new job's Match, including the fallback default.
void check_lane_reuse() {
    const auto base = soo::standard_min_match();
    std::vector<soo::EpisodeJob> jobs;
    std::vector<soo::Match> matches;
    for (size_t index = 0; index < orders.size(); ++index) {
        matches.push_back(ordered_match(index));
        jobs.push_back({opening(matches.back(), matches.back().players[0].id), 100 + index,
                        index == 0 ? std::nullopt : std::optional(matches.back())});
    }
    ProbeModel cyclic(0.25, &matches), reverse(0.5, &matches);
    soo::MinExpertRouter router(cyclic, reverse);
    soo::EpisodeConfig config;
    config.lanes = 1;
    config.threads = 1;
    config.max_batch = 1;
    config.simulations = 2;
    config.max_moves = 1;
    config.dirichlet_epsilon = 0.0;
    soo::EpisodeMetrics metrics;
    const auto episodes = soo::run_episodes(base, jobs, config, router, metrics);
    CHECK_EQ(episodes.size(), jobs.size());
    for (const auto& episode : episodes) CHECK_EQ(episode.moves.size(), size_t{1});
    CHECK_EQ(cyclic.jobs, (std::set<int>{0, 3, 4}));
    CHECK_EQ(reverse.jobs, (std::set<int>{1, 2, 5}));
    CHECK_EQ(cyclic.prepared.load(), cyclic.evaluated);
    CHECK_EQ(reverse.prepared.load(), reverse.evaluated);
}

void check_scheduler_match() {
    const auto match = soo::standard_soo_match();
    const std::vector<soo::Match> matches{match};
    ProbeModel model(0.25, &matches);
    soo::SchedulerConfig config;
    config.games = 1;
    config.threads = 1;
    config.max_batch = 1;
    config.simulations = 2;
    config.stop_after_moves = 1;
    config.seconds = 30.0;
    const auto result = soo::run_scheduler(match, opening(match, match.players[0].id), config, model);
    CHECK_EQ(result.moves, uint64_t{1});
    CHECK_EQ(model.prepared.load(), model.evaluated);
}

// A selected group must retain exactly its three orders and each job's original seed.
void check_selected_balancing() {
    const auto base = soo::standard_min_match();
    for (const auto selected : {Group::cyclic, Group::reverse}) {
        const std::array<size_t, 3> indices = selected == Group::cyclic ?
            std::array<size_t, 3>{0, 3, 4} : std::array<size_t, 3>{1, 2, 5};
        for (const size_t count : {size_t{3}, size_t{6}}) {
            std::vector<soo::EpisodeJob> jobs;
            for (size_t index = 0; index < count; ++index)
                jobs.push_back({opening(base, 1), 1000 + index, std::nullopt});
            soo::balance_episode_turn_orders(base, jobs, selected);
            for (size_t index = 0; index < jobs.size(); ++index) {
                REQUIRE(jobs[index].match.has_value(), "balanced job needs a Match");
                CHECK(same_match(*jobs[index].match, ordered_match(indices[index % 3])));
                CHECK_EQ(jobs[index].seed, 1000 + index);
                CHECK_EQ(jobs[index].initial_state.current_player, orders[indices[index % 3]][0]);
                CHECK_EQ(jobs[index].initial_state.occupancy, opening(base, 1).occupancy);
            }
        }
        std::vector<soo::EpisodeJob> partial(4, {opening(base, 1), 1, std::nullopt});
        expect_invalid([&] { soo::balance_episode_turn_orders(base, partial, selected); });
        const auto soo_match = soo::standard_soo_match();
        std::vector<soo::EpisodeJob> two_player(6, {opening(soo_match, 1), 1, std::nullopt});
        expect_invalid([&] { soo::balance_episode_turn_orders(soo_match, two_player, selected); });
        for (const auto& rotation : soo::topology().physical_to_canonical) {
            auto transformed = rotate_match(base, rotation);
            constexpr std::array<uint8_t, 4> renamed{0, 42, 7, 19};
            for (auto& player : transformed.players) player.id = renamed[player.id];
            std::vector<soo::EpisodeJob> jobs(3, {opening(transformed, 42), 99, std::nullopt});
            soo::balance_episode_turn_orders(transformed, jobs, selected);
            std::set<std::array<uint8_t, 3>> observed;
            for (const auto& job : jobs) {
                REQUIRE(job.match.has_value(), "balanced transformed job needs a Match");
                observed.insert({job.match->players[0].id, job.match->players[1].id, job.match->players[2].id});
            }
            std::set<std::array<uint8_t, 3>> expected;
            for (const auto index : indices)
                expected.insert({renamed[orders[index][0]], renamed[orders[index][1]], renamed[orders[index][2]]});
            CHECK_EQ(observed, expected);
        }
    }
    std::vector<soo::EpisodeJob> all(6, {opening(base, 1), 3, std::nullopt});
    soo::balance_episode_turn_orders(base, all);
    for (size_t index = 0; index < all.size(); ++index)
        CHECK(same_match(*all[index].match, ordered_match(index)));
}

} // namespace

int main() {
    try {
        soo::ensure_topology_configured();
        check_geometric_groups();
        check_mixed_batch_and_finish_stability();
        check_search_ownership();
        check_lane_reuse();
        check_scheduler_match();
        check_selected_balancing();
    } catch (const std::exception& error) {
        soo_test::fail(__FILE__, __LINE__, error.what());
    }
    return soo_test::report("min_experts_test");
}
