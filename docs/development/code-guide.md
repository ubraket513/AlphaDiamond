# Code guide

AlphaDiamond keeps engine, inference and training in one C++26 implementation.
The separate AlphaDiamondDesktop application consumes its installed Runtime SDK.
Start with the [repository map](../../README.md), then use the entrypoints below
to follow a feature through the code. Source files live directly in `src/`,
interfaces in `include/`, and optional diagnostic entrypoints in `benchmarks/`.

## Entrypoints and responsibilities

| Start here | Responsibility and useful symbols |
|---|---|
| [`src/train_main.cpp`](../../src/train_main.cpp) | `main` dispatches training commands; `train`, `execute_stage` and `execute_*_stage` connect the durable run to self-play, replay, training, evaluation and persistence |
| [`src/checkpoint_main.cpp`](../../src/checkpoint_main.cpp) | `main`/`run` expose checkpoint inspection, validation and export |
| [`src/release_main.cpp`](../../src/release_main.cpp) | Model release validation, initialization and promotion CLI |
| [`include/diamond_orchestration/config.hpp`](../../include/diamond_orchestration/config.hpp) | `ProductionConfig` and its section types own JSON parsing, validation and serialization |
| [`include/diamond_orchestration/coordinator.hpp`](../../include/diamond_orchestration/coordinator.hpp) | `Coordinator::run` and `run_bounded` drive durable stage execution |
| [`include/diamond_pipeline/pipeline.hpp`](../../include/diamond_pipeline/pipeline.hpp) | `run_self_play`, `train_replay` and `run_iteration` connect engine episodes, retained replay and learner updates |
| [`include/soo/selfplay.hpp`](../../include/soo/selfplay.hpp) | `run_episodes`, `run_scheduler` and `balance_episode_turn_orders` manage games, inference batches and Min order coverage |
| [`include/soo/mcts.hpp`](../../include/soo/mcts.hpp) | `SearchSession` exposes incremental search and pending evaluations; `MCTS2P::run` drives Soo search |
| [`include/soo/mcts3p.hpp`](../../include/soo/mcts3p.hpp) | Min three-player search and per-seat values |
| [`include/diamond_training/trainer.hpp`](../../include/diamond_training/trainer.hpp) | `Trainer::train` and `training_step` own learning; model snapshots and checkpoint restore keep actor/learner state explicit |
| [`src/checkpoint.cpp`](../../src/checkpoint.cpp) | Checkpoint-v3 generations, payload integrity and exact resume; [`checkpoint_export.cpp`](../../src/checkpoint_export.cpp) handles deployment export |
| [`training-panel/server.mjs`](../../training-panel/server.mjs) | Local HTTP bridge, process discovery, native run-file reads and launch/stop requests |

These names are navigation anchors rather than a second API specification. Follow
the matching `src/*.cpp` implementation when changing a contract. For shared
interfaces, inspect references before editing consumers.

## Following a training iteration

1. Choose a file in [`configs/current/`](../../configs/current/) or explicitly use
   an older experiment from [`configs/experiments/`](../../configs/experiments/).
   `ProductionConfig::from_json` and `validate` establish the resolved contract.
2. `alphadiamond-train` creates or resumes a run and the coordinator advances its
   durable stages. `execute_stage` in `train_main.cpp` is the useful junction for
   understanding which stage writes each report.
3. Pipeline self-play calls the engine scheduler and search. Encoders and model
   evaluation provide features and policy/value predictions; Soo uses a scalar
   value and Min retains one value per seat.
4. Retained episodes feed replay, and `train_replay` invokes learner updates.
   `Trainer` owns optimizer/model state; checkpoint generations capture the
   transactional state used by exact resume.
5. Candidate, arena, rating and promotion stages write the run's durable evidence.
   The panel reads those native files rather than maintaining another training
   state machine. See [the panel guide](../../training-panel/README.md).

For a rule/search change begin in `include/soo/` and the matching `src/` files.
For a model change begin in `include/diamond_model/` and
`include/diamond_training/`. Release contracts are in `include/diamond_release/`;
configuration and run orchestration are in `include/diamond_orchestration/`.

## Recent and historical replay

`training.recent_sample_fraction` defaults to `0`, preserving uniform minibatch
sampling and existing configuration hashes. A value such as `0.75` allocates
three quarters of each batch to the latest iteration's retained samples and the
rest to older replay. Each pool is shuffled and traversed completely before its
next pass. Empty pools allocate the batch to the available pool. Small batches
round the split down while retaining at least one slot for each nonempty pool.

`RecentReplay` retains indices rather than duplicate sample payloads. TRAIN
reconstructs the same sequence from the replay seed and iteration after a restart.
It rejects a budget too small to visit every recent sample and rejects recent
data already evicted by replay capacity. The native CLI checks the ingest manifest
before selecting the latest tail, including idempotently ingested samples.
`train.json` records `recent_samples_drawn`; it is null for uniform sampling or a
recovered stage that did not execute updates in that process. This option can
change at a saved iteration boundary through the existing configuration transition.

