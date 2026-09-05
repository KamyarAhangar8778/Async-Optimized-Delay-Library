# async_delay — Non-blocking delay library for AVR

A **header-only** library that lets the MCU run several delays concurrently without locking up.
Compatible with **ATmega8/16/32** and the **CodeVisionAVR** compiler (not ported to AVR-GCC/Arduino — see "When NOT to use it").

> This file is the **usage contract** — for humans and for an **AI Agent Coder** alike.
> If you are going to use this library in a completely separate project, **reading just this file is enough**:
> setup, all functions, all flags, limits, pitfalls, and a complete minimal program are all here.
> For internal architecture and technical decisions see `ARCHITECTURE.md` — you do not need it just to *use* the library.

---

## 🎯 What is this library for?

Two ways to look at time on a MCU:

| | `delay_ms(500)` (blocking) | `async_delay` (non-blocking) |
|---|---|---|
| The MCU during those 500ms | **Fully locked** — does nothing | **Free** — does everything else |
| Your work | Before the delay, or after it | **At the very moment** the time expires |
| Several delays at once | Impossible | Yes, up to `ASYNC_DELAY_MAX_SLOTS` |
| Good for | One-off init, small values | Blinking, sensor reads, display refresh, state machines |

**In plain language:** you tell the MCU "do X after 500ms" and return to the main loop immediately.

**How it works (one sentence):** a hardware timer interrupts every `1/TICK_HZ` seconds; inside the
ISR the library increments a counter, checks which delays have expired, and for each one either runs a callback
or sets a flag for you to poll in the main loop.

---

## ✅ When to use it?

- **Blinking / timers** — several LEDs or actions with different periods, concurrently.
- **Regular sensor polling** — e.g. read the sensor every 100ms (but do not do heavy work like LCD printing inside the ISR).
- **Display refresh** — update the screen every 200ms without blocking the loop.
- **State machines / staged sequences** — schedule the next step with `async_delay_start` (self-rescheduling is supported).
- **Key debounce / timeouts** — e.g. "if no key pressed for 3 s" or "if UART does not answer within 100ms".
- **Giving the main loop "free time"** — the loop stays available for long work (math, UART, LCD).

## ❌ When NOT to use it?

- When you need **sub-microsecond-precise timing** (library accuracy is ±1 tick, e.g. ±1ms).
- When you want to **stop in the middle of an operation** (e.g. the 1-Wire protocol with very tight timing) — this library only announces "expiry", it does not preempt your code mid-operation.
- When you need a **delay longer than half the counter range** (see the "Maximum reliable delay" table).
- When you just want **a single short simple delay** at startup — `delay_ms` is simpler (e.g. a boot pause).
- When you want **hardware PWM or pulse generation** — use the timer's own PWM/CTC unit, not this library.
- When your compiler is **AVR-GCC, Arduino or ARM** — the `interrupt [...]` syntax, `#asm` and the `SREG` definition belong to CodeVisionAVR; it will not work without a port.

---

## 📥 Installing in a new project — 2 files

1. Copy `async_delay.h` and this `README.md` into the project (put the header next to the other headers or on the compiler include path).
2. Include it in **only one** `.c` file (reason: pitfall 6). Multi-file project? See pattern 7.
3. Continue with the next section (7 steps).

---

## 🧱 Setup — 7 fixed steps

```
1. Include the MCU header (e.g. #include <mega8.h>) — BEFORE the library!
2. Define the config flags BEFORE #including the library (at minimum ASYNC_DELAY_TICK_HZ)
3. Set up a hardware timer to interrupt every 1/TICK_HZ seconds; inside the ISR call only async_delay_tick()
4. In main(), once → async_delay_init()   (before any start and before sei)
5. After all inits → #asm("sei")
6. (Only if ASYNC_DELAY_DEFERRED_CALLBACKS=1) in the main loop → async_delay_poll()
7. Use async_delay_start / _periodic / _elapsed / _cancel
```

**Include order — important:**

```c
#include <mega8.h>              // 1) MCU header — must come before the library

#define ASYNC_DELAY_TICK_HZ     1000   // 2) config — before the library include
#include <async_delay.h>               // 3)
```

