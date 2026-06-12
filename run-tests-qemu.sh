#!/usr/bin/env bash
# Run all phase tests via single-source launch on QEMU.
# Usage: ./run-tests-qemu.sh [timeout_seconds [bench_timeout_seconds]]
#
# Bench.java is a JIT throughput benchmark and is handled differently from the
# correctness tests: it is pre-compiled as a separate step (so the benchmark JVM
# starts with a clean C2 queue) and run as a pre-compiled class.  Single-source
# launch would flood the C2 queue with ~8000 javac framework compilations, preventing
# step()/run() from ever being compiled during the timed window.
set -euo pipefail

JAVA="./tianon-jdk25u-mips64/build/linux-mips64el-server-release/images/jdk/bin/java"
JAVAC="./tianon-jdk25u-mips64/build/linux-mips64el-server-release/images/jdk/bin/javac"
TIMEOUT="${1:-60}"
BENCH_TIMEOUT="${2:-120}"

export QEMU_CPU=Loongson-3A1000
export QEMU_LD_PREFIX=/usr/mips64el-linux-gnuabi64

# Pre-compile Bench.java as a separate step so the benchmark JVM starts with a
# clean C2 queue (no javac framework methods queued ahead of step()/run()).
BENCH_CLASSES="$(mktemp --directory)"
trap 'rm -rf "$BENCH_CLASSES"' EXIT
"$JAVAC" --enable-preview --source 25 -d "$BENCH_CLASSES" tests/phase-3/Bench.java

tests=(
    "tests/phase-1/H2.java"
    "tests/phase-1/H.java foo bar"
    "tests/phase-1/T.java"
    "tests/phase-1/M.java"
    "tests/phase-1/S.java"
    "tests/phase-2/MinYield.java"
    "tests/phase-2/Phase2Test.java"
    "tests/phase-3/CurrentThread.java"
    "tests/phase-3/Phase3Test.java"
)

pass=0
fail=0
hang=0

for entry in "${tests[@]}"; do
    read -r -a parts <<< "$entry"
    file="${parts[0]}"
    args=("${parts[@]:1}")
    name="$(basename "$file" .java)"

    printf "%-20s ... " "$name"
    output=$(timeout --kill-after=5s "$TIMEOUT" "$JAVA" "$file" "${args[@]}" 2>&1) && rc=0 || rc=$?
    if [ $rc -eq 0 ]; then
        echo "PASS"
        pass=$((pass + 1))
    elif [ $rc -eq 124 ]; then
        echo "HANG (timeout ${TIMEOUT}s)"
        hang=$((hang + 1))
        echo "  last output: $(echo "$output" | tail -3 | head -c 500)"
    else
        echo "FAIL (exit $rc)"
        fail=$((fail + 1))
        echo "$output" | tail -20 | sed 's/^/  /'
    fi
done

# Bench: JIT throughput benchmark (pre-compiled, longer timeout).
# Requires >10x speedup vs -Xint to confirm C2 is compiling user code.
printf "%-20s ... " "Bench (JIT)"
bench_out=$(timeout --kill-after=5s "$BENCH_TIMEOUT" \
    "$JAVA" --enable-preview -cp "$BENCH_CLASSES" Bench 2>&1) && bench_rc=0 || bench_rc=$?
if [ $bench_rc -eq 0 ]; then
    echo "PASS"
    echo "$bench_out" | grep -E "warmup:|timed:|PASS:|NOTE:" | sed 's/^/  /'
    pass=$((pass + 1))
elif [ $bench_rc -eq 124 ]; then
    echo "HANG (timeout ${BENCH_TIMEOUT}s)"
    hang=$((hang + 1))
    echo "  last output: $(echo "$bench_out" | tail -3 | head -c 500)"
else
    echo "FAIL (exit $bench_rc)"
    fail=$((fail + 1))
    echo "$bench_out" | tail -20 | sed 's/^/  /'
fi

echo ""
echo "Results: $pass passed, $fail failed, $hang timed out"
[ $fail -eq 0 ] && [ $hang -eq 0 ]
