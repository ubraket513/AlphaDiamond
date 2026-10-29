# AlphaDiamond Engine and Training

This repository owns the native C++26 rules, encoders, search, inference,
training, model export and Runtime SDK. The Qt/QML application now lives in
[AlphaDiamondDesktop](../AlphaDiamondDesktop). Desktop consumes a pinned installed
SDK and has its own build and releases.

The shared engine is implemented once. Runtime consumers link the SDK's core,
search, support, artifact, rating and model targets; they do not compile trainer,
replay orchestration or release tools.

## Build and direct validation

Use a C++26 dialect compiler, CMake 3.21+ and Ninja. Linux daily development
uses GCC 15, mold and sccache. Debug uses `-O0 -g1 -pipe`, with LTO off.
Make detects CPU affinity, quota and available memory for build workers;
override `BUILD_JOBS` on shared hosts. Regression suites and GitHub CI have been
removed; future tests and CI will be designed separately. Validate current
changes with actual builds and direct CLI/product checks.

```sh
make build
# Add a compatible standalone LibTorch prefix for model/training work.
export CMAKE_PREFIX_PATH=/path/to/libtorch
make build PRESET=native-training-linux
make build PRESET=native-asan
```

The default Linux Make preset is `native-core-linux`; select
`PRESET=native-training-linux` for training. The portable `native-core`, `native-runtime` and
`native-training` presets also use Debug. Windows commands use
`tools/native_training.sh` to initialize the native toolchain. No Qt kit or
interpreter is required. See [development build cycle](docs/development/build-cycle.md).

Diagnostic executables are opt-in: add `-DDIAMOND_BUILD_BENCHMARKS=ON` when
configuring a build used for benchmarks/probes. Normal training and Runtime SDK
builds omit them. Legacy `DIAMOND_BUILD_LIBTORCH*` options were removed; select
`DIAMOND_BUILD_MODEL` and `DIAMOND_BUILD_TRAINING` explicitly.

Optimized builds are explicit final outputs: `native-release` or
`native-training-release`. Direct build calls should supply
`--parallel "$(tools/build_resources.sh build)"`. Keep nested
OpenMP/MKL threads within the same resource budget.

## Runtime SDK

Configure inference without training, build all runtime targets, then
install the `Runtime` component:

```sh
cmake --preset native-runtime-linux -B build/sdk -DCMAKE_PREFIX_PATH=/path/to/cpu-libtorch
cmake --build build/sdk --parallel "$(tools/build_resources.sh build)"
cmake --install build/sdk --component Runtime --prefix /path/to/sdk
```

Install after rebuilding the complete runtime graph or its
`engine_sdk_provenance` target when HEAD/source content changes. The manifest
validates the source revision, build configuration, compiler/Torch ABI and file
hashes. Use a matching Debug or Release SDK for the consumer. Existing published SDK artifacts use `engine-sdk/v*` tags; automated SDK
publishing workflows are removed. Model releases retain their existing tags.
See [repository split and SDK upgrades](docs/architecture/repository_split.md).

## Training and model contracts

`alphadiamond-train` owns CPU/CUDA training, transactional checkpoint-v3 and exact
resume. `alphadiamond-checkpoint` validates and exports checkpoints;
`alphadiamond-release` validates and stages format-v3 deployment artifacts.

Current training presets live in `configs/alphazero/current/`: three Min expert
local pilots and Soo bootstrap, production and six-hour configurations.
`configs/alphazero/experiments/` preserves earlier Min presets and the expert
arena configuration for explicit CLI use. Configuration contents are unchanged;
dated reports retain their original paths and recorded hashes.

Model publishing remains here. `models/index.json`, GitHub
`ubraket513/alphadiamond`, and Hugging Face `ubraket513/AlphaDiamond` retain their
identities and download URLs. Desktop's Git remote never determines model URLs.
Training checkpoints and recorded experiment evidence remain in Engine.
SDKs carry canonical runtime topology with provenance; synthetic regression
model fixtures and the regression corpus have been removed.

The lightweight [training panel](training-panel/README.md) shows active CLI and
Agent training jobs, native settings, console output, loss and performance charts.
It can edit settings, launch training and stop a selected process immediately.
Run `npm ci && npm run build && npm start` inside `training-panel`, then open
<http://127.0.0.1:5178>. Development uses Svelte/Vite; the local bridge uses Node's
built-in APIs and reads native run files directly.

- [Product ownership](docs/architecture/products.md)
- [Format-v3 model contract](docs/architecture/model_artifact_v3.md)
- [Historical rules and golden corpus decisions](docs/architecture/decisions.md)
- [Goal-aware encoders](docs/model-training/goal-aware-training.md)
- [Historical Windows runtime validation](docs/native-selfplay/native_windows_runtime.md)

Dated plans and experiment reports preserve historical commands and evidence.
Use this README and the build-cycle guide for current supported commands.
