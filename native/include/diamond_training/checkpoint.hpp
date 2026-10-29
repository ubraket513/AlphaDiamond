#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

#include "diamond_training/trainer.hpp"

namespace diamond_training {

class CheckpointError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

enum class CheckpointInitializationMode {
    scratch,
    warm_start,
    resume,
    audited_legacy_import,
};

enum class CheckpointLoadIntent {
    exact_resume,
    warm_start,
    audited_legacy_import,
};

// v3 records the origin of a run. Digests are lowercase SHA-256 hex strings.
// `model_step` is the native local step represented by this generation; a warm
// start's source_training_step is evidence, not the new run's update counter.
struct CheckpointLineage {
    CheckpointInitializationMode initialization_mode = CheckpointInitializationMode::scratch;
    std::string run_id;
    uint64_t iteration = 0;
    uint64_t model_step = 0;
    std::optional<std::string> parent_digest;
    std::optional<std::string> source_digest;
    std::optional<uint64_t> source_training_step;
    bool optimizer_restored = false;
    bool optimizer_reset = false;
    std::optional<std::string> optimizer_reset_reason;
};

// Callers supply immutable source/config/replay facts; save_checkpoint_v3 adds
// trainer/device/architecture facts. Preserve config bytes/digests as evidence,
// not as a promotion decision. The RNG gap below limits reproducibility claims.
struct CheckpointProvenance {
    std::string source_git_commit = "unavailable";
    std::string resolved_config_bytes = "{}";
    std::string replay_manifest_sha256 =
        "0000000000000000000000000000000000000000000000000000000000000000";
    std::string protocol_ids_json = "{}";
    std::string creation_timestamp = "unavailable";
    std::string environment_json = "{}";
    // LibTorch has no documented stable C++ CPU RNG state archive API in the
    // supported build. This records that verified limitation explicitly.
    std::string rng_state_status = "gap_no_stable_libtorch_cpp_api";
    uint64_t rng_state_version = 0;
};

// v3 model/optimizer digests identify archive files, not canonical tensor content
// or deployment runtime artifacts. Historical native v2 roots lack these fields.
struct CheckpointInfo {
    std::filesystem::path generation;
    uint64_t training_step = 0;
    uint64_t format_version = 2;
    std::optional<CheckpointLineage> lineage;
    std::string model_digest;
    std::string optimizer_digest;
    std::optional<CheckpointProvenance> provenance;
};

// Blocking native persistence API; caller serializes writes and Trainer access.
// A checkpoint root uses CURRENT to select one immutable v2 or v3
// generation. Each generation contains state.pt, optimizer.pt, and training_step.
// A legacy .pt file is deliberately not a valid root. The historical *_v2 reader
// names also accept supported v3 roots and check their manifest/archive digests.
CheckpointInfo inspect_checkpoint_v2(const std::filesystem::path& root);
// Verify the active generation is structurally complete and both LibTorch
// archives can be opened on CPU. Does not validate a destination architecture,
// bind optimizer state or execute a forward pass.
CheckpointInfo validate_checkpoint_v2(const std::filesystem::path& root);
// Copy the active native generation into a new root without changing its payload
// or copying all history. Destination must be new; no raw Python conversion.
CheckpointInfo migrate_checkpoint_v2(const std::filesystem::path& source,
                                     const std::filesystem::path& destination);
CheckpointInfo save_checkpoint_v2(const std::filesystem::path& root, Trainer& trainer);
// Writes the same immutable generation/CURRENT transaction as v2, with a
// strict v3 lineage/provenance manifest. CURRENT replacement is the publication
// point; failed pointer replacement can leave an unreferenced generation.
CheckpointInfo save_checkpoint_v3(const std::filesystem::path& root, Trainer& trainer,
                                  const CheckpointLineage& lineage,
                                  const CheckpointProvenance& provenance = {});
// Stage/validate weights on target, then replace the holder. No AdamW restore;
// inspection still requires/hashes the source optimizer archive for v3 roots.
CheckpointInfo load_checkpoint_v2_weights(const std::filesystem::path& root,
                                          diamond_model::DiamondModel& model,
                                          const ResolvedDevice& target);
// Stage model+AdamW on target and replace Trainer only after all load checks.
// Save-device provenance does not prevent supported CPU/CUDA device remapping.
CheckpointInfo load_checkpoint_v2(const std::filesystem::path& root, Trainer& trainer,
                                  const ResolvedDevice& target);
// `exact_resume` restores model and AdamW state transactionally. `warm_start`
// restores weights into a fresh optimizer at step zero. Legacy import is
// deliberately gated: this native checkpoint reader cannot claim its parity.
// Exact resume requires v3 here; global LibTorch RNG state is not restored.
CheckpointInfo load_checkpoint_v3(const std::filesystem::path& root, Trainer& trainer,
                                  const ResolvedDevice& target, CheckpointLoadIntent intent);

}  // namespace diamond_training
