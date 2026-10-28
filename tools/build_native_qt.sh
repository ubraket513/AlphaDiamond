#!/usr/bin/env sh
set -eu
repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
sdk_prefix=${DIAMOND_ENGINE_SDK_PREFIX:-}
models_dir=''
sdk_lock=${DIAMOND_ENGINE_SDK_LOCK:-}
sdk_archive=${DIAMOND_ENGINE_SDK_ARCHIVE:-}
torch_prefix=${DIAMOND_TORCH_PREFIX:-}
qt_root=${DIAMOND_QT_ROOT:-}
build_dir=$repo/build/desktop-dev
output_dir=dist/diamond-qt
preset=desktop-dev
case "$(uname -s)" in Linux) preset=desktop-linux ;; esac
deploy=0
usage() {
    echo "usage: $0 --sdk-prefix DIR [--models-dir DIR] [--sdk-lock FILE] [--sdk-archive FILE] [--torch-prefix DIR] [--qt-root DIR] [--build-dir DIR] [--output-dir DIR] [--preset desktop-dev|desktop-linux|desktop-release|desktop-preview] [--deploy]" >&2
    exit 2
}
while [ "$#" -gt 0 ]; do
    case "$1" in
        --sdk-prefix) [ "$#" -ge 2 ] || usage; sdk_prefix=$2; shift 2 ;;
        --sdk-lock) [ "$#" -ge 2 ] || usage; sdk_lock=$2; shift 2 ;;
        --sdk-archive) [ "$#" -ge 2 ] || usage; sdk_archive=$2; shift 2 ;;
        --torch-prefix) [ "$#" -ge 2 ] || usage; torch_prefix=$2; shift 2 ;;
        --qt-root) [ "$#" -ge 2 ] || usage; qt_root=$2; shift 2 ;;
        --models-dir) [ "$#" -ge 2 ] || usage; models_dir=$2; shift 2 ;;
        --build-dir) [ "$#" -ge 2 ] || usage; build_dir=$2; shift 2 ;;
        --output-dir) [ "$#" -ge 2 ] || usage; output_dir=$2; shift 2 ;;
        --preset) [ "$#" -ge 2 ] || usage; preset=$2; shift 2 ;;
        --deploy) deploy=1; shift ;;
        --no-deploy) deploy=0; shift ;;
        *) usage ;;
    esac
done
[ -n "$sdk_prefix" ] && [ -d "$sdk_prefix" ] || { echo '--sdk-prefix must name an installed Runtime SDK' >&2; exit 2; }
sdk_prefix=$(CDPATH= cd -- "$sdk_prefix" && pwd -P)
export DIAMOND_ENGINE_SDK_PREFIX=$sdk_prefix
if [ -n "$models_dir" ]; then models_dir=$(CDPATH= cd -- "$models_dir" && pwd -P); fi
if [ -n "$sdk_lock" ]; then sdk_lock=$(CDPATH= cd -- "$(dirname -- "$sdk_lock")" && pwd -P)/$(basename -- "$sdk_lock"); fi
if [ -n "$sdk_archive" ]; then sdk_archive=$(CDPATH= cd -- "$(dirname -- "$sdk_archive")" && pwd -P)/$(basename -- "$sdk_archive"); fi
if [ -n "$qt_root" ]; then qt_root=$(CDPATH= cd -- "$qt_root" && pwd -P); fi
for dependency in "$torch_prefix" "$qt_root"; do
    [ -n "$dependency" ] || continue
    dependency=$(CDPATH= cd -- "$dependency" && pwd -P)
    CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH:+$CMAKE_PREFIX_PATH;}$dependency
    export CMAKE_PREFIX_PATH
done
if [ -n "$qt_root" ]; then export DIAMOND_QT_ROOT=$qt_root; fi
case "$build_dir" in /*|[A-Za-z]:*) ;; *) build_dir=$repo/$build_dir ;; esac
cd "$repo/desktop"
set -- --preset "$preset" -B "$build_dir"
if [ -n "$sdk_lock" ]; then set -- "$@" "-DDIAMOND_ENGINE_SDK_LOCK=$sdk_lock"; fi
if [ -n "$sdk_archive" ]; then set -- "$@" "-DDIAMOND_ENGINE_SDK_ARCHIVE=$sdk_archive"; fi
"$repo/tools/desktop_toolchain.sh" cmake "$@"
"$repo/tools/desktop_toolchain.sh" cmake --build "$build_dir" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-$("$repo/tools/desktop_toolchain.sh" --jobs build)}"
if [ "$deploy" -eq 1 ]; then
    set -- --sdk-prefix "$sdk_prefix" --build-dir "$build_dir" --output-dir "$output_dir"
    if [ -n "$models_dir" ]; then set -- "$@" --models-dir "$models_dir"; fi
    "$repo/tools/deploy_native_qt.sh" "$@"
fi
