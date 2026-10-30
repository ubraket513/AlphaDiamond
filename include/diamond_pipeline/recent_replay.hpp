#pragma once

#include "soo/random.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace diamond_pipeline {

// A TRAIN restart reconstructs the same shuffled passes from the iteration seed.
// Only indices are retained; sample payloads stay in ReplayStore.
class RecentReplay {
  public:
    RecentReplay(std::size_t size, std::size_t recent, std::size_t batch,
                 double fraction, uint64_t seed) : batch_(batch), rng_(seed) {
        if (size == 0 || recent > size || batch == 0 || batch > size ||
            !std::isfinite(fraction) || fraction <= 0 || fraction >= 1)
            throw std::invalid_argument("invalid recent replay sizes or fraction");
        const auto historical = size - recent;
        if (recent != 0 && historical != 0 && batch < 2)
            throw std::invalid_argument("mixed replay needs a batch of at least two");
        recent_per_batch_ = recent == 0 ? 0 : historical == 0 ? batch :
            std::clamp(static_cast<std::size_t>(fraction * batch), std::size_t{1}, batch - 1);
        recent_.resize(recent);
        historical_.resize(historical);
        std::iota(recent_.begin(), recent_.end(), historical);
        std::iota(historical_.begin(), historical_.end(), std::size_t{0});
        shuffle(recent_);
        shuffle(historical_);
    }

    std::size_t recent_per_batch() const noexcept { return recent_per_batch_; }
    std::size_t coverage_steps() const noexcept {
        return recent_.empty() ? 0 : 1 + (recent_.size() - 1) / recent_per_batch_;
    }

    std::vector<std::size_t> next() {
        std::vector<std::size_t> result;
        result.reserve(batch_);
        append(result, recent_, recent_position_, recent_per_batch_);
        append(result, historical_, historical_position_, batch_ - recent_per_batch_);
        shuffle(result);
        return result;
    }

  private:
    void shuffle(std::vector<std::size_t>& values) {
        for (std::size_t remaining = values.size(); remaining > 1; --remaining)
            std::swap(values[remaining - 1], values[rng_.next() % remaining]);
    }
    void append(std::vector<std::size_t>& out, std::vector<std::size_t>& pool,
                std::size_t& position, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            if (position == pool.size()) {
                shuffle(pool);
                position = 0;
            }
            out.push_back(pool[position++]);
        }
    }

    std::size_t batch_;
    std::size_t recent_per_batch_ = 0;
    std::vector<std::size_t> recent_, historical_;
    std::size_t recent_position_ = 0, historical_position_ = 0;
    soo::Rng rng_;
};

}  // namespace diamond_pipeline
