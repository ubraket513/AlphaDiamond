#!/usr/bin/env sh
set -eu

build_dir='build/desktop-release'
output_dir='dist/diamond-qt'
models_dir=''
runtime_smoke=1
sdk_prefix=${DIAMOND_ENGINE_SDK_PREFIX:-}
qt_root=${DIAMOND_QT_ROOT:-}

usage() {
    echo "usage: tools/deploy_native_qt.sh [--build-dir DIR] [--output-dir DIR] --sdk-prefix DIR [--models-dir DIR] [--qt-root DIR] [--skip-runtime-smoke]" >&2
    exit 2
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --build-dir) [ "$#" -ge 2 ] || usage; build_dir=$2; shift 2 ;;
        --output-dir) [ "$#" -ge 2 ] || usage; output_dir=$2; shift 2 ;;
        --sdk-prefix) [ "$#" -ge 2 ] || usage; sdk_prefix=$2; shift 2 ;;
        --models-dir) [ "$#" -ge 2 ] || usage; models_dir=$2; shift 2 ;;
        --qt-root) [ "$#" -ge 2 ] || usage; qt_root=$2; shift 2 ;;
        --skip-runtime-smoke) runtime_smoke=0; shift ;;
        -h|--help) usage ;;
        *) echo "unknown argument: $1" >&2; usage ;;
    esac
done

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) ;;
    *) echo "native Qt deployment is currently supported from Windows Git Bash" >&2; exit 1 ;;
esac

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
case "$build_dir" in
    [A-Za-z]:\\*|[A-Za-z]:/*) build_dir=$(cygpath -u "$build_dir") ;;
    /*) ;;
    *) build_dir=$repo/$build_dir ;;
esac
sdk_prefix=$(cygpath -u "$sdk_prefix")
qt_root=$(cygpath -u "$qt_root")
if [ -n "$models_dir" ]; then
    models_dir=$(cygpath -u "$models_dir")
    [ -d "$models_dir" ] || { echo "models directory missing: $models_dir" >&2; exit 1; }
    models_dir=$(CDPATH= cd -- "$models_dir" && pwd -P)
fi
case "$output_dir" in
    [A-Za-z]:\\*|[A-Za-z]:/*) output_dir=$(cygpath -u "$output_dir") ;;
esac
for required in "$sdk_prefix" "$qt_root"; do
    [ -n "$required" ] && [ -d "$required" ] || {
        echo 'provide --sdk-prefix and --qt-root (or DIAMOND_QT_ROOT)' >&2; exit 1;
    }
done
sdk_prefix=$(CDPATH= cd -- "$sdk_prefix" && pwd -P)
qt_root=$(CDPATH= cd -- "$qt_root" && pwd -P)
configured_sdk=$(sed -n 's/^DIAMOND_ENGINE_SDK_PREFIX:PATH=//p' "$build_dir/CMakeCache.txt" | tr -d '\r')
configured_sdk=$(cygpath -u "$configured_sdk")
configured_sdk=$(CDPATH= cd -- "$configured_sdk" && pwd -P)
[ "$configured_sdk" = "$sdk_prefix" ] || { echo '--sdk-prefix differs from configured SDK' >&2; exit 1; }
[ -f "$sdk_prefix/share/AlphaDiamondEngine/manifest.json" ] || { echo 'SDK manifest missing' >&2; exit 1; }
grep -q '^CMAKE_BUILD_TYPE:STRING=Release' "$build_dir/CMakeCache.txt" || {
    echo 'packaging requires an explicit Release Desktop build and matching Release SDK' >&2; exit 1;
}
exe=''
for candidate in \
    "$build_dir/app/diamond_qt.exe" \
    "$build_dir/app/Release/diamond_qt.exe"
do
    if [ -f "$candidate" ]; then exe=$candidate; break; fi
done
[ -n "$exe" ] || { echo "Qt executable not found under $build_dir" >&2; exit 1; }
if grep -q '^DIAMOND_QML_PREVIEW:BOOL=ON' "$build_dir/CMakeCache.txt"; then
    echo "refusing to package a QML debugging build; use the desktop-release preset" >&2
    exit 1
fi

qt_bin=$qt_root/bin
[ -x "$qt_bin/windeployqt.exe" ] || { echo "Qt deployment tool missing: $qt_bin/windeployqt.exe" >&2; exit 1; }
case "$output_dir" in
    "$repo"/dist/*) destination_name=${output_dir#"$repo"/dist/} ;;
    dist/*) destination_name=${output_dir#dist/} ;;
    *) echo "--output-dir must be a direct child of $repo/dist" >&2; exit 1 ;;
esac
case "$destination_name" in
    ''|.|..|*/*|*\\*) echo "--output-dir must name one direct child of $repo/dist" >&2; exit 1 ;;
esac

cd -- "$repo"
dist_root=dist
mkdir -p -- "$dist_root"
destination=$dist_root/$destination_name

