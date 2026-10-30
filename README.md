# AlphaDiamond Engine and Training

AlphaDiamond is the C++26 engine and training repository for Soo and Min. It owns
the rules, encoders, MCTS, inference, self-play, replay, training, checkpoint and
model release tools. The Qt/QML application lives in the separate
[AlphaDiamondDesktop](../AlphaDiamondDesktop) repository and consumes an installed
Runtime SDK.

Start with [the documentation index](docs/README.md), then read the
[code guide](docs/development/code-guide.md) and
[build cycle](docs/development/build-cycle.md). For training, use the
[training panel guide](training-panel/README.md) and the current configurations
in [`configs/current/`](configs/current/).

## Repository map

| Folder | Purpose |
|---|---|
| [`src/`](src/) | C++ implementations, executable entrypoints and target definitions |
| [`include/`](include/) | Engine, inference, training and orchestration interfaces |
| [`benchmarks/`](benchmarks/) | Optional diagnostics, probes and evaluation entrypoints |
| [`cmake/`](cmake/) | CMake package and build support |
| [`configs/current/`](configs/current/) | Seven supported panel training presets |
| [`configs/experiments/`](configs/experiments/) | Earlier and explicit CLI experiment configurations |
| [`tools/`](tools/) | Build, model publication and research helpers |
| [`training-panel/`](training-panel/) | Local training monitor and launcher |
| [`data/`](data/) | Canonical topology and runtime data |
| [`models/`](models/) | Shipped deployment models, weights and model index |
| [`docs/`](docs/) | Current guides, contracts and historical documentation |
| [`archive/`](archive/) | Preserved benchmark runs, Python runtime data and acceptance evidence |
| `build/<preset>/` | Local build trees; executables are in `bin/` |
| `artifacts/` | Local runs, checkpoints, exports and optional downloads |
| [`graphify-out/`](graphify-out/) | Generated architecture graph |

The former `native/` source wrapper is removed. Historical paths and measurements
are documented in [the archive map](archive/README.md). The former
`TrainAlphaDiamond/` contained storage documentation and two manifests, rather
than another codebase or local bucket contents; those files now live in
[`docs/archive/storage/`](docs/archive/storage/).

## Build

Use a C++26 compiler, CMake 3.21+ and Ninja. Linux development uses GCC 15,
sccache and mold. Debug uses `-O0 -g1 -pipe`, and LTO defaults to off. Make
selects workers from CPU and memory limits; override `BUILD_JOBS` when needed.

```sh
make build
# Add a compatible standalone LibTorch prefix for inference or training.
export CMAKE_PREFIX_PATH=/path/to/libtorch
make build PRESET=training-linux
make build PRESET=asan
```

The default Linux preset is `core-linux`. Portable Debug presets are `core`,
`runtime` and `training`; optimized final outputs use `release` or
`training-release`. Windows toolchain setup uses `tools/cmake.sh`.
Executables are written to `build/<preset>/bin/`, for example
`build/training-linux/bin/alphadiamond-train`.

Benchmarks and probes require `-DDIAMOND_BUILD_BENCHMARKS=ON`; ordinary training
and Runtime SDK builds omit them. Select model and training products with
`DIAMOND_BUILD_MODEL` and `DIAMOND_BUILD_TRAINING`. Direct builds should supply
`--parallel "$(tools/build_resources.sh build)"` and keep nested OpenMP/MKL threads
within that resource budget. Existing regression suites and GitHub CI have been
removed; the [build guide](docs/development/build-cycle.md) explains validation.

## Runtime SDK

Runtime consumers link the shared core, search, support, artifact, rating and
model targets. The SDK excludes trainer, replay orchestration and release tools.

```sh
cmake --preset runtime-linux -DCMAKE_PREFIX_PATH=/path/to/cpu-libtorch
cmake --build --preset runtime-linux --parallel "$(tools/build_resources.sh build)"
cmake --install build/runtime-linux --component Runtime --prefix /path/to/sdk
```

Rebuild the complete runtime graph, including `engine_sdk_provenance`, before
installation when source content or HEAD changes. SDK manifests validate source
revision, configuration, compiler/Torch ABI and file hashes. Desktop must consume
a matching Debug or Release SDK. See
[repository split and SDK upgrades](docs/architecture/repository_split.md).

## Training and models

`alphadiamond-train` owns CPU/CUDA training, transactional checkpoint-v3 and exact
resume. `alphadiamond-checkpoint` validates and exports checkpoints;
`alphadiamond-release` validates and stages format-v3 deployment artifacts.

The seven current presets are three Min expert local pilots, Min `512x12` from
scratch, and Soo bootstrap, production and six-hour runs. The larger Min uses
width 512, 12 residual blocks, all six turn orders, vacancy-prior weight 0.5 and a
three-hour iteration-boundary budget. A strict wall-clock limit also needs a
process/service cap because the native budget finishes the current iteration.
Earlier Min presets and the expert arena configuration are in
[`configs/experiments/`](configs/experiments/).

Set `run_seed` for production self-play: game seeds derive from run, iteration
and job identities. Replay and optimizer seeds remain separate. Resume with the
run's saved configuration, preserving immutable metadata. The standalone
`selfplay_benchmark --config` also uses `run_seed`; `--seed` overrides it.
Reproducing an old benchmark may require its recorded `self_play.seed` as that
explicit override; equal seeds alone do not imply identical production games.

The [training panel](training-panel/README.md) displays active CLI and Agent jobs,
configuration, output, loss and performance. To run it, execute
`npm ci`, `npm run build` and `npm start` inside `training-panel/`, then open
<http://127.0.0.1:5178>.

Model publishing keeps `models/index.json`, GitHub `ubraket513/alphadiamond` and
Hugging Face `ubraket513/AlphaDiamond`. Desktop's Git remote does not change model
URLs. For optional local bucket downloads, use `artifacts/downloads/`; the
[storage guide](docs/archive/storage/README.md) explains native checkpoint
validation and the historical manifests.

Continue with [product ownership](docs/architecture/products.md),
[format-v3 model contracts](docs/architecture/model_artifact_v3.md) and
[goal-aware training](docs/model-training/goal-aware-training.md). Dated plans,
measurements and original command transcripts are historical evidence; use the
current build guide for supported commands.
