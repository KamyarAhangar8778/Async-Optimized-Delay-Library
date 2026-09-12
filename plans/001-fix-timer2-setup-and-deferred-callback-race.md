# Plan 001: Fix Timer2 CTC Prescaler Bug & Deferred Callback Race Condition

> **Executor instructions**: Follow this plan step by step. Run every verification command and confirm the expected result before moving to the next step. If anything in the "STOP conditions" section occurs, stop and report — do not improvise. When done, update the status row for this plan in `plans/README.md`.
>
> **Drift check (run first)**: `git diff --stat cf1b0cb..HEAD -- async_delay.h`
> If any in-scope file changed since this plan was written, compare the "Current state" excerpts against the live code before proceeding; on a mismatch, treat it as a STOP condition.

## Status

- **Priority**: P1
- **Effort**: S
- **Risk**: LOW
- **Depends on**: none
- **Category**: bug
- **Planned at**: commit `cf1b0cb`, 2026-09-11

## Why this matters

1. **Timer2 Prescaler Bug**: `ASYNC_DELAY_SETUP_TIMER2_CTC_8MHZ_1KHZ()` and `ASYNC_DELAY_SETUP_TIMER2_CTC_16MHZ_1KHZ()` set `TCCR2 = 0x0B`, which sets prescaler bits `CS21=1, CS20=1` (prescaler /32) instead of `CS22=1` (prescaler /64). This causes Timer2 to interrupt at 2 kHz instead of 1 kHz, making all non-blocking delays run at double speed on ATmega8/16/32 microcontrollers.
2. **Deferred Callback Race Condition**: When a polling timer (with `callback == NULL`) expires under `ASYNC_DELAY_DEFERRED_CALLBACKS=1`, `_async_delay_expire_slot()` sets `_async_pending_mask |= slotbit`. If this polling slot is subsequently reallocated for a new timer with a callback *before* `async_delay_poll()` runs, `async_delay_poll()` executes the new timer's callback prematurely.
3. **Non-Atomic 16-Bit Mask Read**: In `async_delay_ticks_until_next()`, `_async_active_mask` is checked outside a critical section. When `MAX_SLOTS > 8`, `async_mask_t` is 16-bit, creating a 2-byte torn read race condition on 8-bit AVR when the ISR modifies the active mask.
4. **Invalid Duration Wrap-Around**: Scheduling a timer with `duration > _ASYNC_HALF_RANGE` violates wrap-safe comparison bounds, causing timers to expire immediately.

## Current state

- **File**: `async_delay.h` — Core header-only library.
  - Lines 1358–1361: `_async_active_mask == 0` checked before critical section in `async_delay_ticks_until_next()`.
  - Lines 1457–1470: `_async_pending_mask |= (async_mask_t)~clr;` executed in periodic and one-shot expiry without checking if `_AD_CB(i) != NULL`.
  - Lines 1888–1901: `TCCR2 = 0x0B;` sets Timer2 prescaler /32 instead of /64 (`0x0C`).
  - Lines 966–968: `tgt = (async_tick_t)(now + duration);` computed without checking `duration > _ASYNC_HALF_RANGE`.

## Commands you will need

| Purpose | Command | Expected on success |
|---------|---------|---------------------|
| Unit Matrix | `bash tests/run_all_configs.sh` | ALL CONFIGURATION MATRICES PASSED SUCCESSFULLY! |
| Benchmark | `bash tests/benchmark_avr.sh` | Outputs AVR cycle and latency matrix |

## Scope

**In scope**:
- `async_delay.h`
- `tests/test_async_delay.c`

**Out of scope**:
- `README.md`, `ARCHITECTURE.md` (to be updated in documentation sync step after code verification)
- Changing wrap-safe comparison arithmetic (`_ASYNC_REACHED`)

## Git workflow

- Commit per step or per logical unit; message style: `fix(async_delay): correct Timer2 prescaler and deferred callback race`

## Steps

### Step 1: Correct Timer2 CTC Prescaler Bits in `ASYNC_DELAY_SETUP_TIMER2_CTC_*`

