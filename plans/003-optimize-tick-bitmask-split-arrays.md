# Plan 003: Optimize `async_delay_tick()` — active-slot bitmask + SoA layout + merged flags (full bundle)

> **Executor instructions**: Follow this plan step by step. Run every
> verification command and confirm the expected result before moving to the
> next step. If anything in the "STOP conditions" section occurs, stop and
> report — do not improvise. When done, update the status row for this plan
> in `plans/README.md` unless a reviewer told you they maintain the index.
>
> **Trigger phrase (user)**: «فایل Plan رو بخون و تحلیل کن و پیاده سازی کن»
> — this plan was written to be executed from a **fresh chat**, so it is fully
> self-contained: read it in full, follow the steps, and report the required
> percentage estimates at the end.
>
> **Drift check (run first)**: `git diff --stat 8e9fab4..HEAD -- async_delay.h`
> If any output appears (async_delay.h changed since this plan was written),
> compare the "Current state" excerpts against the live code; on a mismatch,
> treat it as a STOP condition.

## Status

- **Priority**: P1
- **Effort**: M
- **Risk**: MED (ISR path rewritten — but full legacy path is preserved behind a flag)
- **Depends on**: none
- **Category**: performance
- **Planned at**: commit `8e9fab4`, 2026-09-04

## Why this matters

`async_delay_tick()` runs **every tick** — it is the only hot path in the library
(start/cancel/elapsed only run on app calls). Today it is an **O(MAX_SLOTS) linear
scan** of all slots on *every* tick, even when none are active. At the default
MAX_SLOTS=4 and TICK_HZ=1000 this is already ~1–1.5% of CPU; if a hot-loop user
raises MAX_SLOTS or TICK_HZ (e.g. 10 kHz), it scales linearly and burns CPU for
nothing.

This plan replaces the scan with a **1-byte active-slot bitmask** (`_async_active_mask`)
so the tick visits **only the slots that are actually running**, plus three smaller
wins (split arrays, merged state+repeat flag, compile-time half). The user's project
rule (CLAUDE.md §9.2) says speed is top priority and RAM/Flash may be traded if the
change is genuinely worth it — this plan does exactly that, and every piece is gated
behind its own config flag (rule §9.4).

## Performance targets (report these back after implementation)

| Scenario | Today | With bitmask | Win |
|----------|-------|--------------|-----|
| Idle (no active slots) | ~60–120 cycles | ~12–16 cycles | **~70–85% less** |
| 1 active slot | ~60–100 cycles | ~30–45 cycles | **~40–60% less** |
| All 4 active | ~same | ~same (only the 4 bits visited) | ~0% |
| CPU at 1 kHz tick | ~1.5% | <0.3% | **~80% less** |
| CPU at 10 kHz tick | ~15% | ~2–3% | **~80% less** |

These are estimates on AVR (16-bit tick, MAX_SLOTS=4). The executor must re-derive
them from the generated `.lst`/`.asm` after the user builds (CodeVisionAVR writes
`Debug/List/async_delay_test.asm`), and report a real percentage in the final message.

## Current state

The whole library is one header: `async_delay.h` (HEAD `8e9fab4`). Line numbers
below are from that commit.

Key excerpt — data (lines 144–153):

```c
typedef struct {
    async_tick_t     target;    // tick when this delay expires
    async_tick_t     duration;  // stored for periodic re-arm
    async_delay_cb_t callback;  // NULL = polling-only mode
    unsigned char    state;     // FREE / ACTIVE / EXPIRED
    unsigned char    repeat;    // 1 = periodic (auto re-arm), 0 = one-shot
} _async_slot_t;

static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
static volatile async_tick_t _async_tick_counter;
```

Key excerpt — start (lines 168–199): the linear scan for a FREE slot,
`cli`/`sei` atomic read of the 16/32-bit counter, then writes fields.