> ⚠️ Why MCU header first? The library uses `SREG` for its own critical sections, and `SREG` is defined in the MCU header.
> If it is not included first, compilation fails with `SREG undefined`.

The complete minimal program (whole app) is in "Usage patterns" → **pattern 0**.

---

## ⚙️ Configuration — define these BEFORE `#include`

Three main flags (one is mandatory):

```c
#define ASYNC_DELAY_TICK_HZ     1000      // Mandatory! Tick rate. 1000 = 1ms per tick
#define ASYNC_DELAY_TIMER_BITS  16        // Counter width: 8 / 16 / 32   (default 16)
#define ASYNC_DELAY_MAX_SLOTS   4         // Max concurrent delays        (default 4)

#include <async_delay.h>
```

| Flag | Default | Mandatory? | Meaning |
|------|---------|------------|---------|
| `ASYNC_DELAY_TICK_HZ` | — | ✅ Yes | Tick rate; e.g. `1000` = 1ms per tick |
| `ASYNC_DELAY_TIMER_BITS` | `16` | No | Counter width; selects the `async_tick_t` type (`unsigned char/int/long`). Wider = longer delays |
| `ASYNC_DELAY_MAX_SLOTS` | `4` | No | Max concurrent delays. **Real maximum is 8** (masks are 8-bit) |

### All other flags — complete reference (defaults are correct, do not touch without a reason)

| Flag | Default | Group | One-line meaning |
|------|---------|-------|------------------|
| `ASYNC_DELAY_FIX_USED_MASK` | `1` | Correctness | Keeps an expired polling slot reserved until `elapsed` (prevents timer theft) |
| `ASYNC_DELAY_FIX_ATOMIC_MASK` | `1` | Correctness | Critical section preserving `SREG` around main-context updates; keep `1` |
| `ASYNC_DELAY_OPT_BITMASK` | `1` | Speed | Tick visits only active slots (idle tick ~12 cycles instead of ~100) |
| `ASYNC_DELAY_OPT_MERGED_FLAGS` | `1` | Speed | state+repeat in one byte (1 byte less RAM per slot) |
| `ASYNC_DELAY_OPT_UNROLL_TICK` | `1` | Speed | Compile-time index in the tick (~72→~13 cycles per active slot) |
| `ASYNC_DELAY_OPT_NEXT_TARGET` | `1` | Speed | O(1) gate: if nothing is due, the tick returns after one compare |
| `ASYNC_DELAY_OPT_SPLIT_TICK` | `1` | Speed | Idle tick has no locals (avoids a ~30-cycle register spill on every tick) |
| `ASYNC_DELAY_OPT_SPLIT_ARRAYS` | `0` | Speed | Parallel arrays instead of struct — opt-in, measure first |
| `ASYNC_DELAY_CALLBACK_RESCHEDULE` | `1` | Behavior | Slot is freed/re-armed before the callback runs (self-rescheduling works) — **keep `1`** |
| `ASYNC_DELAY_DEFERRED_CALLBACKS` | `0` | Behavior | Callbacks run from `async_delay_poll()` in main — conscious opt-in, see 🚦 |

The two "behavior" flags change observable behavior; the first eight do not (speed/RAM only).

> ⚠️ `MAX_SLOTS` looks allowed up to 254, but since all internal masks are 8-bit, any value above 8
> fails at compile time with `#error` under the default enabled flags.
>
> ⚠️ Compile-time validation: undefined or zero `TICK_HZ`, `TIMER_BITS` outside 8/16/32, and
> `MAX_SLOTS == 0` all produce `#error` — i.e. a config mistake never passes silently.

### If you really need more than 8 slots

First make sure there is no other way (e.g. reusing slots or doubling `duration`).
If not, put all of these **before `#include`** at the same time (one by one they error, because they depend on each other):

```c
#define ASYNC_DELAY_MAX_SLOTS            16   // up to 254 allowed
#define ASYNC_DELAY_OPT_BITMASK          0    // the rest go off with this one:
#define ASYNC_DELAY_OPT_UNROLL_TICK      0
#define ASYNC_DELAY_OPT_NEXT_TARGET      0
#define ASYNC_DELAY_OPT_SPLIT_TICK       0
#define ASYNC_DELAY_FIX_USED_MASK        0
#define ASYNC_DELAY_DEFERRED_CALLBACKS   0
// FIX_ATOMIC_MASK, MERGED_FLAGS and CALLBACK_RESCHEDULE may stay 1
```

