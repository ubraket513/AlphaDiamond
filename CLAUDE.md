# AlphaDiamond Engine and Training

Start with [README.md](README.md) for supported commands and
[the code guide](docs/development/code-guide.md) for the implementation reading order.
The Qt/QML application is in sibling `AlphaDiamondDesktop` and consumes an
installed, pinned Runtime SDK.

## Source map

- `src/`: C++ implementations and CLI entry points. `train_main.cpp` directly
  connects durable stages to self-play, replay, learner and checkpoint modules.
- `include/`: matching C++ headers. The SDK exports only the explicit subset in
  `cmake/InstallEngineSdk.cmake`; training/replay headers are not Runtime APIs.
- `benchmarks/`: explicitly selected diagnostics and probes, outside ordinary
  training and Runtime builds.
- `training-panel/`: Svelte UI and a small Node bridge that launches the CLI and
  reads its files. It does not own a second training pipeline.
- `configs/current/`: presets for new runs. `configs/experiments/` preserves old
  experiment recipes. Existing runs use their stored resolved/active config.
- `models/`, `data/topology/`: shipped models and authoritative Runtime data.
- `artifacts/`: ignored local runs, checkpoints, replay, downloads and evidence.
- `archive/`, `docs/archive/`: past outputs/designs. Their old commands and paths
  describe the historical environment, not current supported entry points.

## Build and verify

CMake owns targets/compiler flags; Make sizes parallel compiler workers from
available CPU, memory and cgroup limits. Linux development uses GCC 15, Ninja,
sccache and mold with `-O0 -g1 -pipe`, no LTO. Use explicit Release builds for
actual training and evaluation. Supply a compatible LibTorch prefix for models.

```sh
make build
CMAKE_PREFIX_PATH=/path/to/libtorch make build PRESET=training-linux
cmake --preset training-release -DCMAKE_PREFIX_PATH=/path/to/libtorch
cmake --build --preset training-release --parallel "$(tools/build_resources.sh build)"
```

Executables are under `build/<preset>/bin/`. `tools/cmake.sh` initializes the
Visual Studio developer environment on Windows and invokes CMake directly on
other platforms. Benchmarks/probes require `-DDIAMOND_BUILD_BENCHMARKS=ON`.
Regression suites and CI were explicitly removed by the user; do not recreate
those as part of cleanup. Verify current work with parallel builds and direct
CLI/product execution, stating what was and was not exercised.

## Preserve the actual contracts

- `src/board.cpp` is the authority for game geometry and standard match setup.
  Keep the five `data/topology/` tables used by Desktop and SDK provenance.
- Actor/learner models own different tensor storage. ModelPool owns evaluation
  snapshots; the caller loads and validates checkpoints before installation.
- Checkpoint-v3 generations and their lineage/config/replay identities are
  immutable evidence. Legacy metadata presence matters; loading must not silently
  select scratch initialization or change a stored configuration.
- Production game RNG derives from `run_seed` and job identities. Historical
  `mcts.seed`/`self_play.seed` fields are metadata, not alternate production RNGs.
- Live trainers can use copied executables/configs in `artifacts/`. Inspect actual
  processes and their paths before moving outputs, stopping compute or launching
  another GPU job. Never restart a user-stopped job without authorization.
- Publishing changes an external service. Existing model index/download URLs and
  Desktop SDK locks do not follow this checkout's folder names.

Use Serena for symbols/references and Graphify for cross-file relationships as
specified by AGENTS.md. Refresh Graphify after supported source changes, including
curated document relationships when files move. Keep implementations direct and
avoid adding wrappers, registries, fallback execution paths or new dependencies
for routine project organization.
