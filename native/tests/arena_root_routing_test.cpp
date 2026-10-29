#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

#include "check.hpp"
#include "soo/arena_router.hpp"
#include "soo/board.hpp"
#include "soo/encoder.hpp"
#include "soo/mcts.hpp"
#include "soo/mcts3p.hpp"
#include "soo/selfplay.hpp"

namespace {

soo::State opening(const soo::Match& match, uint8_t current, uint8_t finished = 0) {
    soo::State state;
    state.current_player = current;
    for (int seat = 0; seat < match.count; ++seat)
        for (const auto cell : soo::topology().camp_positions[match.players[seat].camp])
            state.occupancy[cell] = match.players[seat].id;
    if (!finished) return state;
    for (auto& cell : state.occupancy)
        if (cell == finished) cell = soo::kEmpty;
    for (const auto cell : soo::topology().camp_positions[match.by_id(finished).target_camp]) {
        // Starting and target camps share boundary cells. Move those blockers
        // into the middle so this fixture retains ten pieces per player.
        if (state.occupancy[cell] != soo::kEmpty) {
            bool relocated = false;
            for (int destination = 0; destination < soo::kBoardSize; ++destination) {
                bool in_camp = false;
                for (const auto& camp : soo::topology().camp_positions)
                    in_camp = in_camp || std::find(camp.begin(), camp.end(), destination) != camp.end();
                if (!in_camp && state.occupancy[destination] == soo::kEmpty) {
                    state.occupancy[destination] = state.occupancy[cell];
                    relocated = true;
                    break;
                }
            }
            if (!relocated) throw std::runtime_error("could not relocate fixture blocker");
        }
        state.occupancy[cell] = finished;
    }
    state.place(finished);
    return state;
}

class OwnerEvaluator final : public soo::BatchEvaluator {
  public:
    explicit OwnerEvaluator(const std::vector<uint8_t>& owners, int features)
        : owners_(owners), features_(features) {}

    void prepare(soo::BatchItem& item) override {
        check_owner(item);
        prepared.fetch_add(1, std::memory_order_relaxed);
    }

    void evaluate(std::vector<soo::BatchItem>& batch) override {
        for (auto& item : batch) {
            check_owner(item);
            ++evaluated;
            if (item.state->current_player != item.root_player) ++foreign_leaves;
            item.outcome->priors.assign(item.actions->size(), 1.0 / item.actions->size());
            item.outcome->value = 0.0;
            item.outcome->values.fill(0.0);
        }
    }

    std::atomic<int> prepared{0};
    int evaluated = 0;
    int foreign_leaves = 0;

