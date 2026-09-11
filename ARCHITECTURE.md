# ARCHITECTURE.md — async_delay (Engineered for AI Coding Agents)

Read this document FIRST before touching any file in this repository. It is written specifically for an AI Coding Agent (not human end-users) to instantly reconstruct the exact architectural invariants, execution constraints, memory models, hardware bottlenecks, and historical defect mitigations without needing to parse the entire codebase from scratch.

---

## 1. System Identity & Mission

- **Target Architecture**: 8-bit Microchip/Atmel AVR (`ATmega8`, `ATmega16`, `ATmega32`, `ATmega328P`) clocked at 1–16 MHz.
- **Compiler Targets**: **CodeVisionAVR (Primary)**, **AVR-GCC / Clang (Embedded)**, and **Host Native GCC/Clang** (Linux/x86_64 for automated test harnesses).
- **Core Paradigm**: Single header-only (`async_delay.h`), zero-heap, zero-malloc, static RAM, non-blocking asynchronous event loop driven by a hardware timer ISR (`async_delay_tick()`).
- **Highest Priority Objective**: **Absolute Maximum Execution Speed (Minimal CPU Cycles)** while preserving rock-solid safety against timer wrap-around, ISR concurrency races, and memory corruption.

---

## 2. File Map & Agent Roles

| Path | Primary Agent Role / Purpose |
|---|---|
| `async_delay.h` | **The Core Library**. Defines `ASYNC_DELAY_VERSION 12`. Contains all algorithms, ISR gates, data structures, and conditional compilation flags. |
| `ARCHITECTURE.md` | **Your Internal Blueprint (This file)**. Hardware models, cycle budgets, regression rules, and internal invariants. |
| `README.md` | Public integration contract, hardware timer formulas, and complete API specifications. |
| `async_delay_guide.md` | Persian technical guide for embedded developers using CodeVisionAVR. |
| `async_delay_test.c` | Reference ATmega8 firmware project (Timer2 CTC, LCD, LEDs, buttons). |
| `tests/test_async_delay.c` | Host-based native unit tests covering all matrix permutations. |
| `tests/test_stress.c` | High-frequency stress test (wrap-around arithmetic, self-rescheduling, boundary conditions). |
| `tests/benchmark_compare.c` | Host-based comparative benchmarking binary (Baseline vs Optimized). |
| `tests/compare_performance.sh`| Automated regression detector comparing current code against unoptimized baseline. |
| `tests/verify_avr_cycles.py` | Assembly inspector verifying table placement and absence of shift loops (`__LSLW12`). |
| `tests/run_all_configs.sh` | Full verification pipeline (Unit + Stress + Regression Benchmarks). |

---

## 3. Hardware Architecture & AVR Execution Bottlenecks

### 3.1 The "No Barrel Shifter" Trap
AVR core executes single-bit shifts (`LSL`, `LSR`, `ROL`, `ROR`) in 1 cycle, but **lacks a multi-bit hardware barrel shifter**.
- Dynamic shifts like `(1 << n)` on variable `n` generate a compiler helper call (`RCALL __LSLW12`) costing **10–30 clock cycles**.
- **Agent Rule**: NEVER emit variable bit shifts in hot paths. Always use precomputed lookup tables (`_async_slot_bit`, `_async_first_free_nibble`, `_async_popcount_nibble`).

### 3.2 Register Spills at Function Entry (`__SAVELOCR4`)
CodeVisionAVR automatically saves local variables to the stack upon entering a function **before evaluating any `if` statements**.
- If `async_delay_tick()` declares local variables, idle ticks incur a ~30-cycle penalty even when 0 timers are active!
- **Agent Rule**: `async_delay_tick()` MUST remain free of stack locals. Complex scanning logic must reside in `_async_delay_tick_walk()`.

### 3.3 Atomic Multi-Byte Operations on 8-bit Data Bus
AVR data bus is 8-bit. Reading a 16-bit (`unsigned int`) or 32-bit (`unsigned long`) variable takes 2 to 4 separate load instructions (`LDS`). If a hardware ISR interrupts mid-load, the value becomes corrupt.
- **Agent Rule**: All reads/writes to `_async_tick_counter`, `_async_active_mask`, and `_async_next_target` outside ISR MUST be wrapped in SREG-preserving critical sections (`_ASYNC_SAVE_SREG()`, `#asm("cli")`, `_ASYNC_REST_SREG()`).