Key excerpt — tick (lines 248–320): the O(MAX_SLOTS) scan:

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
            if ((async_tick_t)(_async_tick_counter - _async_slots[i].target) < half)
            {
                // ... 4 branches: periodic / one-shot cb / polling, each with
                //     a RESCHEDULE #if/#else variant ...
            }
        }
    }
}
```

Correctness-critical logic to PRESERVE (ARCHITECTURE.md §6 — do not simplify):
1. Wrap-safe check `(tick - target) < half`.
2. Phase-locked periodic re-arm `target += duration` (NOT `now + duration`).
3. Atomic 16/32-bit counter read with `#asm("cli")`/`#asm("sei")`.
4. `ASYNC_DELAY_CALLBACK_RESCHEDULE` ordering (slot non-ACTIVE before callback).

Config-macro block to match (lines 83–129) — new flags go after the
`ASYNC_DELAY_CALLBACK_RESCHEDULE` block (line 120) and before the
`// ---------- Tick type ...` typedef comment (line 122).

## Config flags (all compile-time `#if`, off-path costs zero)

| Flag | Default | What it gates |
|------|---------|---------------|
| `ASYNC_DELAY_OPT_BITMASK` | `1` | Active-slot bitmask + the "only visit active bits" tick loop. **This is the main win.** |
| `ASYNC_DELAY_OPT_SPLIT_ARRAYS` | `0` | SoA layout: separate `target[] / duration[] / callback[] / state+repeat[]` arrays instead of a struct-of-arrays. Saves per-slot addressing; requires the bitmask flag on to pay off (the bitmask is the fast path). Default `0` to keep the current struct layout unless the user opts in. |
| `ASYNC_DELAY_OPT_MERGED_FLAGS` | `1` | Merge `state` + `repeat` into one `unsigned char` per slot (saves 1 byte/slot, one fewer load in the tick). |

Rationale for the defaults: bitmask and merged-flags are safe, isolated, and save
bytes/cycles with no layout change. Split-arrays changes the data layout, so it is
opt-in and should be measured before adoption.

## Scope

**In scope** (the only file to modify):
- `async_delay.h` — the flag macros, the data layout, and the rewritten
  `async_delay_tick()`, `_async_delay_start_common()`, `async_delay_init()`,
  `async_delay_elapsed()`, `async_delay_cancel()`.

**Out of scope** (do NOT touch):
- `async_delay_test.c`, `async_delay_test.prj`, `async_delay_test.cwp` — test/config, unrelated.
- `G:\Kaveh\CodeVsion\inc\async_delay.h` — the USER's mirror copy; never edit it,
  just tell the user to copy the updated header over (see Step 5).
- `ARCHITECTURE.md` — informational; optional to update, not required.
- Do NOT remove the wrap-safe comparison, the atomic 16/32-bit read, or the
  phase-locked `target += duration` logic (ARCHITECTURE.md §6).

## Commands you will need

There is **no build command** in this environment (CodeVisionAVR is a Windows IDE,
no AVR toolchain installed here). Verification is grep + read-only checks; the real
compile gate is the user's manual build.

| Purpose | Command | Expected on success |
|---------|---------|---------------------|
| Drift check | `git diff --stat 8e9fab4..HEAD -- async_delay.h` | no output |
| Flag present | `grep -n "ASYNC_DELAY_OPT_" async_delay.h` | 3 `#ifndef` / 3 `#define` + uses |
| Bitmask guard | `grep -n "_async_active_mask" async_delay.h` | present in tick + start + cancel + elapsed |
| Split arrays gate | `grep -n "ASYNC_DELAY_OPT_SPLIT_ARRAYS" async_delay.h` | `#ifdef` / `#if` in data section |
| Legacy path intact | `grep -n "for (i = 0; i < ASYNC_DELAY_MAX_SLOTS" async_delay.h` | ≥1 (the `#else` legacy tick loop) |
| Status clean | `git status --short` | only `async_delay.h` (plus `plans/`) modified |

