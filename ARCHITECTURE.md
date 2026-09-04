# ARCHITECTURE.md — async_delay (for the AI/Claude reading this repo)

Read this file before modifying anything in this project. It is written for an AI
(not humans) so you can reconstruct the design, invariants, and constraints without
re-reading the whole header. Where something is subtle or was a past bug, it says so.

---

## 1. What this project is

A **header-only**, non-blocking delay library for AVR (ATmega8/16/32) in
[async_delay.h](async_delay.h), compiled with CodeVisionAVR. Everything lives in
one `.h` file; there is no `.c`. A test project ([async_delay_test.c](async_delay_test.c))
runs on ATmega8 @ 8MHz.

Key design goal: **never block the main loop**. Delays count down inside a hardware
timer ISR; the app either gets a callback or polls.

---

## 2. File manifest

| File | Role |
|------|------|
| `async_delay.h` | The library. Only file you normally edit. ~920 lines (config-heavy; most of it is `#if` variants + comments). |
| `async_delay_test.c` | Test project: ATmega8 @ 8MHz, Timer2 CTC 1ms tick, LCD + LEDs. |
| `async_delay_guide.md` | Persian usage guide (timers, OCR tables, CodeWizard). Human-facing, lower priority for edits. |
| `async_delay_test.prj` | CodeVisionAVR project file. |
| `ARCHITECTURE.md` | This file. |
| `CLAUDE.md` | Project rules (section 9). |

---

## 3. Configuration contract (defined by user BEFORE `#include`)

| Macro | Default | Meaning | Validation |
|-------|---------|---------|------------|
| `ASYNC_DELAY_TICK_HZ` | **none — required** | tick rate in Hz (e.g. 1000 = 1ms tick) | `#error` if undefined or == 0 |
| `ASYNC_DELAY_TIMER_BITS` | `16` | tick counter width: 8 / 16 / 32 | `#error` if not one of these |
| `ASYNC_DELAY_MAX_SLOTS` | `4` | max concurrent delays | `#error` if == 0 or > 254 (0xFF reserved); > 8 also `#error`s with any mask flag on |
| `ASYNC_DELAY_CALLBACK_RESCHEDULE` | `1` | slot made non-ACTIVE before its callback runs (plan 001) | — |
| `ASYNC_DELAY_OPT_BITMASK` | `1` | tick visits only ACTIVE slots (plan 003) | `#error` if MAX_SLOTS > 8 |
| `ASYNC_DELAY_OPT_MERGED_FLAGS` | `1` | state+repeat in one byte (plan 003) | — |
| `ASYNC_DELAY_OPT_SPLIT_ARRAYS` | `0` | parallel arrays instead of a struct (plan 003) | requires BITMASK |
| `ASYNC_DELAY_FIX_USED_MASK` | `1` | **correctness**: separate ALLOCATED mask (plan 004 §4.1) | requires BITMASK |
| `ASYNC_DELAY_FIX_ATOMIC_MASK` | `1` | **correctness**: SREG-preserving critical sections (plan 004 §4.2) | required by NEXT_TARGET when TIMER_BITS ≥ 16 |
| `ASYNC_DELAY_OPT_UNROLL_TICK` | `1` | compile-time slot indices in the tick (plan 004 §7.1) | requires BITMASK |
| `ASYNC_DELAY_OPT_NEXT_TARGET` | `1` | O(1) earliest-target gate (plan 004 §7.2) | requires BITMASK |
| `ASYNC_DELAY_DEFERRED_CALLBACKS` | `0` | **changes behavior**: callbacks run from `async_delay_poll()` in main context (plan 004 §7.3) | requires BITMASK, MAX_SLOTS ≤ 8 |

Flag effect: `TIMER_BITS` selects the type of `async_tick_t`
(`unsigned char` / `unsigned int` / `unsigned long`).

---

## 4. Data structures (exact)

