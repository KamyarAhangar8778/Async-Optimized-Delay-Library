#!/usr/bin/env python3
"""
make_host.py - generate + run host-side tests for async_delay.h (Plan 006).

WHAT THIS DOES, precisely:
  1. Reads ../async_delay.h and writes tests/build/async_delay_host.h with every
     top-level #asm(...) line commented out as `//HOST: #asm("...")`.
     (gcc does not understand CodeVisionAVR's #-directive `#asm`; on the
     single-threaded host ISR-disable is meaningless, so dropping it is safe.
     See plans/006 plan §4.7 for the honesty block: this does NOT validate
     interrupt atomicity - that still needs the user's CodeVision build.)
  2. For each combo in the COMBOS table, emits a one-line gcc invocation:
        gcc -std=gnu89 -Wall -Wextra -Werror -Wdeclaration-after-statement  \
            -D<combo defines> \
            tests/host_stub.h tests/test_async_delay.c \
            -o tests/build/t_<name>
     ...and runs it. Exits non-zero if ANY combo fails to build or run.
  3. With --generate-only it stops after step 1 (for the Phase 0/1 drift checks).

Exit codes:
  0  = all combos built + ran green
  1  = a build or run failed (details on stderr)
  2  = a combo hit one of the header's intentional #error (expected-fail probe)

The combo table here is the source of truth for "verified-good" combinations.
Mirror it exactly against plans/006 plan §4.6.
"""

import os, subprocess, sys, re, itertools, shutil

HERE   = os.path.dirname(os.path.abspath(__file__))
ROOT   = os.path.dirname(HERE)
BUILD  = os.path.join(HERE, "build")
SRC    = os.path.join(ROOT, "async_delay.h")
HOST_H = os.path.join(BUILD, "async_delay_host.h")
STUB   = os.path.join(HERE, "host_stub.h")
DRIVER = os.path.join(HERE, "test_main.c")

# One strip per combo: (label, [extra defines as -DNAME=VALUE ...])
# IMPORTANT: every flag the header reads is ASYNC_DELAY_-prefixed. A bare
# -DOPT_BITMASK=0 would define an unrelated macro and silently do nothing -
# the "combo" would just be the defaults again and differential testing would
# be fake. So every define below carries the full prefix.
# Defaults match the header's own #ifndef block (TIMER_BITS=16, MAX_SLOTS=4,
# TICK_HZ required, all flags default-1 unless overridden).
PB   = "ASYNC_DELAY_OPT_BITMASK"            # bitmask master switch
def combos():
    base = ["-DASYNC_DELAY_TICK_HZ=1000"]
    # legacy: every bitmask-driven opt off collapses to the pure linear scan.
    # The header's #error closure (lines ~281-302) DEMANDS all of these off with
    # BITMASK=0 - supply the whole legal closure explicitly so the combo compiles.
    yield ("legacy_bitmask0",
           base + ["-DASYNC_DELAY_OPT_BITMASK=0",
                   "-DASYNC_DELAY_OPT_UNROLL_TICK=0",
                   "-DASYNC_DELAY_OPT_NEXT_TARGET=0",
                   "-DASYNC_DELAY_OPT_SPLIT_TICK=0",
                   "-DASYNC_DELAY_FIX_USED_MASK=0",
                   "-DASYNC_DELAY_OPT_SPLIT_ARRAYS=0",
                   "-DASYNC_DELAY_DEFERRED_CALLBACKS=0"])
    # each speed/correctness flag individually off.
    # Two exceptions forced by the header's #error closure (lines ~278-304):
    #  - OPT_BITMASK=0 alone is ILLEGAL (UNROLL/NEXT/SPLIT_TICK/FIX_USED
    #    default 1 and each demands BITMASK=1). The legacy combo above is the
    #    only legal BITMASK=0 shape - do NOT yield it here.
    #  - FIX_ATOMIC_MASK=0 alone is ILLEGAL (NEXT_TARGET defaults 1 and
    #    TIMER_BITS defaults 16, so the NEXT+TIMER>=16 guard fires). Pair it
    #    with NEXT_TARGET=0; the combo still proves ATOMIC adds no host-
    #    visible behavior change.
    for f in ("ASYNC_DELAY_OPT_MERGED_FLAGS",
              "ASYNC_DELAY_OPT_NEXT_TARGET", "ASYNC_DELAY_OPT_SPLIT_TICK",
              "ASYNC_DELAY_OPT_UNROLL_TICK", "ASYNC_DELAY_FIX_USED_MASK",
              "ASYNC_DELAY_CALLBACK_RESCHEDULE",
              "ASYNC_DELAY_OPT_SPLIT_ARRAYS"):
        short = f[len("ASYNC_DELAY_"):] if f.startswith("ASYNC_DELAY_") else f
        yield (short+"=0", base + ["-D%s=0" % f])
    yield ("FIX_ATOMIC_MASK=0",
           base + ["-DASYNC_DELAY_FIX_ATOMIC_MASK=0",
                   "-DASYNC_DELAY_OPT_NEXT_TARGET=0"])
    # split arrays on
    yield ("ASYNC_DELAY_OPT_SPLIT_ARRAYS=1", base + ["-DASYNC_DELAY_OPT_SPLIT_ARRAYS=1"])
    # deferred callbacks (needs BITMASK + RESCHEDULE on by default - legal closure)
    yield ("ASYNC_DELAY_DEFERRED_CALLBACKS=1", base + ["-DASYNC_DELAY_DEFERRED_CALLBACKS=1"])
    yield ("DEFERRED=1+RESCHEDULE=0",
           base + ["-DASYNC_DELAY_DEFERRED_CALLBACKS=1",
                   "-DASYNC_DELAY_CALLBACK_RESCHEDULE=0"])
    # slot counts
    for n in (1, 5, 8):
        yield (f"MAX_SLOTS={n}", base + [f"-DASYNC_DELAY_MAX_SLOTS={n}"])
    # counter widths
    for b in (8, 32):
        yield (f"TIMER_BITS={b}", base + [f"-DASYNC_DELAY_TIMER_BITS={b}"])

