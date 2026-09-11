#!/usr/bin/env python3
# avr_benchmark.py - AVR Hardware Benchmark & Before/After Regression Engine
# Measures AVR cycles, timings across 1/2/4/8/16 MHz, and ATmega8/16/32 MCU footprints.

import os
import sys
import json
import argparse
import subprocess
from typing import Dict, Any, Optional

from avr_model import (
    MCU_SPECS,
    SUPPORTED_FREQUENCIES_MHZ,
    cycles_to_time_us,
    calculate_isr_cpu_load_pct,
    calculate_mcu_memory_utilization
)

BASELINE_FILE = os.path.join(os.path.dirname(__file__), ".baseline_benchmark.json")

def compile_and_run_benchmark(flags: str = "", label: str = "CURRENT") -> Dict[str, Any]:
    """Compile and execute benchmark_avr_native.c and return parsed JSON."""
    bin_path = os.path.join(os.path.dirname(__file__), "avr_bench_bin")
    src_path = os.path.join(os.path.dirname(__file__), "benchmark_avr_native.c")
    
    cmd_compile = f"gcc -Wall -Wextra -Wno-unused-function -O2 -I. {flags} {src_path} -o {bin_path}"
    ret = subprocess.run(cmd_compile, shell=True, capture_output=True, text=True)
    if ret.returncode != 0:
        print("Compilation error:", ret.stderr, file=sys.stderr)
        sys.exit(1)
        
    ret_run = subprocess.run([bin_path, label], capture_output=True, text=True)
    if os.path.exists(bin_path):
        os.remove(bin_path)
        
    if ret_run.returncode != 0:
        print("Execution error:", ret_run.stderr, file=sys.stderr)
        sys.exit(1)
        
    try:
        return json.loads(ret_run.stdout)
    except Exception as e:
        print(f"Failed to parse benchmark JSON: {e}\nRaw output:\n{ret_run.stdout}", file=sys.stderr)
        sys.exit(1)

def print_avr_timing_matrix(cycles_dict: Dict[str, int], tick_hz: int = 1000):
    """Print timing matrix across 1, 2, 4, 8, 16 MHz for all operations."""
    print("\n" + "=" * 92)
    print("      AVR Operation Latency & Timing Matrix across Clock Frequencies (1 - 16 MHz)     ")
    print("=" * 92)
    print(f"{'Operation':<26} | {'AVR Cyc':<8} | {'1 MHz':<10} | {'2 MHz':<10} | {'4 MHz':<10} | {'8 MHz':<10} | {'16 MHz':<10}")
    print("-" * 27 + "+" + "-" * 10 + "+" + ("-" * 12 + "+") * 4 + "-" * 11)
    
    for op_name, cyc in cycles_dict.items():
        t1  = f"{cycles_to_time_us(cyc, 1):.2f} µs"
        t2  = f"{cycles_to_time_us(cyc, 2):.2f} µs"
        t4  = f"{cycles_to_time_us(cyc, 4):.2f} µs"
        t8  = f"{cycles_to_time_us(cyc, 8):.2f} µs"
        t16 = f"{cycles_to_time_us(cyc, 16):.2f} µs"
        
        display_name = op_name.replace("_", " ").title()
        print(f"{display_name:<26} | {cyc:<8} | {t1:<10} | {t2:<10} | {t4:<10} | {t8:<10} | {t16:<10}")

    print("=" * 92)
    
    # ISR CPU Overhead
    print("\n[Timer ISR CPU Overhead @ 1 kHz Tick Rate (TICK_HZ=1000)]")
    idle_cyc = cycles_dict.get("idle_tick", 12)
    act_cyc = cycles_dict.get("active_tick_4slots", 20)
    
    print(f"{'F_CPU Frequency':<16} | {'Idle Tick Load %':<20} | {'Active Tick (4 slots) Load %':<30}")
    print("-" * 17 + "+" + "-" * 22 + "+" + "-" * 32)
    for freq in SUPPORTED_FREQUENCIES_MHZ:
        load_idle = calculate_isr_cpu_load_pct(idle_cyc, tick_hz, freq)
        load_act = calculate_isr_cpu_load_pct(act_cyc, tick_hz, freq)
        print(f"{freq} MHz{'':<11} | {load_idle:.4f} %{'':<12} | {load_act:.4f} %")
    print("-" * 73)