---

## 4. Current State & Micro-Benchmarked Performance (v11)

Measured on ATmega8 @ 8 MHz (`TIMER_BITS=16`, `MAX_SLOTS=4`, `TICK_HZ=1000`):

| Operation | Base Cycles | v11 Optimized | Speedup / Impact |
|---|---|---|---|
| **Idle ISR Tick** (0 active slots) | ~85 cyc | **< 15 cycles** | **~5.6x faster** ($O(1)$ early gate) |
| **Active ISR Tick** (all in flight, none due) | ~105 cyc | **< 18 cycles** | **~5.8x faster** (earliest target gate) |
| **Active Count Query** (`active_count()`) | ~45 cyc | **~4 cycles** | **~10x faster** (LUT Popcount $O(1)$) |
| **Slot Allocation** (`start()`) | ~55 cyc | **~12 cycles** | **~4.5x faster** (LUT Alloc $O(1)$) |
| **Fast Cancel** (`cancel()`) | ~48 cyc | **~18 cycles** | **~2.6x faster** (bypasses recompute) |
| **In-Place Restart** (`restart()`) | ~48 cyc | **~32 cycles** | **~1.5x faster** (zero reallocation) |
| **RAM Footprint** (4 slots, 16-bit) | 34 B | **34 Bytes** | **Zero heap, 100% static RAM** |

---

## 5. Algorithmic Invariants & Core Data Structures

### 5.1 Static Data Segment Layout
```c
typedef struct {
    async_tick_t     target;    // Target tick when delay fires
    async_tick_t     duration;  // Period duration (removed if DISABLE_PERIODIC=1)
    async_delay_cb_t callback;  // Callback pointer (removed if DISABLE_CALLBACKS=1)
    unsigned char    flags;     // [1:0]=State (FREE/ACTIVE/EXPIRED), [2]=Repeat
} _async_slot_t;

static _async_slot_t         _async_slots[ASYNC_DELAY_MAX_SLOTS];
static volatile async_tick_t _async_tick_counter;
static volatile async_mask_t _async_active_mask;  // Bit n = RUNNING (tested by ISR)
static volatile async_mask_t _async_used_mask;    // Bit n = ALLOCATED (tested by start)
static volatile async_tick_t _async_next_target;  // Cached minimum active target
static volatile async_mask_t _async_pending_mask; // Deferred callbacks awaiting poll
```

### 5.2 The Wrap-Safe Arithmetic Invariant
```c
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7FFF) // for 16-bit
#define _ASYNC_REACHED(now, t) \
    ((async_tick_t)((async_tick_t)(now) - (async_tick_t)(t)) < _ASYNC_HALF_RANGE)
```
- **Rule**: Correct across unsigned counter overflow as long as delay $< \text{HALF\_RANGE}$ (32,767 ms for 16-bit @ 1kHz).
- **Literal rule**: Width-specific literals (`0x7F`, `0x7FFF`, `0x7FFFFFFF`) MUST be used to prevent ANSI C integer promotion bugs.

### 5.3 $O(1)$ Allocation & Popcount Lookup Tables (v11)
```c
// 16-entry Nibble LUT for O(1) allocation: returns index of lowest 0-bit
static const unsigned char _async_first_free_nibble[16] = {
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 0xFF
};

// 16-entry Nibble LUT for O(1) active count popcount: returns number of 1-bits
static const unsigned char _async_popcount_nibble[16] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};
```
- For up to 16 slots: Evaluated in at most 2 nibble lookups without a single loop or branch.

### 5.4 Two-Mask State Machine
- `_async_used_mask` tracks ALLOCATED slots (prevents `start()` from stealing polling slots).
- `_async_active_mask` tracks RUNNING slots (polled by ISR tick).
- **Core Invariant**: `(_async_active_mask & ~_async_used_mask) == 0` (No slot can be active without being allocated).

---

## 6. Code Modification Rules for the AI Agent

When editing `async_delay.h`:

