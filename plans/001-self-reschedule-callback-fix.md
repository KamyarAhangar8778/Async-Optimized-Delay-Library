# Plan 001: Fix self-rescheduling from async_delay callbacks (one-shot NO_SLOT bug, periodic dual-fire)

> **Executor instructions**: Follow this plan step by step. Run every
> verification command and confirm the expected result before moving to the
> next step. If anything in the "STOP conditions" section occurs, stop and
> report — do not improvise. When done, update the status row for this plan
> in `plans/README.md` unless a reviewer told you they maintain the index.
>
> **Drift check (run first)**: `git diff --stat 338a1c8..HEAD -- async_delay.h`
> If any output appears (async_delay.h changed since this plan was written),
> compare the "Current state" excerpts against the live code; on a mismatch,
> treat it as a STOP condition.

## Status

- **Priority**: P1
- **Effort**: S
- **Risk**: MED
- **Depends on**: none
- **Category**: bug
- **Planned at**: commit `338a1c8`, 2026-09-04

## Why this matters

`async_delay_tick()` currently fires a callback while its slot is still in state
`ACTIVE`, then frees or re-arms the slot AFTER the callback returns. Any app code
that starts a new delay from inside a callback ("self-rescheduling" — a one-shot
that re-triggers itself, or chaining stages) hits two failure modes:

1. **One-shot**: the slot is still `ACTIVE` while the callback runs, so a
   `async_delay_start()` inside the callback finds no free slot and returns
   `0xFF` (`ASYNC_DELAY_NO_SLOT`). The app's new delay silently never starts —
   worst case the app spins forever waiting on it.
2. **Periodic**: the slot stays `ACTIVE`, and a `start()` inside the callback
   occupies a *second* slot → **two timers fire** for what the app intended as one.

Fix: make the slot non-ACTIVE (freed for one-shot, re-armed for periodic) BEFORE
the callback runs, so a `start()` inside the callback grabs a free slot cleanly.
This is a real behavior change to an ISR path, so (per project rule in
`CLAUDE.md` — see Current state) it is gated behind a config flag defaulting ON.

## Current state

The whole library is one header file: `async_delay.h`. The function to change is
`async_delay_tick()`.

Excerpt — `async_delay.h` lines 236–281 (the complete function):

```c
static void async_delay_tick(void)
{
    unsigned char i;
    async_tick_t half;

    _async_tick_counter++;

    // Half of the tick range - used for the wrap-safe comparison.
    half = (async_tick_t)(~((async_tick_t)0) >> 1);

    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (_async_slots[i].state == ASYNC_SLOT_ACTIVE)
        {
            // Wrap-safe "now is past target" test:
            // if (tick - target) < half_range, the delay has elapsed.
            // This stays correct across a 16/32-bit counter wrap, as long
            // as the delay is shorter than half the tick range.
            if ((async_tick_t)(_async_tick_counter - _async_slots[i].target) < half)
            {
                if (_async_slots[i].repeat && _async_slots[i].callback != (void *)0)
                {
                    // Periodic: fire callback, then re-arm using target +=
                    // duration so timing stays steady even if a tick is late.
                    _async_slots[i].callback(i);
                    _async_slots[i].target += _async_slots[i].duration;
                    // state stays ACTIVE
                }
                else
                {
                    // One-shot: fire callback (if any) then free the slot.
                    if (_async_slots[i].callback != (void *)0)
                    {
                        _async_slots[i].callback(i);
                        _async_slots[i].state = ASYNC_SLOT_FREE;
                    }
                    else
                    {
                        // Polling mode: mark EXPIRED, user checks with elapsed()
                        _async_slots[i].state = ASYNC_SLOT_EXPIRED;
                    }
                }
            }
        }
    }
}
```

The block to replace is the body of the `if ((async_tick_t)(_async_tick_counter - _async_slots[i].target) < half)` at lines 254–278.

Config-macro block — `async_delay.h` lines 79–116 (the style to match when adding
the flag; new macro goes after the `ASYNC_DELAY_MAX_SLOTS` validation at line 107,
before the tick-type typedef):

```c
#ifndef ASYNC_DELAY_TIMER_BITS
#define ASYNC_DELAY_TIMER_BITS 16
#endif
#if ASYNC_DELAY_TIMER_BITS != 8 && ASYNC_DELAY_TIMER_BITS != 16 && ASYNC_DELAY_TIMER_BITS != 32
#error "[async_delay] ASYNC_DELAY_TIMER_BITS must be 8, 16, or 32."
#endif
```

Repo conventions to match:
- `//` comments; tabs for indentation (this file uses tabs).
- Config macros: `#ifndef X / #define X <default> / #endif`, then `#if`/`#error`
  validation — see the block above.