Cost: the tick goes from O(1) back to a linear O(N) scan, and expired polling slots are detected via the state
byte (the same legacy path whose behavior is correct, just slower).

---

## ⏱ Tick, duration, and time limits

**The unit of `duration` is ticks, not milliseconds.** Conversion:

```text
ticks = ms × TICK_HZ ÷ 1000        example: 250ms with TICK_HZ=1000 → 250 ticks
                                   example: 250ms with TICK_HZ=100  → 25 ticks
```

With the default `TICK_HZ=1000` the `duration` number equals milliseconds — but do not rely on that; always compute with the formula.

- `duration = 0` is allowed → the delay expires on the **very next tick** (almost immediately).
- ⚠️ `duration` has type `async_tick_t` — it must fit the counter width. With `TIMER_BITS=8` a value of 500 overflows (500→244) and silently misbehaves.

**Maximum reliable delay = half the counter range** (because of the wrap-safe compare; longer delays behave unreliably):

| `TIMER_BITS` | Max reliable delay (ticks) | @ `TICK_HZ=1000` |
|---|---|---|
| 8 | 128 | 128ms |
| 16 (default) | 32767 | ~32.7 seconds |
| 32 | ~2.1 billion (2³¹) | ~24.8 days |

**Accuracy:** the library's logic error is at most **±1 tick** (expiry is detected on the first tick where
`now ≥ target` holds, so real time is between `duration` and `duration+1` ticks). Real accuracy is set by the
MCU clock: external crystal ~±0.005%, internal RC ~±1–3%.

**CPU cost vs `TICK_HZ`** (measured on ATmega8 @ 8MHz with defaults; 4 active slots):

| `TICK_HZ` | Tick CPU usage |
|---|---|
| 1000 (1ms tick) | ~1.3% |
| 10000 (100µs tick) | ~13% |

The tick itself is O(1) — the slot count has no effect on idle-tick cost. Conclusion: raise `TICK_HZ` only
as high as the precision you really need.

---

## 🔌 Timer setup (the most important part)

Any timer (Timer0/1/2 on compatible MCUs) that can interrupt **exactly** every `1/TICK_HZ` seconds is enough;
**CTC** mode is recommended. Inside the ISR call **only** `async_delay_tick()`.

**This is the only thing the library asks of the hardware.**

**Example ATmega8 @ 8MHz, 1ms tick with Timer2 (CTC, /64 prescaler):**

```c
#define OCR2_1MS  124        // 8MHz/64 = 125kHz → 1ms = 125 ticks → OCR = 124

interrupt [TIM2_COMP] void timer2_comp_isr(void)
{
    async_delay_tick();      // the library's clock heart — only this!
}
```

**General OCR formula (any clock, any TICK_HZ):**
`OCR = (F_CPU / Prescaler / TICK_HZ) - 1`
Pick the prescaler so OCR fits the timer range (8-bit timer: max 255).
If it does not fit, use a larger prescaler or a smaller `TICK_HZ`.

| MCU clock | Prescaler | Timer clock | OCR (for 1ms) |
|-----------|-----------|-------------|---------------|
| 1 MHz | /8 | 125 kHz | 124 |
| 8 MHz | /64 | 125 kHz | 124 |
| 16 MHz | /64 | 250 kHz | 249 |

> ⚠️ **Every time you change the frequency, recompute OCR.** With a wrong OCR the tick becomes 2ms and every delay doubles.
> ⚠️ In CodeWizardAVR the Compare field accepts only 2 decimal digits; for 124 write `0x7C`, for 249 write `0xF9`.
> ⚠️ **Make sure the MCU clock in the project settings matches the real clock** — in CodeWizardAVR the
> CPUClock field; 16 with an 8 MHz MCU halves every delay (a real past bug of this project).
> ⚠️ **Enable the timer interrupt flag in `TIMSK` and `sei` at the end** — if the timer never interrupts,
> no delay ever expires and `async_delay_elapsed` always returns 0.
> ⚠️ After setup, sanity-check the tick: a 500ms periodic LED must be 500ms on a real clock;
> if it is 2x/0.5x, check OCR and CPUClock first.

