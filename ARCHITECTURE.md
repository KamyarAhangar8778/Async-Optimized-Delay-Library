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
| `async_delay.h` | The library. Only file you normally edit. ~285 lines. |
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
| `ASYNC_DELAY_MAX_SLOTS` | `4` | max concurrent delays | `#error` if == 0 or > 254 (0xFF reserved) |

Flag effect: `TIMER_BITS` selects the type of `async_tick_t`
(`unsigned char` / `unsigned int` / `unsigned long`).

---

## 4. Data structures (exact)

```c
typedef void (*async_delay_cb_t)(unsigned char slot_id);

typedef struct {
    async_tick_t     target;    // tick value when this delay expires
    async_tick_t     duration;  // stored for periodic re-arm
    async_delay_cb_t callback;  // NULL = polling-only mode
    unsigned char    state;     // FREE / ACTIVE / EXPIRED
    unsigned char    repeat;    // 1 = periodic, 0 = one-shot
} _async_slot_t;

static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
static volatile async_tick_t _async_tick_counter;
```

Slot states: `ASYNC_SLOT_FREE (0)`, `ASYNC_SLOT_ACTIVE (1)`, `ASYNC_SLOT_EXPIRED (2)`.
Error code: `ASYNC_DELAY_NO_SLOT (0xFF)`.

All functions are `static` (header-only, avoids duplicate symbols); data is inside
`#pragma used+` / `#pragma used-` so CodeVisionAVR keeps/emits it.

---

## 5. Public API — exact behavior

| Function | Behavior |
|----------|----------|
| `async_delay_init()` | Zeroes `_async_tick_counter`, sets all slots FREE. Call ONCE before `sei`. |
| `async_delay_start(duration, cb)` | One-shot. Returns slot_id or `0xFF` if no free slot. `duration=0` → expires next tick. |
| `async_delay_start_periodic(duration, cb)` | Periodic, auto re-arm. `cb == NULL` is pointless (degrades to polling one-shot). |
| `async_delay_elapsed(slot_id)` | Polling check. Returns 1 if EXPIRED, frees slot, else 0. Invalid id → safe 0. |
| `async_delay_cancel(slot_id)` | Frees the slot. Invalid id → no-op. |
| `async_delay_tick()` | **ISR-only.** Increments counter, scans ACTIVE slots, fires callbacks. Must run at exactly `TICK_HZ`. |

Slot lifecycle:
```
FREE ─ start() ─► ACTIVE ─ tick ─► EXPIRED ─ elapsed() ─► FREE
  ▲                │    └──────── cancel() ──────────────► FREE
  └────────────────┴──────── cancel() ───────────────────► FREE
```
Periodic slot stays ACTIVE after firing (re-arms). One-shot WITH callback goes
ACTIVE → FREE directly. One-shot WITHOUT callback goes ACTIVE → EXPIRED (waits for poll).

---

## 6. Key algorithms (copy these into working memory)

### 6.1 Wrap-safe expiry check — in `async_delay_tick()`
```c
half = (async_tick_t)(~((async_tick_t)0) >> 1);   // half the range, e.g. 32767
if ((async_tick_t)(_async_tick_counter - _async_slots[i].target) < half)
    /* expired */;
```
Invariant: correct across counter wrap **only if** delay < half the range
(max reliable delay = 128 / 32767 / ~2^31 ticks for 8/16/32-bit). Longer = ambiguous.

### 6.2 Periodic re-arm is phase-locked — in `async_delay_tick()`
```c
_async_slots[i].callback(i);
_async_slots[i].target += _async_slots[i].duration;   // NOT now + duration
```
Adding to the old `target` (not current `now`) means a late tick is compensated on
subsequent cycles — average period stays exact.

### 6.3 Atomic 16/32-bit counter read — in `_async_delay_start_common()`
AVR is 8-bit; reading a volatile 16/32-bit var is NOT atomic (ISR can bump the counter
between byte loads → corrupted value). So:
```c
#if ASYNC_DELAY_TIMER_BITS >= 16
    #asm("cli") ; now = _async_tick_counter ; #asm("sei")
#else
    now = _async_tick_counter;      // 8-bit read is atomic
#endif
```
This was a real past bug (see commit `71e6924`). Don't "simplify" it away.

---

## 7. ISR / timing contract

- A hardware timer (e.g. Timer2 CTC on ATmega8, OCR2=124 @ 8MHz/prescaler-64 → 1ms)
  interrupts at `1/TICK_HZ` and calls only `async_delay_tick()`.
- `async_delay_tick()` runs in ISR context → must stay short; **callbacks run in ISR
  context** → app callbacks must only flip flags/pins (never `delay_ms`, never LCD).
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

---

## 9. Project rules (from CLAUDE.md — follow these)

1. **The user compiles manually.** After your edits they report errors themselves —
   you do NOT have a working build command. State verification limits honestly.
2. **Speed is the top priority.** You MAY trade more RAM/Flash if the change is
   genuinely worth it. After any optimization/refactor, report a **real percentage**
   estimate of the speed change vs. before.
3. **Always read `\ARCHITECTURE.md`** (this file) before changes.
4. **Every new feature gets a flag** (config macro) so the user can enable/disable it.
5. Prefer **clever/smart algorithms** — they're encouraged.
6. Main editable file is `async_delay.h`.

## 10. Editing conventions

- Keep files small; functions focused.
- Match existing style: CodeVisionAVR C, tabs, `#pragma used+`, `#asm("cli")` inline asm.
- Surgical changes only; don't refactor unrelated lines.
- Don't remove the atomic-read or wrap-safe logic (section 6) — they are correctness-critical.