def print_mcu_footprint_table(ram_bytes: int):
    """Print Static RAM & Flash utilization for ATmega8, ATmega16, ATmega32."""
    print("\n" + "=" * 80)
    print("          Target AVR MCU Static Memory Utilization (ATmega8 / 16 / 32)         ")
    print("=" * 80)
    print(f"{'Target MCU':<12} | {'Flash Total':<12} | {'SRAM Total':<12} | {'RAM Used (Library)':<20} | {'SRAM Used %'}")
    print("-" * 13 + "+" + "-" * 14 + "+" + "-" * 14 + "+" + "-" * 22 + "+" + "-" * 13)
    
    for mcu in ["ATmega8", "ATmega16", "ATmega32"]:
        util = calculate_mcu_memory_utilization(mcu, ram_bytes, 0)
        flash_str = f"{util['flash_total'] // 1024} KB"
        ram_str = f"{util['ram_total'] // 1024} KB ({util['ram_total']} B)"
        used_str = f"{ram_bytes} Bytes"
        pct_str = f"{util['ram_pct']:.2f} %"
        print(f"{mcu:<12} | {flash_str:<12} | {ram_str:<12} | {used_str:<20} | {pct_str}")
    print("=" * 80)

def compare_benchmarks(base: Dict[str, Any], curr: Dict[str, Any]) -> int:
    """Compare baseline and current benchmarks, print diff table, return exit code."""
    print("\n" + "=" * 90)
    print(f"   BEFORE vs AFTER COMPARISON: [{base.get('label', 'BASELINE')}] vs [{curr.get('label', 'CURRENT')}]")
    print("=" * 90)
    print(f"{'Operation':<24} | {'Before (Cyc)':<13} | {'After (Cyc)':<13} | {'Delta':<12} | {'Status'}")
    print("-" * 25 + "+" + "-" * 15 + "+" + "-" * 15 + "+" + "-" * 14 + "+" + "-" * 17)
    
    base_cycles = base.get("avr_cycles", {})
    curr_cycles = curr.get("avr_cycles", {})
    
    regressions = 0
    improvements = 0
    
    for op in base_cycles:
        b_val = base_cycles.get(op, 0)
        c_val = curr_cycles.get(op, 0)
        delta = c_val - b_val
        delta_str = f"{'+' if delta > 0 else ''}{delta} cyc"
        
        display_name = op.replace("_", " ").title()
        if delta < 0:
            status = "[FASTER / IMPROVED]"
            improvements += 1
        elif delta > 0:
            status = "[SLOWER / REGRESSION]"
            regressions += 1
        else:
            status = "[EQUIVALENT]"
            
        print(f"{display_name:<24} | {b_val:<13} | {c_val:<13} | {delta_str:<12} | {status}")
        
    print("-" * 90)
    
    # RAM Footprint diff
    b_ram = base.get("static_ram_bytes", 0)
    c_ram = curr.get("static_ram_bytes", 0)
    ram_delta = c_ram - b_ram
    ram_status = "[REDUCED]" if ram_delta < 0 else ("[INCREASED]" if ram_delta > 0 else "[UNCHANGED]")
    print(f"{'Static RAM Footprint':<24} | {b_ram:<10} B   | {c_ram:<10} B   | {'+' if ram_delta > 0 else ''}{ram_delta} B       | {ram_status}")
    print("=" * 90)
    
    if regressions > 0:
        print(f"\n[ALERT] {regressions} OPERATION(S) SUFFERED PERFORMANCE REGRESSION!")
        return 1
    elif improvements > 0:
        print(f"\n[SUCCESS] LIBRARY IS FASTER: {improvements} operation(s) optimized!")
        return 0
    else:
        print("\n[OK] Performance is fully preserved and equivalent.")
        return 0

def main():
    parser = argparse.ArgumentParser(description="AVR Microcontroller Benchmark & Regression Tool for async_delay")
    parser.add_argument("--save-baseline", action="store_true", help="Save current benchmark as baseline snapshot")
    parser.add_argument("--compare", action="store_true", help="Compare current code against saved baseline snapshot")
    parser.add_argument("--flags", type=str, default="", help="Custom compilation flags (e.g. -DASYNC_DELAY_MAX_SLOTS=8)")
    parser.add_argument("--label", type=str, default="CURRENT", help="Label for the benchmark run")
    
    args = parser.parse_args()
    
    current_data = compile_and_run_benchmark(args.flags, args.label)
    
    if args.save_baseline:
        with open(BASELINE_FILE, "w") as f:
            json.dump(current_data, f, indent=2)
        print(f"[OK] Baseline snapshot successfully saved to {BASELINE_FILE}")
        return 0
        
    print_avr_timing_matrix(current_data["avr_cycles"], current_data.get("tick_hz", 1000))
    print_mcu_footprint_table(current_data["static_ram_bytes"])
    
    if args.compare:
        if not os.path.exists(BASELINE_FILE):
            print(f"\n[INFO] Baseline snapshot not found. Saving current run as initial baseline...", file=sys.stderr)
            with open(BASELINE_FILE, "w") as f:
                json.dump(current_data, f, indent=2)
            print(f"[OK] Initial baseline saved to {BASELINE_FILE}")
            return 0
            
        with open(BASELINE_FILE, "r") as f:
            baseline_data = json.load(f)
            
        return compare_benchmarks(baseline_data, current_data)
        
    return 0

if __name__ == "__main__":
    sys.exit(main())