## Design — the bitmask algorithm (copy into working memory)

One new byte: `static volatile unsigned char _async_active_mask;`
bit `n` set ⇒ slot `n` is ACTIVE (state==ACTIVE). The `state` byte is still
written by the functions that need it (elapsed/cancel read EXPIRED); the mask is
the **authoritative** "is this slot running" flag for the tick.

- `start` sets `_async_slots[i].state = ACTIVE; _async_active_mask |= (1<<i);`
  (mask set is inside the existing `cli`/`sei` window only for 16/32-bit; for 8-bit
  the whole read+write is already atomic).
- `cancel` / `elapsed(free)` clears `_async_active_mask &= ~(1<<i)`.
- `tick` walks the mask bits only:

```c
if (ASYNC_DELAY_OPT_BITMASK) {
    /* m = _async_active_mask; while (m) { i = lowbit(m); m &= m-1; ... } */
}
```

where lowbit is a **manual `if` chain** for MAX_SLOTS=4
(`i = 0; if (!(m&1)) { i = 1; if (!(m&2)) { ... } }`) — unrolled, no table, no
loop. For MAX_SLOTS 5–8 the tick uses a simple bit-skip scan (only the bits set in
`m`). A 256-byte lowest-set-bit LUT is a possible further upgrade, but it is NOT
part of this plan — it is not needed to hit the performance targets and adds
Flash for little gain at these slot counts.

The scan order changes from "always 0..MAX_SLOTS" to "bit order" — this is safe:
slots are independent, and self-rescheduling already relies on "callback frees its
own slot before running" (flag 001 behavior, preserved).

### Mask semantics (important — read twice)

- **bit set ⟺ slot state == ACTIVE.** The mask is the authoritative
  "running" set for the tick. Idle detection is `if (mask == 0) return;`.
- `start()`: set the bit (inside the existing `cli`/`sei` window for 16/32-bit).
- `cancel()`: clear the bit.
- In the tick, when a one-shot fires we clear the bit **before** the callback
  (so a self-reschedule can reuse the slot, and the local `m` copy already has
  that bit removed so the slot is not double-processed this pass).
- Polling expiry: set `state = EXPIRED` AND clear the bit — `elapsed()` checks
  `state == EXPIRED`, not the mask, so it still works and the tick stops visiting
  the dead slot.
- Concurrency: the mask is a single byte; single-byte read/write is atomic on
  AVR. The tick reads it once per call into a local `m` and works off the copy.

### Merged flags encoding (when `ASYNC_DELAY_OPT_MERGED_FLAGS` is 1)

One byte per slot replaces `state` + `repeat`:

```c
#define ASYNC_FLAG_REPEAT  0x04
// flags bits: [7..3] unused, [2] repeat, [1..0] state (FREE=0 ACTIVE=1 EXPIRED=2)
```

State checks must then use the low 2 bits (`flags & 0x03`) — a periodic slot has
`flags = ACTIVE|REPEAT = 5`, so `flags == ACTIVE` alone would be wrong. The access
macro `_AD_STATE(i)` below handles this.

## Steps

### Step 1 — Add the three optimization flags

Insert after the `ASYNC_DELAY_CALLBACK_RESCHEDULE` block (ends ~line 120) and
before the `// ---------- Tick type ...` comment (line 122):

