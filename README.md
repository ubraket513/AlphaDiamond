# AlphaDiamond Engine and Training

This repository owns the native C++26 rules, encoders, search, inference,
training, model export and Runtime SDK. The Qt/QML application now lives in
[AlphaDiamondDesktop](../AlphaDiamondDesktop). Desktop consumes a pinned installed
SDK and has its own build, tests and releases.

The shared engine is implemented once. Runtime consumers link the SDK's core,
search, support, artifact, rating and model targets; they do not compile trainer,
replay orchestration or release tools.

## Build and test

Use a C++26 dialect compiler, CMake 3.21+ and Ninja. Linux daily development
uses GCC 15, mold and sccache. Debug uses `-O0 -g1 -pipe`, with LTO off.
Make detects CPU affinity, quota and available memory for both build and test
workers; override `BUILD_JOBS` and `TEST_JOBS` on shared hosts.

```sh
make test-native
# Add a compatible standalone LibTorch prefix for model/training work.
export CMAKE_PREFIX_PATH=/path/to/libtorch
make test-training
make test-native-asan
```

The default Linux Make presets are `native-ci-linux` and
`native-training-linux`. The portable `native-ci`, `native-runtime` and
`native-training` presets also use Debug. Windows commands use
`tools/native_training.sh` to initialize the native toolchain. No Qt kit or
interpreter is required. See [development build cycle](docs/development/build-cycle.md).

Optimized builds are explicit final outputs: `native-release` or
`native-training-release`. Direct CMake/CTest calls should supply
`--parallel "$(tools/build_resources.sh build)"` and
`--parallel "$(tools/build_resources.sh test)"`, respectively. Keep nested
OpenMP/MKL threads within the same resource budget.

## Runtime SDK

Configure inference without training, build all runtime targets, test, then
install the `Runtime` component:

```sh
cmake --preset native-gcc-mold -B build/sdk -DCMAKE_PREFIX_PATH=/path/to/cpu-libtorch
cmake --build build/sdk --parallel "$(tools/build_resources.sh build)"
ctest --test-dir build/sdk --parallel "$(tools/build_resources.sh test)" --output-on-failure
cmake --install build/sdk --component Runtime --prefix /path/to/sdk
```

Install after rebuilding the complete runtime graph or its
`engine_sdk_provenance` target when HEAD/source content changes. The manifest
validates the source revision, build configuration, compiler/Torch ABI and file
hashes. Use a matching Debug or Release SDK for the consumer. Published SDK
artifacts use `engine-sdk/v*` tags; model releases retain their existing tags.
See [repository split and SDK upgrades](docs/architecture/repository_split.md).

## Training and model contracts

`alphadiamond-train` owns CPU/CUDA training, transactional checkpoint-v3 and exact
resume. `alphadiamond-checkpoint` validates and exports checkpoints;
`alphadiamond-release` validates and stages format-v3 deployment artifacts.

Model publishing remains here. `models/index.json`, GitHub
`ubraket513/alphadiamond`, and Hugging Face `ubraket513/AlphaDiamond` retain their
identities and download URLs. Desktop's Git remote never determines model URLs.
Training checkpoints and the full frozen training corpus remain in Engine;
SDKs carry only canonical topology and minimal runtime fixtures with provenance.

- [Product ownership](docs/architecture/products.md)
- [Format-v3 model contract](docs/architecture/model_artifact_v3.md)
- [Rules and frozen golden corpus](docs/architecture/decisions.md)
- [Goal-aware encoders](docs/model-training/goal-aware-training.md)
- [Historical Windows runtime validation](docs/native-selfplay/native_windows_runtime.md)

Dated plans and experiment reports preserve historical commands and evidence.
Use this README and the build-cycle guide for current supported commands.
