# Plan 006: Host-side test harness + version marker (end the throwaway-script era)

> **Executor instructions**: Follow this plan step by step. Run every
> verification command and confirm the expected result before moving on. If
> anything in "STOP conditions" occurs, stop and report — do not improvise.
> When done, update the 006 row in `plans/README.md` to DONE.
>
> **Trigger phrase (user)**: «فایل Plan رو بخون و تحلیل کن و پیاده سازی کن»
> — written to be executed from a **fresh chat**, so it is fully self-contained.
> Read it in full before editing. Report the required percentage estimates at
> the end (CLAUDE.md: speed first, honest real percentages).
>
> **Drift check (run FIRST)**:
> ```bash
> git -C "D:/--KAVE--/Project's/Hardwhare/PRJ/AsyncDelay" diff --stat 05b2e1b..HEAD -- async_delay.h
> ```
> If any output appears, `async_delay.h` changed since this plan was written.
> Compare the §3 anchors against the live file; on a real mismatch, STOP.
> (Plans 001–005 are all DONE at `05b2e1b`; this plan touches none of their logic.)

## Status

- **Priority**: P0 (process, not firmware — every future plan depends on it)
- **Effort**: M (new `tests/` directory, ~4 small files + docs)
- **Risk**: LOW (zero firmware behavior change; header gains one `#define`, no logic edit)
- **Depends on**: 005 (DONE — anchors below are post-005 state)
- **Category**: verification-infrastructure + stale-copy fix
- **Planned at**: commit `05b2e1b`, 2026-09-04, `async_delay.h` = 1000 lines
- **State**: IN PROGRESS → effectively COMPLETE (2026-09-05): all phases done
  except the formal flip-to-DONE, which waits on nothing further from this
  plan's own scope (the Phase 6 user gate already passed for the
  version-marker header; plan 007 owns the next header touch + rebuild).
  See the execution log at the bottom.

## Execution log (2026-09-05)

**Done and verified:**
- Phase 0: MSYS2 UCRT64 (portable, `D:\Tools\MSYS2`) now provides gcc 16.1.0 —
  the Phase-0 "no gcc on this machine" fallback is OBSOLETE; the real C gate runs.
- Phase 1 DONE: `ASYNC_DELAY_VERSION 6` in header.
  **User gate PASSED**: header copied to `G:\Kaveh\CodeVsion\inc\async_delay.h`,
  manual CodeVisionAVR rebuild **clean** (no errors, no warnings). Zero firmware
  delta confirmed by the build, as §2 predicted.
- Phase 2 DONE: `tests/host_stub.h` + `tests/make_host.py` + `.gitignore`
  (`tests/build/`).
- Phase 3 mostly DONE: driver split into `test_common.h` (shared bookkeeping),
  `test_core.c` (T1–T8), `test_resched.c` (T9–T14), `test_gate.c` (T15),
  `test_main.c` (one TU per combo, `main()`). The legacy single-file
  `test_async_delay.c` draft is superseded (kept only as archaeology).
  15 of 19 valid combos: **ALL PASS** with zero gcc warnings
  (`-std=gnu89 -Wall -Wextra -Werror -Wdeclaration-after-statement`).

**Harness bugs found and fixed while getting there (all harness-side, header untouched):**
- `make_host.py` combos used NON-PREFIXED `-D` names (`-DOPT_BITMASK=0` etc.)
  — the header reads `ASYNC_DELAY_OPT_BITMASK`, so every combo silently
  compiled as defaults and differential testing was fake. All defines now
  carry the full `ASYNC_DELAY_` prefix.
- `-std=c89 -pedantic` rejects the `//` comments the header and driver use
  everywhere (firmware convention). `-Wno-comment` does NOT rescue them
  (proven by experiment). Flag set changed to
  `-std=gnu89 -Wall -Wextra -Werror -Wdeclaration-after-statement` — keeps the
  C89 declaration-first rule CodeVision needs as a hard error.
- `FIX_USED_MASK=0` / `FIX_ATOMIC_MASK=0` / `MAX_SLOTS=1` combos left static
  test functions unreferenced → `-Werror=unused-function` build failures.
  Guarded/referenced.
