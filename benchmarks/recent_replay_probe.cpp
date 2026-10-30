#include "diamond_pipeline/recent_replay.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

int main() {
    using diamond_pipeline::RecentReplay;
    const auto require = [](bool ok) {
        if (!ok) throw std::runtime_error("recent replay invariant failed");
    };
    RecentReplay sampler(100, 12, 8, .75, 7);
    RecentReplay resumed(100, 12, 8, .75, 7);
    std::set<std::size_t> recent;
    for (int batch = 0; batch < 2; ++batch) {
        const auto indices = sampler.next();
        require(indices == resumed.next());
        require(indices.size() == 8);
        require(std::count_if(indices.begin(), indices.end(), [](auto i) { return i >= 88; }) == 6);
        for (auto i : indices) {
            require(i < 100);
            if (i >= 88) require(recent.insert(i).second);
        }
    }
    require(recent.size() == 12);
    require(sampler.recent_per_batch() == 6);
    require(sampler.coverage_steps() == 2);
    for (int batch = 0; batch < 30; ++batch) require(sampler.next() == resumed.next());
    for (auto count : {std::size_t{0}, std::size_t{1}, std::size_t{7}, std::size_t{8}}) {
        RecentReplay edge(8, count, 8, .75, 19);
        std::set<std::size_t> seen;
        for (std::size_t n = 0; n < std::max(std::size_t{1}, edge.coverage_steps()); ++n) {
            for (auto i : edge.next()) {
                require(i < 8);
                if (i >= 8 - count) seen.insert(i);
            }
        }
        require(seen.size() == count);
    }
    bool rejected = false;
    try { RecentReplay invalid(8, 9, 8, .75, 7); }
    catch (const std::invalid_argument&) { rejected = true; }
    require(rejected);
    std::cout << "recent replay: coverage, ratio, restart determinism and edge pools passed\n";
}
