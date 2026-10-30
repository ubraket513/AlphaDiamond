#pragma once

#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

#include "soo/random.hpp"

namespace soo::detail {

// Preserve the operation order pinned by the frozen search fixtures.
inline double exploration_bonus(double prior, uint32_t parent_visits, uint32_t edge_visits,
                                double c_puct) {
    const double parent = static_cast<double>(parent_visits < 1 ? 1u : parent_visits);
    return c_puct * prior * std::sqrt(parent) / (1.0 + static_cast<double>(edge_visits));
}

// Both search families use the same visit sampling and action-id tie break.
// Reuse the session's scratch buffer without changing its RNG draw sequence.
inline int32_t select_from_visits(std::span<const int32_t> actions,
                                  std::span<const uint32_t> visits, double temperature, Rng& rng,
                                  std::vector<double>& weights) {
    if (temperature > 0.0) {
        weights.clear();
        weights.reserve(visits.size());
        double weight_total = 0.0;
        for (const uint32_t count : visits) {
            const double weight = std::pow(static_cast<double>(count), 1.0 / temperature);
            weights.push_back(weight);
            weight_total += weight;
        }
        // All-zero weights fall back to a uniform draw.
        if (!(weight_total > 0.0)) {
            for (double& weight : weights)
                weight = 1.0;
        }
        return actions[rng.weighted_index(weights)];
    }

    size_t best = 0;
    for (size_t i = 1; i < actions.size(); ++i) {
        if (visits[i] > visits[best] || (visits[i] == visits[best] && actions[i] < actions[best])) {
            best = i;
        }
    }
    return actions[best];
}

} // namespace soo::detail
