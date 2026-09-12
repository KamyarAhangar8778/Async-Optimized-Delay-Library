# Plan 003: Expand Test Matrix for Timer Macros, Duration Limits & Concurrency

> **Executor instructions**: Follow this plan step by step. Run every verification command and confirm the expected result before moving to the next step. If anything in the "STOP conditions" section occurs, stop and report — do not improvise. When done, update the status row for this plan in `plans/README.md`.
>
> **Drift check (run first)**: `git diff --stat cf1b0cb..HEAD -- tests/test_async_delay.c tests/test_stress.c`
> If any in-scope file changed since this plan was written, compare the "Current state" excerpts against the live code before proceeding; on a mismatch, treat it as a STOP condition.

## Status

- **Priority**: P2
- **Effort**: S
- **Risk**: LOW
- **Depends on**: plans/001-fix-timer2-setup-and-deferred-callback-race.md
- **Category**: tests
- **Planned at**: commit `cf1b0cb`, 2026-09-11

## Why this matters

1. **Hardware Timer Macro Testing**: The hardware timer setup macros (`ASYNC_DELAY_SETUP_TIMER1_CTC_*`, `ASYNC_DELAY_SETUP_TIMER2_CTC_*`, `async_delay_hw_timer1_init`) were previously excluded from host unit tests because AVR registers (`TCCR1A`, `TCCR2`, `OCR2`, etc.) are hardware-specific. Adding mock registers in host test mode allows automated verification of prescaler bit patterns and OCR calculations.
2. **Boundary Validation Testing**: Validating that `duration > _ASYNC_HALF_RANGE` safely returns `ASYNC_DELAY_NO_SLOT` or `0` ensures invalid durations cannot break the wrap-safe arithmetic.
3. **16-Slot Concurrency & Deferred Callbacks Coverage**: Adding test coverage for 16-slot configurations under `ASYNC_DELAY_DEFERRED_CALLBACKS=1` guarantees no race conditions or mask corruption occur during high-frequency timer expirations.

## Current state

- **File**: `tests/test_async_delay.c` — Host unit test suite (70 tests).
- **File**: `tests/test_stress.c` — High-frequency stress test harness.
- **File**: `tests/run_all_configs.sh` — Full test execution script.

## Commands you will need

| Purpose | Command | Expected on success |
|---------|---------|---------------------|
| Unit Matrix | `bash tests/run_all_configs.sh` | ALL CONFIGURATION MATRICES PASSED SUCCESSFULLY! |

## Scope

**In scope**:
- `tests/test_async_delay.c`
- `tests/test_stress.c`

**Out of scope**:
- `async_delay.h` logic changes (handled in Plans 001 and 002)

## Git workflow

- Commit per step; message style: `test(async_delay): add unit tests for hardware macros and boundary conditions`

## Steps

### Step 1: Add Hardware Timer Macro Mock Registers & Unit Test in `test_async_delay.c`

In `tests/test_async_delay.c`:
Define mock register variables for host testing if not defined (`TCCR1A`, `TCCR1B`, `TCCR2`, `OCR1AH`, `OCR1AL`, `OCR2`, `TIMSK`, `TCNT1H`, `TCNT1L`, `TCNT2`).
Add `test_hardware_timer_setup()`:
- Invoke `ASYNC_DELAY_SETUP_TIMER2_CTC_8MHZ_1KHZ()` and assert `TCCR2 == 0x0C` and `OCR2 == 124`.
- Invoke `ASYNC_DELAY_SETUP_TIMER2_CTC_16MHZ_1KHZ()` and assert `TCCR2 == 0x0C` and `OCR2 == 249`.
- Invoke `ASYNC_DELAY_SETUP_TIMER1_CTC_8MHZ_1KHZ()` and assert `OCR1AH == 0x03` and `OCR1AL == 0xE7` (OCR = 999).
- Invoke `async_delay_hw_timer1_init(8000000UL, 1000)` and verify registers match.

**Verify**: `gcc -Wall -Wextra -O2 -I. tests/test_async_delay.c -o tests/runner_bin && ./tests/runner_bin` → all pass.

### Step 2: Add Duration Boundary Test in `test_async_delay.c`

Add `test_duration_boundary_limits()`:
- Test `async_delay_start(_ASYNC_HALF_RANGE + 1, NULL)` → returns `ASYNC_DELAY_NO_SLOT`.
- Test `async_delay_start(_ASYNC_HALF_RANGE, NULL)` → succeeds.
- Test `async_delay_restart(id, _ASYNC_HALF_RANGE + 1)` → returns `0`.

**Verify**: `bash tests/run_all_configs.sh` → all configuration matrices pass.

### Step 3: Add 16-Slot Deferred Callbacks Stress Suite in `test_stress.c`

In `tests/test_stress.c`:
Add test case triggering high-frequency deferred callback polling with 16 active slots and concurrent cancellations/restarts.

**Verify**: `bash tests/run_all_configs.sh` → all pass cleanly.

## Test plan

- Run `bash tests/run_all_configs.sh` and confirm all new unit tests pass across all configurations.

## Done criteria

- [ ] `test_hardware_timer_setup()` verifies register values for all Timer1/Timer2 macros.
- [ ] `test_duration_boundary_limits()` verifies upper duration bounds.
- [ ] `bash tests/run_all_configs.sh` exits 0 with 0 failures.

## STOP conditions

- If host compilation fails due to duplicate mock register declarations, wrap mock declarations in `#ifndef __AVR__`.

## Maintenance notes

- Ensure all test functions are called in `main()` of `test_async_delay.c`.
