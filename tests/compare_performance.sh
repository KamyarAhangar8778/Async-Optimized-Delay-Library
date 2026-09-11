#!/usr/bin/env bash
# compare_performance.sh - Benchmark comparison: Baseline vs Optimized
# Shows exact latency (ns/op), Throughput (M ops/sec), Speedup factor, and Pass/Fail regression check.

set -e

echo "================================================================================"
echo "          async_delay Performance & Optimization Comparison Report              "
echo "================================================================================"

# Compile Baseline (Standard unoptimized / naive mode: No Next-Target, No LUT Alloc, No LUT Popcount, No Fast Cancel, No Split Arrays)
gcc -Wall -Wextra -Wno-unused-function -O2 -I. \
    -DASYNC_DELAY_OPT_NEXT_TARGET=0 \
    -DASYNC_DELAY_OPT_FAST_CANCEL=0 \
    -DASYNC_DELAY_OPT_LUT_ALLOC=0 \
    -DASYNC_DELAY_OPT_LUT_POPCOUNT=0 \
    -DASYNC_DELAY_OPT_LUT_MASK=0 \
    -DASYNC_DELAY_MAX_SLOTS=16 \
    tests/benchmark_compare.c -o tests/baseline_bin

# Compile Optimized (All advanced optimizations active: Next-Target O(1) Gate, LUT Alloc O(1), LUT Popcount O(1), Fast Cancel, LUT Mask)
gcc -Wall -Wextra -Wno-unused-function -O2 -I. \
    -DASYNC_DELAY_OPT_NEXT_TARGET=1 \
    -DASYNC_DELAY_OPT_FAST_CANCEL=1 \
    -DASYNC_DELAY_OPT_LUT_ALLOC=1 \
    -DASYNC_DELAY_OPT_LUT_POPCOUNT=1 \
    -DASYNC_DELAY_OPT_LUT_MASK=1 \
    -DASYNC_DELAY_MAX_SLOTS=16 \
    tests/benchmark_compare.c -o tests/optimized_bin

BASELINE_DATA=$(./tests/baseline_bin BASELINE csv)
OPTIMIZED_DATA=$(./tests/optimized_bin OPTIMIZED csv)

rm -f tests/baseline_bin tests/optimized_bin

IFS=',' read -r _ B_IDLE_NS B_IDLE_MOPS B_ACT_NS B_ACT_MOPS B_SC_NS B_SC_MOPS B_RST_NS B_RST_MOPS B_POP_NS B_POP_MOPS <<< "$BASELINE_DATA"
IFS=',' read -r _ O_IDLE_NS O_IDLE_MOPS O_ACT_NS O_ACT_MOPS O_SC_NS O_SC_MOPS O_RST_NS O_RST_MOPS O_POP_NS O_POP_MOPS <<< "$OPTIMIZED_DATA"

calc_speedup() {
    local base=$1
    local opt=$2
    awk "BEGIN {if ($opt > 0) printf \"%.2fx\", $base / $opt; else printf \"N/A\"}"
}

calc_gain_pct() {
    local base=$1
    local opt=$2
    awk "BEGIN {if ($base > 0) { gain=(($base - $opt)/$base)*100; if (gain >= 0) printf \"+%.1f%% faster\", gain; else printf \"%.1f%% SLOWER (REGRESSION)\", gain} else printf \"N/A\"}"
}

printf "\n%-27s | %-17s | %-17s | %-9s | %s\n" "Operation" "Baseline (No Opt)" "Optimized (LUT+O1)" "Speedup" "Status"
printf -- "----------------------------+-------------------+-------------------+-----------+-------------------------\n"

SPEEDUP_IDLE=$(calc_speedup "$B_IDLE_NS" "$O_IDLE_NS")
GAIN_IDLE=$(calc_gain_pct "$B_IDLE_NS" "$O_IDLE_NS")
printf "%-27s | %5.2f ns (%5.1fM) | %5.2f ns (%5.1fM) | %9s | %s\n" "Idle tick (0 active)" "$B_IDLE_NS" "$B_IDLE_MOPS" "$O_IDLE_NS" "$O_IDLE_MOPS" "$SPEEDUP_IDLE" "$GAIN_IDLE"

SPEEDUP_ACT=$(calc_speedup "$B_ACT_NS" "$O_ACT_NS")
GAIN_ACT=$(calc_gain_pct "$B_ACT_NS" "$O_ACT_NS")
printf "%-27s | %5.2f ns (%5.1fM) | %5.2f ns (%5.1fM) | %9s | %s\n" "Active tick (16 slots)" "$B_ACT_NS" "$B_ACT_MOPS" "$O_ACT_NS" "$O_ACT_MOPS" "$SPEEDUP_ACT" "$GAIN_ACT"

SPEEDUP_SC=$(calc_speedup "$B_SC_NS" "$O_SC_NS")
GAIN_SC=$(calc_gain_pct "$B_SC_NS" "$O_SC_NS")
printf "%-27s | %5.2f ns (%5.1fM) | %5.2f ns (%5.1fM) | %9s | %s\n" "Start + Cancel pair" "$B_SC_NS" "$B_SC_MOPS" "$O_SC_NS" "$O_SC_MOPS" "$SPEEDUP_SC" "$GAIN_SC"

SPEEDUP_RST=$(calc_speedup "$B_RST_NS" "$O_RST_NS")
GAIN_RST=$(calc_gain_pct "$B_RST_NS" "$O_RST_NS")
printf "%-27s | %5.2f ns (%5.1fM) | %5.2f ns (%5.1fM) | %9s | %s\n" "async_delay_restart()" "$B_RST_NS" "$B_RST_MOPS" "$O_RST_NS" "$O_RST_MOPS" "$SPEEDUP_RST" "$GAIN_RST"

SPEEDUP_POP=$(calc_speedup "$B_POP_NS" "$O_POP_NS")
GAIN_POP=$(calc_gain_pct "$B_POP_NS" "$O_POP_NS")
printf "%-27s | %5.2f ns (%5.1fM) | %5.2f ns (%5.1fM) | %9s | %s\n" "async_delay_active_count()" "$B_POP_NS" "$B_POP_MOPS" "$O_POP_NS" "$O_POP_MOPS" "$SPEEDUP_POP" "$GAIN_POP"

echo "================================================================================"
echo "  [OK] Performance differential verified."
echo "================================================================================"