1. **Strict C89 Declarations**: Declare ALL variables at the very beginning of the block before any executable statements. CodeVisionAVR will fail compilation otherwise.
2. **Reserved Keyword Ban**: NEVER use `bit`, `flash`, `eeprom`, `sfrb`, `sfrw`, `interrupt`, or `funcused` as variable names. Use `slotbit`, `mask_val`, etc.
3. **No Inline `#asm` Inside Macros**: CodeVisionAVR preprocessor breaks inline assembly macros. Keep `#asm("cli")` inline at the physical call site.
4. **Preserve Volatiles First**: In `async_delay_tick()`, `_async_tick_counter++` MUST be executed before any early-exit check.
5. **Phase-Locked Periodic Arming**: In periodic mode, new target MUST be `_async_slots[i].target += duration`, NEVER `now + duration`.
6. **Regression Verification**: After making edits, always run:
   ```bash
   bash tests/run_all_configs.sh
   ```
   Inspect Phase 3 table to confirm no speed regressions occurred.
7. **Synchronize Documentation**: Any change in flags, APIs, or limits MUST be updated in `README.md`, `async_delay_guide.md`, and this `ARCHITECTURE.md`.

---

## 3. Configuration contract (defined by user BEFORE `#include`)

| Macro | Default | Meaning | Validation |
|-------|---------|---------|------------|
| `ASYNC_DELAY_TICK_HZ` | **none — required** | tick rate in Hz (e.g. 1000 = 1ms tick) | `#error` if undefined or == 0 |
| `ASYNC_DELAY_TIMER_BITS` | `16` | tick counter width: 8 / 16 / 32 | `#error` if not one of these |
| `ASYNC_DELAY_MAX_SLOTS` | `4` | max concurrent delays | `#error` if == 0 or > 16 under bitmask; max 254 in legacy mode |
| `ASYNC_DELAY_CALLBACK_RESCHEDULE` | `1` | slot made non-ACTIVE before its callback runs (enables self-reschedule) | — |
| `ASYNC_DELAY_OPT_BITMASK` | `1` | tick visits only ACTIVE slots via bitmask (supports up to 16 slots) | `#error` if MAX_SLOTS > 16 |
| `ASYNC_DELAY_OPT_LUT_MASK` | `1` | bit-mask lookup table eliminates `__LSLW12` runtime shift loops on AVR | — |
| `ASYNC_DELAY_DISABLE_CALLBACKS` | `0` | polling-only mode: removes callback pointer (saves 2B RAM/slot + flash) | incompatible with `DEFERRED_CALLBACKS` |
| `ASYNC_DELAY_DISABLE_PERIODIC` | `0` | one-shot only mode: removes duration storage (saves 2B RAM/slot + flash) | — |
| `ASYNC_DELAY_OPT_FAST_CANCEL` | `1` | bypasses $O(N)$ recompute in `cancel()` if cancelled slot wasn't earliest | requires BITMASK & NEXT_TARGET |
| `ASYNC_DELAY_FEATURE_RESTART` | `1` | enables `async_delay_restart()` to retarget without reallocating slot ID | — |
| `ASYNC_DELAY_OPT_MERGED_FLAGS` | `1` | state+repeat packed into one byte | — |
| `ASYNC_DELAY_OPT_SPLIT_ARRAYS` | `0` | parallel arrays instead of a struct | requires BITMASK |
| `ASYNC_DELAY_FIX_USED_MASK` | `1` | **correctness**: separate ALLOCATED mask preventing slot theft | requires BITMASK |
| `ASYNC_DELAY_FIX_ATOMIC_MASK` | `1` | **correctness**: SREG-preserving critical sections | required by NEXT_TARGET when TIMER_BITS ≥ 16, or MAX_SLOTS > 8 |
| `ASYNC_DELAY_OPT_UNROLL_TICK` | `1` | compile-time slot indices in the tick (avoids shift/mul loops, up to 16 slots) | requires BITMASK |
| `ASYNC_DELAY_OPT_UNROLL_RECOMPUTE` | `1` | compile-time slot indices in `_async_recompute_next` (removes loop/indexing overhead) | requires NEXT_TARGET |
| `ASYNC_DELAY_OPT_NEXT_TARGET` | `1` | O(1) earliest-target gate before touching slots | requires BITMASK |
| `ASYNC_DELAY_OPT_SPLIT_TICK` | `1` | keep tick fast path local-free; sweep walk lives in callee | requires BITMASK |
| `ASYNC_DELAY_DEFERRED_CALLBACKS` | `0` | **changes behavior**: callbacks run from `async_delay_poll()` in main context | requires BITMASK, MAX_SLOTS ≤ 16 |

Flag effect: `TIMER_BITS` selects the type of `async_tick_t`
(`unsigned char` / `unsigned int` / `unsigned long`).

