#!/usr/bin/env sh
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
build_dir=$repo/build/desktop-preview
# Preview uses the same Debug SDK ABI with QML debugging enabled.
"$repo/tools/build_native_qt.sh" --preset desktop-preview --build-dir "$build_dir" --no-deploy "$@"
export QMLPREVIEW_HOTRELOAD=1
"$repo/tools/desktop_toolchain.sh" cmake --build "$build_dir" --target diamond_qt_preview --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-$("$repo/tools/desktop_toolchain.sh" --jobs build)}"