# Combos that are EXPECTED to fail with a compile error (the header's #error lines).
# These are run and confirmed to produce a non-zero exit from the #error - that
# counts as PASS for the harness (the error is the correct behavior).
def invalid_combos():
    # MAX_SLOTS=9 against otherwise-defaults: BITMASK defaults 1, so the
    # BITMASK>8-slots #error fires (line ~278).
    yield ("MAX_SLOTS=9-defaults",
           ["-DASYNC_DELAY_TICK_HZ=1000", "-DASYNC_DELAY_MAX_SLOTS=9"])
    # BITMASK=0 keeps ONLY the #error-driven dependents off; SPLIT_TICK=1
    # re-armed on top must trip the SPLIT_TICK-requires-BITMASK #error.
    yield ("SPLIT_TICK=1+BITMASK=0",
           ["-DASYNC_DELAY_TICK_HZ=1000", "-DASYNC_DELAY_OPT_BITMASK=0",
            "-DASYNC_DELAY_OPT_UNROLL_TICK=0",
            "-DASYNC_DELAY_OPT_NEXT_TARGET=0",
            "-DASYNC_DELAY_FIX_USED_MASK=0",
            "-DASYNC_DELAY_OPT_SPLIT_ARRAYS=0",
            "-DASYNC_DELAY_DEFERRED_CALLBACKS=0",
            "-DASYNC_DELAY_OPT_SPLIT_TICK=1"])
    # DEFERRED=1 + MAX_SLOTS=9: the deferred-pending-mask #error (line ~296).
    yield ("DEFERRED=1+MAX_SLOTS=9",
           ["-DASYNC_DELAY_TICK_HZ=1000",
            "-DASYNC_DELAY_DEFERRED_CALLBACKS=1",
            "-DASYNC_DELAY_MAX_SLOTS=9"])

def rewrite_asm(src_text):
    """Return src_text with every `^[ \\t]*#asm` line turned into a //HOST comment."""
    out = []
    n_rewritten = 0
    for line in src_text.splitlines(keepends=True):
        m = re.match(r'^([ \t]*)#asm\b', line)
        if m:
            n_rewritten += 1
            indent = m.group(1)
            # preserve everything after '#asm' on the line, keep original newline
            rest = line[len(indent):]  # starts with '#asm...'
            # reconstruct keeping original trailing newline
            nl = ''
            if rest.endswith('\n'):
                nl = '\n'
                rest = rest[:-1]
            # drop possible trailing \r
            cr = ''
            if rest.endswith('\r'):
                cr = '\r'
                rest = rest[:-1]
            out.append(f"{indent}//HOST: {rest}{cr}{nl}")
        else:
            out.append(line)
    return ''.join(out), n_rewritten

def generate():
    os.makedirs(BUILD, exist_ok=True)
    with open(SRC, encoding="utf-8", newline="") as f:
        src = f.read()
    host_text, n = rewrite_asm(src)
    # FAIL-FAST: the plan §3 anchor set says exactly 6 #asm lines.
    if n != 6:
        sys.stderr.write(
            f"make_host: STOP - expected 6 #asm lines but rewrote {n}. "
            f"The header changed; re-check plans/006 plan §3 anchors.\n")
        return 1
    # The generated header must reference the host stub so SREG resolves.
    # The driver includes host_stub.h BEFORE this header, so SREG is defined.
    with open(HOST_H, "w", encoding="utf-8", newline="") as f:
        f.write("// AUTO-GENERATED by tests/make_host.py - do not edit.\n")
        f.write("// Source: async_delay.h (repo HEAD). #asm lines stubbed as //HOST:\n")
        f.write(host_text)
    print(f"[gen] wrote {HOST_H} (rewrote {n} #asm lines)")
    return 0

