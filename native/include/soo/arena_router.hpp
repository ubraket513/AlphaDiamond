#pragma once

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "soo/selfplay.hpp"

namespace soo {

// Each real mover owns the network used throughout its search. Descendant
// positions can belong to other players without changing that ownership.
class ArenaModelRouter final : public BatchEvaluator {
  public:
    ArenaModelRouter(BatchEvaluator& candidate, BatchEvaluator& champion,
                     std::vector<int> candidate_player_by_job)
        : candidate_(candidate), champion_(champion),
          candidate_player_by_job_(std::move(candidate_player_by_job)) {}

    void prepare(BatchItem& item) override {
        (candidate_owns_root(item) ? candidate_ : champion_).prepare(item);
    }

    void evaluate(std::vector<BatchItem>& batch) override {
        std::vector<BatchItem> candidate;
        std::vector<BatchItem> champion;
        candidate.reserve(batch.size());
        champion.reserve(batch.size());
        for (const auto& item : batch)
            (candidate_owns_root(item) ? candidate : champion).push_back(item);
        candidate_.evaluate(candidate);
        champion_.evaluate(champion);
    }

  private:
    bool candidate_owns_root(const BatchItem& item) const {
        if (!item.state || !item.encoded)
            throw std::invalid_argument("arena evaluation requires a state and encoding");
        if (item.job < 0 ||
            static_cast<std::size_t>(item.job) >= candidate_player_by_job_.size())
            throw std::invalid_argument("arena evaluation requires a known job");
        const int candidate_player = candidate_player_by_job_[static_cast<std::size_t>(item.job)];
        const auto& players = item.encoded->canonical_player_ids;
        const auto contains = [&players](int player) {
            return player != 0 && std::find(players.begin(), players.end(), player) != players.end();
        };
        if (!contains(item.root_player) || !contains(candidate_player))
            throw std::invalid_argument("arena evaluation requires known root and candidate players");
        return item.root_player == candidate_player;
    }

    BatchEvaluator& candidate_;
    BatchEvaluator& champion_;
    std::vector<int> candidate_player_by_job_;
};

} // namespace soo