```c
typedef void (*async_delay_cb_t)(unsigned char slot_id);

typedef struct {                // ASYNC_DELAY_OPT_MERGED_FLAGS == 1 layout
    async_tick_t     target;    // tick value when this delay expires
    async_tick_t     duration;  // stored for periodic re-arm
    async_delay_cb_t callback;  // NULL = polling-only mode
    unsigned char    flags;     // [1:0]=state (FREE/ACTIVE/EXPIRED), [2]=repeat
} _async_slot_t;                // (MERGED_FLAGS==0 splits this into state+repeat)

static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
static volatile async_tick_t  _async_tick_counter;
static volatile unsigned char _async_active_mask;    // OPT_BITMASK: bit n RUNNING
static volatile unsigned char _async_used_mask;      // FIX_USED_MASK: bit n ALLOCATED
static volatile async_tick_t  _async_next_target;    // OPT_NEXT_TARGET: earliest target
static volatile unsigned char _async_pending_mask;   // DEFERRED_CALLBACKS: cb owed
```

Access goes through `_AD_TARGET/_AD_DUR/_AD_CB/_AD_FLAGS/_AD_STATE/_AD_REPEAT`
so the struct and split-array layouts share one body of code.

Slot states: `ASYNC_SLOT_FREE (0)`, `ASYNC_SLOT_ACTIVE (1)`, `ASYNC_SLOT_EXPIRED (2)`.
Error code: `ASYNC_DELAY_NO_SLOT (0xFF)`.

RAM at the default flags, MAX_SLOTS=4, TIMER_BITS=16: 28 (slots) + 2 (counter)
+ 1 (active) + 1 (used) + 2 (next_target) = **34 bytes**.

All functions are `static` (header-only, avoids duplicate symbols); data is inside
`#pragma used+` / `#pragma used-` so CodeVisionAVR keeps/emits it.

---

## 5. Public API — exact behavior

| Function | Behavior |
|----------|----------|
| `async_delay_init()` | Zeroes the counter and every mask, sets all slots FREE. Call ONCE before `sei`. |
| `async_delay_start(duration, cb)` | One-shot. Returns slot_id or `0xFF` if no free slot. `duration=0` → expires next tick. |
| `async_delay_start_periodic(duration, cb)` | Periodic, auto re-arm. `cb == NULL` is pointless (degrades to polling one-shot). |
| `async_delay_elapsed(slot_id)` | Polling check. Returns 1 if EXPIRED, frees slot **and its allocation**, else 0. Invalid id → safe 0. |
| `async_delay_cancel(slot_id)` | Frees the slot, clears every mask bit, recomputes the next target. Invalid id → no-op. |
| `async_delay_tick()` | **ISR-only.** Increments counter, then O(1)-gates on the earliest target before touching any slot. Must run at exactly `TICK_HZ`. |
| `async_delay_poll()` | Only exists when `ASYNC_DELAY_DEFERRED_CALLBACKS=1`. Drains deferred callbacks in MAIN context. Call it from the main loop or callbacks never fire. |

Slot lifecycle:
```
FREE ─ start() ─► ACTIVE ─ tick ─► EXPIRED ─ elapsed() ─► FREE
  ▲                │    └──────── cancel() ──────────────► FREE
  └────────────────┴──────── cancel() ───────────────────► FREE
```
Periodic slot stays ACTIVE after firing (re-arms). One-shot WITH callback goes
ACTIVE → FREE directly. One-shot WITHOUT callback goes ACTIVE → EXPIRED (waits for poll).

An EXPIRED slot is **still allocated** — `start()` will not reuse it until
`elapsed()` releases it (see §6.5). Forgetting `elapsed()` leaks the slot; that
is the deliberate trade for never silently losing a caller's timer.

---

## 6. Key algorithms (copy these into working memory)

### 6.1 Wrap-safe expiry check — `_ASYNC_REACHED` macro
```c
#define _ASYNC_HALF_RANGE ((async_tick_t)(~((async_tick_t)0) >> 1))   // e.g. 32767
#define _ASYNC_REACHED(now, t) \
    ((async_tick_t)((async_tick_t)(now) - (async_tick_t)(t)) < _ASYNC_HALF_RANGE)
```
Read it as "`t` has been reached at `now`", equivalently "`t` is not later than
`now`". It lives in exactly one place so the tick, the next-target minimum and
`start()` cannot drift apart. Bit-identical to the old inline
`(counter - target) < half`.

Invariant: correct across counter wrap **only if** delay < half the range
(max reliable delay = 128 / 32767 / ~2^31 ticks for 8/16/32-bit). Longer = ambiguous.
This same invariant is what makes "earliest target" well-defined in §6.4.

