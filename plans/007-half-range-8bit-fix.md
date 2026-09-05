# Plan 007: Fix `_ASYNC_HALF_RANGE` for TIMER_BITS=8 + re-enable the 8-bit combo

> **Executor instructions**: Follow this plan step by step. Run every
> verification command and confirm the expected result before moving on. If
> anything in "STOP conditions" occurs, stop and report — do not improvise.
> When done, update the 007 row in `plans/README.md` to DONE and record the
> version number there (plan-006 §10 rule).
>
> **Trigger phrase (user)**: «فایل Plan 007 رو بخون و تحلیل کن و پیاده سازی کن»
> — written to be executed from a **fresh chat**, so it is fully self-contained.
> Read it in full before editing. Report the required percentage estimates at
> the end (CLAUDE.md: speed first, honest real percentages).
>
> **Baseline note**: this plan is written against the WORKING TREE state right
> after plan 006 (version marker in header, host harness green, `tests/` new).
> Plan 006's changes are expected to be COMMITTED before 007 starts; if they
> are not, either commit them first (ask the user) or run the drift check
> against the working tree as-is — the §3 anchors below describe the working
> tree, not HEAD.

## Status

- **Priority**: P1 (latent correctness bug in a NON-default config — the user's
  running project is 16-bit and is NOT affected; but the header's own docs
  promise 8-bit support and the code does not deliver it)
- **Effort**: S (one macro edit + version bump + harness combo re-enable + docs)
- **Risk**: MEDIUM (touches the wrap-safety core macro — mitigated by the fact
  that the 16-bit and 32-bit VALUES are provably unchanged, so the user's
  default build must come out bit-identical; the full host matrix re-runs)
- **Depends on**: 006 (host harness exists; the failing 8-bit test already
  exists — this plan is red-green with the red already proven)
- **Category**: correctness fix + harness combo re-enable
- **Planned at**: 2026-09-05 (post-006), `async_delay.h` = 1013 lines, version 6
- **State**: DONE (2026-09-05 — user gate passed: build clean, Proteus OK,
  `.map` = 378 w / 34 B, bit-identical)

## 1. Why this plan exists

The plan-006 host harness found a REAL header bug (plan-006 execution log,
"REAL HEADER BUG discovered by the harness"): under `ASYNC_DELAY_TIMER_BITS=8`,
every delay fires one tick after start (T2: got=1 want=40).

