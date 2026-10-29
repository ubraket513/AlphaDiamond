#pragma once

#include <cstdint>
#include <span>
#include <string>

#include <torch/torch.h>

#include "diamond_model/soo_model.hpp"
#include "diamond_training/device.hpp"
#include "diamond_training/training_sample.hpp"

namespace diamond_training {

enum class PolicyLossDomain {
    full,
    legal,
};

// Learner loss/optimizer settings; update budgets and replay sampling live above
// Trainer, and inference precision belongs to the actor's ModelPool.
struct TrainingConfig {
    double learning_rate;
    double weight_decay;
    PolicyLossDomain policy_loss_domain = PolicyLossDomain::full;
};

// One update's telemetry. samples_per_second excludes replay sampling, self-play
// and checkpoint I/O. CUDA memory fields require peak_cuda_memory_available;
// allocator peaks are device/process observations, not exclusive model memory.
struct TrainingMetrics {
    double total_loss;
    double policy_loss;
    double value_loss;
    uint64_t training_step;
    double collation_seconds;
    double h2d_seconds;
    double forward_seconds;
    double backward_seconds;
    double optimizer_seconds;
    double total_step_seconds;
    double samples_per_second;
    uint64_t peak_cuda_allocated_bytes;
    uint64_t peak_cuda_reserved_bytes;
    bool peak_cuda_memory_available;
};

enum class ModelRole {
    actor,
    learner,
};

// Rebuilds a model from source architecture and copies every named parameter
// and buffer to target. The result owns distinct storage and has the requested
// role applied: learner is trainable; actor is eval with frozen parameters.
// Candidate is a checkpoint/promotion state, not a different tensor role.
diamond_model::DiamondModel snapshot_model(const diamond_model::DiamondModel& source,
                                           const Compatibility& compatibility, torch::Device target,
                                           ModelRole role);

// Expands a same-family legacy model with zero-initialized goal input columns.
// Every existing parameter and buffer is copied; source storage is untouched.
diamond_model::DiamondModel migrate_goal_inputs(const diamond_model::DiamondModel& source,
                                                const Compatibility& target,
                                                const ResolvedDevice& device);

// Explicit initialization: zeros only the final value layer under no-grad.
// Keeping this outside construction preserves its RNG draws and other tensors.
// It creates neutral initial values, not a constraint maintained during training.
void zero_value_head(const diamond_model::DiamondModel& model);

// A versioned SHA-256 identity over sorted named parameters then named buffers.
// Requires finite FP32 data; copies to contiguous CPU little-endian bytes and
// may synchronize CUDA. This is not a state.pt archive-file digest.
std::string canonical_model_digest(const diamond_model::DiamondModel& model);

// Owns a private trainable snapshot and AdamW bound to its parameter identities.
// The caller serializes updates/snapshots/checkpoint access. Updates change only
// memory; durable progress requires a separate checkpoint publication.
class Trainer {
  public:
    Trainer(diamond_model::DiamondModel model, Compatibility compatibility, TrainingConfig config,
            const ResolvedDevice& device);

    // One update from a borrowed batch. Invalid samples fail before gradient
    // reset; later failures are not a transaction over weights/optimizer state.
    TrainingMetrics train(std::span<const TrainingSample> samples);
    uint64_t training_step() const {
        return training_step_;
    }
    const Compatibility& compatibility() const {
        return compatibility_;
    }
    const TrainingConfig& config() const {
        return config_;
    }
    const ResolvedDevice& device() const {
        return device_;
    }
    // model() and learner() alias the same module; neither creates a snapshot.
    diamond_model::DiamondModel& model() {
        return model_;
    }
    const diamond_model::DiamondModel& model() const {
        return model_;
    }
    diamond_model::DiamondModel& learner() {
        return model_;
    }
    const diamond_model::DiamondModel& learner() const {
        return model_;
    }
    torch::optim::AdamW& optimizer() {
        return optimizer_;
    }
    // Records an update already performed by the distillation path; does not
    // execute AdamW itself. The optimizer must remain bound to learner tensors.
    void record_external_optimizer_step();
    // Checkpoint bookkeeping only: does not deserialize weights or optimizer.
    void restore_checkpoint_state(TrainingConfig config, uint64_t training_step) {
        config_ = config;
        training_step_ = training_step;
    }

  private:
    diamond_model::DiamondModel model_;
    Compatibility compatibility_;
    TrainingConfig config_;
    ResolvedDevice device_;
    torch::optim::AdamW optimizer_;
    uint64_t training_step_ = 0;
};

// Copies the migration into a fresh learner's existing tensors, preserving its
// optimizer bindings. Requires step zero and no accumulated optimizer state.
void migrate_goal_inputs(const diamond_model::DiamondModel& source, Trainer& target);

} // namespace diamond_training