```c
// ============ Optimization flags ============
// ASYNC_DELAY_OPT_BITMASK   : 1 = active-slot bitmask; async_delay_tick()
//                             visits ONLY the slots whose bit is set in
//                             _async_active_mask (idle tick ~12 cycles vs ~100).
#ifndef ASYNC_DELAY_OPT_BITMASK
#define ASYNC_DELAY_OPT_BITMASK 1
#endif

// ASYNC_DELAY_OPT_MERGED_FLAGS : 1 = merge state + repeat into one flags byte
//                             per slot (saves 1 byte/slot + one load in tick).
#ifndef ASYNC_DELAY_OPT_MERGED_FLAGS
#define ASYNC_DELAY_OPT_MERGED_FLAGS 1
#endif

// ASYNC_DELAY_OPT_SPLIT_ARRAYS : 1 = split the slot struct into parallel arrays
//                             (target/duration/callback/flags). Slightly faster
//                             AVR addressing (no struct offsets). Opt-in.
#ifndef ASYNC_DELAY_OPT_SPLIT_ARRAYS
#define ASYNC_DELAY_OPT_SPLIT_ARRAYS 0
#endif

// Constraints on the opt combinations:
#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_MAX_SLOTS > 8
#error "[async_delay] ASYNC_DELAY_OPT_BITMASK supports at most 8 slots (8-bit mask)."
#endif
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS && !ASYNC_DELAY_OPT_BITMASK
#error "[async_delay] ASYNC_DELAY_OPT_SPLIT_ARRAYS requires ASYNC_DELAY_OPT_BITMASK=1."
#endif
```

**Verify**: `grep -n "ASYNC_DELAY_OPT_" async_delay.h` → the 3 `#ifndef`/`#define`
pairs plus the constraint `#if`s.

### Step 2 — Data layout, access macros, mask, LUT, half-range

Replace the `// ---------- Internal data ...` section (struct + arrays + counter,
lines 142–153) with:

```c
// ---------- Internal data (static to avoid multiple-definition) ----------

#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define ASYNC_FLAG_REPEAT  0x04   // flags bit 2 = periodic
typedef struct {
    async_tick_t     target;
    async_tick_t     duration;
    async_delay_cb_t callback;
    unsigned char    flags;       // [1:0]=state [2]=repeat
} _async_slot_t;
#else
typedef struct {
    async_tick_t     target;
    async_tick_t     duration;
    async_delay_cb_t callback;
    unsigned char    state;
    unsigned char    repeat;
} _async_slot_t;
#endif

#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
static async_tick_t     _async_target[ASYNC_DELAY_MAX_SLOTS];
static async_tick_t     _async_duration[ASYNC_DELAY_MAX_SLOTS];
static async_delay_cb_t _async_callback[ASYNC_DELAY_MAX_SLOTS];
static unsigned char    _async_flags[ASYNC_DELAY_MAX_SLOTS];
#else
static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
#endif

static volatile async_tick_t _async_tick_counter;
#if ASYNC_DELAY_OPT_BITMASK
static volatile unsigned char _async_active_mask;   // bit n = slot n ACTIVE
#endif

// Uniform per-slot access (works for both layouts)
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
#define _AD_TARGET(i)  (_async_target[i])
#define _AD_DUR(i)     (_async_duration[i])
#define _AD_CB(i)      (_async_callback[i])
#define _AD_FLAGS(i)   (_async_flags[i])
#else
#define _AD_TARGET(i)  (_async_slots[i].target)
#define _AD_DUR(i)     (_async_slots[i].duration)
#define _AD_CB(i)      (_async_slots[i].callback)
#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define _AD_FLAGS(i)   (_async_slots[i].flags)
#else
#define _AD_FLAGS(i)   (_async_slots[i].state)
#endif
#endif
#if ASYNC_DELAY_OPT_MERGED_FLAGS
#define _AD_STATE(i)   ((unsigned char)(_AD_FLAGS(i) & 0x03))
#define _AD_REPEAT(i)  ((_AD_FLAGS(i) & ASYNC_FLAG_REPEAT) != 0)
#else
#define _AD_STATE(i)   (_AD_FLAGS(i))
#define _AD_REPEAT(i)  (_async_slots[i].repeat)
#endif

// Half the tick range - compile-time constant (was recomputed every tick).
#define _ASYNC_HALF_RANGE ((async_tick_t)(~((async_tick_t)0) >> 1))
```