- Project rule (`CLAUDE.md`, translated from Persian): "For every new feature you
  build, make a flag so I can enable/disable it." This fix changes callback
  ordering, so it gets a flag. (Compile-time `#if`, so the inactive path costs
  zero bytes/cycles.)

## Commands you will need

There is **no build command** in this environment (CodeVisionAVR is a Windows IDE
and the AVR toolchain is not installed here). Verification is read-only + grep +
a final manual build by the user.

| Purpose | Command | Expected on success |
|---------|---------|---------------------|
| Drift check | `git diff --stat 338a1c8..HEAD -- async_delay.h` | no output |
| Flag present | `grep -n "ASYNC_DELAY_CALLBACK_RESCHEDULE" async_delay.h` | `#ifndef`, `#define`, and the `#if`/`#else` uses |
| Old order gone | `grep -n "callback(i)" async_delay.h` | every call site after the state/target line (or inside `#else`) |
| Doc note added | `grep -n "Self-rescheduling" async_delay.h` | 1 match |
| Status clean | `git status --short` | only `async_delay.h` (plus `plans/`) modified |

## Scope

**In scope** (the only files you should modify):
- `async_delay.h` — the flag macro, the reordered tick branches, one doc-comment line.

**Out of scope** (do NOT touch, even though they look related):
- `async_delay_test.c` — no test self-reschedules inside a callback today; the
  fix doesn't need a test change (a host harness is a separate, unselected plan).
- `async_delay_test.prj`, `async_delay_test.cwp` — build/config, unrelated.
- `G:\Kaveh\CodeVsion\inc\async_delay.h` — the USER's mirror copy; never edit it,
  just tell the user to copy the updated header over.
- `ARCHITECTURE.md` — informational; optional to update, not required.
- Do NOT touch the wrap-safe comparison, the atomic 16/32-bit read, or the
  phase-locked `target += duration` logic — all correctness-critical.

## Git workflow

- Branch: `advisor/001-self-reschedule-callback-fix` (repo has no observed
  branch convention; use this).
- One commit for this plan. Message style (match `git log` short subjects, no
  conventional-commit prefix used in this repo):
  `Fix one-shot NO_SLOT and periodic dual-fire on callback self-reschedule`
  with a body line `Co-Authored-By: Claude Code <noreply@anthropic.com>`.
- Do NOT push or open a PR unless the operator instructed it.

## Steps

### Step 1: Add the flag macro

In `async_delay.h`, in the config-macro block, after the `ASYNC_DELAY_MAX_SLOTS`
validation (line 107) and before the `// ---------- Tick type ...` comment
(line 109), insert:

```c
// Order of operations when a callback fires in async_delay_tick():
//   1 = make the slot non-ACTIVE BEFORE calling the callback, so a callback
//       that starts a new delay (self-reschedule) grabs a free slot cleanly.
//   0 = legacy order (callback first, slot freed/re-armed after) — can return
//       0xFF from start() inside a one-shot callback, or double-fire on periodic.
#ifndef ASYNC_DELAY_CALLBACK_RESCHEDULE
#define ASYNC_DELAY_CALLBACK_RESCHEDULE 1
#endif
```

**Verify**: `grep -n "ASYNC_DELAY_CALLBACK_RESCHEDULE" async_delay.h` → 3 matches
(`#ifndef`, `#define`, and the `#if` added in step 2).

### Step 2: Reorder the two fire branches in `async_delay_tick`

Replace the body of the block at lines 254–278 (from `if ((async_tick_t)(_async_tick_counter - _async_slots[i].target) < half)` through its closing `}`) with:

```c
#if ASYNC_DELAY_CALLBACK_RESCHEDULE
                if (_async_slots[i].repeat && _async_slots[i].callback != (void *)0)
                {
                    // Periodic: re-arm FIRST so the slot stays ACTIVE for its next
                    // cycle; the callback then runs with this slot still busy, so a
                    // self-reschedule from the callback lands in a DIFFERENT slot.
                    _async_slots[i].target += _async_slots[i].duration;
                    _async_slots[i].callback(i);
                    // state stays ACTIVE
                }
                else
                {
                    // One-shot: free the slot BEFORE the callback so a
                    // self-reschedule can reuse this very slot.
                    if (_async_slots[i].callback != (void *)0)
                    {
                        _async_slots[i].state = ASYNC_SLOT_FREE;
                        _async_slots[i].callback(i);
                    }
                    else
                    {
                        // Polling mode: mark EXPIRED, user checks with elapsed()
                        _async_slots[i].state = ASYNC_SLOT_EXPIRED;
                    }
                }
#else
                if (_async_slots[i].repeat && _async_slots[i].callback != (void *)0)
                {
                    _async_slots[i].callback(i);
                    _async_slots[i].target += _async_slots[i].duration;
                    // state stays ACTIVE
                }
                else
                {
                    if (_async_slots[i].callback != (void *)0)
                    {
                        _async_slots[i].callback(i);
                        _async_slots[i].state = ASYNC_SLOT_FREE;
                    }
                    else
                    {
                        _async_slots[i].state = ASYNC_SLOT_EXPIRED;
                    }
                }
#endif
```

