# Plan 002: Fix CPUClock mismatch in async_delay_test.cwp (16 vs 8 MHz)

> **Executor instructions**: Follow this plan step by step. Run every
> verification command and confirm the expected result before moving to the
> next step. If anything in the "STOP conditions" section occurs, stop and
> report — do not improvise. When done, update the status row for this plan
> in `plans/README.md` unless a reviewer told you they maintain the index.
>
> **Drift check (run first)**: `git diff --stat 338a1c8..HEAD -- async_delay_test.cwp async_delay_test.c`
> If any output appears (either file changed since this plan was written),
> compare the "Current state" excerpts against the live files; on a mismatch,
> treat it as a STOP condition.

## Status

- **Priority**: P3
- **Effort**: S
- **Risk**: LOW
- **Depends on**: none
- **Category**: docs/config
- **Planned at**: commit `338a1c8`, 2026-09-04

## Why this matters

The test project is built and configured for an **8 MHz** ATmega8, but the
CodeWizard configuration file `async_delay_test.cwp` records `CPUClock=16`. The
`.cwp` file is what CodeWizardAVR re-reads to regenerate peripheral init code. If
the user ever re-runs the wizard on this project, it will generate timer init code
for 16 MHz, but the project's comment (async_delay_test.c:28) and the `.prj`
(`CPUClock=8000000`) both assume 8 MHz. Because OCR/compare values depend on the
clock (125 kHz vs 250 kHz timer clock → `OCR2 = 124` vs `249`), a 16 MHz
regeneration silently produces a **2 ms tick** instead of 1 ms — every delay in the
app doubles.

Fix: align `CPUClock` to 8, and anchor the 8 MHz assumption with a short comment
in `async_delay_test.c` so the mismatch cannot silently recur.

## Current state

Excerpt — `async_delay_test.cwp` lines 1–7 (the mismatch is line 6):

```ini
[CodeWizard]
Toolset=AVR
[Chip]
Type=ATmega8
CPUClock=16
```

Excerpt — `async_delay_test.c` lines 27–29 (the 8 MHz assumption, correct):

```c
// Timer2 at 8 MHz: prescaler /64 -> timer clock = 125 kHz
// 1 ms = 125 ticks -> OCR2 = 125 - 1 = 124
#define OCR2_1MS  124
```

Excerpt — `async_delay_test.c` lines 1–14 (the file header comment where the
anchor note belongs):

```c
/*******************************************************
async_delay Library Test Project
Chip type           : ATmega8
Clock frequency     : 8.000000 MHz (internal RC oscillator)

Tests the async_delay library:
  - Timer2 CTC generates 1ms ticks
  ...
*******************************************************/
```

Supporting facts (context only — do not edit these):
- `async_delay_test.prj` lines 141 & 476: `CPUClock=8000000` (8 MHz) — already
  correct.
- `.cwp` is the CodeWizardAVR wizard-state file, distinct from `.prj` (the
  compiler build file). The wizard uses `.cwp` when regenerating init code.
- The guide `async_delay_guide.md` documents the clock→OCR dependency
  (125 kHz → 124, 250 kHz → 249) and warns that re-running the wizard on the wrong
  clock silently changes the tick period.

Repo conventions to match:
- `.cwp` is INI-style (`[Section]` / `Key=Value`) — change the value only, keep
  formatting.
- `async_delay_test.c` uses C block comments `/* ... */` for its header banner and
  `//` for inline comments; tabs for indentation in code.

## Commands you will need

There is **no build command** in this environment (CodeVisionAVR IDE + no AVR
toolchain installed). Verification is grep + read-only checks.

| Purpose | Command | Expected on success |
|---------|---------|---------------------|
| Drift check | `git diff --stat 338a1c8..HEAD -- async_delay_test.cwp async_delay_test.c` | no output |
| Value fixed | `grep -n "CPUClock" async_delay_test.cwp` | `CPUClock=8` |
| Anchor present | `grep -n "8.000000 MHz" async_delay_test.c` | ≥1 match (the header line) |
| Consistency | `grep -n "CPUClock" async_delay_test.prj` | `CPUClock=8000000` (unchanged) |
| Status clean | `git status --short` | only `async_delay_test.cwp` + `async_delay_test.c` (plus `plans/`) modified |

