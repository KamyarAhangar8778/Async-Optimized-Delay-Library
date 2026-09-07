# tests/ — host-side test harness (plan 006)

Compiles the **real** `async_delay.h` on the host (x86, gcc) and runs behavioral
tests over the flag-combo matrix. The library is never modified to test it —
`#asm(...)` lines are commented out in a generated copy.

## How to run

gcc must be on PATH (MSYS2 UCRT64 at `D:\Tools\MSYS2` on this machine):

```bash
export PATH="/d/Tools/MSYS2/ucrt64/bin:$PATH"
python tests/make_host.py        # generate + compile + run the full matrix
python tests/check_flags.py      # static structural checks (pure Python, no gcc)
```

`make_host.py --generate-only` stops after writing `build/async_delay_host.h`
(no gcc needed — useful for drift checks).

What a full green run looks like: every valid combo prints `ALL PASS` + `[ok]`,
every invalid combo prints `[ok:invalid] ... correctly rejected`, and
`summary: 0 failure(s)`.

## What is tested (T1–T15)

| Test | Proves |
|------|--------|
| T1 | idle counting: counter advances every tick with no slots |
| T2 | one-shot fires exactly once, at the exact due tick, slot reusable |
| T3 | polling lifecycle: `elapsed()` 0→1, slot freed on read |
| T4 | overflow: full slots → `start` returns `ASYNC_DELAY_NO_SLOT` |
| T5 | cancel: cancelled delay never fires |
| T6 | periodic phase-lock: fires at exact multiples (`target += duration`) |
| T7 | used-vs-active: EXPIRED-but-unpolled slot is NOT handed out by `start` |
| T8 | inverted: with `FIX_USED_MASK=0` the theft DOES happen (harness can see the bug); under `BITMASK=0` the legacy state-byte path is asserted correct instead |
| T9 | self-reschedule from a one-shot callback (slot reuse per RESCHEDULE/DEFERRED mode) |
| T10 | self-start from a periodic callback lands in a DIFFERENT slot |
| T11 | cancel-of-minimum: survivor still fires exactly on time |
| T12 | counter wrap: delay crossing the wrap fires exactly once |
| T13 | `duration=0` fires on the very next tick |
| T14 | deferred: callback NOT run before `poll()`, exactly once after |
| T15 | next-target gate: active-but-not-due → zero fires, counter exact |

T16 is not a separate test — it is the matrix itself: every combo runs
T1–T15, so turning any `OPT_*` off must not change observable behavior.

## File map

- `make_host.py` — generator/runner; combo matrix (source of truth for
  verified-good combinations). Every `-D` carries the full `ASYNC_DELAY_`
  prefix — a bare `-DOPT_BITMASK=0` silently compiles defaults instead
  (this bit us once).
- `if_eval.py` — directive-only C preprocessor evaluator + comment/string
  blanking, shared by the checker (kept in its own file to honor the 300-line
  rule).
- `check_flags.py` — static structural checks per combo: brace balance, C89
  declaration-after-statement, `_ASYNC_SAVE_SREG`/`_ASYNC_REST_SREG` pairing
  per function, CVAVR keywords (`bit`/`flash`/`eeprom`/...) as identifiers —
  plus 3 intentional `#error` probes and a mutation-sanity check
  (a mutated COPY of the header must make it scream; the repo header is never
  mutated).
- `host_stub.h` — `SREG` stand-in (the only MCU-provided symbol the header
  needs).
- `test_common.h` + `test_core.c` + `test_resched.c` + `test_gate.c` +
  `test_main.c` — the C89 driver; one TU per combo (the library is
  file-static, so each binary owns one library instance).
- `build/` — generated, gitignored, never edited. Delete freely; regenerate
  by re-running the scripts.

## 8-bit counter width (fixed by plan 007)

`TIMER_BITS=8` IS in the matrix since plan 007. The harness originally exposed
a real header bug there — `_ASYNC_HALF_RANGE` degenerated under 8-bit integer
promotion and every 8-bit delay fired one tick after start (T2 got=1 want=40).
The macro is now per-width literals (0x7F/0x7FFF/0x7FFFFFFF) and the combo
passes T1–T15. Note: the 8-bit config is host-verified but has never been
compiled by CodeVisionAVR — a clean CV build of that config remains the gate
if anyone ever ships it.

## What host tests can and cannot prove (honesty block — do not soften)

Host tests prove **LOGIC**: masks, wrap math, lifecycle, flag-combo
equivalence. They do NOT prove:

- **Cycle counts** — the ARCHITECTURE.md §2.1 table stays hand-derived until
  someone measures with a debug-pin toggle + Proteus scope.
- **Interrupt atomicity** — `cli`/`sei` are stubbed out (`//HOST:` comments)
  and single-threaded host execution makes SREG save/restore meaningless.
  SREG-restore correctness rests on review + the user's CodeVisionAVR build.
- **CodeVisionAVR codegen quirks** — register spills (`__SAVELOCR`),
  `__LSLW12`, bit-shift folding. Only the real compiler's `.asm` shows those.

The user's manual CodeVisionAVR build remains the compile gate; run Proteus
whenever firmware bytes change.

## Extending

New behavior → write the new `T<n>` test FIRST and watch it fail, then fix
the header and watch it pass. Keep the T8-style inverted test for every
correctness fix. New combo in ARCHITECTURE.md §3? Add it to BOTH
`make_host.py` and `check_flags.py` — the two matrices must mirror each other.

Compiler flags: `-std=gnu89 -Wall -Wextra -Werror -Wdeclaration-after-statement`.
`-Wno-unused-parameter` is added ONLY for `legacy_bitmask0` (the legacy
expiry path genuinely never uses its `clr` mask parameter) — keep it scoped
to that combo; never weaken the header to silence a warning.