def build_and_run(label, extra):
    out = os.path.join(BUILD, f"t_{label.replace('/','_')}")
    # -std=gnu89 (NOT c89 -pedantic): the pair c89+pedantic rejects the //
    # comments this header AND driver use everywhere by firmware convention,
    # and -Wno-comment cannot rescue them (proven by experiment 2026-09-05).
    # -Wdeclaration-after-statement keeps the one C89 rule CodeVisionAVR
    # actually needs, as a hard error via -Werror.
    cmd = ["gcc", "-std=gnu89", "-Wall", "-Wextra", "-Werror",
           "-Wdeclaration-after-statement",
           "-I", HERE,
           "-DASYNC_DELAY_HOST_MODE=1"] + extra + [STUB, DRIVER, "-o", out]
    # legacy_bitmask0: _async_delay_expire_slot()'s `clr` parameter is genuinely
    # unused there (the legacy path never clears by mask). Harness-side silence
    # only - the header stays untouched (plan 006 decision, 2026-09-05).
    if label == "legacy_bitmask0":
        cmd.insert(1, "-Wno-unused-parameter")
    # compile
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stderr.write(f"[build:FAIL] {label}\n")
        sys.stderr.write(r.stdout)
        sys.stderr.write(r.stderr)
        return ("build", r.returncode)
    # run
    r = subprocess.run([out], cwd=BUILD, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stdout.write(r.stdout)  # tests print their own PASS/FAIL
        sys.stderr.write(f"[run:FAIL] {label} exit={r.returncode}\n")
        sys.stderr.write(r.stderr)
        return ("run", r.returncode)
    sys.stdout.write(r.stdout)
    print(f"[ok] {label}")
    return ("ok", 0)

def main():
    args = sys.argv[1:]
    if "--generate-only" in args:
        return generate()

    rc = generate()
    if rc:
        return rc

    gcc = shutil.which("gcc")
    if not gcc:
        # The recorded environment (2026-09-04) has no gcc. Do not pretend.
        sys.stderr.write(
            "make_host: gcc not found on PATH. Install (MSYS2 recommendation):\n"
            "  winget install --id MSYS2.MSYS2\n"
            "  then in an UCRT64 mksh: pacman -S mingw-w64-ucrt-x86_64-gcc\n"
            "and add C:\\msys64\\ucrt64\\bin to PATH.\n"
            "This is the Phase 0/1 fallback from plans/006 plan §3.\n")
        # Still succeed on the static checks (§4.5) that DO run under Python.
        return 0 if "--no-gcc-ok" in args else 3

    failures = 0
    for label, extra in combos():
        kind, code = build_and_run(label, extra)
        if kind != "ok":
            failures += 1

    # Invalid combos: must FAIL to compile (proves the header's #error guards work).
    for label, extra in invalid_combos():
        out = os.path.join(BUILD, f"t_inv_{label.replace('/','_')}")
        cmd = ["gcc", "-std=gnu89", "-Wall", "-Wextra", "-Werror",
               "-Wdeclaration-after-statement",
               "-I", HERE, "-DASYNC_DELAY_HOST_MODE=1"] + extra + [STUB, "-c", "-o", out + ".o", "-x", "c", "-"]
        # feed the generated header through gcc's preprocessor via -include
        cmd2 = ["gcc", "-std=gnu89", "-Wall", "-Wextra", "-Werror",
               "-Wdeclaration-after-statement",
               "-I", HERE, "-DASYNC_DELAY_HOST_MODE=1"] + extra + \
               ["-include", HOST_H, "-S", "-o", os.devnull, STUB]
        r = subprocess.run(cmd2, cwd=ROOT, capture_output=True, text=True)
        if r.returncode == 0:
            sys.stderr.write(f"[invalid:MISSED] {label} compiled but #error was expected\n")
            failures += 1
        else:
            # #error text must appear
            if "#error" in r.stderr or "#error" in r.stdout:
                print(f"[ok:invalid] {label} correctly rejected")
            else:
                sys.stderr.write(f"[invalid:UNKNOWN] {label} failed but not via #error:\n")
                sys.stderr.write(r.stderr)
                failures += 1

    print(f"\nsummary: {failures} failure(s)")
    return 1 if failures else 0

if __name__ == "__main__":
    sys.exit(main())