Note: keep the existing header comment lines above this section. If
`ASYNC_DELAY_OPT_MERGED_FLAGS` is 0, `_AD_FLAGS` is the `state` byte and
`_AD_STATE`/`_AD_REPEAT` keep working for the tick code below. No LUT here — for
MAX_SLOTS>4 the tick uses a simple bit-skip scan (Step 6); a 256-byte
lowest-set-bit table is only worth adding if measurement (Step 9) shows it pays.

**Verify**: `grep -n "_async_active_mask\|_ASYNC_HALF_RANGE" async_delay.h` → all present.

### Step 3 — `async_delay_init`

```c
static void async_delay_init(void)
{
    unsigned char i;
    _async_tick_counter = 0;
#if ASYNC_DELAY_OPT_BITMASK
    _async_active_mask = 0;
#endif
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        _AD_FLAGS(i) = ASYNC_SLOT_FREE;
}
```

**Verify**: logic only (no compiler); grep that `_async_active_mask = 0` is present.

### Step 4 — `_async_delay_start_common`

```c
static unsigned char _async_delay_start_common(async_tick_t duration,
                                               async_delay_cb_t callback,
                                               unsigned char repeat)
{
    unsigned char i;
    async_tick_t now;

    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
#if ASYNC_DELAY_OPT_BITMASK
        if ((_async_active_mask & (1 << i)) == 0)   // bit clear => slot FREE
#else
        if (_AD_STATE(i) == ASYNC_SLOT_FREE)
#endif
        {
#if ASYNC_DELAY_TIMER_BITS >= 16
            // AVR is 8-bit: reading a 16/32-bit volatile var is NOT atomic.
            // Disable interrupts briefly to get a consistent snapshot.
            #asm("cli")
            now = _async_tick_counter;
            #asm("sei")
#else
            now = _async_tick_counter;
#endif
            _AD_DUR(i)    = duration;
            _AD_TARGET(i) = now + duration;
            _AD_CB(i)     = callback;
#if ASYNC_DELAY_OPT_MERGED_FLAGS
            // state + repeat in one byte
            _AD_FLAGS(i)  = (unsigned char)(ASYNC_SLOT_ACTIVE |
                                            (repeat ? ASYNC_FLAG_REPEAT : 0));
#else
            // separate state byte (and repeat, only in the struct layout)
            _AD_FLAGS(i)  = ASYNC_SLOT_ACTIVE;
#if ASYNC_DELAY_OPT_SPLIT_ARRAYS
            _async_repeat[i] = repeat;      // split layout: own repeat array
#else
            _async_slots[i].repeat = repeat;  // struct layout
#endif
#endif
#if ASYNC_DELAY_OPT_BITMASK
            // single-byte write, done LAST so the slot is fully set before it
            // becomes visible to the tick's mask walk. ISR never writes the
            // mask, so no cli is needed for this byte itself.
            _async_active_mask |= (1 << i);
#endif
            return i;
        }
    }
    return ASYNC_DELAY_NO_SLOT;
}
```

Note: the split-array layout (`SPLIT_ARRAYS=1, MERGED_FLAGS=0`) needs a
`_async_repeat[]` array too — add it in Step 2's data section if you choose that
combination. **Simplest valid combination (recommended)**: `BITMASK=1,
MERGED_FLAGS=1, SPLIT_ARRAYS=0` — with merged-flags on, repeat lives inside
`_AD_FLAGS` and no extra array exists. The `cli`/`sei` around `now =
_async_tick_counter` (16/32-bit only) already serializes start-vs-ISR; the mask
write sits inside that window for 16/32-bit and is a plain single-byte write for
8-bit. The mask write is placed AFTER all other writes so the slot is fully set
before it becomes visible to the tick's mask walk.

**Verify** (manual trace): a FREE slot is found by its cleared mask bit; the slot's
fields are set; the bit is set last. An active slot never matches. No other code
path writes `_AD_FLAGS` before the bit is set.

