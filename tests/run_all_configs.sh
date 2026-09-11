#!/usr/bin/env bash
set -e

echo "========================================================"
echo "  Running async_delay Full Test Matrix across Configurations"
echo "========================================================"

FAILED=0

run_config() {
    local name="$1"
    shift
    local flags="$@"
    echo -e "\n[CONFIG] Testing: ${name} (Flags: ${flags:-DEFAULT})"
    gcc -Wall -Wextra -Wno-unused-function -O2 -I. ${flags} tests/test_async_delay.c -o tests/runner_bin
    ./tests/runner_bin
    if [ $? -ne 0 ]; then
        echo ">>> FAILED: ${name}"
        FAILED=1
    fi
}

run_stress() {
    local name="$1"
    shift
    local flags="$@"
    echo -e "\n[STRESS] Testing: ${name} (Flags: ${flags:-DEFAULT})"
    gcc -Wall -Wextra -Wno-unused-function -O2 -I. ${flags} tests/test_stress.c -o tests/stress_bin
    ./tests/stress_bin
    if [ $? -ne 0 ]; then
        echo ">>> FAILED STRESS: ${name}"
        FAILED=1
    fi
}

echo -e "\n>>> Phase 1: Unit Test Matrix <<<"
run_config "Default (16-bit, 4 slots)"
run_config "8-bit Timer, 8 slots" -DASYNC_DELAY_TIMER_BITS=8 -DASYNC_DELAY_MAX_SLOTS=8
run_config "32-bit Timer, 16 slots" -DASYNC_DELAY_TIMER_BITS=32 -DASYNC_DELAY_MAX_SLOTS=16
run_config "Deferred Callbacks" -DASYNC_DELAY_DEFERRED_CALLBACKS=1
run_config "Split Arrays Layout" -DASYNC_DELAY_OPT_SPLIT_ARRAYS=1
run_config "Polling-Only Footprint (No CB, No Periodic)" -DASYNC_DELAY_DISABLE_CALLBACKS=1 -DASYNC_DELAY_DISABLE_PERIODIC=1
run_config "Next-Target Disabled" -DASYNC_DELAY_OPT_NEXT_TARGET=0
run_config "Fast-Cancel Disabled" -DASYNC_DELAY_OPT_FAST_CANCEL=0

echo -e "\n>>> Phase 2: Stress & Concurrency Matrix <<<"
run_stress "Stress - Default (16-bit, 4 slots)"
run_stress "Stress - 8-bit Timer, 8 slots" -DASYNC_DELAY_TIMER_BITS=8 -DASYNC_DELAY_MAX_SLOTS=8
run_stress "Stress - 32-bit Timer, 16 slots" -DASYNC_DELAY_TIMER_BITS=32 -DASYNC_DELAY_MAX_SLOTS=16
run_stress "Stress - Deferred Callbacks" -DASYNC_DELAY_DEFERRED_CALLBACKS=1

echo -e "\n>>> Phase 3: Performance & Optimization Differential Report <<<"
bash tests/compare_performance.sh

rm -f tests/runner_bin tests/stress_bin tests/bench_bin tests/run_test

echo "========================================================"
if [ $FAILED -eq 0 ]; then
    echo "  ALL CONFIGURATION MATRICES PASSED SUCCESSFULLY!"
    echo "========================================================"
    exit 0
else
    echo "  SOME CONFIGURATIONS FAILED!"
    echo "========================================================"
    exit 1
fi