Verified-good combinations (checked by preprocessing the header for each and by
the user's builds): all-defaults; each `OPT_*`/`FIX_*` individually at 0;
`SPLIT_ARRAYS=1`; `MERGED_FLAGS=0`; `RESCHEDULE=0`; `DEFERRED=1` (alone and with
`RESCHEDULE=0`); `BITMASK=0` full-legacy; `MAX_SLOTS` 1/4/5/8; `TIMER_BITS`
8/16/32. Only all-defaults and `SPLIT_ARRAYS`-off have been compiled on
hardware — the rest passed structural checks only.

---

## 4. Data structures (exact)

```c
typedef void (*async_delay_cb_t)(unsigned char slot_id);

#if ASYNC_DELAY_MAX_SLOTS <= 8
typedef unsigned char async_mask_t;
#else
typedef unsigned int  async_mask_t;
#endif

typedef struct {                // ASYNC_DELAY_OPT_MERGED_FLAGS == 1 layout
    async_tick_t     target;    // tick value when this delay expires
    async_tick_t     duration;  // stored for periodic re-arm
    async_delay_cb_t callback;  // NULL = polling-only mode
    unsigned char    flags;     // [1:0]=state (FREE/ACTIVE/EXPIRED), [2]=repeat
} _async_slot_t;                // (MERGED_FLAGS==0 splits this into state+repeat)

static _async_slot_t _async_slots[ASYNC_DELAY_MAX_SLOTS];
static volatile async_tick_t _async_tick_counter;
static volatile async_mask_t _async_active_mask;     // OPT_BITMASK: bit n RUNNING
static volatile async_mask_t _async_used_mask;       // FIX_USED_MASK: bit n ALLOCATED
static volatile async_tick_t _async_next_target;     // OPT_NEXT_TARGET: earliest target
static volatile async_mask_t _async_pending_mask;    // DEFERRED_CALLBACKS: cb owed
```

Access goes through `_AD_TARGET/_AD_DUR/_AD_CB/_AD_FLAGS/_AD_STATE/_AD_REPEAT`
so the struct and split-array layouts share one body of code.

Slot states: `ASYNC_SLOT_FREE (0)`, `ASYNC_SLOT_ACTIVE (1)`, `ASYNC_SLOT_EXPIRED (2)`.
Error code: `ASYNC_DELAY_NO_SLOT (0xFF)`.

RAM at the default flags, MAX_SLOTS=4, TIMER_BITS=16: 28 (slots) + 2 (counter)
+ 1 (active) + 1 (used) + 2 (next_target) = **34 bytes**. Confirmed against
`Debug/List/async_delay_test.map`. (When `MAX_SLOTS > 8`, `async_mask_t` expands to 16 bits = 2 bytes per mask).

All functions and data are `static` — header-only, so this avoids duplicate
symbols if more than one translation unit includes it. Note the header does
**not** use `#pragma used+`/`used-` (an earlier revision of this file claimed it
did; it never appeared in the code). CodeVisionAVR emits the statics because
they are referenced; if you ever add a static that is only touched from inline
asm, that is when you would need the pragma.

---

## 5. Public API — exact behavior

| Function | Behavior |
|----------|----------|
| `async_delay_init()` | Zeroes the counter and every mask, sets all slots FREE. Call ONCE before `sei`. |
| `async_delay_start(duration, cb)` | One-shot. Returns slot_id or `0xFF` if no free slot. `duration=0` → expires next tick. |
| `async_delay_start_periodic(duration, cb)` | Periodic, auto re-arm. `cb == NULL` is pointless (degrades to polling one-shot). |
| `async_delay_elapsed(slot_id)` | Polling check. Returns 1 if EXPIRED, frees slot **and its allocation**, else 0. Invalid id → safe 0. |
| `async_delay_cancel(slot_id)` | Frees the slot, clears every mask bit, recomputes the next target. Invalid id → no-op. |
| `async_delay_tick()` | **ISR-only.** Increments counter, then O(1)-gates on the earliest target before touching any slot. Declares no locals (§6.7); the sweep lives in `_async_delay_tick_walk()`. Must run at exactly `TICK_HZ`. |
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
// Per-width literals (0x7F / 0x7FFF / 0x7FFFFFFF for 8/16/32-bit)
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7FFF)                      // 16-bit case
#define _ASYNC_REACHED(now, t) \
    ((async_tick_t)((async_tick_t)(now) - (async_tick_t)(t)) < _ASYNC_HALF_RANGE)