---

## 🧩 API — six functions

| Function | Signature | Role |
|----------|-----------|------|
| `async_delay_init()` | `void async_delay_init(void)` | Frees all slots and zeroes the counter. **Once, in main, before any `start` and before `sei`** |
| `async_delay_start(duration, cb)` | `unsigned char` | One-shot delay → slot number (0 to MAX_SLOTS-1) or `ASYNC_DELAY_NO_SLOT` (0xFF) if full |
| `async_delay_start_periodic(duration, cb)` | `unsigned char` | Repeating delay (self re-arms) until `cancel` → slot number |
| `async_delay_elapsed(slot_id)` | `unsigned char` | For polling mode; returns `1` and **frees** the slot if expired, else `0`. Safe to call every loop |
| `async_delay_cancel(slot_id)` | `void` | Cancels a delay (one-shot or periodic) and frees the slot |
| `async_delay_poll()` | `void` | **Exists only when `ASYNC_DELAY_DEFERRED_CALLBACKS=1`**. Runs deferred callbacks in the main loop |

**Which function, where? (concurrency contract)**

| Function | Timer ISR only | Main / inside this library's callback only |
|----------|----------------|--------------------------------------------|
| `async_delay_tick()` | ✅ | ❌ Never |
| `init/start/elapsed/cancel` | ❌ | ✅ |
| `async_delay_poll()` | ❌ | ✅ (deferred mode only) |
| Default callback | ✅ Runs right there — keep very short | — |
| Deferred callback | — | ✅ From inside `poll` — anything allowed |

