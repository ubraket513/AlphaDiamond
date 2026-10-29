# Development build cycle

Daily work uses **Debug**. On Linux, the supported fast toolchain is GCC 15,
Ninja, sccache and mold. GNU Debug compilation uses `-O0 -g1 -pipe`; Release
retains the compiler's optimized defaults. Link-time optimization defaults to
off in every configuration. No optimization flags are added to dependencies.

Direct build presets use eight build jobs unless overridden. `tools/build_resources.sh`
detects CPU affinity, cgroup quotas and memory headroom and reserves memory for
the OS/linker. Override `BUILD_JOBS=N` in Make or pass an explicit parallel count
to CMake. Keep nested OpenMP/MKL work within the same host resource budget.
Regression suites and existing GitHub CI are removed; validate through actual
builds and direct CLI/product checks until the user designs new tests and CI.

## Native engine and inference

```sh
make build
make build PRESET=training-linux

# Direct preset invocations always supply the detected counts explicitly.
BUILD_JOBS=$(tools/build_resources.sh build)
cmake --preset runtime-linux
cmake --build --preset runtime-linux --parallel "$BUILD_JOBS"
```

On Linux, default Make core/training routes select `core-linux` and
`training-linux`. The explicit `runtime-linux` runtime preset selects `/usr/bin/g++-15`, `sccache`, Ninja
and `-fuse-ld=mold`. Install these tools before configuring; mold must be on
the compiler's executable search path. `core`, `runtime`,
`training` also default to Debug
and remain usable on other supported platforms. `asan` remains Debug.
Use a fresh build directory when changing compiler or generator.

`compile_commands.json` records the actual flags for direct inspection. Debug
uses `-O0 -g1 -pipe`; Release keeps optimized defaults and LTO remains off unless
explicitly selected.

## Desktop and the SDK

The separate AlphaDiamondDesktop repository owns its Makefile, launchers,
preview and packaging commands. It consumes the installed SDK selected by its
root `engine-sdk.lock.json`. Match Debug/Release and all compiler/Torch ABI lock
fields. Its helpers do not rebuild Engine or training. QML-only edits reuse the
installed SDK and native compiler cache.

See [SDK upgrades](../architecture/repository_split.md) for the explicit source
build and package boundaries. Engine can configure/build with no Qt kit.

## Optimized final outputs

Select optimized builds explicitly for final training or runtime/SDK output:

```sh
BUILD_JOBS=$(tools/build_resources.sh build)
cmake --preset training-release
cmake --build --preset training-release --parallel "$BUILD_JOBS"
```

`release` also retains Release output. On Linux, add `-G Ninja`,
`-DCMAKE_CXX_COMPILER=/usr/bin/g++-15`,
`-DCMAKE_CXX_COMPILER_LAUNCHER="$HOME/.local/bin/sccache"` and
`-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold` to the first configure of these
portable Release presets. Set shared/module linker flags similarly when needed.
Release remains free of LTO unless explicitly enabled.

## Build selections

Use `core-linux`, `runtime-linux` or `training-linux` for
Linux development. The old `native-ci`, `native-ci-linux` and `native-gcc-mold`
preset names were replaced by these product names; existing build directories
can be reconfigured explicitly with `-B`. No compatibility alias is retained.

Model and training selection uses `DIAMOND_BUILD_MODEL` and
`DIAMOND_BUILD_TRAINING`. Runtime builds exclude replay/learner code. Diagnostics
and probes require the existing `DIAMOND_BUILD_BENCHMARKS=ON` option; ordinary
training presets leave it off. Use a diagnostics build explicitly, for example:

```sh
cmake --preset training-linux -B build/diagnostics -DDIAMOND_BUILD_BENCHMARKS=ON
cmake --build build/diagnostics --parallel "$(tools/build_resources.sh build)"
```

## Source and executable locations

C++ implementations and target definitions live in `src/`, public interfaces in
`include/`, and optional diagnostic entrypoints in `benchmarks/`. Preset names no
longer use the former `native-` prefix. `tools/cmake.sh` supplies the portable
toolchain wrapper. Each normal preset writes executables to
`build/<preset>/bin/`; an explicit `-B build/NAME` writes to `build/NAME/bin/`.
For example, the training Release CLI is
`build/training-release/bin/alphadiamond-train`. Existing running jobs keep their
original copied executable and resolved configuration. See the
[code guide](code-guide.md) for entrypoint responsibilities.