## Scope

**In scope** (the only files you should modify):
- `async_delay_test.cwp` — change `CPUClock=16` to `CPUClock=8` (line 6).
- `async_delay_test.c` — add a short comment line noting the 8 MHz requirement.

**Out of scope** (do NOT touch, even though they look related):
- `async_delay_test.prj` — already correct (8000000); leave it.
- `async_delay_guide.md` — informational; optional to update, not required.
- `G:\Kaveh\CodeVsion\inc\async_delay.h` — the user's mirror; never edit it.
- Any timer init code in `async_delay_test.c` — it is already correct for 8 MHz.

## Git workflow

- Branch: `advisor/002-fix-cpuclock-cwp-mismatch` (repo has no observed branch
  convention; use this).
- One commit. Message style (match `git log` short subjects):
  `Align CodeWizard CPUClock to 8 MHz in test project (was 16)`
  with a body line `Co-Authored-By: Claude Code <noreply@anthropic.com>`.
- Do NOT push or open a PR unless the operator instructed it.

## Steps

### Step 1: Fix the value in `async_delay_test.cwp`

Edit line 6 from:

```ini
CPUClock=16
```

to:

```ini
CPUClock=8
```

Do not change any other line in the file.

**Verify**: `grep -n "CPUClock" async_delay_test.cwp` → `CPUClock=8`

### Step 2: Anchor the 8 MHz requirement in `async_delay_test.c`

In the file-header comment block, add one line under the `Clock frequency` line
(after line 4):

```c
Clock frequency     : 8.000000 MHz (internal RC oscillator)
// KEEP this at 8 MHz: Timer2 OCR2 = 124 below is calculated for 125 kHz.
// If you change the clock, recompute OCR2 and update async_delay_test.cwp CPUClock.
```

Note: use `//` comments inside the existing `/* ... */` banner (the file already
mixes `//` inline comments with the `/* */` header — the exact comment style is
flexible; the content is what matters).

**Verify**: `grep -n "8.000000 MHz" async_delay_test.c` → at least 1 match
(the original header line, now with the note below it).

### Step 3: Manual verification (external — user action)

In your final report, tell the user: **"No rebuild strictly required (this is a
config + comment change), but rebuild `async_delay_test` in CodeVisionAVR and
confirm No errors, and verify the tick is still 1 ms (OCR2=124)."** If the user
re-runs the CodeWizard later, the generated init code will now match 8 MHz.

## Test plan

No automated tests exist for this repo (CodeVisionAVR IDE + hardware/Proteus
testing). The change is a one-line config value plus a comment; the compile gate
is the user's manual build.

## Done criteria

ALL must hold:

- [ ] `grep -n "CPUClock" async_delay_test.cwp` → `CPUClock=8`
- [ ] `grep -n "CPUClock" async_delay_test.prj` → `CPUClock=8000000` (unchanged)
- [ ] `grep -n "8.000000 MHz" async_delay_test.c` → ≥1 match
- [ ] `git status --short` → only `async_delay_test.cwp` + `async_delay_test.c` (plus `plans/`) modified
- [ ] `plans/README.md` status row for 002 updated to DONE

## STOP conditions

Stop and report back (do not improvise) if:

- `async_delay_test.cwp` line 6 is not `CPUClock=16` (drift since `338a1c8`).
- The user's `.prj` shows a different clock (if the project moved to 16 MHz
  intentionally, this fix would be wrong — report and ask).
- You're tempted to also "fix" anything in `async_delay_test.c` beyond the one
  comment — out of scope.

## Maintenance notes

- The `.cwp` file is regenerated/re-saved whenever the user runs CodeWizardAVR, so
  the value can drift again. If the user later regenerates for a real 16 MHz
  board, they must change `CPUClock`, recompute `OCR2` (to 249), and update both
  `.prj` and the header comment in one pass.
- Reviewer focus: confirm the diff touches only the one config line and the
  comment — no stray edits to timer code.
