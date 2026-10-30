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