### 6.2 Periodic re-arm is phase-locked — in `async_delay_tick()`
```c
_async_slots[i].callback(i);
_async_slots[i].target += _async_slots[i].duration;   // NOT now + duration
```
Adding to the old `target` (not current `now`) means a late tick is compensated on
subsequent cycles — average period stays exact.

### 6.3 Atomic multi-byte reads + SREG-preserving critical sections
AVR is 8-bit; reading a volatile 16/32-bit var is NOT atomic (ISR can bump the counter
between byte loads → corrupted value). This was a real past bug (commit `71e6924`).
Don't "simplify" it away.

Plan 004 widened that window to cover every shared byte, and replaced the bare
`sei` with an SREG restore:
```c
#define _ASYNC_CRIT_DECL     unsigned char _ad_sreg;
#define _ASYNC_SAVE_SREG()   _ad_sreg = SREG
#define _ASYNC_REST_SREG()   SREG = _ad_sreg
...
    _ASYNC_SAVE_SREG();
    #asm("cli")
    /* counter read + mask RMWs + _async_next_target, all in one window */
    _ASYNC_REST_SREG();
```
Two reasons this shape and not a macro-wrapped `cli`:
1. A bare `sei` would turn interrupts **on** even if the caller had them off —
   `start()` called before `sei` in `main()`, or from inside the app's own
   critical section. Restoring SREG preserves whatever the caller had.
2. `#asm(...)` does not reliably survive CodeVisionAVR macro expansion, so the
   `cli` is written literally at each call site.

`SREG` comes from the device header (`sfrb SREG=0x3f;` in `mega8.h`), which the
user includes before this one.

### 6.4 Earliest-target gate — O(1) tick (`ASYNC_DELAY_OPT_NEXT_TARGET`)
`_async_next_target` caches the earliest `target` among ACTIVE slots. The tick
increments the counter, checks the mask, then:
```c
if (!_ASYNC_REACHED(_ad_now, _async_next_target))
    return;              /* nothing due: one compare, regardless of slot count */
```
Recomputed by `_async_recompute_next()` (O(N)) **only** when a slot actually
expired, and inside `cancel()`. `start()` needs just one compare.

**Safety direction (do not get this backwards)**: the cached value may be
*earlier* than the true minimum — that only costs one wasted slot walk. It must
**never** be later, or a slot fires late. Hence: `start()` takes the min
unconditionally, `cancel()` always does a full recompute (the cancelled slot may
have been the minimum). Never "optimize" the cancel recompute away.

While `_async_active_mask == 0` the cached value is meaningless; the mask test
runs first, so nothing reads it. There is no sentinel value — every tick value
is a legal target.

### 6.5 Used vs active mask (`ASYNC_DELAY_FIX_USED_MASK`)
Three slot states, one bit — the ACTIVE mask alone cannot tell FREE from
EXPIRED, so pre-004 `start()` handed out polling slots whose owner had not
called `elapsed()` yet, silently destroying a live timer. Two masks fix it:

| Event | `_async_used_mask` (ALLOCATED) | `_async_active_mask` (RUNNING) |
|-------|-------------------------------|-------------------------------|
| `start()` | set bit | set bit |
| tick: one-shot **with** callback fires | clear bit | clear bit |
| tick: polling (no callback) expires | **keep set** | clear bit |
| tick: periodic re-arms | keep set | keep set |
| `elapsed()` returns 1 | clear bit | (already clear) |
| `cancel()` | clear bit | clear bit |

`start()` allocates from the used mask; the tick walks the active mask.
Invariant: **`_async_active_mask & ~_async_used_mask == 0`** — never running
without being allocated.

Also in the ISR path: when a one-shot slot is freed, its callback pointer is
copied to a local, nulled in the slot, then called. A freed slot must not hold a
live `ICALL` target.

### 6.6 Unrolled tick (`ASYNC_DELAY_OPT_UNROLL_TICK`)
`_AD_TICK_SLOT(n)` is invoked once per slot with a **literal** `n`. That is the
whole optimization: with a runtime index, CodeVisionAVR emits `RCALL __LSLW12`
(a bit-at-a-time shift loop) for `1 << i`, a `MUL` for the `i * sizeof(slot)`
offset, and `RCALL __GETW1P` to load `.target` — roughly 72 cycles per slot.
With a literal, all three fold into `SBRS` + `LDS` + absolute addressing (~13).