### Step 5 — `async_delay_elapsed` and `async_delay_cancel`

```c
static unsigned char async_delay_elapsed(unsigned char slot_id)
{
    if (slot_id >= ASYNC_DELAY_MAX_SLOTS)
        return 0;

#if ASYNC_DELAY_OPT_BITMASK
    if (_async_active_mask & (1 << slot_id))      // still running -> not elapsed
        return 0;
#endif
    if (_AD_STATE(slot_id) == ASYNC_SLOT_EXPIRED)
    {
        _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
        return 1;
    }
    return 0;
}

static void async_delay_cancel(unsigned char slot_id)
{
    if (slot_id < ASYNC_DELAY_MAX_SLOTS)
    {
#if ASYNC_DELAY_OPT_BITMASK
        _async_active_mask &= (unsigned char)~(1 << slot_id);
#endif
        _AD_FLAGS(slot_id) = ASYNC_SLOT_FREE;
    }
}
```

**Verify**: `grep -n "_async_active_mask &=" async_delay.h` → 1 match (cancel);
`grep -n "_async_active_mask & (1" async_delay.h` → 1 match (elapsed).

### Step 6 — `async_delay_tick` (the hot path)

Split the expired-slot processing into a helper (keeps the bitmask walk and the
legacy scan both thin; call/ret is paid only for slots that actually expire,
which is rare compared to the scan):

```c
// ISR-context: process slot i that has been confirmed expired.
static void _async_delay_expire_slot(unsigned char i)
{
#if ASYNC_DELAY_CALLBACK_RESCHEDULE
    if (_AD_REPEAT(i) && _AD_CB(i) != (void *)0)
    {
        // Periodic: re-arm FIRST (phase-locked target += duration), callback
        // runs with the slot still ACTIVE -> a self-reschedule lands elsewhere.
        _AD_TARGET(i) += _AD_DUR(i);
        _AD_CB(i)(i);
    }
    else
    {
        if (_AD_CB(i) != (void *)0)
        {
            // One-shot: free BEFORE the callback so self-reschedule reuses it.
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
            _AD_CB(i)(i);
        }
        else
        {
            // Polling mode: EXPIRED, cleared from mask; elapsed() polls state.
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_EXPIRED;
        }
    }
#else
    // Legacy ordering (flag 001 = 0): callback fires while slot still ACTIVE.
    if (_AD_REPEAT(i) && _AD_CB(i) != (void *)0)
    {
        _AD_CB(i)(i);
        _AD_TARGET(i) += _AD_DUR(i);
    }
    else
    {
        if (_AD_CB(i) != (void *)0)
        {
            _AD_CB(i)(i);
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_FREE;
        }
        else
        {
#if ASYNC_DELAY_OPT_BITMASK
            _async_active_mask &= (unsigned char)~(1 << i);
#endif
            _AD_FLAGS(i) = ASYNC_SLOT_EXPIRED;
        }
    }
#endif
}

static void async_delay_tick(void)
{
#if ASYNC_DELAY_OPT_BITMASK
    unsigned char m;
    _async_tick_counter++;              // MUST increment even when idle
    m = _async_active_mask;
    if (m == 0)
        return;                         // idle tick: ~12-16 cycles total

#if ASYNC_DELAY_MAX_SLOTS > 4
    // 5..8 slots: scan at most 8 bits, skipping cleared ones. Idle still returns
    // above; this only runs when at least one slot is ACTIVE.
    {
        unsigned char i;
        for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
        {
            if (m & (1 << i))
            {
                if ((async_tick_t)(_async_tick_counter - _AD_TARGET(i)) < _ASYNC_HALF_RANGE)
                    _async_delay_expire_slot(i);
            }
        }
    }
#else
    // 1..4 slots: direct low-bit chain — no loop, no table.
    while (m)
    {
        unsigned char i;
        if (m & 1)
            i = 0;
        else if (m & 2)
            i = 1;
        else if (m & 4)
            i = 2;
        else
            i = 3;
        m &= (unsigned char)~(1 << i);
        if ((async_tick_t)(_async_tick_counter - _AD_TARGET(i)) < _ASYNC_HALF_RANGE)
            _async_delay_expire_slot(i);
    }
#endif
#else
    // Legacy: full linear scan (behavior identical to before when all flags 0).
    unsigned char i;
    _async_tick_counter++;
    for (i = 0; i < ASYNC_DELAY_MAX_SLOTS; i++)
    {
        if (_AD_STATE(i) == ASYNC_SLOT_ACTIVE)
        {
            if ((async_tick_t)(_async_tick_counter - _AD_TARGET(i)) < _ASYNC_HALF_RANGE)
                _async_delay_expire_slot(i);
        }
    }
#endif
}
```

