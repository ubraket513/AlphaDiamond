#!/usr/bin/env sh
# Shared Engine/Desktop worker sizing. No interpreter or build-tree dependency.
set -eu
[ "$#" -eq 1 ] && [ "$1" = build ] || { echo "usage: $0 build" >&2; exit 2; }
# Size compiler workers from this host's actual affinity, quota and available
# memory. Leave room for the linker/desktop; Make's BUILD_JOBS is the override.
per_worker_mb=2048
reserve_mb=1024
proc_root=/proc
cgroup_root=/sys/fs/cgroup
positive_integer() { case "$1" in ''|*[!0-9]*|0) return 1 ;; *) return 0 ;; esac; }
cpus=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || printf '%s' "${NUMBER_OF_PROCESSORS:-1}")
if command -v nproc >/dev/null 2>&1; then cpus=$(unset OMP_NUM_THREADS OMP_THREAD_LIMIT; nproc); fi
positive_integer "$cpus" || cpus=1
if [ -r "$proc_root/self/status" ]; then
    affinity_cpus=$(awk '/^Cpus_allowed_list:/ {n=split($2,groups,","); total=0; for(i=1;i<=n;i++) {parts=split(groups[i],ends,"-"); total+=(parts==2?ends[2]-ends[1]+1:1)} print total}' "$proc_root/self/status")
    if positive_integer "$affinity_cpus" && [ "$cpus" -gt "$affinity_cpus" ]; then cpus=$affinity_cpus; fi
fi
memory_mb=''
if [ -r "$proc_root/meminfo" ]; then memory_mb=$(awk '/MemAvailable:/ {print int($2/1024)}' "$proc_root/meminfo"); fi
if [ -z "$memory_mb" ] && command -v powershell.exe >/dev/null 2>&1; then
    memory_mb=$(powershell.exe -NoProfile -Command '[int]((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1024)' | tr -d '\r')
fi
if [ -z "$memory_mb" ] && command -v sysctl >/dev/null 2>&1; then
    bytes=$(sysctl -n hw.memsize 2>/dev/null || printf 0)
    if positive_integer "$bytes"; then memory_mb=$((bytes/1024/1024)); fi
fi

limit_cpu() {
    quota=$1 period=$2
    positive_integer "$quota" && positive_integer "$period" || return 0
    quota_cpus=$((quota/period))
    [ "$quota_cpus" -gt 0 ] || quota_cpus=1
    [ "$cpus" -le "$quota_cpus" ] || cpus=$quota_cpus
}
limit_memory() {
    limit=$1 used=$2
    case "$limit" in ''|*[!0-9]*) return 0 ;; esac
    case "$used" in ''|*[!0-9]*) return 0 ;; esac
    # awk avoids overflow for the effectively unlimited cgroup-v1 sentinel.
    remaining=$(awk -v cap="$limit" -v used="$used" 'BEGIN {n=int((cap-used)/1048576); printf "%.0f\n", (n>0?n:0)}')
    if [ -z "$memory_mb" ] || [ "$remaining" -lt "$memory_mb" ]; then memory_mb=$remaining; fi
}
member_directory() {
    mount=$1 member=$2 filesystem=$3
    mounted_root=''
    if [ -r "$proc_root/self/mountinfo" ]; then
        mounted_root=$(awk -v mount="$mount" -v filesystem="$filesystem" '$5==mount {for(i=6;i<=NF;i++) if($i=="-" && $(i+1)==filesystem) {print $4; exit}}' "$proc_root/self/mountinfo")
    fi
    case "$mounted_root" in
        ''|/) ;;
        *) case "$member" in "$mounted_root") member='' ;; "$mounted_root"/*) member=${member#"$mounted_root"} ;; esac ;;
    esac
    printf '%s%s\n' "$mount" "$member"
}
scan_limits() {
    directory=$1 root=$2
    [ -d "$directory" ] || directory=$root
    while :; do
        if [ -r "$directory/cpu.max" ]; then
            read -r quota period < "$directory/cpu.max"
            limit_cpu "$quota" "$period"
        fi
        if [ -r "$directory/cpu.cfs_quota_us" ] && [ -r "$directory/cpu.cfs_period_us" ]; then
            limit_cpu "$(cat "$directory/cpu.cfs_quota_us")" "$(cat "$directory/cpu.cfs_period_us")"
        fi
        if [ -r "$directory/memory.max" ] && [ -r "$directory/memory.current" ]; then
            limit_memory "$(cat "$directory/memory.max")" "$(cat "$directory/memory.current")"
        fi
        if [ -r "$directory/memory.limit_in_bytes" ] && [ -r "$directory/memory.usage_in_bytes" ]; then
            limit_memory "$(cat "$directory/memory.limit_in_bytes")" "$(cat "$directory/memory.usage_in_bytes")"
        fi
        [ "$directory" != "$root" ] || break
        directory=${directory%/*}
        case "$directory" in "$root"|"$root"/*) ;; *) break ;; esac
    done
}
if [ -r "$proc_root/self/cgroup" ]; then
    while IFS=: read -r hierarchy controllers member; do
        case "$controllers" in
            '') scan_limits "$(member_directory "$cgroup_root" "$member" cgroup2)" "$cgroup_root" ;;
            *)
                case ",$controllers," in *,cpu,*)
                    for controller in cpu cpu,cpuacct cpuacct,cpu; do
                        [ ! -d "$cgroup_root/$controller" ] || scan_limits "$(member_directory "$cgroup_root/$controller" "$member" cgroup)" "$cgroup_root/$controller"
                    done ;;
                esac
                case ",$controllers," in *,memory,*)
                    [ ! -d "$cgroup_root/memory" ] || scan_limits "$(member_directory "$cgroup_root/memory" "$member" cgroup)" "$cgroup_root/memory" ;;
                esac ;;
        esac
    done < "$proc_root/self/cgroup"
fi
# Also honor limits when the namespace mounts its own cgroup as the mount root.
scan_limits "$cgroup_root" "$cgroup_root"
jobs=$cpus
if [ -n "$memory_mb" ]; then
    memory_jobs=$(((memory_mb-reserve_mb)/per_worker_mb))
    [ "$memory_jobs" -gt 0 ] || memory_jobs=1
    [ "$jobs" -le "$memory_jobs" ] || jobs=$memory_jobs
fi
printf '%s\n' "$jobs"