If anyone later adds a runtime-indexed caller, the entire win silently
disappears. There is deliberately **no** lowest-set-bit LUT and no hand-written
ASM here — see plan 004 §7.4 for why both were rejected.

---

## 7. ISR / timing contract

- A hardware timer (e.g. Timer2 CTC on ATmega8, OCR2=124 @ 8MHz/prescaler-64 → 1ms)
  interrupts at `1/TICK_HZ` and calls only `async_delay_tick()`.
- `async_delay_tick()` runs in ISR context → must stay short; **callbacks run in ISR
  context** → app callbacks must only flip flags/pins (never `delay_ms`, never LCD).
  The exception is `ASYNC_DELAY_DEFERRED_CALLBACKS=1`, where callbacks run from
  `async_delay_poll()` in main context and may do anything.
- `_async_tick_counter++` MUST be the first statement in the tick, before any
  early return. Otherwise the counter stops advancing on idle ticks and every
  delay drifts. Subtle trap — do not reorder.
- Concurrency contract: the ISR (tick) writes `_async_active_mask`,
  `_async_pending_mask`, `_async_next_target` and the slot fields. Main
  (`start`/`cancel`/`elapsed`/`poll`) writes all of them too, but only inside a
  critical section (§6.3). Any new API touching them must take the same section.
- Library logic error is at most ±1 tick. Real accuracy is set by the clock source
  (external crystal ~±0.005%, internal RC ~±1–3%).

---

## 8. Test project coverage (`async_delay_test.c`)

| Test | Mode | Pass criterion |
|------|------|----------------|
| LED0 blink 500ms | periodic + callback | callback fires repeatedly from ISR |
| LED1 blink 750ms | polling + `elapsed()` | poll in loop, slot freed + re-started |
| Cancel 2000ms delay early | cancel | callback never fires |
| LCD refresh / 200ms | polling | real-time pacing, loop not blocked |
| Slot overflow (5th start) | all 4 slots busy | returns `0xFF` |
| `loop_count` (unsigned long) | main loop | proves loop is never blocked |

There is no automated test harness (CodeVisionAVR IDE + hardware/Proteus only).
Plan 004 added a throwaway Python model of the algorithm to run its 12 logic
traces (mask transitions, wrap behavior, next-target invariant); it was deleted
after use. If you change the tick, the masks, or `_ASYNC_REACHED`, rebuild that
model rather than trusting a read-through — the traces caught the polarity
questions that eyeballing does not.

---

## 9. Project rules (from CLAUDE.md — follow these)

1. **The user compiles manually.** After your edits they report errors themselves —
   you do NOT have a working build command. `G:\Kaveh\CodeVsion\BIN\cvavrcl.exe`
   exists but demands a license, so it is not usable here. State verification
   limits honestly.
2. **Speed is the top priority.** You MAY trade more RAM/Flash if the change is
   genuinely worth it. After any optimization/refactor, report a **real percentage**
   estimate of the speed change vs. before.
3. **Always read `\ARCHITECTURE.md`** (this file) before changes.
4. **Every new feature gets a flag** (config macro) so the user can enable/disable it.
5. Prefer **clever/smart algorithms** — they're encouraged.
6. Main editable file is `async_delay.h`.

## 10. Editing conventions

- Keep files small; functions focused.
- Match existing style: CodeVisionAVR C, 4-space indent, `//` comments,
  `(void *)0` for NULL, `#asm("cli")` inline asm. No `u`/`U` literal suffixes —
  the rest of the codebase and the CVAVR headers do not use them.
- Declare all locals at the top of a function: CodeVisionAVR follows C89 block
  rules, so a declaration after a statement is an error. When a local only
  exists under some flag combination, wrap the declaration in the same `#if`.
- Surgical changes only; don't refactor unrelated lines.
- Don't remove the atomic-read, wrap-safe, phase-locked-re-arm, or
  counter-increment-first logic (section 6) — all correctness-critical.
- The user's mirror copy at `G:\Kaveh\CodeVsion\inc\async_delay.h` is what the
  project actually compiles against. Never edit it; tell the user to copy.
