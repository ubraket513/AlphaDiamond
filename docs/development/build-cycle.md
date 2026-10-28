# Development build cycle

Daily work uses **Debug**. On Linux, the supported fast toolchain is GCC 15,
Ninja, sccache and mold. GNU Debug compilation uses `-O0 -g1 -pipe`; Release
retains the compiler's optimized defaults. Link-time optimization defaults to
off in every configuration. No optimization flags are added to dependencies.

The native presets provide a safe fallback of eight build jobs and twelve
CTest workers. `tools/build_resources.sh` is shared by Engine/
Desktop Make and CI routes. It detects CPU affinity, cgroup CPU quotas, host/
cgroup memory headroom (including ancestor limits), and reserves memory for the
OS and linker. Make and Desktop launchers also constrain nested OpenMP/MKL work. Override `make` with `BUILD_JOBS=N`
and `TEST_JOBS=N`, or a direct invocation with
`cmake --build --preset … --parallel N` and `ctest --preset … --parallel N`.
The Desktop shell helpers honor `CMAKE_BUILD_PARALLEL_LEVEL` and
`CTEST_PARALLEL_LEVEL`.

## Native engine and inference

```sh
make build
make test-native
make test-training

# Direct preset invocations always supply the detected counts explicitly.
BUILD_JOBS=$(tools/build_resources.sh build)
TEST_JOBS=$(tools/build_resources.sh test)
cmake --preset native-gcc-mold
cmake --build --preset native-gcc-mold --parallel "$BUILD_JOBS"
ctest --preset native-gcc-mold --parallel "$TEST_JOBS"
```

On Linux, default Make core/training routes select `native-ci-linux` and
`native-training-linux`. The explicit `native-gcc-mold` runtime preset selects `/usr/bin/g++-15`, `$HOME/.local/bin/sccache`, Ninja
and `-fuse-ld=mold`. Install these tools before configuring; mold must be on
the compiler's executable search path. `native-ci`, `native-runtime`,
`native-training` also default to Debug
and remain usable on other supported platforms. `native-asan` remains Debug.
Use a fresh build directory when changing compiler or generator.

`compile_commands.json` records the actual compile flags. The
`development_toolchain` CTest builds a small executable with implicit Debug,
explicit Debug and Release and checks generated commands: Debug requires `-O0 -g1 -pipe`, Release keeps
`-O3`, and neither enables LTO. It also verifies the compiler launcher and mold
when those tools are configured, then executes both binaries.

## Desktop and the SDK

The separate AlphaDiamondDesktop repository owns its Makefile, launchers,
preview and packaging commands. It consumes the installed SDK selected by its
root `engine-sdk.lock.json`. Match Debug/Release and all compiler/Torch ABI lock
fields. Its helpers do not rebuild Engine or training. QML-only edits reuse the
installed SDK and native compiler cache.

See [SDK upgrades](../architecture/repository_split.md) for the explicit source
fallback and relocation tests. Engine can configure/build/test with no Qt kit.

## Optimized final outputs

Select optimized builds explicitly for final training or runtime/SDK output:

```sh
BUILD_JOBS=$(tools/build_resources.sh build)
TEST_JOBS=$(tools/build_resources.sh test)
cmake --preset native-training-release
cmake --build --preset native-training-release --parallel "$BUILD_JOBS"
ctest --preset native-training-release --parallel "$TEST_JOBS"
```

`native-release` also retains Release output. On Linux, add `-G Ninja`,
`-DCMAKE_CXX_COMPILER=/usr/bin/g++-15`,
`-DCMAKE_CXX_COMPILER_LAUNCHER="$HOME/.local/bin/sccache"` and
`-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold` to the first configure of these
portable Release presets. Set shared/module linker flags similarly when needed.
Release remains free of LTO unless explicitly enabled.