The `#else` block is byte-for-byte the original code, so the legacy path is
preserved when the flag is 0.

**Verify** (manual logic trace — no compiler available):
- One-shot with callback: `state` is set `FREE` first, then the callback runs. A
  `async_delay_start(...)` from inside the callback scans `_async_slots` from 0,
  finds this slot `FREE`, fills it → returns its id, **no 0xFF**. ✅
- One-shot without callback (polling): unchanged → `EXPIRED` as before. ✅
- Periodic: `target += duration` happens first, slot stays `ACTIVE` the whole
  time, so `start()` inside the callback takes a different slot → no second timer
  on this slot. ✅
- Re-entry safety: the `for` loop reads `_async_slots[i].state` only at the top of
  each iteration; slot `i` is not revisited this pass, so a self-reschedule that
  reuses slot `i` is not double-processed. ✅

### Step 3: Update the header doc comment

In the top-of-file comment block, in the `Error Handling` section (lines 66–74),
append one line:

```c
//   - Self-rescheduling (start() inside a callback): one-shot callbacks are
//     freed before firing, so a new start() inside them works. Periodic
//     callbacks re-arm before firing; do NOT re-start the SAME periodic id
//     from inside its callback (it would create a second timer).
```

**Verify**: `grep -n "Self-rescheduling" async_delay.h` → 1 match.

### Step 4: Manual build gate (external — user action)

In your final report, tell the user: **"Copy `async_delay.h` to
`G:\Kaveh\CodeVsion\inc\async_delay.h` and rebuild `async_delay_test` in
CodeVisionAVR."** Confirm the build reports No errors. This is the real compile
gate — there is no toolchain in the execution environment.

## Test plan

No automated test harness exists for this repo yet (a host-side harness is a
separate unselected plan). Instead:
- Manual logic trace from Step 2 covering: one-shot+callback self-reschedule,
  one-shot polling, periodic self-reschedule (different slot), slot re-entry.
- If the user has the Proteus simulation (`Simulation/AsyncTestSimulaion.pdsprj`),
  suggest running it: LED blink (500/750 ms), cancel, and overflow tests must
  behave identically to before (regression for the flag=1 ordering). The overflow
  test ("5th start returns 0xFF") is the key regression: with the fix, only
  genuinely-full slot sets return 0xFF.

## Done criteria

ALL must hold:

- [ ] `grep -n "ASYNC_DELAY_CALLBACK_RESCHEDULE" async_delay.h` → `#ifndef`, `#define`, and both `#if`/`#else` uses present
- [ ] `grep -n "callback(i)" async_delay.h` → each call is AFTER the state/target mutation line (or inside the `#else` legacy block)
- [ ] The `#else` block is byte-for-byte the ORIGINAL code (the diff is confined to the `#if` true-branch)
- [ ] `grep -n "Self-rescheduling" async_delay.h` → 1 match
- [ ] `git status --short` → only `async_delay.h` (plus `plans/`) modified
- [ ] User reports CodeVisionAVR build: No errors (external gate)
- [ ] `plans/README.md` status row for 001 updated to DONE

## STOP conditions

Stop and report back (do not improvise) if:

- `async_delay.h` lines 236–281 don't match the excerpt (drift since `338a1c8`).
- The flag name `ASYNC_DELAY_CALLBACK_RESCHEDULE` collides with an existing macro.
- You're tempted to also "improve" the loop, the wrap-safe comparison, the atomic
  read, or the phase-locked re-arm — leave them; they are correctness-critical.
- The user's mirror `G:\Kaveh\CodeVsion\inc\async_delay.h` is missing or differs —
  do NOT fix it, just report.

## Maintenance notes

- If a host-side test harness is added later, add a regression test: inside a
  one-shot callback call `async_delay_start` and assert it returns a valid id
  (not 0xFF).
- The periodic re-arm now happens BEFORE the callback runs. The correct pattern
  for "restart this periodic delay" from inside its own callback is
  `async_delay_cancel(id)` then `async_delay_start_periodic(...)` — document this
  in any future usage example.
- Reviewer focus: the ISR path. No new races were introduced — only single-byte
  state writes and the `target` write were reordered, both already executed within
  the ISR's `cli`-free context (single writer).