```
Read it as "`t` has been reached at `now`", equivalently "`t` is not later than
`now`". It lives in exactly one place so the tick, the next-target minimum and
`start()` cannot drift apart. Bit-identical to the old inline
`(counter - target) < half`.

**Integer Promotion Safeguard:** The half range is computed with per-width literals
(0x7F / 0x7FFF / 0x7FFFFFFF) instead of bitwise expressions like `~0 >> 1`.
In C89, an 8-bit `~0` undergoes integer promotion to signed int (`-1`), keeping
the sign through arithmetic shift; using explicit width literals prevents
unexpected promotion issues across all conforming compilers.

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
between byte loads → corrupted value). Don't "simplify" it away.

The critical section covers counter reads, mask operations, and next-target updates,
preserving and restoring SREG rather than issuing a bare `sei`:
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
Recomputed by `_async_recompute_next()` (O(N)) after every slot walk, and inside
`cancel()`. `start()` needs just one compare.

The recompute after a walk is **unconditional**, not gated on "did something
fire". Reaching the walk means the compare above held, and `_async_next_target`
is always the exact minimum of the ACTIVE targets — so some ACTIVE slot has
`target == _async_next_target`, satisfies the same compare, and fires. At least
one slot always fires in a walk, so the cached minimum is always stale there.
An unconditional recompute is O(N) and idempotent.

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
EXPIRED. Without a separate used mask, `start()` would hand out polling slots
whose owner had not called `elapsed()` yet, destroying a live timer. Two masks fix it:

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
ASM here, avoiding runtime code-size and cycle overhead in CVAVR.

The unrolled chain is wrapped in `_AD_TICK_SWEEP()`, with per-slot
`_AD_TICK_S1`..`_AD_TICK_S7` macros that expand to nothing above `MAX_SLOTS`.
That keeps one copy of the sweep body even though two tick shapes use it (§6.7),
so the A/B flag cannot make them drift apart.

### 6.7 Split tick — the CodeVisionAVR register-spill trap (`ASYNC_DELAY_OPT_SPLIT_TICK`)
**CVAVR spills register locals at function *entry*, before any branch.** A hot
function with a cheap early-exit path therefore pays the spill on the path that
does nothing.

Earlier revisions hoisted three locals into `async_delay_tick()` (`_ad_now` as a 16-bit
pair, `_ad_m`, `_ad_fired`) and CVAVR emitted:

```asm
_async_delay_tick_G000:
	RCALL __SAVELOCR4        ; ~15 cycles, before the mask is even read
	...
	BREQ _0x2020005          ; idle exit
_0x2020005:
	RCALL __LOADLOCR4        ; ~15 cycles
	RET