The explicit `recent_replay_probe` and `recent_replay_integration` targets check
coverage, restart determinism, configuration transitions, replay reads and CPU
training. The integration probe takes a new scratch directory and an existing
production config. Both require `DIAMOND_BUILD_BENCHMARKS=ON`.

### Hard-position continuation experiment

`min_reanalysis ENDPOINTS CHECKPOINT GROUP control|reanalyse NEW_OUTPUT` continues
audited 500-ply Min training cutoffs with four seeds per distinct position. It
checks camps, pieces, partial rankings and dynamics fingerprints before loading
the group checkpoint; `--validate ENDPOINTS GROUP` performs only those checks.
The control uses 512 simulations. Reanalysis uses 2,048 and temperature 1 on
repeated positions; both allow 512 additional moves without root noise. The
original repetition history is not restored, so this is a new continuation.

Only completed continuations receive terminal-rank targets. The executable
checks those targets, preserves any known first finisher, and round-trips the
existing episode artifact format. It writes diagnostics and episodes into a new
directory, without ingesting replay or training. Completion rate alone does not
establish stronger play. This fixed experiment requires CUDA and the benchmark
build option; it is not an ordinary training configuration.

## Fixed-data learning diagnostics

`min_learning_diagnostic` measures policy/value errors and parameter updates on
a copied replay store. `--evaluation-replay DIR` uses a separate replay and
rejects overlapping game IDs. Without it, the reported evaluation scope is
`training_pool_reference`, not a held-out validation set. Both full-action and
listed-action policy cross entropy and KL are reported; listed actions include
zero-visit entries, matching the learner's legal loss.

`--learning-rate X` changes AdamW's learning rate after exact checkpoint loading,
preserving moments and the update counter. Supply the original checkpoint's
configuration for loading; the report records loaded and effective rates.
`--checkpoint-out NEW_DIR` saves the resulting native v3 checkpoint with the
updated configuration and source lineage. Exact resumption of that output must
use its updated configuration. Keep whole games out of training when creating
validation data, and evaluate playing strength before promoting a checkpoint.

For completed reanalysis episodes, `--episode-artifact FILE` and
`--episode-operation-id ID` import targets into a replay copy directly inside
the output directory. The original replay must be copied there first. Incomplete
episodes and duplicate imports are rejected. A nonzero configured
`training.recent_sample_fraction` uses the existing `RecentReplay` sampler for
that newly imported tail and requires enough updates to cover every new row.
The report records imported rows and recent/history draw counts. This diagnostic
path does not invent targets for the trajectory before the saved endpoint.

## Expert-router comparisons

`min_expert_arena` builds each actor from its checkpoint's verified architecture,
so a router can combine different model sizes. Cyclic and reverse actors must
still declare their matching training groups. A single reference defaults to
the `all` group; `--shared-turn-order-group reverse` explicitly permits a model
trained only on reverse orders as the shared reference without relabeling it.

`--population-mode paired` runs both one-router-versus-two-reference and
one-reference-versus-two-router compositions on the same opening schedule and
seeds. The default remains `single`; an extra shared control is only supported
in that mode. Match rows retain a known first winner even when the remaining
players hit the move limit. Compare whole opening blocks and report unresolved
winners as bounds rather than silently discarding those games.

## Building and finding output

The [build cycle](build-cycle.md) is the current command reference.
`CMakeLists.txt` and `src/CMakeLists.txt` own target definitions; `Makefile` and
`tools/cmake.sh` provide command routing. Linux development presets are
`core-linux`, `runtime-linux` and `training-linux`. Optimized final outputs use
`release` or `training-release`.

A normal preset writes executables to `build/<preset>/bin/`, for example
`build/training-release/bin/alphadiamond-train`. A custom `-B build/NAME` writes
them to `build/NAME/bin/`. Benchmarks and probes require
`DIAMOND_BUILD_BENCHMARKS=ON`; they are not part of ordinary training or SDK builds.
Use resource-aware parallel build counts as described in the build guide.

## Data and historical records

`data/topology/` holds canonical runtime tables. `models/` owns the model index
and release metadata. `artifacts/` holds local runs, checkpoints and exports;
`artifacts/downloads/` is an optional local destination for downloaded bucket
objects. Preserve a run's saved configuration and immutable checkpoint identities
when resuming it.

The former `TrainAlphaDiamond/` held a storage README and two manifests, with no
actual bucket contents in this checkout. They now live in
[docs/archive/storage](../archive/storage/README.md), including the historical
Python checkpoint-sidecar schema. Native checkpoint-v3 validation is implemented
in `src/checkpoint.cpp` and exposed by `alphadiamond-checkpoint validate`.

[The documentation index](../README.md) distinguishes current guides from plans,
measurements and migration records. [The raw data archive](../../archive/README.md)
records the old-to-new folder map. Historical command transcripts retain their
original build paths and measured settings; use current guides to locate today's
source and executables.