is_reparse_point() {
    path=$1
    [ -L "$path" ] && return 0
    [ -e "$path" ] || return 1
    windows_path=$(cygpath -w "$PWD/$path")
    cmd.exe //d //s //c "fsutil reparsepoint query \"$windows_path\" >nul 2>nul" >/dev/null 2>&1
}

if is_reparse_point "$dist_root"; then
    echo "deployment root must not be a reparse point: $dist_root" >&2
    exit 1
fi
if [ -e "$destination" ]; then
    if is_reparse_point "$destination"; then
        echo "refusing to replace deployment reparse point: $destination" >&2
        exit 1
    fi
    rm -rf -- "$destination"
fi
mkdir -p -- "$destination/assets/sounds"
cp -f -- "$exe" "$destination/diamond_qt.exe"
cp -f -- "$repo/native/qt/assets/sounds/move.wav" "$destination/assets/sounds/move.wav"

# Qt owns its dependency closure, including the MSVC redistributable.
"$qt_bin/windeployqt.exe" --release --qmldir "$repo/native/qt/qml" "$destination/diamond_qt.exe"
printf '%s\n' '[Paths]' 'Plugins = .' 'Qml2Imports = qml' >"$destination/qt.conf"

# Only the exact LibTorch selected by the standalone configure may supply DLLs.
# An unrelated activated training/CUDA environment is never searched.
torch_cmake_dir=$(sed -n 's/^Torch_DIR:PATH=//p' "$build_dir/CMakeCache.txt" | tr -d '\r' | tail -n 1)
[ -n "$torch_cmake_dir" ] || { echo 'configured Torch_DIR missing' >&2; exit 1; }
torch_cmake_dir=$(cygpath -u "$torch_cmake_dir")
torch_prefix=$(CDPATH= cd -- "$torch_cmake_dir/../../.." && pwd -P)
torch_lib=$torch_prefix/lib
[ -f "$torch_lib/c10.dll" ] && [ -f "$torch_lib/torch_cpu.dll" ] || {
    echo 'CMake-selected LibTorch CPU DLLs missing' >&2; exit 1;
}
if [ -f "$torch_lib/torch_cuda.dll" ] || [ -f "$torch_lib/c10_cuda.dll" ]; then
    echo 'Desktop packages require the pinned CPU LibTorch distribution' >&2; exit 1
fi
for file in "$torch_lib"/*.dll; do
    [ -f "$file" ] || continue
    case "$(basename -- "$file")" in python*.dll|torch_python*.dll) continue ;; esac
    cp -f -- "$file" "$destination/"
done
for name in c10.dll torch_cpu.dll; do
    cmp -s -- "$torch_lib/$name" "$destination/$name" || { echo "bundled $name differs from configured LibTorch" >&2; exit 1; }
done

sync_binary=$sdk_prefix/bin/alphadiamond-rating-sync.exe
[ -f "$sync_binary" ] || { echo "SDK rating tool missing: $sync_binary" >&2; exit 1; }
cp -f -- "$sync_binary" "$destination/"
mkdir -p -- "$destination/tools"
cp -f -- "$repo/tools/sync_ratings.sh" "$destination/tools/"
if [ -n "$models_dir" ]; then
    [ -f "$models_dir/index.json" ] || { echo "model bundle index missing: $models_dir/index.json" >&2; exit 1; }
    cp -R -- "$models_dir" "$destination/models"
fi

forbidden=$(find "$destination" -type f \( \
    -iname 'python*.dll' -o -iname 'torch_python*.dll' -o -iname 'PySide*' -o -iname 'qtawesome*' \
    -o -path '*/site-packages/*' \) -print -quit)
[ -z "$forbidden" ] || { echo "Python/PySide runtime leaked into deployment: $forbidden" >&2; exit 1; }

run_smoke() {
    argument=$1
    platform=offscreen
    if [ "$argument" = '--window-smoke' ]; then platform=windows; fi
    windows_destination=$(cygpath -w "$PWD/$destination")
    windows_root=${SystemRoot:-C:\\Windows}
    (cd -- "$destination" && env -i \
        MSYS2_ARG_CONV_EXCL='*' \
        SystemRoot="$windows_root" \
        WINDIR="$windows_root" \
        PATH="$windows_destination;$windows_root\\System32;$windows_root" \
        QT_QPA_PLATFORM="$platform" \
        ./diamond_qt.exe "$argument")
}

if [ "$runtime_smoke" -eq 1 ]; then
    for argument in --smoke --game-smoke --worker-smoke --rotation-smoke \
        --analysis-smoke --replay-smoke --failure-smoke --sound-smoke --window-smoke
    do
        run_smoke "$argument"
    done
    if [ -n "$models_dir" ]; then
        run_smoke --soo-smoke
        run_smoke --models-smoke
    fi
fi

echo "Native Qt deployment created at: $repo/$destination"
if [ "$runtime_smoke" -eq 1 ]; then
    echo "Packaged runtime smoke checks passed."
else
    echo "Packaged runtime smoke checks were explicitly skipped."
fi