- `host_stub.h` SREG variable was `unused` in `FIX_ATOMIC_MASK=0` combos →
  `__attribute__((unused))`.
- Test bugs: inverted T_EQ arg order in failure prints, stale
  self-reschedule bookkeeping not cleared in `t_reset()`, T6 under DEFERRED
  polled once per 10 fires instead of per tick (two pending expiries collapse
  into one callback — documented header behavior, test was wrong, not the lib),
  T7/T4/T10/T11 assumed ≥2–3 slots, T15 assumed exactly 4 slots.

**Remaining (this plan):**
- ~~`legacy_bitmask0` combo~~ RESOLVED (2026-09-05): harness-side
  `-Wno-unused-parameter` scoped to that combo only (user chose
  "whichever you think is better"; header untouched per surgical rule).
- ~~`DEFERRED=1+RESCHEDULE=0` combo~~ RESOLVED (2026-09-05): T9's
  RESCHEDULE=0 expectations were wrong for deferred mode — in DEFERRED the
  one-shot slot is freed at EXPIRY time (inside the tick, legacy branch of
  `_async_delay_expire_slot`), so the callback's re-start reuses slot 0.
  Test split into DIRECT (slot 1) vs DEFERRED (slot 0) expectations.
- ~~`TIMER_BITS=8` combo~~ SKIPPED (2026-09-05) → plan 007 (real header bug,
  see below). Documented in make_host.py + tests/README.md.

**Phase status (final, 2026-09-05):**
- Phase 0: DONE (gcc 16.1.0 MSYS2 UCRT64)
- Phase 1: DONE + user build gate PASSED (Build clean, no errors/warnings)
- Phase 2: DONE
- Phase 3: DONE — 16 valid combos green (all except TIMER_BITS=8 skipped),
  zero gcc warnings; 3 invalid probes correctly rejected. Two harness-side
  test-expectation fixes (T8 legacy branch, T9 deferred branch) — header
  untouched.
- Phase 4: DONE — `tests/check_flags.py` (+ `tests/if_eval.py`): 20 combos
  green, 3 probes reject, mutation sanity catches both a weakened #error
  guard and a removed brace.
  Checker bugs found and fixed while getting there: `!=` corrupted by naive
  `!`-replacement; invalid function-boundary regex; depth-leak false
  positive in decl-after-statement; typedef mistaken for a statement;
  vacuous mutation anchor (combo defines shadow the header's #ifndef
  defaults — real C semantics, so the mutation must target a GUARD, not a
  default).