```

~30 cycles on **every** tick, so the idle tick went ~88 → ~117 cycles even
though the loaded path got faster (~425 → ~135).

The fix: `async_delay_tick()` declares **no locals at all** (counter increment,
mask test, gate compare — all straight off the volatiles), and the register-hungry
sweep lives in `_async_delay_tick_walk()`, called only when a slot is genuinely
due. The trade is 2 extra `LDS` pairs (~8 cycles) on the loaded path in exchange
for ~30 on every path.

Consequence to preserve: the walk must read `_async_tick_counter` and
`_async_active_mask` **once each into locals** at its top. Without that, the
per-slot compares reload the volatiles every iteration and the optimization is undone.

Generalize this: any future hot path in this codebase with an early return needs
the same shape. Check the generated `.asm` for `__SAVELOCR` rather than assuming.

### 6.8 Fast-Path Cancel (`ASYNC_DELAY_OPT_FAST_CANCEL`)
In earlier revisions, `async_delay_cancel()` unconditionally invoked `_async_recompute_next()`.
`_async_recompute_next()` performs a full loop over all `MAX_SLOTS`, computing wrap-distance differences and tracking the minimum deadline.
However, canceling a slot only invalidates `_async_next_target` if:
1. The slot was currently `ACTIVE`.
2. The slot's `target` matched the cached `_async_next_target`.
3. There are still other active slots remaining (`_async_active_mask != 0`).

Under `ASYNC_DELAY_OPT_FAST_CANCEL=1`, `async_delay_cancel()` records `was_active = _AD_STATE(slot_id) == ASYNC_SLOT_ACTIVE` and `old_target = _AD_TARGET(slot_id)`. After clearing the slot masks:
- If `!was_active`, the active minimum did not change -> no recompute.
- If `_async_active_mask == 0`, no active slots remain -> no recompute.
- If `old_target != _async_next_target`, the cancelled slot was not the earliest deadline -> no recompute.

Only if `was_active && old_target == _async_next_target && _async_active_mask != 0` is `_async_recompute_next()` called. This transforms cancel into an $O(1)$ ~22-cycle operation on most paths.

### 6.9 Timer Restart & Retargeting (`async_delay_restart`)
When managing software watchdog timers, button debounces, communication packet timeouts, or dynamic pacing, code frequently needs to extend or re-trigger a timer.
Previously, this required calling `async_delay_cancel(id)` followed by `async_delay_start(dur, cb)`:
- Wasted cycles searching for a free slot.
- Discarded the allocated slot ID and allocated a potentially new ID.
- Increased risk of allocation failure under heavy loads.

`async_delay_restart(slot_id, new_duration)` (enabled via `ASYNC_DELAY_FEATURE_RESTART=1`) updates an existing slot in-place:
1. Validates `slot_id < MAX_SLOTS` and `_AD_STATE(slot_id) != ASYNC_SLOT_FREE`.
2. Computes the new deadline from the current counter: `new_target = _async_tick_counter + new_duration`.
3. Updates `_AD_TARGET(slot_id) = new_target` and `_AD_DUR(slot_id) = new_duration`.
4. Restores state to `ASYNC_SLOT_ACTIVE` (even if it was `ASYNC_SLOT_EXPIRED`) and sets its bit in `_async_active_mask`.
5. Updates `_async_next_target` using $O(1)$ fast-path test if earlier, or recomputes if the slot formerly held the earliest deadline and was extended further.
6. Returns `1` on success, `0` if slot is invalid or not in use.

### 6.10 Scalable 16-Slot Mask Support (`async_mask_t`)
Prior to v8, bitmask operations strictly assumed `unsigned char` (8 bits), limiting `MAX_SLOTS <= 8`.
`async_delay.h` now dynamically defines:
```c
#if ASYNC_DELAY_MAX_SLOTS <= 8
typedef unsigned char async_mask_t;
#else
typedef unsigned int  async_mask_t;
#endif
```
- When `MAX_SLOTS <= 8`, `async_mask_t` is 8-bit (`unsigned char`), maintaining exact cycle counts and memory footprints.
- When `9 <= MAX_SLOTS <= 16`, `async_mask_t` automatically expands to a 16-bit integer (`unsigned int`).
- `_AD_TICK_SWEEP()` was expanded from 8 slots to 16 slots (`_AD_TICK_S0`..`_AD_TICK_S15`), with each unrolled slot gated by compile-time macros (`#if ASYNC_DELAY_MAX_SLOTS > n`).
- Concurrency protection (`_ASYNC_SAVE_SREG()`) is automatically enforced when `MAX_SLOTS > 8` to ensure multi-byte mask updates are atomic on 8-bit AVRs.

### 6.11 Shift-Optimized Slot Allocation (`_async_delay_start_common`)
Inside `_async_delay_start_common()`, searching for a free slot required computing `(1 << i)` for each index. In CodeVisionAVR, shifting by a variable index emits calls to `__LSLW12` (runtime shift loop).
The allocation loop now maintains `slotbit = 1` and shifts `slotbit <<= 1` on each step. Furthermore, `cur_active = _async_active_mask` is cached into a local variable before updating `_async_next_target`, avoiding repeated volatile reloads.

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
| LED0 blink 500ms | periodic + DEFERRED callback | callback fires repeatedly, drained by `async_delay_poll()` from the main loop |
| LED1 blink 750ms | polling + `elapsed()` | poll in loop, slot freed + re-started |
| Cancel 2000ms delay early | cancel | callback never fires |
| Restart in-place (300ms) | `async_delay_restart` | retargets active slot in-place, fails safely on free slot |
| Power-saving & Utility | `is_active`, `active_count`, `remaining`, `ticks_until_next` | returns exact status and remaining ticks |
| LCD refresh / 200ms | polling | real-time pacing, loop not blocked |
| Slot overflow (5th start) | all 4 slots busy | returns `0xFF` |
| `loop_count` (unsigned long) | main loop | proves loop is never blocked |

