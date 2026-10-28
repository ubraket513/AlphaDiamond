#!/usr/bin/env sh
# Desktop-only tools; no training environment or Python activation is required.
set -eu

usage() {
    echo "usage: $0 [--sdk-prefix DIR] [--qt-root DIR] cmake|ctest [args...] | --jobs build|test" >&2
    exit 2
}

worker_count() {
    kind=$1
    cpus=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || printf '%s' "${NUMBER_OF_PROCESSORS:-1}")
    if command -v nproc >/dev/null 2>&1; then cpus=$(unset OMP_NUM_THREADS OMP_THREAD_LIMIT; nproc); fi
    jobs=$cpus
    # Reserve one GiB for OS/linking; each LibTorch compilation may use two GiB.
    if [ "$kind" = build ]; then
        memory_mb=''
        if [ -r /proc/meminfo ]; then memory_mb=$(awk '/MemAvailable:/ {print int($2/1024)}' /proc/meminfo); fi
        if [ -r /sys/fs/cgroup/memory.max ]; then
            limit=$(cat /sys/fs/cgroup/memory.max)
            used=$(cat /sys/fs/cgroup/memory.current)
            case "$limit" in ''|*[!0-9]*) ;; *)
                remaining=$(((limit-used)/1024/1024))
                if [ -z "$memory_mb" ] || [ "$remaining" -lt "$memory_mb" ]; then memory_mb=$remaining; fi ;;
            esac
        fi
        if [ -z "$memory_mb" ] && command -v powershell.exe >/dev/null 2>&1; then
            memory_mb=$(powershell.exe -NoProfile -Command '[int]((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1024)' | tr -d '\r')
        fi
        if [ -z "$memory_mb" ] && command -v sysctl >/dev/null 2>&1; then
            bytes=$(sysctl -n hw.memsize 2>/dev/null || printf 0)
            memory_mb=$((bytes/1024/1024))
        fi
        if [ -n "$memory_mb" ]; then
            memory_jobs=$(((memory_mb-1024)/2048))
            [ "$memory_jobs" -gt 0 ] || memory_jobs=1
            [ "$jobs" -le "$memory_jobs" ] || jobs=$memory_jobs
        fi
    fi
    [ "$jobs" -gt 0 ] || jobs=1
    printf '%s\n' "$jobs"
}

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