Notes for the executor:
- The `_async_tick_counter++` MUST be the first statement (before the idle
  early-return), otherwise the counter stops advancing when no slots are active
  and all delays drift. This is a subtle correctness trap — do not reorder it.
- In the `>4` branch a plain skip-scan of 8 bits is used instead of a LUT because
  a low-nibble LUT needs offset handling that is easy to get wrong; idle is still
  O(1) via the mask check. A full 256-byte `_async_lowbit8[256]` (value = lowest
  set bit, 0xFF if 0) is a valid optional upgrade if Flash budget allows.
- The `_async_delay_expire_slot` helper is `static`, ISR-context, keeps callbacks
  inside — same constraint as before.

**Verify** (manual trace — no compiler):
- Idle: mask==0 → return after `counter++`. Counter keeps counting. ✅
- 1 one-shot active: counter++, m=1, i=0, expired test, helper frees slot + clears
  mask bit before callback. Self-reschedule from callback sees the slot FREE. ✅
- 1 periodic active: helper re-arms `target += duration`, mask bit stays set,
  callback runs. Slot is not double-processed this pass (`m` already had bit
  cleared locally). ✅
- All 4 active, two expire: only the two expired slots call the helper; each local
  `m` clear prevents re-visiting. ✅
- Wrap: `_ASYNC_HALF_RANGE` and `(now - target) < half` are bit-identical to the
  original expression. ✅

### Step 7 — Update the top-of-file docs

In the header's doc comment "Accuracy & limits" and "Error Handling" sections,
add one line each (grep-able, so the executor can verify):

```
//   - Idle tick cost is ~1/5 of a full scan when ASYNC_DELAY_OPT_BITMASK=1.
//   - Optimization flags: ASYNC_DELAY_OPT_BITMASK (1), ASYNC_DELAY_OPT_MERGED_FLAGS (1),
//     ASYNC_DELAY_OPT_SPLIT_ARRAYS (0). See plans/003 for the full design.
```

**Verify**: `grep -n "ASYNC_DELAY_OPT_BITMASK" async_delay.h` → present in flags + docs.

### Step 8 — Manual build gate (external — user action)

In your final report, tell the user: **"Copy `async_delay.h` to
`G:\Kaveh\CodeVsion\inc\async_delay.h` and rebuild `async_delay_test` in
CodeVisionAVR."** Confirm the build reports **No errors** (it may show warnings —
the user accepts warnings; errors are the gate). This is the real compile gate —
there is no toolchain in the execution environment.

### Step 9 — Report the percentage estimates

After the user reports the build result, produce the required speed comparison
(CLAUDE.md §9.2). Best source: `Debug/List/async_delay_test.asm` (CodeVisionAVR
generates it) — count instructions in `async_delay_tick` before/after, or time the
idle loop in the Proteus simulation. Report a **real percentage estimate**, not a
vague "faster": e.g. "idle tick went from ~90 to ~14 cycles → ~84% less CPU on the
tick path; at 1 kHz tick that is ~1.5% → ~0.2% of CPU total."

