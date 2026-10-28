#!/usr/bin/env sh
# Desktop-only tools; no training environment or Python activation is required.
set -eu

usage() {
    echo "usage: $0 [--sdk-prefix DIR] [--qt-root DIR] cmake|ctest [args...] | --jobs build|test" >&2
    exit 2
}

resource_tool=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)/build_resources.sh
worker_count() { "$resource_tool" "$1"; }

[ "$#" -gt 0 ] || usage
if [ "$1" = --jobs ]; then
    [ "$#" -eq 2 ] || usage
    case "$2" in build|test) worker_count "$2"; exit ;; *) usage ;; esac
fi
sdk_prefix=${DIAMOND_ENGINE_SDK_PREFIX:-}
qt_root=${DIAMOND_QT_ROOT:-}
while [ "$#" -gt 0 ]; do
    case "$1" in
        --sdk-prefix) [ "$#" -ge 2 ] || usage; sdk_prefix=$2; shift 2 ;;
        --qt-root) [ "$#" -ge 2 ] || usage; qt_root=$2; shift 2 ;;
        *) break ;;
    esac
done
[ "$#" -gt 0 ] || usage
tool=$1; shift
case "$tool" in cmake|ctest) ;; *) usage ;; esac
export OMP_NUM_THREADS=${OMP_NUM_THREADS:-1} MKL_NUM_THREADS=${MKL_NUM_THREADS:-1}
export CMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL:-$(worker_count build)}
export CTEST_PARALLEL_LEVEL=${CTEST_PARALLEL_LEVEL:-$(worker_count test)}
for workers in "$CMAKE_BUILD_PARALLEL_LEVEL" "$CTEST_PARALLEL_LEVEL"; do
    case "$workers" in ''|*[!0-9]*|0) echo 'worker counts must be positive bounded integers' >&2; exit 2 ;; esac
done

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        tool_path=$(command -v "$tool") || { echo "$tool is unavailable" >&2; exit 1; }
        inherited_path=$PATH
        vsdevcmd=''
        vswhere='/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe'
        if [ -x "$vswhere" ]; then
            installation_path=$("$vswhere" \
                -latest \
                -products '*' \
                -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
                -property installationPath | tr -d '\r' | sed -n '1p')
            if [ -n "$installation_path" ]; then
                candidate=$(cygpath -u "$installation_path")/Common7/Tools/VsDevCmd.bat
                if [ -f "$candidate" ]; then vsdevcmd=$candidate; fi
            fi
        fi

        if [ -z "$vsdevcmd" ]; then
        for candidate in \
            '/c/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/VsDevCmd.bat' \
            '/c/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat' \
            '/c/Program Files/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/VsDevCmd.bat'
        do
            if [ -f "$candidate" ]; then
                vsdevcmd=$candidate
                break
            fi
        done
        fi
        if [ -z "$vsdevcmd" ]; then
            echo "Visual Studio x64 developer environment is unavailable" >&2
            exit 1
        fi

        environment_dump=$(mktemp "${TMPDIR:-/tmp}/alphadiamond-vs-env.XXXXXX")
        trap 'rm -f -- "$environment_dump"' EXIT HUP INT TERM
        vsdevcmd_windows=$(cygpath -d "$vsdevcmd")
        if ! cmd.exe //d //s //c "call $vsdevcmd_windows -arch=x64 -host_arch=x64 >nul && set" >"$environment_dump"; then
            echo "failed to initialize the Visual Studio x64 environment" >&2
            exit 1
        fi
        sed -i 's/\r$//' "$environment_dump"

        developer_path=''
        fallback_path=''
        while IFS='=' read -r name value; do
            case "$name" in
                ''|[0-9]*|*[!A-Za-z0-9_]*) continue ;;
                PATH|Path)
                    converted_path=$(cygpath -p "$value")
                    case "$value" in
                        *\\VC\\Tools\\MSVC\\*) developer_path=$converted_path ;;
                        *)
                            if [ -z "$fallback_path" ]; then fallback_path=$converted_path; fi
                            ;;
                    esac
                    ;;
                *) export "$name=$value" ;;
            esac
        done <"$environment_dump"

        PATH=${developer_path:-$fallback_path}
        export PATH


        PATH=$PATH:$inherited_path
        for runtime_dir in "$sdk_prefix/bin" "${DIAMOND_TORCH_RUNTIME_DIR:-}" "$qt_root/bin"; do
            [ -d "$runtime_dir" ] || continue
            runtime_dir=$(cygpath -u "$runtime_dir")
            PATH=$runtime_dir:$PATH
        done
        export PATH
        exec "$tool_path" "$@"
        ;;
    *) exec "$tool" "$@" ;;
esac