In `async_delay.h`:
Replace:
```c
#define ASYNC_DELAY_SETUP_TIMER2_CTC_16MHZ_1KHZ() do { \
        TCCR2 = 0x0B; /* CTC mode (WGM21=1), Prescaler /64 */ \
        TCNT2 = 0x00; \
        OCR2  = 249;  /* 16MHz / (64 * 1000Hz) - 1 = 249 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER2_CTC_8MHZ_1KHZ() do { \
        TCCR2 = 0x0B; /* CTC mode (WGM21=1), Prescaler /64 */ \
        TCNT2 = 0x00; \
        OCR2  = 124;  /* 8MHz / (64 * 1000Hz) - 1 = 124 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)
```
With:
```c
#define ASYNC_DELAY_SETUP_TIMER2_CTC_16MHZ_1KHZ() do { \
        TCCR2 = (1 << 3) | (1 << 2); /* CTC mode (WGM21=1), Prescaler /64 (CS22=1) -> 0x0C */ \
        TCNT2 = 0x00; \
        OCR2  = 249;  /* 16MHz / (64 * 1000Hz) - 1 = 249 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)

#define ASYNC_DELAY_SETUP_TIMER2_CTC_8MHZ_1KHZ() do { \
        TCCR2 = (1 << 3) | (1 << 2); /* CTC mode (WGM21=1), Prescaler /64 (CS22=1) -> 0x0C */ \
        TCNT2 = 0x00; \
        OCR2  = 124;  /* 8MHz / (64 * 1000Hz) - 1 = 124 */ \
        TIMSK |= 0x80; /* Enable OCIE2 */ \
    } while (0)
```

**Verify**: `bash tests/run_all_configs.sh` → all pass.

### Step 2: Fix Pending Mask Race Condition in `_async_delay_expire_slot()`

In `async_delay.h`:
In `_async_delay_expire_slot()`, ensure `_async_pending_mask` is ONLY set if `_AD_CB(i) != (void *)0`:
```c
#if ASYNC_DELAY_DEFERRED_CALLBACKS
            if (_AD_CB(i) != (void *)0)
                _async_pending_mask |= (async_mask_t)~clr;
#else
```
And similarly for periodic callbacks in deferred mode, verify `_AD_CB(i) != (void *)0` before setting `_async_pending_mask`.

**Verify**: `bash tests/run_all_configs.sh` → all pass.

### Step 3: Move `_async_active_mask` Read inside Critical Section in `async_delay_ticks_until_next()`

In `async_delay.h`:
Move `if (_async_active_mask == 0) return 0;` inside the SREG critical section in `async_delay_ticks_until_next()`:
```c
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
        _ASYNC_SAVE_SREG();
#if defined(__GNUC__) || defined(__clang__)
        _ASYNC_ASM_CLI();
#else
#asm("cli")
#endif
#endif
#if ASYNC_DELAY_OPT_BITMASK
        if (_async_active_mask == 0)
        {
#if ASYNC_DELAY_TIMER_BITS >= 16 || ASYNC_DELAY_FIX_ATOMIC_MASK
            _ASYNC_REST_SREG();
#endif
            return 0;
        }
#endif
        now = _async_tick_counter;
```

**Verify**: `bash tests/run_all_configs.sh` → all pass.

### Step 4: Add Duration Boundary Validation (`duration > _ASYNC_HALF_RANGE`)

In `_async_delay_start_common()` and `async_delay_restart()`:
Check if `duration == 0 || duration > _ASYNC_HALF_RANGE`. If `duration > _ASYNC_HALF_RANGE`, return `ASYNC_DELAY_NO_SLOT` (or `0` for restart).

**Verify**: `bash tests/run_all_configs.sh` → all pass.

## Test plan

- Execute `bash tests/run_all_configs.sh` to verify that all 70 unit tests, 60 stress test suites, performance comparisons, and AVR benchmarks pass cleanly across all flag permutations.

## Done criteria

- [ ] `TCCR2` setting in `ASYNC_DELAY_SETUP_TIMER2_CTC_*` macros evaluates to `0x0C` (prescaler /64).
- [ ] `_async_pending_mask` is never set for NULL callback polling timers in deferred mode.
- [ ] `_async_active_mask` is accessed atomically inside critical section in `async_delay_ticks_until_next()`.
- [ ] `duration > _ASYNC_HALF_RANGE` returns `ASYNC_DELAY_NO_SLOT` / `0`.
- [ ] `bash tests/run_all_configs.sh` exits 0 with 0 failures.

## STOP conditions

- If any host compiler or CodeVisionAVR compatibility warning occurs on atomic macros, stop and report.
- If existing unit test assertions fail under `ASYNC_DELAY_DEFERRED_CALLBACKS=1`, stop and investigate.

## Maintenance notes

- Increment `ASYNC_DELAY_VERSION` to `2` in `async_delay.h`.
- Document `TCCR2 = 0x0C` correction in `README.md` and `async_delay_guide.md`.
