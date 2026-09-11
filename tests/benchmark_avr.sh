#!/usr/bin/env bash
# benchmark_avr.sh - Single-command AVR Benchmarking & Before/After Comparison Tool
# Usage:
#   bash tests/benchmark_avr.sh          # Run full AVR timing & MCU matrix benchmark
#   bash tests/benchmark_avr.sh --save   # Save current state as baseline snapshot
#   bash tests/benchmark_avr.sh --diff   # Compare current code against saved baseline

set -e

if [ "$1" == "--save" ] || [ "$1" == "-s" ]; then
    echo "Saving current library state as benchmark baseline snapshot..."
    python3 tests/avr_benchmark.py --save-baseline
elif [ "$1" == "--diff" ] || [ "$1" == "-d" ] || [ "$1" == "--compare" ]; then
    echo "Comparing current library state against baseline snapshot..."
    python3 tests/avr_benchmark.py --compare
else
    python3 tests/avr_benchmark.py
fi
