#pragma once

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <vector>

#include "diamond_model/soo_model.hpp"
#include "diamond_pipeline/replay.hpp"
#include "diamond_training/device.hpp"
#include "diamond_training/trainer.hpp"
#include "soo/selfplay.hpp"

namespace diamond_pipeline {

// Actor execution dtype, separate from learner dtype and FP32's TF32 math mode.
// Reduced precision requires CUDA; legal normalization/CPU packing use FP32.
enum class InferencePrecision { fp32, fp16, bf16 };

class PipelineError : public std::runtime_error { public: using std::runtime_error::runtime_error; };
class CancelledError final : public PipelineError { public: using PipelineError::PipelineError; };
class DeadlineExceededError final : public PipelineError { public: using PipelineError::PipelineError; };
class IncompatibleCheckpointError final : public PipelineError { public: using PipelineError::PipelineError; };

// Owns detached actor snapshots for one evaluator thread. The caller serializes
// install/activate/evaluate and stats access; this class starts no worker threads.
// Batch items borrow lane storage only for the duration of evaluate().
// Callers load checkpoints before installing a snapshot for evaluation.
class ModelPool final : public soo::BatchEvaluator {
  public:
    struct EvaluationStats final {
        std::size_t forward_calls = 0;
        std::size_t h2d_transfers = 0;
        std::size_t d2h_transfers = 0;
        std::size_t batch_size = 0;
        std::size_t max_legal_actions = 0;

        // Per-batch component telemetry, not GPU utilization. CUDA event spans
        // require DIAMOND_EVAL_STAGE_TIMING; otherwise these are host intervals
        // even on CUDA, where enqueue/synchronization affect stage attribution.
        // CPU intervals are synchronous. Event readback can change throughput.
        double collation_seconds = 0.0;
        double h2d_seconds = 0.0;
        double forward_seconds = 0.0;
        double policy_postprocess_seconds = 0.0;
        double d2h_seconds = 0.0;
        double scatter_seconds = 0.0;

        bool operator==(const EvaluationStats&) const = default;
    };

    ModelPool(std::size_t capacity, diamond_training::ResolvedDevice device,
              InferencePrecision precision = InferencePrecision::fp32);

    // Copies source storage; key hashes the FP32 snapshot before inference casts.
    // A new key at capacity fails rather than silently evicting another actor.
    ModelKey install(const Compatibility& compatibility, const diamond_model::DiamondModel& source);
    void activate(const ModelKey& key);
    const ModelKey& active_key() const;
    void require_compatible(const Compatibility& expected) const;
    // Checks only at this call boundary; does not interrupt an in-flight forward.
    void require_ready(std::stop_token stop, std::chrono::steady_clock::time_point deadline) const;
    // Validates/stages the entire result before replacing caller-owned outcomes.
    void evaluate(std::vector<soo::BatchItem>& batch) override;
    // Successful nonempty batches since reset: counts/times and batch rows sum;
    // max_legal_actions retains the maximum. Empty or rejected batches add none.
    EvaluationStats accumulated_evaluation_stats() const noexcept {
        return accumulated_evaluation_stats_;
    }
    void reset_evaluation_stats() noexcept {
        accumulated_evaluation_stats_ = EvaluationStats{};
    }

  private:
    struct ResidentModel final {
        Compatibility compatibility;
        diamond_model::DiamondModel actor;
    };

    std::size_t capacity_;
    diamond_training::ResolvedDevice device_;
    InferencePrecision precision_;
    std::map<ModelKey, ResidentModel> models_;
    std::optional<ModelKey> active_;
    EvaluationStats accumulated_evaluation_stats_;

    // Reallocated only when shape/dtype requires it; pinned for CUDA transfers.
    // One evaluator thread owns these buffers. The blocking result copy completes
    // prior input use before the next call may overwrite them; no lock is added.
    torch::Tensor staging_features_;
    torch::Tensor staging_legal_indices_;
    torch::Tensor staging_valid_mask_;
};

}  // namespace diamond_pipeline
