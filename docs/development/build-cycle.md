# Development build cycle

Daily work uses **Debug**. On Linux, the supported fast toolchain is GCC 15,
Ninja, sccache and mold. GNU Debug compilation uses `-O0 -g1 -pipe`; Release
retains the compiler's optimized defaults. Link-time optimization defaults to
off in every configuration. No optimization flags are added to dependencies.

The native presets provide a fallback of eight build jobs. `tools/build_resources.sh`
detects CPU affinity, cgroup quotas and memory headroom and reserves memory for
the OS/linker. Override `BUILD_JOBS=N` in Make or pass an explicit parallel count
to CMake. Keep nested OpenMP/MKL work within the same host resource budget.
Regression suites and existing GitHub CI are removed; validate through actual
builds and direct CLI/product checks until the user designs new tests and CI.

## Native engine and inference

```sh
make build
make build PRESET=native-training-linux

# Direct preset invocations always supply the detected counts explicitly.
BUILD_JOBS=$(tools/build_resources.sh build)
cmake --preset native-gcc-mold
cmake --build --preset native-gcc-mold --parallel "$BUILD_JOBS"
```

On Linux, default Make core/training routes select `native-ci-linux` and
`native-training-linux`. The explicit `native-gcc-mold` runtime preset selects `/usr/bin/g++-15`, `$HOME/.local/bin/sccache`, Ninja
and `-fuse-ld=mold`. Install these tools before configuring; mold must be on
the compiler's executable search path. `native-ci`, `native-runtime`,
`native-training` also default to Debug
and remain usable on other supported platforms. `native-asan` remains Debug.
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
cmake --preset native-training-release
cmake --build --preset native-training-release --parallel "$BUILD_JOBS"
```

`native-release` also retains Release output. On Linux, add `-G Ninja`,
`-DCMAKE_CXX_COMPILER=/usr/bin/g++-15`,
`-DCMAKE_CXX_COMPILER_LAUNCHER="$HOME/.local/bin/sccache"` and
`-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold` to the first configure of these
portable Release presets. Set shared/module linker flags similarly when needed.
Release remains free of LTO unless explicitly enabled.