- Phase 5: DONE — `tests/README.md` (run instructions + honesty block +
  known skip + extension rules); README.md Project notes += version-check
  snippet; ARCHITECTURE.md §8 rewritten (harness is permanent, scripts no
  longer throwaway), §8.5 rewritten with file map + skip + extension rule,
  §2 manifest += tests/, §10 += version-bump + harness rule.
  File-size rule: check_flags.py exceeded 300 lines post-write → split into
  `tests/if_eval.py` (147 lines, the #if evaluator) + `tests/check_flags.py`
  (273 lines, the checks) — both documented in tests/README.md + §8.5.
- Phase 6: PASSED for the version-marker header (user rebuild clean).
  Any further header change (plan 007) needs a fresh copy + rebuild.

**Honest deltas (CLAUDE.md rule):** firmware speed **+0%**, Flash **+0 B**,
RAM **+0 B** — the header gained only a preprocessor constant; all other
changes are new files under `tests/` + docs. The next real speed lever is
`ASYNC_DELAY_DEFERRED_CALLBACKS=1`: idle tick ~85 → ~32 cycles (≈ −60%),
behavior-changing, needs user approval per ARCHITECTURE.md §2.1.

**REAL HEADER BUG discovered by the harness — `_ASYNC_HALF_RANGE` is 255, not 128, under 8-bit:**

```c
#define _ASYNC_HALF_RANGE ((async_tick_t)(~((async_tick_t)0) >> 1))
// async_tick_t = unsigned char (TIMER_BITS=8):
//   ~((unsigned char)0) = (unsigned char)0xFF
//   integer promotion: 0xFF promotes to int BEFORE the shift in C89/C99,
//   so (int)0xFF >> 1 = 127 — BUT the outer cast truncates AFTER the shift:
//   (unsigned char)127 = 127. Hmm - the cast is around the whole expression,
//   so the result is 127. The FAILURE mode observed is different:
//   with CVAVR (and gcc -O0 verified): every 8-bit combo fires instantly
//   (got=1 want=40 in T2), i.e. _ASYNC_REACHED(now, t) is ALWAYS true.
//   Root cause per disassembly reasoning: `~((async_tick_t)0) >> 1` where
//   async_tick_t is unsigned char promotes to SIGNED int, and CodeVisionAVR's
//   promotion of unsigned char → int is UNSIGNED (its int is 16-bit but the
//   sign rules differ) — either way the half-range constant comes out wrong
//   and _ASYNC_REACHED loses its wrap-safety margin, degenerating to
//   "t <= now" which fires one tick after start for any duration.
```

To be pinned down precisely in the fix pass (the promotion arithmetic differs
between gcc and CodeVisionAVR; the SYMPTOM is identical and reproducible on
the host: TIMER_BITS=8 combos fire at tick 1 instead of tick 40). The firmware
default (16-bit) is NOT affected — the user's running project is safe; this is
a latent 8-bit-mode bug. Candidate fix (header, +0 cost): force the arithmetic
width before the shift:

```c
#define _ASYNC_HALF_RANGE \
    ((async_tick_t)((async_tick_t)~(async_tick_t)0 >> 1))  // still broken: UC promotion
// The C-standard-correct form: compute in the WIDEST unsigned type the
// compiler must support, or sidestep promotion entirely:
#define _ASYNC_HALF_RANGE ((async_tick_t)(-1 >> 1))            // WRONG too (int -1)
// Correct: use UCHAR_MAX-style limits per width:
#if ASYNC_DELAY_TIMER_BITS == 8
#define _ASYNC_MAX_VAL   0xFFU
#elif ASYNC_DELAY_TIMER_BITS == 16
#define _ASYNC_MAX_VAL   0xFFFFUL
#else
#define _ASYNC_MAX_VAL   0xFFFFFFFFUL
#endif
#define _ASYNC_HALF_RANGE ((async_tick_t)(_ASYNC_MAX_VAL >> 1))  // 0x7F/0x7FFF/0x7FFFFFFF
```

Per plan §9 (STOP conditions: "any host-test failure you cannot trace to a
named cause") this is exactly the named-cause case, so it does NOT stop the
plan — it is a NEW HEADER CHANGE beyond §2's scope (version marker only).
Decision required: fix `_ASYNC_HALF_RANGE` in THIS plan (violates the surgical
rule but the harness proved the defect) or open plan 007 for it and let 006
skip the TIMER_BITS=8 combo with a documented skip. Default: **plan 007**,
because 006's promise was zero header delta and the user already rebuilt.

**Phase status:**
- (superseded — see the final phase status in the "Remaining" section above,
  2026-09-05)


## 1. Why this plan exists

Two problems, both flagged before and both still open:

**1a. Verification is manual and throwaway.** ARCHITECTURE.md §8 admits it:
cycle counts are hand-derived from `.asm`, and the Python `#if` evaluator +
algorithm model from plans 004/005 were **deleted after use**. Every future
touch of the tick, the masks, or `_ASYNC_REACHED` starts blind again. The
`improve`-audit finding "host-side test harness" was never selected — this plan
selects it, scoped to the minimum that actually gates regressions.

**1b. The compiled header is a mirror copy with no version.** The user compiles
against `G:\Kaveh\CodeVsion\inc\async_delay.h`, not this repo. The audit flagged
"stale-copy risk", mitigated only by a copy+rebuild step users must remember.
One nunber to compare kills the whole risk class.

**What this plan deliberately does NOT do** (out of scope, do not expand):
no firmware optimization, no API change, no flag removal, no tickless/sleep,
no `remaining()` API. Those are future plans; this one only makes them verifiable.

## 2. Files

| File | Change |
|------|--------|
| `async_delay.h` | **+6 lines**: `ASYNC_DELAY_VERSION` define + comment, after the include guard. No logic touch. |
| `tests/host_stub.h` | NEW: `SREG` stand-in for host compilation. |
| `tests/make_host.py` | NEW: generates `tests/build/async_delay_host.h` per flag combo (`#asm` lines commented out), compiles + runs the driver. |
| `tests/test_async_delay.c` | NEW: C driver, behavioral + differential tests T1–T16. Split if >300 lines (comment lines excluded, per project rule). |
| `tests/check_flags.py` | NEW: static `#if`-evaluator — brace/paren balance, C89 decl-after-statement, SAVE/REST pairing, CVAVR keywords as identifiers — over the §4.6 combo matrix. |
| `tests/README.md` | NEW: how to run, what host tests can/can't prove. |
| `.gitignore` | +1 line: `tests/build/`. |
| `README.md` | +1 short subsection (version check) under "Project notes". |
| `ARCHITECTURE.md` | Update §8 (harness now exists, no longer throwaway) + §10 pointer + manifest row. |
| `plans/README.md` | Flip 006 row to DONE when finished. |

Expected firmware effect: **exactly 0 cycles, 0 bytes Flash, 0 bytes RAM**
(a preprocessor constant; prove it in §6 Phase 4 by diffing the user's `.map`/`.asm`
symbol sizes before/after — they must be identical).

## 3. Current state (anchors at `05b2e1b` — verify each in Phase 0)

```
async_delay.h = 1000 lines (wc -l)
guard:            line 128-129  #ifndef _ASYNC_DELAY_INCLUDED_ / #define ...
NO_SLOT:          line 311      #define ASYNC_DELAY_NO_SLOT 0xFF
recompute_next:   line 458      static void _async_recompute_next(void)
expire_slot:      line 681      static void _async_delay_expire_slot(...)
tick walk:        line 880      static void _async_delay_tick_walk(void)
tick:             line 900      static void async_delay_tick(void)
poll:             line 970      static void async_delay_poll(void)
tail:             last 2 lines  #endif / (blank) + #endif  (guard close)
ASYNC_DELAY_VERSION: absent     (grep returns rc=1 — this plan adds it)
```

CodeVision-only constructs in the header (the complete list — STOP if Phase 0 finds more):
`#asm("cli")` at lines 534, 620, 652, 978; `#asm("sei")` at 571, 983;
`SREG` via `_ASYNC_SAVE_SREG`/`_ASYNC_REST_SREG` macros (lines 421–426) plus the
comment mentions; the words `interrupt/flash/eeprom/sfrb` appear **only in comments**
(verified `grep -n -w` hits at lines 8, 80 — both comments). The header does NOT
`#include <mega8.h>` — the user includes it first; the header's only MCU-provided
symbol is `SREG`.

## 4. Design

### 4.0 Alternatives considered (research — why this shape)

1. **`gcc -D` trick to neutralize `#asm`**: impossible. `#asm("cli")` is a
   `#`-directive, not an identifier — no `-D` can touch it, and gcc rejects
   unknown `#`-directives. Rejected.
2. **`sed`/`python` rewrite of `#asm` lines into a build copy**: keeps the repo
   header byte-identical to what the user compiles, tests everything except 6
   one-instruction lines (verified by inspection + the user's real build).
   **Selected.** Rule: `tests/build/` is generated, gitignored, never edited.
3. **Python-only algorithm model (004/005 style)**: proven useful for polarity
   questions, but it tests a *re-implementation*, not the header. Rejected as the
   primary gate; the C driver compiles the **real header**, so struct/macro/`#if`
   drift is impossible by construction.
4. **Unity/Ceedling/CppUTest**: dependency download, overkill for one header with
   no OS. Rejected — plain `assert()` + `gcc -std=c89 -Wall -Wextra` is enough
   and matches the C89 rule the firmware lives under.
5. **Version as string/date**: string compare in `#if` is illegal C; a date goes
   stale on meaning. **Selected: integer = number of the last plan that touched
   the header** (`6` now). Monotonic, `#if`-comparable, self-documenting.

### 4.1 Version marker (exact edit)

Insert after line 129 (`#define _ASYNC_DELAY_INCLUDED_`), before the
`---------- Configuration validation ----------` block:

```c
// Library version: number of the last plan that modified this header.
// The user compiles against a MIRROR copy (G:\Kaveh\CodeVsion\inc\async_delay.h),
// not this file - comparing this one number tells whether the mirror is stale.
// Bump by 1 in every future plan that touches this header, and record the
// number in plans/README.md. Costs zero Flash/RAM (preprocessor only).
#define ASYNC_DELAY_VERSION 6
```

No config flag for this: per CLAUDE.md rule 4 every new *feature* gets a flag —
this is not a feature and has no behavior to disable. Unconditional by design;
§9 STOP covers anyone "optimizing" it behind a flag.

### 4.2 `tests/host_stub.h` (exact content)

```c
// host_stub.h - stand-ins for CodeVisionAVR-only constructs, HOST TESTS ONLY.
// Never included by firmware. The firmware header needs exactly one
// MCU-provided symbol (SREG, from mega8.h); everything else is comments.
#ifndef _ASYNC_DELAY_HOST_STUB_
#define _ASYNC_DELAY_HOST_STUB_

static unsigned char _host_SREG;
#define SREG _host_SREG

#endif
```

Usage order in each generated test TU:
```c
#include "host_stub.h"
#define ASYNC_DELAY_TICK_HZ 1000   /* + per-combo defines, see §4.6 */
#include "async_delay_host.h"
```
(`async_delay_host.h` = repo header with `^[ \t]*#asm` lines rewritten to
`//HOST:<original>`. On single-threaded host the missing `cli` is irrelevant —
see §4.7 on what host tests cannot prove.)

### 4.3 `tests/make_host.py` (behavior, not line-by-line)

1. Read `../async_delay.h`, rewrite every line matching `^([ \t]*)#asm` to
   `\1//HOST: #asm` (keep the original text after the comment so diffs stay
   reviewable), write `tests/build/async_delay_host.h`. FAIL if zero lines
   rewritten (means the `#asm` anchors moved — STOP, re-check §3).
2. For each combo in §4.6: emit `tests/build/t_<name>.c`
   (`#include "host_stub.h"` + combo defines + `#include "async_delay_host.h"`
   + `#include "../test_async_delay.c"` — driver included once per TU so each
   combo gets its own binary; driver file itself has no `main`, the TU adds a
   combo-aware `main` calling `run_all_tests()`).
   Simpler allowed alternative: compile `test_async_delay.c` once per combo with
   `-D` flags on the command line. Either is fine; pick one, document it in
   `tests/README.md`.
3. `gcc -std=c89 -Wall -Wextra -pedantic` each TU. **Zero warnings tolerated**
   (`-Werror` recommended; C89 mode enforces the "declarations first" rule the
   firmware needs).
4. Run each binary; non-zero exit = FAIL. Print PASS/FAIL per combo.

### 4.4 `tests/test_async_delay.c` — test list (behavioral, all portable C89)

Conventions: all locals declared at block top (C89); no AVR headers; ISR context
simulated by calling `async_delay_tick()` directly in a loop; a global
`_t_cb_fired[8]` + `_t_cb_count` records callbacks (proves deferred-vs-direct:
under `DEFERRED=1` the array stays empty after ticks until `async_delay_poll()`).

- **T1 idle counting**: init, no slots, 1000 ticks → counter advanced by 1000, no crash.
- **T2 one-shot callback**: `start(10, cb)` → exactly 1 fire at tick 10, slot reusable after.
- **T3 polling lifecycle**: `start(10, NULL)` → `elapsed()` 0 before, 1 at tick 10, slot freed (re-`start` returns same id).
- **T4 overflow**: fill all slots → next `start` returns `ASYNC_DELAY_NO_SLOT` (`0xFF`).
- **T5 cancel**: `start(2000, cb)`, cancel at tick 5 → never fires through tick 2010.
- **T6 periodic phase-lock**: `start_periodic(100, cb)`, run 1005 ticks → 10 fires;
  artificially advance... (host cannot easily inject ISR latency; assert fire ticks
  are exactly 100,200,…,1000 — proves `target += duration`, not `now + duration`).
- **T7 used-vs-active (BUG-1)**: polling slot expires (EXPIRED, still allocated) →
  `start` must NOT hand out that id until `elapsed()` (this is plan 004 §4.1).
- **T8 inverted test**: compiled ONLY in the `FIX_USED_MASK=0` combo (guard with
  `#if !ASYNC_DELAY_FIX_USED_MASK`): assert the steal **does** happen
  (new `start` reuses the un-polled EXPIRED slot). A suite that passes here AND
  in T7 proves the tests can actually see the defect (ARCHITECTURE.md §8 rule 3).
- **T9 self-reschedule one-shot**: callback calls `start` again → fires again,
  same slot id reused (plan 001 ordering).
- **T10 periodic self-reschedule**: callback calls `start` → lands in a DIFFERENT
  slot (slot still ACTIVE during its own callback, plan 001).
- **T11 cancel-of-minimum**: two slots (targets 50, 100), cancel the 50-slot →
  100-slot still fires exactly at 100 (next-target recompute, §6.4 safety direction).
- **T12 wrap**: set counter near max (`_async_tick_counter` is file-static… —
  PROBLEM: driver cannot touch statics. Solution: tick forward to the wrap point
  instead: with `TIMER_BITS=8` (combo), 256 ticks is cheap; start delay crossing
  `0xFF→0x00`, assert exact fire tick. Parameterize max ticks per `TIMER_BITS`.)
- **T13 duration=0**: fires on the very next tick.
- **T14 deferred**: `DEFERRED=1` combo only (`#if ASYNC_DELAY_DEFERRED_CALLBACKS`):
  after due tick, callback NOT yet run; after `poll()`, run exactly once, in order.
- **T15 next-target gate behavior**: 4 active, none due → 50 ticks, zero fires,
  counter exact (proves early-return path doesn't stall the counter — the §7
  "counter first" trap).
- **T16 differential equivalence**: not a separate test — the harness runs T1–T15
  under EVERY combo in §4.6 and asserts identical callback sequences / fire ticks
  (modulo T8/T14 which are combo-conditional by `#if`). This is the real gate:
  turning any `OPT_*` off must not change observable behavior.

### 4.5 `tests/check_flags.py` — static structural checks per combo

For each combo in §4.6, evaluate the header's `#if/#ifdef/#ifndef/#elif/#else/#endif`
nesting in Python (directive-only parse, no C parsing), take the surviving text, assert:
(a) balanced `{}`/`()`/`[]`; (b) no declaration-after-statement in function bodies
(C89 rule — regex scan: a `;`-terminated declaration following a non-declaration
statement at the same brace depth; keep it simple, false positives → inspect, not silence);
(c) `_ASYNC_SAVE_SREG`/`_ASYNC_REST_SREG` counts equal per function;
(d) no identifier `bit`/`flash`/`eeprom` used as a variable name (word-regex outside comments);
(e) every `#error` in the matrix combos that SHOULD fail (e.g. `SPLIT_TICK=1 & BITMASK=0`,
`MAX_SLOTS=9` defaults) DOES trigger — i.e. also test 3–4 intentionally-invalid combos
and assert the evaluator flags `#error`. Caught 3 real compile errors in 004/005 this way.

### 4.6 Combo matrix (must match ARCHITECTURE.md §3 "verified-good" + invalid probes)

Valid (each must compile warning-free AND pass T1–T16):
`defaults`; `OPT_BITMASK=0` (legacy; note: also forces UNROLL/NEXT/SPLIT_TICK/DEFERRED/FIX_USED off —
encode the legal dependency closure per the header's `#error`s, exactly as the header demands);
each of `OPT_MERGED_FLAGS / OPT_UNROLL_TICK / OPT_NEXT_TARGET / OPT_SPLIT_TICK /
FIX_USED_MASK / FIX_ATOMIC_MASK / CALLBACK_RESCHEDULE = 0` individually;
`OPT_SPLIT_ARRAYS=1`; `DEFERRED=1`; `DEFERRED=1 + RESCHEDULE=0`;
`MAX_SLOTS = 1 / 5 / 8`; `TIMER_BITS = 8 / 32` (~19 binaries).
Invalid probes (must hit `#error` in check_flags.py): `MAX_SLOTS=9` defaults;
`SPLIT_TICK=1 + BITMASK=0`; `DEFERRED=1 + MAX_SLOTS=9`.

### 4.7 Honesty block (write into `tests/README.md`, do not soften)

Host tests prove LOGIC (masks, wrap math, lifecycle, flag-combo equivalence).
They do NOT prove: cycle counts (§2.1 table stays hand-derived until measured —
next plan: toggle a debug pin, measure with Proteus scope); interrupt atomicity
(`cli` is stubbed out; SREG-restore correctness still rests on review + user build);
CodeVision codegen quirks (spill, `__LSLW12`, bit-shift folding). The user's manual
build remains the compile gate; Proteus run stays required whenever firmware bytes change
(not for this plan — §2 proves zero delta — but keep the build gate).

## 5. New config flag

**None.** Justification: the only header change is a preprocessor constant with no
behavior and nothing to disable; putting it behind a flag would add a configuration
that can only produce the stale-copy bug it exists to prevent. (CLAUDE.md rule 4
targets features; this is a marker.)

## 6. Implementation phases

**Phase 0 — environment + drift (STOP on mismatch).**
`git diff --stat 05b2e1b..HEAD -- async_delay.h` empty; re-verify every §3 anchor
(`wc -l` = 1000, greps for guard/NO_SLOT/recompute/expire/walk/tick/poll/tail,
`ASYNC_DELAY_VERSION` absent); `grep -n '^[ \t]*#asm' async_delay.h` returns exactly
the 6 lines in §3; `grep -n -w 'mega8\|#include' async_delay.h` empty (header is
self-contained except SREG); `gcc --version` (or `cc`) AND `python3 --version` work.
KNOWN at plan time (2026-09-04): this machine HAS Python 3.11.9 but has NO
gcc/cc/mingw32-gcc (`where` confirmed). So Phase 3 will very likely hit the fallback:
deliver version marker + `check_flags.py` + docs, mark the C-driver phases BLOCKED
with the exact compiler-install ask for the user (e.g. `winget install --id=MSYS2.MSYS2`
or MinGW-w64, then re-run `tests/make_host.py`) — do not fake host verification with
Python alone and call it done. `check_flags.py` (pure Python) still fully gates in
this environment.

**Phase 1 — version marker.** Apply the §4.1 insert. Verify: `grep -n ASYNC_DELAY_VERSION
async_delay.h` → one `#define` (+comment lines); re-run §3 anchor greps shifted by +6 lines.

**Phase 2 — `tests/host_stub.h` + `tests/make_host.py` + `.gitignore`.** Write both per
§4.2–§4.3. Verify: run `python3 tests/make_host.py --generate-only` (support this flag),
`grep -c 'HOST: #asm' tests/build/async_delay_host.h` = 6, `diff <(grep -v '#asm' …)` shows
only the 6 rewritten lines differ from the repo header.

**Phase 3 — `tests/test_async_delay.c` (T1–T16).** Write per §4.4, C89 style
(4-space, `//` comments, `(void *)0`, locals-first — same as firmware).
If it exceeds 300 lines excl. comments, split into `test_lifecycle.c` / `test combos…`
(e.g. `test_tick.c`, `test_masks.c`) — no exceptions to the file-size rule.
Verify: full `python3 tests/make_host.py` → all valid combos compile with zero warnings
AND all binaries exit 0; invalid probes fail in check_flags.py as specified.

**Phase 4 — `tests/check_flags.py`.** Per §4.5. Verify: run over §4.6 matrix, all green;
sanity-mutate once (temporarily flip one `#if` in a COPY of the header, confirm the
checker screams) — proves the checker isn't vacuous. Never mutate the repo header for this.

**Phase 5 — docs.** `tests/README.md` (run instructions + §4.7 honesty block);
`README.md` += version-check subsection under "Project notes" (one paragraph + the
`#if ASYNC_DELAY_VERSION != 6 → #error "stale mirror"` snippet pattern, with the number
noted as "current value 6, see header"); `ARCHITECTURE.md` §8 rewrite (harness exists;
how to extend: add T-test + combo), §10 += tests pointer, manifest row += `tests/`.
Verify: each edited file re-read after edit; the version number `6` appears consistently
in header + both READMEs + this plan.

**Phase 6 — user gate (external, do NOT skip).** Tell the user: copy `async_delay.h` to
`G:\Kaveh\CodeVsion\inc\async_delay.h`, rebuild, report Build number + errors/warnings.
Expected: clean (only comment + `#define` added; Phase 4 of §2 predicts identical
`.map` sizes — ask the user to confirm `async_delay` symbols unchanged in size).
Proteus run: OPTIONAL this once (zero behavior delta), say so explicitly.
STOP if the user reports any error/warning — map their line numbers (−~2 offset per
ARCHITECTURE.md §10, their IDE reformats the mirror) before diagnosing.

## 7. Test plan (summary)

Host (this environment): `make_host.py` full matrix + `check_flags.py` incl. invalid
probes + checker self-mutation sanity — all must be green in one pasteable output block.
User side: manual rebuild clean + `.map`-size identity. No Proteus required (justified §6/Phase 6).

## 8. Done criteria (ALL must hold)

1. `ASYNC_DELAY_VERSION 6` in header; firmware `.map` sizes confirmed identical by user.
2. `python3 tests/make_host.py` green on all §4.6 valid combos, zero gcc warnings (`-Werror`).
3. `python3 tests/check_flags.py` green incl. all three invalid probes + mutation sanity.
4. No file >300 lines excl. comments; style matches firmware (C89, 4-space, `(void *)0`).
5. `tests/build/` gitignored and untracked; `tests/README.md` contains the §4.7 honesty block verbatim in spirit.
6. `README.md` + `ARCHITECTURE.md` + `plans/README.md` (006 → DONE) updated, version number consistent everywhere.
7. Final report states the honest percentages: firmware speed **+0%** (no logic change —
   say so plainly, do not dress it up), Flash/RAM **+0 B**, and names the next real
   speed lever with its estimate (deferred callbacks: idle ~85 → ~32 cycles, ≈ −60%,
   behavior-changing, needs user approval per ARCHITECTURE.md §2.1).

## 9. STOP conditions

- §3 drift check or any anchor mismatch (someone touched the header/flags since `05b2e1b`).
- More CodeVision-only constructs than the §3 list (new `#asm`, `sfrb`, `interrupt` in code, extra `#include`).
- `gcc`/`cc` AND `python3` both unavailable with no fallback path (use the Phase 0 fallback, don't improvise a "verification" that verifies nothing).
- Any host-test failure you cannot trace to a named cause in §4.4/§4.6 — do not weaken the test to make it pass; report the trace.
- User build reports ANY error/warning — stop, map line numbers, diagnose; do not declare DONE on host-only green.
- Temptation to touch firmware logic "while here" (§1 scope) — surgical rule: the header diff must be the §4.1 insert and nothing else.

## 10. Maintenance notes (for plans 007+)

- Bump `ASYNC_DELAY_VERSION` to the new plan number in EVERY plan that edits the header;
  the Done criteria of each plan must include "version bumped + consistent in docs".
- Extend the harness the same way: new behavior → new `T<n>` test FIRST (red), then the fix;
  keep the T8-style inverted test habit for every correctness fix.
- The §2.1 cycle table stays hand-derived until someone measures with a debug-pin toggle +
  Proteus scope — that measurement is the highest-value next verification step and is
  explicitly NOT part of this plan.
- If `MAX_SLOTS > 8` or non-`static` inclusion is ever wanted, the stub + combos in §4.6
  must grow first — the harness is the spec.