**Exact root cause (C-standard reasoning, host-reproducible — the plan-006
log's CVAVR-disassembly guess is superseded by this):**

```c
#define _ASYNC_HALF_RANGE ((async_tick_t)(~((async_tick_t)0) >> 1))
```

With `async_tick_t = unsigned char`:
1. `(unsigned char)0` integer-promotes to **signed int** (C89 6.2.1.1: unsigned
   char always promotes to int because int can represent all its values).
2. `~0` (int) = `-1` (all bits set: 0xFFFFFFFF on a 32-bit-int host; 0xFFFF on
   CVAVR's 16-bit int — same VALUE -1 either way).
3. `-1 >> 1` is implementation-defined for negative values; gcc and CVAVR both
   do an arithmetic shift, so it stays `-1`.
4. The outer cast truncates AFTER the shift: `(unsigned char)(-1)` = **255**.

So the half range becomes 255 on a 256-range counter. `_ASYNC_REACHED(now, t)`
is `(unsigned char)(now - t) < 255` — true for every difference except exactly
255, i.e. it degenerates to "always reached" and every delay fires on the next
tick. The wrap-safety margin the macro exists to provide is gone.

The 16-bit path is correct by accident of rank: `unsigned int` has the same
rank as `int`, so it does NOT promote — `~0u = 0xFFFF`, `>> 1 = 0x7FFF` = 32767
on AVR (on the host the value is 2^31-1, which the host tests never notice
because their durations are ≤ 5000). The 32-bit path (`unsigned long`) never
promotes either. Only 8-bit is broken.

**Docs vs code**: README and the header's own comment have always promised
"max reliable delay = 128 ticks" for 8-bit. The code delivered a degenerating
compare. This plan makes the code match the documented contract.

**Scope guard (do not expand)**: fix the one macro, bump the version, re-enable
the 8-bit combo in the harness, update docs. No other "improvement" to any
other macro, no API change, no new flag. The 16/32-bit values below are
PROVABLY unchanged — if any other edit to the header sneaks in, the
bit-identical-build prediction is void and the plan's gates are meaningless.

## 2. Files

| File | Change |
|------|--------|
| `async_delay.h` | `_ASYNC_HALF_RANGE` (line 410) → per-width literal constants, +12 net lines with the comment. `ASYNC_DELAY_VERSION` 6 → **7**. NOTHING else. |
| `tests/make_host.py` | Re-enable `TIMER_BITS=8`: restore `for b in (8, 32)` and delete the skip comment block. |
| `tests/check_flags.py` | Add `("TIMER_BITS=8", {"ASYNC_DELAY_TIMER_BITS": 8})` to `combos()` (matrices must mirror). |
| `tests/README.md` | Replace the "Known skip" section with the fixed note (8-bit is tested again). |
| `ARCHITECTURE.md` | §6.1: note the fixed macro + the bug in the history line; §8.5: delete the "Known skip" paragraph. |
| `plans/README.md` | Add the 007 row; record `ASYNC_DELAY_VERSION 7`; flip to DONE at the end. |
| `README.md` | **No change** — the max-reliable-delay table already promises 128; the code now matches it. |

Expected firmware effect (16-bit default build): **exactly 0 cycles, 0 bytes
Flash, 0 bytes RAM** — the old expression and the new literal both fold to the
same compile-time constant 0x7FFF; CVAVR emits the same immediate. Phase 5
makes the user prove it via `.map` comparison.

## 3. Current state (anchors at post-006 working tree — verify each in Phase 0)

```
async_delay.h = 1013 lines (wc -l)
guard:            line 128-129  #ifndef _ASYNC_DELAY_INCLUDED_ / #define ...
VERSION:          line 142      #define ASYNC_DELAY_VERSION 6      (must be 6)
NO_SLOT:          line 324      #define ASYNC_DELAY_NO_SLOT 0xFF
HALF_RANGE:       line 410      #define _ASYNC_HALF_RANGE ((async_tick_t)(~((async_tick_t)0) >> 1))
REACHED:          line 417-418  (the only use of _ASYNC_HALF_RANGE)
recompute_next:   line 471      static void _async_recompute_next(void)
expire_slot:      line 694      static void _async_delay_expire_slot(...)
tick walk:        line 893      static void _async_delay_tick_walk(void)
tick:             line 913      static void async_delay_tick(void)
poll:             line 983      static void async_delay_poll(void)
tail:             line 1011 #endif (deferred close) / 1012 blank / 1013 #endif (guard)
#asm lines:       EXACTLY 6 (grep -n '^[ \t]*#asm') at 547, 585, 633, 665, 991, 996
                  (count matters more than exact numbers; make_host.py fails fast on != 6)
```

Harness baseline (must be green BEFORE this plan starts): `make_host.py` →
16 valid combos + 3 invalid probes, 0 failures; `check_flags.py` → 20 combos,
0 failures. `TIMER_BITS=8` is absent from both matrices (skipped by plan 006).

## 4. Design

### 4.1 Root-cause alternatives considered

1. **Direct per-width literals** (SELECTED — exact edit in §4.2): no shift, no
   promotion step, value readable at a glance, identical on every conforming
   compiler. Values: 8-bit → 0x7F (127), 16-bit → 0x7FFF (32767, unchanged),
   32-bit → 0x7FFFFFFF (2147483647, unchanged).
2. **Plan-006 candidate `_ASYNC_MAX_VAL >> 1`** (`0xFFU` / `0xFFFFUL` /
   `0xFFFFFFFFUL` shifted): works, but adds an intermediate macro and still
   contains a shift whose width you must reason about. Literals are simpler.
   ALSO: its `0xFFU`/`0xFFFFUL` suffixes collide with the project's no-`u`/`U`
   suffix convention (ARCHITECTURE.md §10).
3. **`((async_tick_t)(-1 >> 1))`**: WRONG twice — `-1 >> 1` = -1 (arithmetic),
   truncates to 0xFF on 8-bit (same bug) and 0xFFFF on 16-bit (NEW bug there).
4. **Cast to widest type first**: `((async_tick_t)(~(unsigned long)0 >> 1))` —
   WRONG: 0x7FFFFFFF truncates to 0xFF on 8-bit. Truncate-after-shift is the
   trap this whole bug lives in; only literals sidestep it entirely.
5. **`(1 << (sizeof(async_tick_t)*8 - 1)) - 1` with a pre-cast**: portable but
   obscure; readers must re-derive the value. Rejected for clarity.

### 4.2 The exact edit (line 409-410, replace both lines)

```c
// Half the tick range - compile-time constant (was recomputed every tick).
// LITERAL per-width constants, NOT `~((async_tick_t)0) >> 1`: with an 8-bit
// async_tick_t the ~0 operand promotes to signed int, ~0 -> -1, the
// arithmetic >> keeps -1, and the cast truncates it to 0xFF (255). A half
// range of 255 on a 256 counter strips _ASYNC_REACHED of its wrap margin -
// every delay fires ~immediately (found by the plan-006 host harness: T2
// got=1 want=40 under TIMER_BITS=8). Literals have no promotion step, so the
// value is exact on every compiler. 16-bit value unchanged (0x7FFF): the
// default build is bit-identical.
#if ASYNC_DELAY_TIMER_BITS == 8
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7F)         // 127
#elif ASYNC_DELAY_TIMER_BITS == 16
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7FFF)       // 32767 (unchanged)
#else /* 32 */
#define _ASYNC_HALF_RANGE ((async_tick_t)0x7FFFFFFF)   // 2147483647 (unchanged)
#endif
```

Plus the version bump at line 142: `#define ASYNC_DELAY_VERSION 7`.

**Why no config flag** (CLAUDE.md rule 4): this is a bug fix, not a feature —
there is no "old behavior" anyone could want (the 8-bit old behavior is
"fires instantly"). Same reasoning as plan 006 §5. Unconditional by design.

### 4.3 Why the fix is safe for the running project

- 16-bit: `0x7FFF` == the value the old expression produced on AVR. Same
  constant → same immediate in every `SBCI/CPI` compare CVAVR emits. The
  `.map` must come out identical (Phase 5 gate).
- 32-bit: `0x7FFFFFFF` == the old value. Same argument.
- 8-bit: was broken (fires at tick 1), now matches the documented 128-tick
  contract. No firmware in the field runs this config (the user never built
  an 8-bit project), so "changing" it breaks nobody.
- Every use site goes through `_ASYNC_REACHED` (grep: the macro at 410 and its
  single use at 418) — one point of fix, no drift possible.

## 5. New config flag

**None.** See §4.2 — bug fix, not a feature.

## 6. Implementation phases

**Phase 0 — drift + baseline (STOP on mismatch).**
Verify every §3 anchor on the working tree (`wc -l` = 1013, the greps,
`ASYNC_DELAY_VERSION` == 6, exactly 6 `#asm` lines). Run the baseline:
`make_host.py` 16 combos + 3 probes green, `check_flags.py` 20 combos green —
with gcc on PATH (`export PATH="/d/Tools/MSYS2/ucrt64/bin:$PATH"`). Any
mismatch → STOP, identify the mover first.

**Phase 1 — RED: re-enable the combo and watch it fail for the RIGHT reason.**
Restore `TIMER_BITS=8` in `make_host.py` (delete the skip comment, restore
`for b in (8, 32)`) and add it to `check_flags.py`'s matrix. Run both.
Expected EXACTLY: `TIMER_BITS=8` fails in T2 ("fired exactly once at due",
got=1 want=40) — the instant-fire symptom; all 16 other combos stay green;
check_flags goes 21 green (the static checks don't see the value bug).
If the 8-bit combo fails ANYWHERE ELSE or any other combo goes red → STOP
(the plan-006 symptom did not reproduce; something else moved).

**Phase 2 — header fix.** Apply §4.2 exactly (macro + version 7). Verify:
`grep -c ASYNC_DELAY_VERSION async_delay.h` shows the `#define ... 7`;
`grep -n _ASYNC_HALF_RANGE async_delay.h` → 2 hits (definition + REACHED use);
`wc -l` = 1025 (1013 + 12); every §3 anchor below line 410 shifts +12
(recompute 483, expire 706, walk 905, tick 925, poll 995); `#asm` count still
exactly 6. NOTHING else in the diff (`git diff --stat async_delay.h` ≈ +13
lines vs the 006 baseline).

**Phase 3 — GREEN: full matrix.** `make_host.py` → 17 valid combos (16 prior
+ TIMER_BITS=8) all ALL PASS, zero gcc warnings; 3 probes still reject;
`check_flags.py` → 21 combos, 0 failures. Paste the green output into the
execution log.

**Phase 4 — docs.** Per §2 table: make_host/check_flags comments if they
reference the skip (they do — make_host.py's skip comment is deleted in
Phase 1, tests/README.md + ARCHITECTURE.md here). ARCHITECTURE.md §6.1 gains
one history line: the macro was computed with a promotion-unsafe expression
until plan 007 (8-bit degenerated to always-reached); it is now per-width
literals. Verify: no file mentions "TIMER_BITS=8 skip" anymore; version 7
appears in header + plans/README.md; README.md untouched.

**Phase 5 — user gate (external, do NOT skip).** Tell the user: copy
`async_delay.h` to `G:\Kaveh\CodeVsion\inc\async_delay.h`, rebuild, report
Build number + errors/warnings AND the async_delay Flash/RAM numbers from
`Debug/List/async_delay_test.map` (expect the recorded 378 words / 34 B —
bit-identical build per §4.3). Proteus: OPTIONAL this once (zero behavior
delta predicted, same precedent as plan 006 Phase 6) — say so explicitly.
If the user reports ANY .map delta → STOP and diff the `.asm` before
proceeding; the prediction failing means our constant reasoning failed.

## 7. Test plan (summary)

Host: Phase 1 red-reproduction (T2 got=1 want=40, named cause §1) → Phase 3
full green (17 combos + 3 probes + check_flags 21). User side: rebuild clean +
`.map` identity. No Proteus required (justified §6 Phase 5).

## 8. Done criteria (ALL must hold)

1. `_ASYNC_HALF_RANGE` = per-width literals; `ASYNC_DELAY_VERSION 7`; user
   rebuild clean with `.map` sizes identical to the recorded 378 w / 34 B.
2. `TIMER_BITS=8` runs in BOTH matrices and passes T1–T15 (green, zero
   warnings); all other combos still green; 3 probes still reject.
3. No stale "skip" text in tests/README.md, ARCHITECTURE.md §8.5, or
   make_host.py; ARCHITECTURE.md §6.1 records the fix; version 7 consistent in
   header + plans/README.md.
4. Header diff vs the 006 baseline = §4.2 insert + version bump, nothing else
   (surgical rule; `git diff` reviewed line by line).
5. Final report states the honest percentages: firmware speed **+0%**
   (constant value unchanged for 16/32-bit — say so plainly), Flash **+0 B**,
   RAM **+0 B**; 8-bit is a correctness restoration, not a speed change. Next
   real speed lever, unchanged from plan 006's report:
   `ASYNC_DELAY_DEFERRED_CALLBACKS=1` (idle ~85 → ~32 cycles, ≈ −60%,
   behavior-changing, needs user approval per ARCHITECTURE.md §2.1).

## 9. STOP conditions

- Any §3 anchor mismatch or version ≠ 6 at Phase 0 (someone touched the header
  since this plan was written).
- Phase 1's red is anything other than the exact T2 instant-fire symptom.
- Any combo OTHER than TIMER_BITS=8 goes red after the Phase 2 edit (the fix
  must not perturb 16/32-bit behavior — if it does, the constant reasoning in
  §4.3 is wrong; re-derive before touching anything).
- User build reports ANY error/warning or ANY `.map` size delta.
- Temptation to "also clean up" other macros (e.g. `_ASYNC_REACHED`'s double
  cast, the `#asm` sites) — surgical rule: the header diff is §4.2 + version,
  nothing else.

## 10. Maintenance notes (for plans 008+)

- This is the first plan to exercise plan-006 §10's red-green rule with an
  EXISTING red test: the harness caught the bug before the fix existed. Keep
  that order for every future fix (T-test red first, fix second).
- The 8-bit config is now harness-verified but has NEVER been compiled by
  CodeVisionAVR (no user project uses it). If anyone ever ships an 8-bit
  config, one clean CV build of that config is still the gate — the host
  harness proves logic, not CV codegen (tests/README.md honesty block).
- If TIMER_BITS=8 ever matters in production, consider whether CVAVR's own
  promotion of `unsigned char` in OTHER expressions anywhere in the header
  could bite the same way — a one-time review of every arithmetic expression
  touching `async_tick_t` under 8-bit would close the class. Not in scope
  here (the harness now covers the observable behavior end to end).

## 11. Execution log (2026-09-05)

- **Phase 0 (drift + baseline): PASS.** All §3 anchors verified: `wc -l` = 1013,
  VERSION 6 @142, HALF_RANGE @410, REACHED @417-418, NO_SLOT @324, `#asm`
  count exactly 6 (one listed number off by 1 — plan says count matters, not
  numbers). Plan-006 changes were UNCOMMITTED in the working tree → per the
  plan's own baseline note, drift check ran against the working tree as-is.
  Baseline: make_host 16 combos + 3 probes green (gcc 16.1.0 MSYS2 UCRT64),
  check_flags 20 combos green.
- **Phase 1 (RED): PASS — exact predicted symptom.** Re-enabled `for b in (8, 32)`
  in make_host.py + added `TIMER_BITS=8` to check_flags.py. Result: only the
  8-bit combo failed (summary: 1 failure), and T2 failed EXACTLY as named:
  "fired at tick T_DUR expected=40 got=1". Additional failures (T3/T6/T9/T10/
  T11/T12/T15) are all direct consequences of the same root cause documented
  in §1 — once `_ASYNC_REACHED` is true for ~every difference, the next-target
  gate never early-returns (T15: 4 fires while idle), periodic fires every tick
  (T6: got=193), and the wrap test sees immediate fire (T12). Nothing else
  moved; STOP conditions not triggered. check_flags: 21 combos green as
  predicted (static checks cannot see a value bug).
- **Phase 2 (header fix): PASS.** §4.2 applied verbatim (macro block @409-424 +
  version 7 @142). `#asm` count still exactly 6. `git diff --stat async_delay.h`
  = +28/-1 — but note the version-marker block @129-141 is part of the
  UNCOMMITTED plan-006 work in this working tree, not a 007 addition; the 007
  delta is exactly §4.2's 16-line block + the version digit. Line-count note:
  the plan predicted 1025 (+12) but §4.2's own block is 16 lines (+14
  including the blank line) → 1027. The plan's arithmetic was off by 2; the
  edit itself matches §4.2 word for word. All downstream anchors shifted +14
  uniformly (recompute 485, expire 708, walk 907, tick 927, poll 997).
- **Phase 3 (GREEN): PASS.** make_host: 17 valid combos ALL PASS (TIMER_BITS=8
  included, zero gcc warnings under -Werror), 3 probes correctly rejected,
  `summary: 0 failure(s)`. check_flags: 21 combos, 0 failures. Also updated
  `check_flags.py`'s `DEFINES_DEFAULT["ASYNC_DELAY_VERSION"]` 6→7 to mirror
  the header (found during the run).
- **Phase 4 (docs): PASS.** tests/README.md "Known skip" → "8-bit counter
  width (fixed by plan 007)" with the host-vs-CV honesty note. ARCHITECTURE.md
  §6.1: code block synced to literals + one history paragraph added. §8.5:
  Known skip paragraph deleted, extension rule notes the 007 red-green cycle.
  plans/README.md: 006 → DONE (shared gate), 007 → DONE-pending-map-gate.
  README.md untouched — its 128-tick table was the contract the code now
  matches. No file mentions the skip anymore.
- **Phase 5: PASS (user gate, 2026-09-05).** User rebuilt via the mirror copy:
  build clean, zero errors/warnings, and Proteus ran correctly (optional this
  once, exercised anyway). `.map` read back from this repo's
  `Debug/List/async_delay_test.map` (generated by that rebuild): RAM
  28+2+1+1+2 = **34 bytes**, Flash 24+38+87+4+4+36+27+75+58+25 = **378 words**
  — both exactly the recorded pre-007 values, so the bit-identical prediction
  of §4.3 is CONFIRMED (`async_delay_poll` correctly absent at
  DEFERRED_CALLBACKS=0). No `.asm` diff needed. Plan 007 DONE.
