# Development build cycle

Daily work uses **Debug**. On Linux, the supported fast toolchain is GCC 15,
Ninja, sccache and mold. GNU Debug compilation uses `-O0 -g1 -pipe`; Release
retains the compiler's optimized defaults. Link-time optimization defaults to
off in every configuration. No optimization flags are added to dependencies.

The native presets provide a safe fallback of eight build jobs and twelve
CTest workers. `make` and the Desktop shell helpers detect available CPUs and
memory to select worker counts, reserving memory for the OS and linker. They
also constrain nested OpenMP/MKL work. Override `make` with `BUILD_JOBS=N`
and `TEST_JOBS=N`, or a direct invocation with
`cmake --build --preset … --parallel N` and `ctest --preset … --parallel N`.
The Desktop shell helpers honor `CMAKE_BUILD_PARALLEL_LEVEL` and
`CTEST_PARALLEL_LEVEL`.

## Native engine and inference

```sh
cmake --preset native-gcc-mold
make build PRESET=native-gcc-mold
make test-native PRESET=native-gcc-mold
```

This Linux preset selects `/usr/bin/g++-15`, `$HOME/.local/bin/sccache`, Ninja
and `-fuse-ld=mold`. Install these tools before configuring; mold must be on
the compiler's executable search path. `native-ci`, `native-runtime`,
`native-training`, `native-qt` and `native-qt-preview` also default to Debug
and remain usable on other supported platforms. `native-asan` remains Debug.
Use a fresh build directory when changing compiler or generator.

`compile_commands.json` records the actual compile flags. The
`development_toolchain` CTest builds a small executable with implicit Debug,
explicit Debug and Release and checks generated commands: Debug requires `-O0 -g1 -pipe`, Release keeps
`-O3`, and neither enables LTO. It also verifies the compiler launcher and mold
when those tools are configured, then executes both binaries.

## Desktop and the SDK

```sh
tools/build_native_qt.sh --sdk-prefix /path/to/debug-sdk --no-deploy
make test-qt SDK_PREFIX=/path/to/debug-sdk
tools/run_native_qt.sh --sdk-prefix /path/to/debug-sdk
```

Desktop consumes the installed engine SDK selected by `desktop/sdk.lock`.
Its Debug consumer must use a Debug SDK. A Release SDK and Debug consumer are
incompatible even on Linux: choose the matching artifact and lock before
changing configuration. The desktop helpers configure only the Desktop
project, and do not rebuild the engine or training stack.

Ninja reuses unchanged outputs, while sccache reuses compiler results. Keep
the build directory and compiler cache between iterations. A QML-only edit
can use the preview helper without compiling the native engine.

## Optimized application and final training

Select optimization explicitly for an application build or final training:

```sh
cmake --preset native-qt-release
cmake --build --preset native-qt-release
ctest --preset native-qt-release

cmake --preset native-training-release
cmake --build --preset native-training-release
ctest --preset native-training-release
```

The existing `native-release` and Windows `native-package` presets also retain
Release output. On Linux, add `-G Ninja`, `-DCMAKE_CXX_COMPILER=/usr/bin/g++-15`,
`-DCMAKE_CXX_COMPILER_LAUNCHER="$HOME/.local/bin/sccache"` and
`-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold` to the first configure of these
cross-platform Release presets to use the same compiler cache and linker.
Set shared/module linker flags similarly when building shared/module outputs.
Release remains free of LTO unless it is explicitly enabled by the caller.
