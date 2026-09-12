# Plan 002: Optimize Recompute Sweep & Refactor Unified CLI/SEI Macros

> **Executor instructions**: Follow this plan step by step. Run every verification command and confirm the expected result before moving to the next step. If anything in the "STOP conditions" section occurs, stop and report — do not improvise. When done, update the status row for this plan in `plans/README.md`.
>
> **Drift check (run first)**: `git diff --stat cf1b0cb..HEAD -- async_delay.h`
> If any in-scope file changed since this plan was written, compare the "Current state" excerpts against the live code before proceeding; on a mismatch, treat it as a STOP condition.

## Status

- **Priority**: P1
- **Effort**: S
- **Risk**: LOW
- **Depends on**: plans/001-fix-timer2-setup-and-deferred-callback-race.md
- **Category**: perf
- **Planned at**: commit `cf1b0cb`, 2026-09-11

## Why this matters

1. **Recompute Sweep Optimization**: When `ASYNC_DELAY_OPT_UNROLL_RECOMPUTE=1` recalculates `_async_next_target`, `_AD_RECOMP_SWEEP()` scans all 16 slot macro checks sequentially without checking whether all active slots have already been found (`m == 0`). In benchmarks, this causes a throughput penalty during `Start + Cancel` pairs when only 1 or 2 slots are active out of 16. Adding early exit (`if (m == 0) return;`) inside the unrolled sweep restores $O(\text{active})$ efficiency.
2. **Unified Platform Assembly Layer**: The conditional platform logic (`#if defined(__GNUC__) || defined(__clang__)`) is repeated across 13 functions in `async_delay.h`. Unifying the SREG save/restore and CLI invocation into a structured portability macro cleans up code structure while respecting CodeVisionAVR's rule that `#asm("cli")` must remain at the physical call site.

## Current state

- **File**: `async_delay.h`
  - Lines 680–688: `_AD_RECOMP_SLOT(n)` checks `m & _AD_SLOT_BIT(n)` but does not clear `m` or exit early when `m == 0`.
  - Lines 634–653: SREG critical section macros defined.
  - Lines 857, 960, 1007, 1068, 1105, 1136, 1158, 1204, 1273, 1328, 1365, 1411, 1795: Repeated platform checks for CLI/SEI.

## Commands you will need

| Purpose | Command | Expected on success |
|---------|---------|---------------------|
| Unit Matrix | `bash tests/run_all_configs.sh` | ALL CONFIGURATION MATRICES PASSED SUCCESSFULLY! |
| Benchmark | `bash tests/compare_performance.sh` | Benchmark performance report |

## Scope

**In scope**:
- `async_delay.h`

**Out of scope**:
- Altering CodeVisionAVR inline `#asm("cli")` call sites (must remain literal per `ARCHITECTURE.md` §6.3 rule).

## Git workflow

- Commit per step; message style: `perf(async_delay): early exit in unrolled recompute sweep`

## Steps

### Step 1: Add Early Exit to Unrolled Recompute Sweep (`_AD_RECOMP_SLOT`)

In `async_delay.h`:
Update `_AD_RECOMP_SLOT(n)`:
```c
#define _AD_RECOMP_SLOT(n)                                \
    if (m & _AD_SLOT_BIT(n))                              \
    {                                                     \
        if (first || _ASYNC_REACHED(best, _AD_TARGET(n))) \
        {                                                 \
            best = _AD_TARGET(n);                         \
            first = 0;                                    \
        }                                                 \
        m &= (async_mask_t)~_AD_SLOT_BIT(n);              \
        if (m == 0)                                       \
        {                                                 \
            _async_next_target = best;                    \
            return;                                       \
        }                                                 \
    }
```

**Verify**: `bash tests/run_all_configs.sh` → all pass and Phase 3 performance report shows improved throughput.

### Step 2: Streamline Atomic Interrupt Gate Macros

Define unified helper macros `_ASYNC_CLI()` and `_ASYNC_SEI()` in the platform block:
```c
#if defined(__GNUC__) || defined(__clang__)
#define _ASYNC_CLI() _ASYNC_ASM_CLI()
#define _ASYNC_SEI() sei()
#else
#define _ASYNC_CLI() #asm("cli")
#define _ASYNC_SEI() #asm("sei")
#endif
```
Note: Keep `#asm("cli")` and `#asm("sei")` literal at CodeVisionAVR physical call sites as required by `ARCHITECTURE.md` §6.3, using `_ASYNC_CLI()` / `_ASYNC_SEI()` for GCC/Clang sections.

**Verify**: `bash tests/run_all_configs.sh` → all pass.

## Test plan

- Run `bash tests/run_all_configs.sh` to ensure all 70 unit tests and 60 stress test configurations pass.
- Run `bash tests/compare_performance.sh` to confirm performance gains.

## Done criteria

- [ ] `_AD_RECOMP_SLOT(n)` exits immediately when `m == 0`.
- [ ] No compilation warnings or errors in GCC, Clang, or CodeVisionAVR mode.
- [ ] `bash tests/run_all_configs.sh` exits 0.

## STOP conditions

- If CodeVisionAVR fails preprocessor macro expansion on inline assembly, stop and keep literal `#asm("cli")` at call sites.

## Maintenance notes

- Increment `ASYNC_DELAY_VERSION` to `3` in `async_delay.h`.
