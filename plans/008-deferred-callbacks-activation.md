# Plan 008: Activate `ASYNC_DELAY_DEFERRED_CALLBACKS=1` on the test project + measure the real ISR saving

> **Executor instructions**: Follow this plan step by step. Run every
> verification command and confirm the expected result before moving on. If
> anything in "STOP conditions" occurs, stop and report — do not improvise.
> When done, update the 008 row in `plans/README.md` to DONE and record the
> version number there.
>
> **Trigger phrase**: user asked "what next", executor chose this from
> ARCHITECTURE.md §2.1's "one remaining lever" and the user said
> «فقط قبلش برای اون Plan بنویس» — so the plan exists BEFORE any edit.
>
> **Baseline note**: written against the post-007 working tree
> (commit `5b22bc9`), `async_delay.h` = 1027 lines, version 7.

## Status

- **Priority**: P1 (largest remaining speed lever: predicted idle tick
  ~85 → ~32 cycles, ≈ −60% ISR cost; also closes the "never compiled by
  CodeVisionAVR" honesty gap for the deferred config)
- **Effort**: S (one `#define` + one `poll()` call in the test project + docs +
  measurement; ZERO header edits — the deferred path already exists and is
  host-tested)
- **Risk**: LOW-MEDIUM (behavior change is REAL and must be stated: LED0's
  callback now runs from the main loop, not the ISR. The test project is the
  vehicle precisely because its behaviors are observable. Header untouched →
  no version bump, `.map` predictability is not a gate here — Flash/RAM WILL
  change by design, that is the measurement, not a failure.)
- **Depends on**: 007 (harness green incl. deferred combos; commit `5b22bc9`)
- **Category**: performance activation + measurement + firmware-gate closure
- **Planned at**: 2026-09-05 (post-007), `async_delay.h` = 1027 lines, version 7
- **State**: DONE (2026-09-05 — build clean, Proteus OK, .map 397w/35B, measured in Debug/List/*.map+*.asm; header version 7 unchanged)

## 1. Why this plan exists

ARCHITECTURE.md §2.1 records the trade since plan 004: the tick ISR saves
11 registers + SREG (~54 cycles/tick) purely because the tick may `ICALL` a
user callback. With `ASYNC_DELAY_DEFERRED_CALLBACKS=1` the ISR never calls —
it only sets a bit in `_async_pending_mask`; the app drains callbacks from
`async_delay_poll()` in main context. Predicted idle tick: ~85 → ~32 cycles.
The flag exists, the logic is host-proven (T14: callback NOT run before
`poll()`, exactly once after; deferred combos green in both matrices), but:

1. **It has never been turned on in the firmware test project.** The §2.1
   numbers are hand-derived from `.asm` reading, and the deferred config has
   never been compiled by CodeVisionAVR at all (same honesty gap plan 007
   closed for 8-bit — see plans/007 §10).
2. The **real measured** idle-tick cost with DEFERRED=1 is the missing number
   in §2.1. This plan fills it from the generated `.asm`, the same way 004/005
   did, then records it.

**Behavior change (must be stated to the user, CLAUDE.md rule: this flag is
precisely why user approval is required):**

| Aspect | DEFERRED=0 (current) | DEFERRED=1 (this plan) |
|---|---|---|
| Callback context | ISR | main loop (via `async_delay_poll()`) |
| Callback latency | 0 (same tick) | ≤ 1 main-loop iteration |
| Repeated expiry between polls | each fires | collapses into one call |
| Callback restrictions | flags/pins only, no LCD/delay_ms | none (may do anything) |
| App obligation | none | MUST call `async_delay_poll()` in the main loop |

For the test project: `cb_led0_toggle` only flips a flag — it is deferred-safe
by construction. Latency change is invisible on an LED. Collapse risk needs
poll() to be skipped for >500ms, impossible: the main loop only blocks on
LCD writes (~ms scale).

**Scope guard (do not expand)**: flip the flag in the test project, add the
one `poll()` call, update docs, measure. NO header edit, NO version bump
(header did not change — version stays 7), NO new flag (the flag already
exists), NO "improvement" to `async_delay_poll()`.

## 2. Files

| File | Change |
|------|--------|
| `async_delay.h` | **NO CHANGE. Version stays 7.** |
| `async_delay_test.c` | 3 surgical edits (§4.2): `#define ASYNC_DELAY_DEFERRED_CALLBACKS 1` in the config block; `async_delay_poll();` as first statement of the `while(1)` loop; comment updates (ISR warning + header comment). |
| `ARCHITECTURE.md` | §8 LED0 row → "deferred + callback"; §2.1 gains the measured DEFERRED=1 column/rows after Phase 5; §3 verified-good list note that DEFERRED=1 is now firmware-compiled. |
| `plans/README.md` | Add the 008 row (this plan), flip to DONE after Phase 5. |
| `README.md` | No change required (usage contract already documents DEFERRED=1 usage incl. the poll() obligation; nothing API-level changes). |

## 3. Current state (anchors — verify in Phase 0)

```
async_delay.h = 1027 lines (wc -l)
VERSION:          line 142   #define ASYNC_DELAY_VERSION 7   (must be 7)
poll():           line 997   static void async_delay_poll(void)  (inside #if DEFERRED)
pending mask:     line 378   static volatile unsigned char _async_pending_mask;
#asm lines:       EXACTLY 6
harness:          make_host 17 combos green incl. DEFERRED=1 and DEFERRED=1+RESCHEDULE=0;
                  check_flags 21 combos green
test project:     async_delay_test.c 193 lines; config block lines 22-25
                  (TIMER_BITS 16, MAX_SLOTS 4, TICK_HZ 1000); no DEFERRED define
                  anywhere; while(1) loop at line 145; ISR at line 70 calls
                  async_delay_tick() only
```

## 4. Design

### 4.1 Why activate in the test project rather than just measure

The §2.1 lever is only real if the app shape that uses it is proven on real
toolchain + Proteus. The test project is the reference app: it has a periodic
callback (LED0), polling slots (LED1, LCD), cancel, overflow — the exact
matrix DEFERRED=1 must survive. Activating it there gives: (a) the first real
CV compile of the deferred config, (b) a Proteus-visible behavior check, (c)
the measured `.asm` numbers for §2.1. A standalone measurement project would
prove (c) only.

### 4.2 The exact test-project edits (surgical, 3 sites)

1. Config block, after line 25 (`#define ASYNC_DELAY_TICK_HZ 1000`):
```c
#define ASYNC_DELAY_DEFERRED_CALLBACKS 1  // callbacks run from async_delay_poll() in main loop
```
2. Main loop, FIRST statement inside `while (1)`:
```c
        async_delay_poll();   // drain deferred callbacks (LED0) - MUST be first
```
   (First = lowest latency for the deferred callback; also keeps the loop's
   existing structure intact.)
3. Comments that lie under DEFERRED=1 get fixed:
   - test file header comment: add one line noting LED0 callback is deferred
     (runs from main loop).
   - `cb_led0_toggle` comment: "runs in ISR" → "deferred: runs from
     async_delay_poll() in main context".

`timer2_comp_isr` stays untouched — it still calls only `async_delay_tick()`
(that is the whole point: the ISR gets cheaper).

### 4.3 What the measurement will show (prediction, to be verified in Phase 5)

- `async_delay_poll` appears in Flash (`_G000` symbol), roughly 30-45 words
  (critical section + mask swap + 4-slot loop + ICALL).
- `timer2_comp_isr` shrinks slightly or stays (it already only calls the tick).
- `async_delay_tick` idle path: no `__SAVELOCR` reason remains tied to the
  callback (the walk was already split out in 005; the walk itself keeps its
  spills but is only entered when due). Predicted idle tick ≈ 32 cycles.
- RAM +1 byte: `_async_pending_mask` (0x01xx region), total 34 → **35 B**.
- `LED0` blink period in Proteus: unchanged 500ms (pending bit set at expiry,
  drained within one loop iteration — loop period is µs).

These are PREDICTIONS. Phase 5 records the real numbers; if the idle tick is
NOT ~32, the §2.1 claim gets corrected, not rationalized.

### 4.4 Why no new config flag

CLAUDE.md rule 4 asks a flag for every NEW feature. This plan adds no feature:
it turns on an existing, documented, harness-tested flag for the reference
app, which is exactly what that flag's contract requires (app adds one
poll() call). The enable point IS the flag.

## 5. Implementation phases

**Phase 0 — drift check.** Verify §3 anchors (1027 lines, version 7, 6
`#asm`, harness green, test-project anchors). Mismatch → STOP.

**Phase 1 — (no RED needed).** This is a config activation, not a logic
change: the deferred path already has its harness tests (T14) written in 006
and green. Re-running the matrix IS the regression gate here. Run both
scripts, confirm 17/21 green — this also re-proves the combo that is about to
be firmware-compiled.

**Phase 2 — test project edits.** Apply §4.2's 3 edits. Verify: `grep -n
DEFERRED async_delay_test.c` → exactly 1 define + comment line; `grep -n
async_delay_poll async_delay_test.c` → exactly 1 call site; nothing else
changed (`git diff` reviewed line by line).

**Phase 3 — docs (pre-gate).** ARCHITECTURE.md §8 LED0 row → deferred;
plans/README.md 008 row IN PROGRESS. §2.1 numbers WAIT for Phase 5 (no
unmeasured numbers go into the table — the file's own rule).

**Phase 4 — user gate.** Tell the user: rebuild the test project (mirror copy
of the header is UNCHANGED — no copy needed this time; only `async_delay_test.c`
changed... note: if the mirror is separate for the .c too, copy that file),
report errors/warnings, then run Proteus and confirm: LED0 blinks ~500ms,
LED1 blinks ~750ms, Loop counter runs, Ov:OK, Cnc:OK.

**Phase 5 — measure + record.** Read `Debug/List/async_delay_test.map`:
expect `_async_pending_mask` present (+1 B RAM → 35 B) and
`async_delay_poll_G000` present. Read `Debug/List/async_delay_test.asm`:
hand-derive the idle-tick cycle count (path: ISR entry → counter++ → mask
test → return) and the poll() cost. Update ARCHITECTURE.md §2.1 with a
DEFERRED=1 row/column and the honest percentages; flip plans/README.md 008 to
DONE; append the execution log to this file.

## 6. Test plan (summary)

Host: full matrix re-run (Phase 1). Firmware: user rebuild + Proteus
behavioral checks (Phase 4). Measurement: `.map` + `.asm` hand-derivation
(Phase 5). No new T-test needed — T14 already asserts the deferred contract;
the harness IS the red-green proof from 006.

## 7. Done criteria (ALL must hold)

1. Test project compiles clean (user reports no errors/warnings); Proteus:
   LED0 ~500ms, LED1 ~750ms, Loop counting, Ov:OK, Cnc:OK.
2. `.map` shows `_async_pending_mask` (35 B RAM) + `async_delay_poll_G000`.
3. ARCHITECTURE.md §2.1 carries the MEASURED DEFERRED=1 numbers (not the
   prediction, unless they match).
4. Header untouched: version 7, 1027 lines, `git diff async_delay.h` empty.
5. plans/README.md 008 DONE; execution log appended; honest percentage report.

## 8. STOP conditions

- Any harness combo red in Phase 1 (header drifted — find the mover first).
- User build errors, or Proteus shows LED0 not blinking (deferred drain broken
  in real codegen — diff the `.asm` of `async_delay_poll` before touching
  anything).
- Phase 5 idle-tick measurement wildly off prediction (>2× — means the
  §2.1/§6.7 spill model is wrong somewhere; re-derive, do not fudge).
- Temptation to "also" touch the header (poll() is fine as-is).

## 9. Maintenance notes (for plans 009+)

- If the user adopts DEFERRED=1 as the project default, future callback
  features must be written main-context-safe (reentrancy vs the main loop is
  then impossible, but latency/collapse semantics apply — document per API).
- The `.asm` measurement method (hand-count the idle path) stays the standard
  until a debug-pin + Proteus scope rig exists (§8.5 honesty block).

## 10. Execution log (2026-09-05)

- **Phase 0–1**: anchors verified (1027 lines, version 7, 6 `#asm`, test 193 lines); host harness re-run: 17 combos + 3 invalid probes green, check_flags 21 green.
- **Phase 2**: applied §4.2 exactly (config `#define`, `async_delay_poll()` at `while(1)` entry, comment fixes). `git diff async_delay_test.c` matches plan; `git diff async_delay.h` empty; harness still green.
- **Phase 3**: `ARCHITECTURE.md` §8 LED0 row → deferred (pre-gate). `plans/README.md` 008 → IN PROGRESS.
- **Phase 4 — user gate**: user rebuilt; **build clean, no errors/warnings**. Proteus: LED0 ~500ms, LED1 ~750ms, Loop counting, `Ov:OK Cnc:OK` — confirms deferred drain works. ✅
- **Phase 5 — measure (.map + .asm, Build 26, ATmega8 @ 8MHz, MAX_SLOTS=4, TIMER_BITS=16, DEFERRED=1)**:
  - `.map` Flash: async funcs `async_delay_init 26 + recompute 37 + start_common 87 + start 4 + start_periodic 4 + elapsed 36 + cancel 32 + expire_slot 53 + tick_walk 56 + tick 25 + poll 37 = 397w` (005 default was 378w → **+19w**, of which `poll 37` − `expire 22`).
  - `.map` RAM: `_async_slots 28 + tick_counter 2 + active 1 + used 1 + next_target 2 + pending_mask 1 = 35B` (was 34B → **+1B**, the pending mask at `0x0182`).
  - `.asm` ISR (`_timer2_comp_isr`): prologue unchanged — `ST -Y,R0/R1/R15/R22-R27/R30/R31` + `IN/ST SREG` (12 pushes). No smaller save-list produced. Body: `RCALL _async_delay_tick_G000` only. Epilogue: matching `LD/OUT/RETI`.
  - `.asm` tick (`_async_delay_tick_G000`): `NO locals`, no `__SAVELOCR` — same shape as 005. Path: `LDI/LD/ADIW/ST` counter increment → `LDS/CPI/BRNE RET` idle → `LDS*4 + RCALL SUBOPT_0x9 + BRLO RET` gate → `RCALL walk` when due.
  - `.asm` poll (`_async_delay_poll_G000`): `RCALL __SAVELOCR4` + `IN/cli + LDS/ST/OUT` pending mask swap + `CPI/BREQ` empty + `LDI/CPI/BRSH` loop head + `__LSLW12 + AND/BREQ + SUBOPT_0x17/0x8 + SBIW/BREQ + PUSH/POP/ICALL` per set bit. Only `ICALL` in the library lives here now.
  - **Finding**: the documented prediction `~85→~32 idle` **did not materialize** — CVAVR did not elide the ISR spill when `ICALL` left the ISR. `expire_slot` shrank 75→53 as expected (no direct callback), `poll` appeared at 37w, but ISR entry/exit stays ~54c. So **ISR speed +0%** for this plan; cost moved to main context (`poll` ~25c idle, + `ICALL` per firing). The real win is **what callbacks may now do** (LCD, delay_ms), plus the firmware gate for the config. Update made in `ARCHITECTURE.md` §2.1 reflects this honestly.
  - Header untouched: `1027 lines, version 7, git diff async_delay.h empty`. No version bump (header did not change — stated in §2 and §5).
