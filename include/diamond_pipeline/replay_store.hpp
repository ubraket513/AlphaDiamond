#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "diamond_pipeline/replay.hpp"

namespace diamond_pipeline {

// Ingest accounting separates idempotent game replays from newly accepted data.
// Aborted games count as games, but contribute no learner samples.
struct ReplayIngestReport {
    std::size_t accepted_games = 0;
    std::size_t duplicate_games = 0;
    std::size_t accepted_samples = 0;
    std::size_t duplicate_samples = 0;
};

// Last successful nonempty draw's selection-map entries/copied rows; not persisted.
struct ReplaySamplingStats {
    std::size_t selection_slots = 0;
    std::size_t copied_samples = 0;
};

// The training seed for one minibatch.  Sampling is a pure function of the
// replay contents and this seed, so the same replay contents replayed at the
// same iteration and local step always yield the same minibatch -- which is
// what makes a killed TRAIN stage restartable from its beginning without any
// durable sampler state.
uint64_t replay_sampling_seed(uint64_t replay_seed, uint64_t iteration, uint64_t training_step);

// Metadata-only skips chunk payload reads and sample materialization. Both modes
// may perform unreachable-chunk cleanup; this is not a read-only open mode.
enum class ReplayContents { full, metadata_only };
// must_exist refuses a missing manifest, but still permits open-time cleanup.
enum class ReplayOpenMode { create_if_missing, must_exist };

// Move-only owner of one compatibility namespace and its resident replay view.
// No internal locking: the run owner serializes ingest, prune and sampling.
class ReplayStore {
  public:
    // A new namespace publishes an empty manifest; an existing manifest supplies
    // the persisted capacity. Full opens verify/decode chunks into memory.
    ReplayStore(std::filesystem::path root, Compatibility compatibility, std::size_t capacity,
                uint64_t seed, ReplayContents contents = ReplayContents::full,
                ReplayOpenMode open_mode = ReplayOpenMode::create_if_missing);
    // Writes complete-episode chunks, publishes the manifest, then cleans old
    // chunks. Cleanup may fail after publication; an exception need not roll back
    // the committed store. Conflicting reuse of a game ID is rejected.
    std::size_t ingest(std::span<const Episode> episodes);
    ReplayIngestReport ingest_iteration(std::span<const Episode> episodes);
    std::size_t size() const noexcept;
    std::filesystem::path manifest_path() const;
    // Hashes the manifest file's bytes, not the in-memory sample vector.
    std::string manifest_digest() const;
    uint64_t replay_seed() const noexcept;
    // Deterministic, without-replacement selection for this seed/pool order.
    // Copies memory without file I/O or durable RNG state. It still updates
    // last_sampling_stats(), so const does not imply concurrent-call safety.
    // Metadata-only stores reject sampling, including a zero-count request.
    std::vector<TrainingSample> sample(std::size_t count, uint64_t seed) const;
    // Copies in caller-specified order, including repeats; rejects invalid indices.
    std::vector<TrainingSample> samples_at(std::span<const std::size_t> indices) const;
    ReplaySamplingStats last_sampling_stats() const noexcept;
    // Publishes retained episode descriptors before deleting unreachable chunks.
    void prune();

    ~ReplayStore();
    ReplayStore(ReplayStore&&) noexcept;
    ReplayStore& operator=(ReplayStore&&) noexcept;
    ReplayStore(const ReplayStore&) = delete;
    ReplayStore& operator=(const ReplayStore&) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace diamond_pipeline
