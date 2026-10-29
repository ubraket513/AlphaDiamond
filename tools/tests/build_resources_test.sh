#!/usr/bin/env sh
set -eu
resource_tool=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)/build_resources.sh
fixture=$(mktemp -d "${TMPDIR:-/tmp}/diamond-resources.XXXXXX")
trap 'rm -rf -- "$fixture"' EXIT HUP INT TERM
mkdir -p "$fixture/proc/self" "$fixture/cgroup/scope" "$fixture/bin"
printf 'MemAvailable: 67108864 kB\n' > "$fixture/proc/meminfo"
printf '0::/scope\n' > "$fixture/proc/self/cgroup"
cat > "$fixture/bin/nproc" <<'STUB'
#!/bin/sh
[ -z "${OMP_NUM_THREADS:-}" ] && [ -z "${OMP_THREAD_LIMIT:-}" ] || exit 2
printf '%s\n' "${TEST_AFFINITY_CPUS:-8}"
STUB
chmod +x "$fixture/bin/nproc"
export PATH="$fixture/bin:$PATH"
export DIAMOND_RESOURCE_PROC_ROOT="$fixture/proc"
export DIAMOND_RESOURCE_CGROUP_ROOT="$fixture/cgroup"
export OMP_NUM_THREADS=1 OMP_THREAD_LIMIT=1
expect() {
    expected=$1 kind=$2
    actual=$("$resource_tool" "$kind")
    [ "$actual" = "$expected" ] || { echo "$kind: expected $expected workers, got $actual" >&2; exit 1; }
}
# Unbounded controls still stop at the affinity budget; OMP never serializes it.
printf 'max 100000\n' > "$fixture/cgroup/scope/cpu.max"
printf 'max\n' > "$fixture/cgroup/scope/memory.max"
printf '0\n' > "$fixture/cgroup/scope/memory.current"
expect 8 build; expect 8 test
export TEST_AFFINITY_CPUS=2
expect 2 build; expect 2 test
unset TEST_AFFINITY_CPUS
# /proc affinity is a fallback when nproc does not report the effective mask.
printf 'Cpus_allowed_list:\t0-1,4-5\n' > "$fixture/proc/self/status"
expect 4 build; expect 4 test
rm "$fixture/proc/self/status"
# A tighter parent CPU quota and memory headroom apply to child cgroups too.
printf '200000 100000\n' > "$fixture/cgroup/cpu.max"
expect 2 build; expect 2 test
printf 'max 100000\n' > "$fixture/cgroup/cpu.max"
printf '2684354560\n' > "$fixture/cgroup/memory.max"
printf '536870912\n' > "$fixture/cgroup/memory.current"
printf '4294967296\n' > "$fixture/cgroup/scope/memory.max"
expect 1 build; expect 3 test
# Cgroup namespace mount roots may strip a host path from membership.
printf 'max\n' > "$fixture/cgroup/memory.max"
printf '2147483648\n' > "$fixture/cgroup/scope/memory.max"
printf '0::/host/group/scope\n' > "$fixture/proc/self/cgroup"
printf '1 0 0:1 /host/group %s rw - cgroup2 cgroup rw\n' "$fixture/cgroup" > "$fixture/proc/self/mountinfo"
expect 1 build; expect 3 test
# Both controller-specific cgroup-v1 accounting and unlimited sentinels work.
rm -f "$fixture/proc/self/mountinfo" "$fixture/cgroup/cpu.max" "$fixture/cgroup/memory.max" "$fixture/cgroup/memory.current"
mkdir -p "$fixture/cgroup/memory/scope" "$fixture/cgroup/cpu/scope"
printf '2:cpu,cpuacct:/scope\n3:memory:/scope\n' > "$fixture/proc/self/cgroup"
printf '300000\n' > "$fixture/cgroup/cpu/scope/cpu.cfs_quota_us"
printf '100000\n' > "$fixture/cgroup/cpu/scope/cpu.cfs_period_us"
printf '3221225472\n' > "$fixture/cgroup/memory/scope/memory.limit_in_bytes"
printf '1073741824\n' > "$fixture/cgroup/memory/scope/memory.usage_in_bytes"
expect 1 build; expect 3 test
printf '0\n' > "$fixture/cgroup/memory/scope/memory.limit_in_bytes"
expect 1 build; expect 1 test
printf '9223372036854771712\n' > "$fixture/cgroup/memory/scope/memory.limit_in_bytes"
printf '%s\n' -1 > "$fixture/cgroup/cpu/scope/cpu.cfs_quota_us"
expect 8 build; expect 8 test
printf 'build_resources: affinity, OMP, nested quota, v2/v1 memory and namespace checks passed\n'
