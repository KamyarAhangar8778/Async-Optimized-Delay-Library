# Plan 005: Codebase Design Deepening & Module Completeness

## Overview
Applied the principles from `.gemini/skills/codebase-design/SKILL.md` (Deep Modules, Clean Seams, Test Surface Alignment) to `async_delay.h` and the surrounding project artifacts.

## Key Actions Taken
1. **Deepened Hardware Timer Seam**:
   - Expanded Timer 2 preset macros (`ASYNC_DELAY_SETUP_TIMER2_CTC_4MHZ_1KHZ`, `2MHZ_1KHZ`, `1MHZ_1KHZ`) to complement Timer 1 macros.
   - Added `async_delay_hw_timer2_init(f_cpu_hz, tick_hz)` dynamic initialization helper function for Timer 2.
2. **Aligned Test Surface with Public Seam**:
   - Expanded `tests/test_async_delay.c` with unit tests for Timer2 4MHz/2MHz/1MHz setup macros and dynamic `async_delay_hw_timer2_init()`.
3. **Synchronized Build Specifications & Versioning**:
   - Bumped `ASYNC_DELAY_VERSION` from 4 to 5 in `async_delay.h`.
   - Updated compile-time version assertions and macro references across `README.md`, `ARCHITECTURE.md`, and `async_delay_guide.md`.
4. **Verification**:
   - Ran full automated matrix (`tests/run_all_configs.sh`). All unit tests, stress tests, and AVR cycle/RAM regression checks passed green with zero performance loss.