> Do not call `start` from your own other ISRs (the library's critical section is designed for main vs the tick ISR;
> the only ISR allowed to call `start` is this library's own callback — pattern 3).

**Slot lifecycle:**

```text
FREE ─ start() ─► ACTIVE ─ tick ─► EXPIRED ─ elapsed() ─► FREE
  ▲                │    └──────── cancel() ──────────────► FREE
  └────────────────┴──────── cancel() ───────────────────► FREE
```

A periodic slot stays ACTIVE after each expiry (re-arm). A one-shot slot with a callback goes straight to FREE;
a polling slot without a callback goes to EXPIRED and waits for `elapsed`.

**Vital notes — read them all:**

- `duration` is in **ticks**, not milliseconds (see "Tick, duration, and time limits").
- `callback` is a function with the **exact** signature `void my_cb(unsigned char slot_id)`.
  `(void *)0` means no callback (polling mode).
- `start_periodic` with `cb = (void *)0` is **meaningless** — it degrades to a one-shot polling delay.
  Periodic delays stop only via `async_delay_cancel`.
- **An expired polling slot stays allocated until you call `elapsed` on it** — i.e. `start` will not
  give it to someone else. Forgetting `elapsed` means a **slot leak**, and after a while every `start`
  returns `0xFF`. This is deliberate: instead of silently losing your timer, it waits for `elapsed`.
- `slot_id` is the value `start` returned; store it for `elapsed`/`cancel`.
  **But ids are recyclable indexes (0 to MAX_SLOTS-1):** after a slot is freed, the next `start` may hand out
  the same id — so a stored id is valid only until `elapsed`/`cancel`.
- `duration=0` → expires on the next tick (almost immediately).
- Periodic delays are **phase-locked**: if a tick arrives late, later periods compensate and the average period
  stays exact (re-arm is done with `target += duration`, not `now + duration`).
- Always check the "no free slot" error with `ASYNC_DELAY_NO_SLOT` (0xFF) — never proceed unchecked.
- `elapsed`/`cancel` with an invalid `slot_id` are safe (return `0` / do nothing respectively) — but you **must not**
  pass a recycled id that now belongs to someone else's timer (pitfall 12).

**Callback vs polling modes:**

| Mode | How | When |
|------|-----|------|
| **Callback** | `async_delay_start(500, my_cb)` | When the "at expiry" job is short (flip a flag, hit a pin) |
| **Polling** | `async_delay_start(500, (void *)0)` then `async_delay_elapsed(id)` | When the post-expiry job is heavy (LCD, UART) |
| **Deferred (DEFERRED)** | `ASYNC_DELAY_DEFERRED_CALLBACKS=1` + `async_delay_poll()` in the loop | When you want callbacks but the job is heavy — next section |

**Callback rules (default mode — callback runs inside the ISR):**

- Keep the callback **very short**: flip a flag, hit a pin, write a variable — done.
- **Forbidden in a callback:** `delay_ms`, LCD printing, long UART sends, heavy loops, anything taking more than a few microseconds.
- Heavy work? Either **poll**, or enable **deferred mode**.
- Self-rescheduling (`start` from inside a one-shot callback) is allowed and supported — pattern 3 (because the slot
  is freed before the callback; you owe that to `CALLBACK_RESCHEDULE=1` — do not set it to `0`).

---

## 🚦 Deferred mode — `ASYNC_DELAY_DEFERRED_CALLBACKS`

Default is `0` and callbacks run inside the ISR. If you define this **before `#include`**:

```c
#define ASYNC_DELAY_DEFERRED_CALLBACKS 1
```

The behavior changes — know these three differences:

1. **The ISR no longer runs callbacks**; it only marks them. Callbacks run from `async_delay_poll()` in **main context**.
2. **You must call `async_delay_poll()` in the main loop** — if you do not, callbacks **never run** (no error, silently).
3. In return a callback may do anything: `delay_ms`, LCD, UART — because it is no longer inside the ISR.

Costs/details: callback latency is at most "one main-loop iteration"; and if a periodic slot expires twice
before one `poll`, two calls collapse into **one** (each slot has only one flag bit).
Periodic accuracy is preserved (re-arm still happens inside the ISR; only callback execution is delayed).
Side bonus: the idle tick gets faster too (a callback in the ISR means ICALL plus register saving).

**When to enable it?** When the expiry event needs heavy work and you do not want to write the polling pattern by hand.

---

## 📝 Usage patterns

### Pattern 0: complete minimal program (whole main.c)

```c
// ===== ATmega8 @ 8MHz, 1ms tick, LED blink every 500ms =====
#include <mega8.h>                        // (1) MCU header — before the library!

#define ASYNC_DELAY_TICK_HZ    1000       // (2) config — before the include
#define ASYNC_DELAY_TIMER_BITS 16
#define ASYNC_DELAY_MAX_SLOTS  4
#include <async_delay.h>                  // (3)

unsigned char led_state = 0;

void blink_cb(unsigned char slot_id)      // callback — short! flag only
{
    led_state ^= 1;
}

interrupt [TIM2_COMP] void timer2_comp_isr(void)
{
    async_delay_tick();                   // (4) only this inside the ISR
}

void main(void)
{
    DDRB.0 = 1;                           // LED output

    // Timer2: CTC, /64 prescaler → interrupt every 1ms  (8MHz/64 = 125kHz)
    OCR2  = 124;                          // 125kHz / 125 = 1kHz  (in CodeWizard: 0x7C)
    TCCR2 = 0x0C;                         // WGM21=1 (CTC) + CS22:20=100 (/64)
    TIMSK = 0x80;                         // OCIE2 — Timer2 compare interrupt
    TIFR  = 0x80;                         // clear compare flag

    async_delay_init();                   // (5) once — before any start

    async_delay_start_periodic(500, blink_cb);   // every 500ms

    #asm("sei")                           // (6) end of setup

    while (1)
    {
        PORTB.0 = led_state;
        // rest of the loop — never blocks anywhere
    }
}
```

### Pattern 1: LED blink (periodic + callback)

```c
unsigned char led_state = 0;

void blink_cb(unsigned char slot_id)   // callback — keep it short!
{
    led_state ^= 1;                    // just a flag
}

// in main():
async_delay_start_periodic(500, blink_cb);   // once every 500ms

// in the loop:
PORTB.0 = led_state;
```

### Pattern 2: polling (no callback)

```c
unsigned char id = async_delay_start(1000, (void *)0);   // no callback

while (1)
{
    if (async_delay_elapsed(id))      // 1000 ticks done?
    {
        do_something();                          // heavy work, safe here
        id = async_delay_start(1000, (void *)0); // schedule again
    }
    // rest of the loop...
}
```

### Pattern 3: self-reschedule — staging work

Start the next delay from inside a callback (the library supports this — a one-shot slot is freed before its
callback runs):

```c
unsigned char step = 0;

void step_cb(unsigned char slot_id)
{
    step++;                              // go to next step
    if (step < 3)
        async_delay_start(500, step_cb); // again in 500ms
    // when step==3, we stop scheduling → done
}

// in main():
async_delay_start(500, step_cb);
```

### Pattern 4: cancelling a delay

```c
unsigned char id = async_delay_start(2000, do_something);
// ... changed your mind:
async_delay_cancel(id);                  // never runs
```

### Pattern 5: "restarting" or changing the period of a periodic delay from inside its own callback

> ⚠️ Since a periodic slot re-arms before the callback, a fresh `start` of the **same id** creates a second timer.
> The correct pattern:

```c
async_delay_cancel(id);                      // cancel the old one first
id = async_delay_start_periodic(500, cb);    // then start again
```

### Pattern 6: heavy callback with deferred mode (DEFERRED_CALLBACKS)

```c
// before #include:
#define ASYNC_DELAY_DEFERRED_CALLBACKS 1

void refresh_lcd_cb(unsigned char slot_id)
{
    lcd_clear();                        // now allowed: heavy work in main context
    lcd_putsf("hello");
}

// in main():
async_delay_start_periodic(200, refresh_lcd_cb);

// in the main loop:
while (1)
{
    async_delay_poll();                 // without this, callbacks never run!
    // rest of the loop...
}
```

### Pattern 7: multi-file projects (important — the library is `static`)

All library data and functions are `static`: if you include the header in two `.c` files, you build **two
separate timer devices** of which only one gets ticked. So include the header in **one** `.c` file only
(e.g. `timer_mgr.c`), and let the other files talk to it via flags and wrappers:

```c
// ===== timer_mgr.c — the only file that sees the library =====
#include <mega8.h>
#define ASYNC_DELAY_TICK_HZ 1000
#include <async_delay.h>

unsigned char g_sensor_ready = 0;        // flag shared with other files

void sensor_cb(unsigned char slot_id)
{
    g_sensor_ready = 1;                  // flag only — short
}

void timer_mgr_init(void)
{
    // ... timer setup + async_delay_init + starts ...
}
```

```c
// ===== sensor.c — never include async_delay.h here! =====
extern unsigned char g_sensor_ready;     // borrow the flag from timer_mgr

void sensor_task(void)
{
    if (g_sensor_ready)
    {
        g_sensor_ready = 0;
        read_sensor_heavy();             // heavy work, in main
    }
}
```

Rule: timing logic in `timer_mgr.c`, heavy logic in other files, communication only via `extern` flags/functions.

---

## ⚠️ Compiler limits (CodeVisionAVR C) — read before writing code

1. **Declarations first in a block (C89).** A variable declaration after a statement is an error. Wrong: `do_x(); unsigned char i;` —
   right: all `unsigned char`s at the top of the function.
2. **`bit` is not a variable name.** `bit` (like `flash`/`eeprom`/`interrupt`) is a reserved CodeVision word;
   a variable with that name gives "invalid combination of type specifiers". Use `slotbit`/`maskbit`.
3. **No `u/U` literal suffixes** (like `500u`) — the rest of the code and the CVAVR headers do not use them.
4. **`NULL` means `(void *)0`.** Example: `async_delay_start(500, (void *)0)`.
5. **Enable interrupts with `#asm("sei")`** (not a library function).

---

## 🚫 Common pitfalls — run this checklist before shipping

1. **Forgot `elapsed`?** An expired polling slot stays allocated until `elapsed`; forgetting = leak = `0xFF` later.
2. **Heavy ISR callback** — flags/pins only. Heavy → polling or pattern 6.
3. **Restarting a periodic delay from inside its own callback** → `cancel` first, then `start` (pattern 5), otherwise you build a second timer.
4. **Recheck OCR / project clock** — wrong = every delay stretches or shrinks.
5. **MCU header included before `async_delay.h`** (the library needs `SREG`).
6. **Include `async_delay.h` in only “one” `.c` file**, all calls from that same file
   (multi-file way: pattern 7).
7. **Set `DEFERRED_CALLBACKS=1`?** Then `async_delay_poll()` in the main loop — otherwise no callback ever runs.
8. **Delay longer than half the counter range?** Unreliable — widen `TIMER_BITS` or lower `TICK_HZ`.
9. **Took `duration` for milliseconds?** It is ticks! `ticks = ms × TICK_HZ ÷ 1000`. And it must fit `TIMER_BITS`.
10. **Do not forget `sei`** and enable the timer interrupt flag in `TIMSK` — otherwise no tick ever comes and everything sleeps.
11. **Check the `0xFF` error from `start`** (`ASYNC_DELAY_NO_SLOT`) — e.g. for logging or fallback.
12. **Do not reuse a stored id after `elapsed`/`cancel`** — ids are recycled and may now
    point to someone else's timer.
13. **Do not set `CALLBACK_RESCHEDULE` to `0`** — self-rescheduling (pattern 3) breaks.
14. **Do not touch the tick counter by hand** — `tick++` must stay the first statement of the tick; the order matters.

---

## 🩺 Troubleshooting — symptom → probable cause

| Symptom | Check this first |
|---------|------------------|
| Nothing ever expires / `elapsed` always 0 | Tick ISR never fires: `TIMSK`, `#asm("sei")`, CTC mode, OCR value |
| All delays became 2x or 0.5x | OCR does not match the clock; project CPUClock field differs from the real MCU |
| `start` always returns `0xFF` | Slot leak: `elapsed` never called somewhere; or all slots genuinely full |
| Deferred mode: callback never runs | `async_delay_poll()` missing from the main loop |
| LCD/UART garbled from inside a callback | Heavy work forbidden in ISR → polling or deferred |
| `SREG undefined` error | MCU header (`mega8.h`) not included before the library |
| `invalid combination of type specifiers` error | You have a variable named `bit` — rename it |
| `must declare first in block` error | Declaration after statement (C89 rule) — move to top of function |
| `#error ... at most 8 slots` error | `MAX_SLOTS > 8` with default flags — see the "more than 8 slots" section |
| Two timers running after a periodic restart | You did not `cancel` first (pattern 5) |
| Each file behaves separately / one is dead | Header included in two `.c` files (pitfall 6, pattern 7) |

---

## 📦 Project notes

- **Header-only, everything `static`** — that is what makes the "only one .c file" rule unavoidable (pitfall 6, pattern 7).
- **Version marker / stale-copy check** — the header defines `ASYNC_DELAY_VERSION`
  (integer; current value **6** — always trust the `#define` in the header over this
  text). It equals the number of the last plan that modified the header. If you keep a
  copy of `async_delay.h` in another project, detect a stale copy at compile time:

  ```c
  #if ASYNC_DELAY_VERSION != 6
  #error "async_delay.h copy is stale - copy the current header over and rebuild"
  #endif
  ```

  Update the compared number whenever you update the header copy. Costs zero
  Flash/RAM (preprocessor only).
- **Size with defaults** (`TIMER_BITS=16`, `MAX_SLOTS=4`, `TICK_HZ=1000`) on ATmega8:
  **~34 bytes RAM**, **~378 words Flash** (library functions only, ~9% of ATmega8).
- **RAM formula** (with `MERGED_FLAGS=1`): each slot is `2×sizeof(async_tick_t) + 3` bytes (e.g. 7 bytes in 16-bit mode),
  plus the counter (`sizeof`) + active mask (1) + used mask (1) + nearest target (`sizeof`).
  16-bit 4-slot example: 28 + 2 + 1 + 1 + 2 = 34 bytes.
- **Tick cost is O(1)** — an idle tick returns fast no matter how many slots are active (CPU table in ⏱).
- The hardware tick must be **exactly** `TICK_HZ`; any deviation translates directly into every delay's error.

---

## 🔧 Internal architecture

This file deliberately explains nothing internal. If you want to work on `async_delay.h` itself, change a flag,
or understand the tick algorithm, read `ARCHITECTURE.md` first — the invariants and historical pitfalls are documented there.
