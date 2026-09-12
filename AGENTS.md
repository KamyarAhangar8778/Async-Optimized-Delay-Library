# AGENTS.md: AI Agent Operational Instructions & Constraints

This document defines the system identity, execution constraints, compiler rules, testing commands, and invariants for AI agents working on `async_delay.h`.

---

## 1. System Identity & Mission

- **Target Architecture**: 8-bit Microchip/Atmel AVR (`ATmega8`, `ATmega16`, `ATmega32`) clocked at 1–16 MHz.
- **Compiler Targets**: **CodeVisionAVR**, **AVR-GCC / Clang**, and **Host Native GCC/Clang** (Linux x86_64).
- **Core Library File**: `async_delay.h` (header-only, zero-heap, 100% static RAM).
- **Build Version**: `ASYNC_DELAY_VERSION 5`.

---

## 2. File Map & Primary Roles

| File / Path | Purpose & Role |
|---|---|
| `async_delay.h` | Core header-only library containing all algorithms, ISR gates, data structures, and configuration macros. |
| `ARCHITECTURE.md` | Architectural invariants, cycle budgets, hardware constraints, and data structure layouts. |
| `AGENTS.md` | AI Agent operational guidelines, compiler constraints, and testing protocols (this file). |
| `CLAUDE.md` | Project preferences and speed optimization directives. |
| `README.md` | Public technical documentation, CTC timer formulas, and API reference. |
| `async_delay_guide.md` | Persian integration guide for CodeVisionAVR developers. |
| `async_delay_test.c` | Reference ATmega8 CodeVisionAVR firmware project (Timer2 CTC, LCD, LEDs). |
| `tests/run_all_configs.sh` | Primary verification script (runs unit tests, stress matrix, and cycle benchmarks). |
| `tests/benchmark_avr.sh` | AVR cycle-accurate latency, timer ISR overhead, and RAM footprint profiling tool. |
| `tests/test_async_delay.c` | Host-based native C unit test suite (90 assertions). |
| `tests/test_stress.c` | High-frequency stress test suite for boundary conditions and wrap-around arithmetic. |

---

## 3. Strict Compiler & Hardware Constraints

1. **CodeVisionAVR C89 Variable Declarations**:
   - ALL local variables MUST be declared at the very beginning of a function or block before any executable statements.
2. **CodeVisionAVR Reserved Keyword Ban**:
   - NEVER use `bit`, `flash`, `eeprom`, `sfrb`, `sfrw`, `interrupt`, or `funcused` as variable names or parameter names. Use descriptive alternatives like `slotbit` or `mask_val`.
3. **No Multi-Bit Variable Shift Loops**:
   - AVR lacks a hardware barrel shifter. Expressions like `(1 << n)` on variable `n` trigger `RCALL __LSLW12` (10–30 cycles). Always use lookup tables (`_async_slot_bit`, `_async_first_free_nibble`, `_async_popcount_nibble`).
4. **Register Spill Prevention (`__SAVELOCR4`)**:
   - CodeVisionAVR spills local variables to stack upon function entry before evaluating `if` conditions. Hot paths with early exits (like `async_delay_tick()`) MUST remain local-free.
5. **Inline Assembly Macro Safety**:
   - CodeVisionAVR preprocessor does not reliably expand inline assembly inside macros. Keep `#asm("cli")` inline at physical call sites and keep SREG save/restore in macros.
6. **Volatile Counter Increment First**:
   - In `async_delay_tick()`, `_async_tick_counter++` MUST run before any early-exit or mask checks so time never freezes.
7. **Configurability**:
   - Every new feature or optimization MUST be gated behind a configurable macro flag (e.g., `#ifndef ASYNC_DELAY_FEATURE_X`).

---

## 4. Testing & Verification Protocol

Always verify changes using the automated test suite before completing any task:

```bash
# 1. Run full test matrix (Unit tests + Stress suites + AVR Cycle Benchmarks):
bash tests/run_all_configs.sh

# 2. Save baseline performance snapshot before making library changes:
bash tests/benchmark_avr.sh --save

# 3. Compare changes against baseline to detect speed or memory regressions:
bash tests/benchmark_avr.sh --diff
```

---

## 5. Architectural Invariants

- **Zero Heap**: Memory allocation is 100% static in RAM/Flash.
- **Wrap-Safe Arithmetic**: `_ASYNC_REACHED(now, target)` uses per-width unsigned subtraction and explicit width literals (`0x7FFF`).
- **Atomic Concurrency**: Shared volatile state (`_async_tick_counter`, `_async_active_mask`, `_async_used_mask`) accessed outside ISR must be wrapped in SREG-preserving critical sections (`_ASYNC_SAVE_SREG()`, `#asm("cli")`, `_ASYNC_REST_SREG()`).
- **Phase-Locked Periodic Arming**: Periodic targets re-arm via `target += duration` (never `now + duration`).