## Test plan

No automated test harness exists for this repo (CodeVisionAVR IDE + hardware/Proteus
testing). Instead:

- **Manual logic trace** covering: idle tick (counter still advances), 1 one-shot,
  1 periodic (re-arm + mask stays set), 2 concurrent slots, self-reschedule from a
  callback (slot freed + mask bit cleared before the callback), polling expiry
  (mask bit cleared, state=EXPIRED, `elapsed()` frees).
- **Regression**: the Proteus simulation `Simulation/AsyncTestSimulaion.pdsprj`
  covers LED0 blink (500ms periodic), LED1 blink (750ms polling), cancel test,
  slot-overflow (5th start → 0xFF), and loop-not-blocked. All must behave
  identically to before.
- **Config matrix** (compile each to prove flags don't break): `{0,0,0}` (pure
  legacy), `{1,0,0}` (bitmask + struct + split state), `{1,1,0}` (recommended),
  `{1,1,1}` (all on). The user builds; the executor checks the user's report.

## Done criteria

ALL must hold:

- [ ] `grep -n "ASYNC_DELAY_OPT_" async_delay.h` → 3 `#ifndef`/`#define` pairs + constraint `#if`s
- [ ] `grep -n "_async_active_mask" async_delay.h` → in tick, start, cancel, elapsed
- [ ] `grep -n "_ASYNC_HALF_RANGE" async_delay.h` → defined once, used in tick
- [ ] `grep -n "_async_delay_expire_slot" async_delay.h` → defined once, called from tick
- [ ] Legacy path intact: `grep -n "for (i = 0; i < ASYNC_DELAY_MAX_SLOTS" async_delay.h` → ≥1 (the `#else` tick loop)
- [ ] `git status --short` → only `async_delay.h` (plus `plans/`) modified
- [ ] User reports CodeVisionAVR build: **No errors** (external gate)
- [ ] Percentage estimates reported in the final message (Step 9)
- [ ] `plans/README.md` status row for 003 updated to DONE

## STOP conditions

Stop and report back (do not improvise) if:

- `async_delay.h` line numbers/snippets in "Current state" don't match the live
  file (drift since `8e9fab4`).
- The recommended combination `BITMASK=1, MERGED_FLAGS=1, SPLIT_ARRAYS=0` fails to
  build or behaves differently in the Proteus regression — do NOT "fix" by
  reordering the counter increment or weakening the wrap-safe test.
- You find yourself wanting to also "improve" the wrap-safe comparison, the atomic
  16/32-bit read, or the phase-locked `target += duration` — all correctness-critical.
- `ASYNC_DELAY_MAX_SLOTS > 8` is requested with BITMASK on (the flag's constraint
  `#error` fires — that combination is unsupported by design).

## Maintenance notes

- The mask is a single byte; it becomes the concurrency contract: **main loop
  writes it, ISR reads a local copy once per tick.** A future host-side test
  harness should assert: mask bit set exactly when state==ACTIVE, cleared on
  cancel/expiry; idle tick still increments the counter.
- If someone later raises MAX_SLOTS above 8 they must either turn BITMASK off or
  widen the mask (unsigned int) — the constraint `#error` guards this.
- Reviewer focus: the ISR path. Confirm `counter++` is first (idle still counts),
  the mask write in start/cancel is a single byte, and `_async_delay_expire_slot`
  preserves the RESCHEDULE ordering (001) exactly.
- The recommended flag combo is `ASYNC_DELAY_OPT_BITMASK=1,
  ASYNC_DELAY_OPT_MERGED_FLAGS=1, ASYNC_DELAY_OPT_SPLIT_ARRAYS=0`. Only enable
  SPLIT_ARRAYS if measurement (Step 9) shows it pays off for the user's MAX_SLOTS.
