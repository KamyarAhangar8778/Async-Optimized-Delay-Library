# Plan 004: Add Power-Saving IDLE Sleep Helper (`async_delay_sleep_idle`)

> **Executor instructions**: Follow this plan step by step. Run every verification command and confirm the expected result before moving to the next step. If anything in the "STOP conditions" section occurs, stop and report — do not improvise. When done, update the status row for this plan in `plans/README.md`.
>
> **Drift check (run first)**: `git diff --stat cf1b0cb..HEAD -- async_delay.h`
> If any in-scope file changed since this plan was written, compare the "Current state" excerpts against the live code before proceeding; on a mismatch, treat it as a STOP condition.

## Status

- **Priority**: P3
- **Effort**: S
- **Risk**: LOW
- **Depends on**: plans/001-fix-timer2-setup-and-deferred-callback-race.md, plans/002-optimize-recompute-and-refactor-cli-macros.md
- **Category**: feature
- **Planned at**: commit `cf1b0cb`, 2026-09-11

## Why this matters

`async_delay_ticks_until_next()` calculates remaining ticks until the next active timer expires, allowing applications to put the MCU to sleep in CPU IDLE mode to save energy. However, entering sleep mode safely on AVR microcontrollers requires enabling interrupts atomically before executing `sleep_cpu()`. Adding `async_delay_sleep_idle()` under `ASYNC_DELAY_FEATURE_SLEEP` provides a safe, standard helper for low-power operation.

## Current state

- **File**: `async_delay.h`
  - Lines 1348–1402: `async_delay_ticks_until_next()` calculates sleep duration.
  - No direct sleep helper function is present in `async_delay.h`.

## Commands you will need

| Purpose | Command | Expected on success |
|---------|---------|---------------------|
| Unit Matrix | `bash tests/run_all_configs.sh` | ALL CONFIGURATION MATRICES PASSED SUCCESSFULLY! |

## Scope

**In scope**:
- `async_delay.h`
- `README.md`, `async_delay_guide.md`

**Out of scope**:
- Non-idle sleep modes (Deep sleep / Power-down disables hardware timer clock on AVR).

## Git workflow

- Commit per step; message style: `feat(async_delay): add low-power IDLE sleep helper async_delay_sleep_idle`

## Steps

### Step 1: Add `ASYNC_DELAY_FEATURE_SLEEP` Configuration Flag & Helper in `async_delay.h`

In `async_delay.h`:
Add feature flag declaration:
```c
#ifndef ASYNC_DELAY_FEATURE_SLEEP
#define ASYNC_DELAY_FEATURE_SLEEP 0
#endif
```

In Utility API section:
```c
#if ASYNC_DELAY_FEATURE_SLEEP
#if defined(__AVR__)
#include <avr/sleep.h>
    // Atomically enter AVR IDLE sleep mode until the next timer tick interrupt.
    static inline void async_delay_sleep_idle(void)
    {
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_enable();
#if defined(__GNUC__) || defined(__clang__)
        sei();
        sleep_cpu();
#else
#asm("sei")
#asm("sleep")
#endif
        sleep_disable();
    }
#else
    // Host simulation fallback for sleep_idle
    static inline void async_delay_sleep_idle(void)
    {
        (void)0;
    }
#endif
#endif
```

**Verify**: `bash tests/run_all_configs.sh` → all pass.

### Step 2: Document `async_delay_sleep_idle` in `README.md` and `async_delay_guide.md`

Update documentation to show usage pattern:
```c
if (async_delay_ticks_until_next() > 0) {
    async_delay_sleep_idle();
}
```

**Verify**: `bash tests/run_all_configs.sh` → all pass.

## Test plan

- Run `bash tests/run_all_configs.sh` and verify compilation and test suite execution with `ASYNC_DELAY_FEATURE_SLEEP=1`.

## Done criteria

- [ ] `async_delay_sleep_idle()` compiles without warnings on AVR and Host targets.
- [ ] Documented in `README.md` and `async_delay_guide.md`.
- [ ] `bash tests/run_all_configs.sh` exits 0.

## STOP conditions

- If host compiler fails on `<avr/sleep.h>`, ensure `#if defined(__AVR__)` guard is strictly active.

## Maintenance notes

- Increment `ASYNC_DELAY_VERSION` to `4` in `async_delay.h`.