### 8.1 Host Unit Test Suite & Automated CI

The repository contains a native host-based test suite (`tests/test_async_delay.c`, `tests/test_stress.c`, and `tests/benchmark_cycles.c`) with a multi-configuration test runner (`tests/run_all_configs.sh`) and continuous integration workflow (`.github/workflows/ci.yml`).

The test suite validates:
- Initialization and clean reset
- One-shot and periodic delays (polling and callback)
- In-ISR vs Deferred callback execution (`async_delay_poll`)
- Cancellation and `async_delay_cancel_all()`
- In-place retargeting via `async_delay_restart()`
- Counter wrap-around boundary arithmetic across 8, 16, and 32-bit tick counters
- High-frequency continuous wrap-around with interleaving periodic delays
- Dynamic power-saving recalculation (`async_delay_ticks_until_next`) during runtime cancellation/re-targeting
- Self-rescheduling from inside callbacks (`ASYNC_DELAY_CALLBACK_RESCHEDULE=1`)
- Boundary delays (0-tick immediate expiration and 1-tick delay)
- Slot exhaustion, memory reuse, and leak prevention
- Zero-overhead compile-time LUT bitmask verification
- Native CPU throughput & operation latency profiling

Every commit is tested against the matrix: Default (16-bit), 8-bit/8-slots, 32-bit/16-slots, Deferred Callbacks, Split Arrays, Polling-Only Footprint, and Unoptimized fallbacks.

### Verification Guidelines

1. **Static Validation**: Verify brace and parenthesis balance, strict C89 rules (all variable declarations at block start), `_ASYNC_SAVE_SREG()` / `_ASYNC_REST_SREG()` pairing, and ensure no CodeVisionAVR reserved words are used as variable identifiers.
2. **Firmware Verification**: The reference project `async_delay_test.c` serves as the integration benchmark in CodeVisionAVR / Proteus, validating CTC timer generation, ISR countdown, deferred callback execution, and cancellation without memory leaks.
3. **Logic Invariants**: Ensure critical logic properties (wrap-around arithmetic, atomic reads, phase-locked periodic re-arm, and separate used/active masks) are preserved across all modifications.

---

## 9. Project rules (from CLAUDE.md — follow these)

1. **Manual Compilation & Verification**: CodeVisionAVR compiles AVR targets directly on the user's environment. Always state verification limits honestly.
2. **Speed is the top priority**: You MAY trade more RAM/Flash if the change is genuinely worth it. After any optimization/refactor, report an estimated cycle/speed change vs. before.
3. **Always read `ARCHITECTURE.md`** before changes.
4. **Configurability**: Every optional feature or optimization gets a flag (config macro) so the user can enable/disable it.
5. Prefer **clean, efficient algorithms**.
6. Main library file is `async_delay.h`.

## 10. Editing conventions

- Keep files small; functions focused.
- Match existing style: CodeVisionAVR C, 4-space indent, `//` comments,
  `(void *)0` for NULL, `#asm("cli")` inline asm. No `u`/`U` literal suffixes —
  the rest of the codebase and the CVAVR headers do not use them.
- **Declare all locals at the top of a function.** CodeVisionAVR follows C89
  block rules, so a declaration after a statement is an error. When a local only
  exists under some flag combination, wrap the declaration in the same `#if`.
- **`bit` is a CodeVisionAVR type specifier, not a free identifier.** So are
  `flash`, `eeprom`, `sfrb`, `sfrw`, `interrupt`, `funcused`. Naming a variable
  `bit` produces invalid type combination errors. Use descriptive names like
  `slotbit` instead.
- **`#asm(...)` does not reliably survive macro expansion** in CVAVR. Write the
  literal `#asm("cli")` at each call site and keep only the SREG save/restore in
  macros (§6.3).
- Multi-line `#define` continuations and nested `#if` inside a function body both
  work cleanly in CodeVisionAVR.
- Surgical changes only; don't refactor unrelated lines.
- Don't remove the atomic-read, wrap-safe, phase-locked-re-arm, or
  counter-increment-first logic (section 6) — all correctness-critical.
- After changing the API, flags, or limits, update **`README.md`** and **`async_delay_guide.md`** as well to keep documentation in sync.
- Increment `ASYNC_DELAY_VERSION` when making major library revisions.