  private:
    void check_owner(const soo::BatchItem& item) const {
        const auto job = item.job < 0 ? size_t{0} : static_cast<size_t>(item.job);
        if (job >= owners_.size() || item.root_player != owners_[job])
            throw std::runtime_error("request lost the actual move's root player");
        if (!item.state || item.state->has_placed(item.state->current_player))
            throw std::runtime_error("finished player received a nonterminal evaluation");
        if (!item.encoded || item.encoded->feature_count != features_)
            throw std::runtime_error("episode search dropped its requested encoding");
    }
    const std::vector<uint8_t>& owners_;
    int features_;
};

void check_episode_owners(int count, soo::EncodingVersion encoding) {
    const auto base = count == 2 ? soo::standard_soo_match() : soo::standard_min_match();
    std::vector<uint8_t> order;
    for (int id = 1; id <= count; ++id) order.push_back(static_cast<uint8_t>(id));
    std::vector<soo::EpisodeJob> jobs;
    std::vector<uint8_t> owners;
    do {
        auto match = base;
        for (int seat = 0; seat < count; ++seat) match.players[seat] = base.by_id(order[seat]);
        for (uint8_t current = 1; current <= count; ++current) {
            for (uint8_t finished = 0; finished <= (count == 3 ? count : 0); ++finished) {
                if (finished == current) continue;
                soo::EpisodeJob job;
                job.initial_state = opening(match, current, finished);
                job.seed = 123 + jobs.size();
                job.match = match;
                jobs.push_back(job);
                owners.push_back(current);
            }
        }
    } while (std::next_permutation(order.begin(), order.end()));
    CHECK_EQ(jobs.size(), count == 2 ? size_t{4} : size_t{54});
    const int features = encoding == soo::EncodingVersion::legacy_v1
                             ? (count == 2 ? 4 : 6) : (count == 2 ? 6 : 9);
    OwnerEvaluator evaluator(owners, features);
    soo::EpisodeConfig config;
    config.lanes = 2;
    config.threads = 2;
    config.max_batch = 2;
    config.simulations = 32;
    config.max_moves = 1;
    config.dirichlet_epsilon = 0.0;
    config.encoding = encoding;
    soo::EpisodeMetrics metrics;
    const auto episodes = soo::run_episodes(base, jobs, config, evaluator, metrics);
    CHECK_EQ(episodes.size(), jobs.size());
    CHECK_EQ(evaluator.prepared.load(), evaluator.evaluated);
    CHECK(evaluator.foreign_leaves > 0);
    for (const auto& episode : episodes) CHECK_EQ(episode.moves.size(), size_t{1});
}

void check_scheduler_owners() {
    const auto match = soo::standard_soo_match();
    const std::vector<uint8_t> owners{2};
    OwnerEvaluator evaluator(owners, 4);
    soo::SchedulerConfig config;
    config.games = 1;
    config.threads = 1;
    config.max_batch = 1;
    config.simulations = 32;
    config.stop_after_moves = 1;
    config.seconds = 30.0;
    const auto result = soo::run_scheduler(match, opening(match, 2), config, evaluator);
    CHECK_EQ(result.moves, uint64_t{1});
    CHECK_EQ(evaluator.prepared.load(), evaluator.evaluated);
    CHECK(evaluator.foreign_leaves > 0);
}

class ConstantModel final : public soo::BatchEvaluator {
  public:
    explicit ConstantModel(double scalar) : scalar_(scalar) {}
    void evaluate(std::vector<soo::BatchItem>& batch) override {
        for (auto& item : batch) {
            ++calls;
            item.outcome->priors.assign(item.actions->size(), 1.0 / item.actions->size());
            item.outcome->value = scalar_;
            // Canonical vectors deliberately differ between models. One
            // foreign leaf is enough to expose a change of model ownership.
            item.outcome->values = scalar_ == 0.2 ? std::array<double, 3>{0.9, 0.0, -0.9}
                                                  : std::array<double, 3>{-0.7, 0.2, 0.5};
        }
    }
    int calls = 0;
  private:
    double scalar_;
};

void check_search_uses_owners_model(int count) {
    const auto base = count == 2 ? soo::standard_soo_match() : soo::standard_min_match();
    std::vector<uint8_t> order;
    for (int id = 1; id <= count; ++id) order.push_back(static_cast<uint8_t>(id));
    do {
        auto match = base;
        for (int seat = 0; seat < count; ++seat) match.players[seat] = base.by_id(order[seat]);
        for (uint8_t current = 1; current <= count; ++current) {
            for (bool candidate_owns_root : {false, true}) {
                ConstantModel candidate(0.2);
                ConstantModel champion(0.7);
                const int candidate_player = candidate_owns_root ? current : current % count + 1;
                soo::ArenaModelRouter router(candidate, champion, {candidate_player});
                const auto state = opening(match, current);
                soo::MCTSConfig config;
                config.simulations = 1;
                if (count == 2) {
                    soo::SearchSession session(match, config);
                    session.begin(state, 0.0, false);
                    while (session.advance() == soo::SearchSession::Status::NeedsEvaluation) {
                        soo::EvalOutcome outcome;
                        std::vector<soo::BatchItem> batch{{&session.pending_state(),
                            &session.pending_features(), &session.pending_actions(), 0,
                            &outcome, 1, 0, current}};
                        router.evaluate(batch);
                        session.supply(outcome);
                    }
                    CHECK_EQ(session.result().root_mean_value, candidate_owns_root ? -0.2 : -0.7);
                } else {
                    soo::SearchSession3P session(match, config);
                    session.begin(state, 0.0);
                    while (session.advance() == soo::SearchSession3P::Status::NeedsEvaluation) {
                        soo::EvalOutcome outcome;
                        std::vector<soo::BatchItem> batch{{&session.pending_state(),
                            &session.pending_features(), &session.pending_actions(), 0,
                            &outcome, 3, 0, current}};
                        router.evaluate(batch);
                        session.supply({outcome.priors, outcome.values});
                    }
                    const auto& result = session.result();
                    for (size_t i = 0; i < result.visit_counts.size(); ++i)
                        if (result.visit_counts[i])
                            CHECK_EQ(result.q_vectors[i][match.seat_of(current)],
                                     candidate_owns_root ? -0.9 : 0.5);
                }
                CHECK_EQ(candidate.calls, candidate_owns_root ? 2 : 0);
                CHECK_EQ(champion.calls, candidate_owns_root ? 0 : 2);
            }
        }
    } while (std::next_permutation(order.begin(), order.end()));
}

void check_router_validates_owner() {
    const auto match = soo::standard_min_match();
    // Player 1 owned the search and has now finished at this descendant;
    // ownership stays with player 1 while player 3 is about to act.
    auto state = opening(match, 3, 1);
    const auto encoded = soo::encode(state, match);
    std::vector<int32_t> actions;
    soo::canonical_legal_action_ids(state, match, actions);
    soo::EvalOutcome outcome;
    std::vector<soo::BatchItem> batch{{&state, &encoded, &actions, 0, &outcome, 3, 0, 1}};
    ConstantModel candidate(0.2);
    ConstantModel champion(0.7);
    soo::ArenaModelRouter router(candidate, champion, {1});
    router.evaluate(batch);
    CHECK_EQ(outcome.value, 0.2);
    CHECK_EQ(champion.calls, 0);

    for (const uint8_t invalid : {uint8_t{0}, uint8_t{4}}) {
        batch.front().root_player = invalid;
        bool rejected = false;
        try { router.evaluate(batch); }
        catch (const std::invalid_argument&) { rejected = true; }
        CHECK(rejected);
    }
    batch.front().root_player = 1;
    for (const int invalid_job : {-1, 1}) {
        batch.front().job = invalid_job;
        bool rejected = false;
        try { router.evaluate(batch); }
        catch (const std::invalid_argument&) { rejected = true; }
        CHECK(rejected);
    }
    batch.front().job = 0;
    soo::ArenaModelRouter invalid_candidate(candidate, champion, {4});
    bool rejected = false;
    try { invalid_candidate.evaluate(batch); }
    catch (const std::invalid_argument&) { rejected = true; }
    CHECK(rejected);
}

} // namespace

int main() {
    try {
        soo::ensure_topology_configured();
        for (const auto encoding : {soo::EncodingVersion::legacy_v1, soo::EncodingVersion::goals_v2}) {
            check_episode_owners(2, encoding);
            check_episode_owners(3, encoding);
        }
        check_scheduler_owners();
        check_search_uses_owners_model(2);
        check_search_uses_owners_model(3);
        check_router_validates_owner();
    } catch (const std::exception& error) {
        soo_test::fail(__FILE__, __LINE__, error.what());
    }
    return soo_test::report("arena_root_routing_test");
}
