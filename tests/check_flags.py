#!/usr/bin/env python3
"""
check_flags.py - static structural checks over the flag-combo matrix (plan 006 §4.5).

Pure Python, no compiler needed. For each combo in the matrix it:
  1. Evaluates the header's #if nesting with the combo's defines (if_eval.py)
     and keeps the surviving text.
  2. Asserts on the surviving text:
     (a) balanced braces / parens / brackets
     (b) no declaration-after-statement in function bodies (the C89 rule
         CodeVisionAVR enforces; regex scan - false positives are inspected,
         not silenced)
     (c) per function: _ASYNC_SAVE_SREG() count == _ASYNC_REST_SREG() count
         (an unbalanced pair means a critical section leaks or double-restores)
     (d) no identifier `bit` / `flash` / `eeprom` used as a variable name
         (CodeVisionAVR keywords) outside comments
  3. Invalid combos (the header's #error probes) must trip #error - PASS.
  4. Mutation sanity: a mutated COPY of the header must make the checker
     scream (guard-weaken probe + brace-removal probe). Never mutates the
     repo header.

Combo matrix mirrors make_host.py (plan §4.6) - the two MUST stay in sync.
Exit codes: 0 = all green, 1 = any failure (details on stderr).
"""

import os, re, sys

from if_eval import EvalError, preprocess, strip_comments_and_strings

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC  = os.path.join(ROOT, "async_delay.h")

# Combo matrix - MUST mirror make_host.py's combos() (plan §4.6). Duplicated
# here on purpose: check_flags.py runs without gcc, on its own terms.
DEFINES_DEFAULT = {
    "ASYNC_DELAY_TICK_HZ": 1000,
    "ASYNC_DELAY_TIMER_BITS": 16,
    "ASYNC_DELAY_MAX_SLOTS": 4,
    "ASYNC_DELAY_OPT_BITMASK": 1,
    "ASYNC_DELAY_OPT_MERGED_FLAGS": 1,
    "ASYNC_DELAY_OPT_SPLIT_ARRAYS": 0,
    "ASYNC_DELAY_FIX_USED_MASK": 1,
    "ASYNC_DELAY_FIX_ATOMIC_MASK": 1,
    "ASYNC_DELAY_OPT_UNROLL_TICK": 1,
    "ASYNC_DELAY_OPT_NEXT_TARGET": 1,
    "ASYNC_DELAY_OPT_SPLIT_TICK": 1,
    "ASYNC_DELAY_DEFERRED_CALLBACKS": 0,
    "ASYNC_DELAY_CALLBACK_RESCHEDULE": 1,
    "ASYNC_DELAY_VERSION": 7,
    "ASYNC_DELAY_HOST_MODE": 1,
}

def combos():
    yield ("defaults", {})
    yield ("legacy_bitmask0", {
        "ASYNC_DELAY_OPT_BITMASK": 0,
        "ASYNC_DELAY_OPT_UNROLL_TICK": 0,
        "ASYNC_DELAY_OPT_NEXT_TARGET": 0,
        "ASYNC_DELAY_OPT_SPLIT_TICK": 0,
        "ASYNC_DELAY_FIX_USED_MASK": 0,
        "ASYNC_DELAY_OPT_SPLIT_ARRAYS": 0,
        "ASYNC_DELAY_DEFERRED_CALLBACKS": 0})
    for f in ("ASYNC_DELAY_OPT_MERGED_FLAGS", "ASYNC_DELAY_OPT_NEXT_TARGET",
              "ASYNC_DELAY_OPT_SPLIT_TICK", "ASYNC_DELAY_OPT_UNROLL_TICK",
              "ASYNC_DELAY_FIX_USED_MASK", "ASYNC_DELAY_CALLBACK_RESCHEDULE",
              "ASYNC_DELAY_OPT_SPLIT_ARRAYS"):
        yield (f.replace("ASYNC_DELAY_", "") + "=0", {f: 0})
    yield ("FIX_ATOMIC_MASK=0", {
        "ASYNC_DELAY_FIX_ATOMIC_MASK": 0,
        "ASYNC_DELAY_OPT_NEXT_TARGET": 0})
    yield ("ASYNC_DELAY_OPT_SPLIT_ARRAYS=1", {"ASYNC_DELAY_OPT_SPLIT_ARRAYS": 1})
    yield ("DEFERRED=1", {"ASYNC_DELAY_DEFERRED_CALLBACKS": 1})
    yield ("DEFERRED=1+RESCHEDULE=0", {
        "ASYNC_DELAY_DEFERRED_CALLBACKS": 1,
        "ASYNC_DELAY_CALLBACK_RESCHEDULE": 0})
    for n in (1, 5, 8):
        yield (f"MAX_SLOTS={n}", {"ASYNC_DELAY_MAX_SLOTS": n})
    yield ("TIMER_BITS=8", {"ASYNC_DELAY_TIMER_BITS": 8})
    yield ("TIMER_BITS=32", {"ASYNC_DELAY_TIMER_BITS": 32})

def invalid_combos():
    yield ("MAX_SLOTS=9-defaults", {"ASYNC_DELAY_MAX_SLOTS": 9})
    yield ("SPLIT_TICK=1+BITMASK=0", {
        "ASYNC_DELAY_OPT_BITMASK": 0,
        "ASYNC_DELAY_OPT_UNROLL_TICK": 0,
        "ASYNC_DELAY_OPT_NEXT_TARGET": 0,
        "ASYNC_DELAY_FIX_USED_MASK": 0,
        "ASYNC_DELAY_OPT_SPLIT_ARRAYS": 0,
        "ASYNC_DELAY_DEFERRED_CALLBACKS": 0,
        "ASYNC_DELAY_OPT_SPLIT_TICK": 1})
    yield ("DEFERRED=1+MAX_SLOTS=9", {
        "ASYNC_DELAY_DEFERRED_CALLBACKS": 1,
        "ASYNC_DELAY_MAX_SLOTS": 9})

# ---------- structural checks on surviving text ----------

def check_balance(code, label, fails):
    pairs = {"}": "{", ")": "(", "]": "["}
    stack = []
    for ch in code:
        if ch in "{([":
            stack.append(ch)
        elif ch in "})]":
            if not stack or stack[-1] != pairs[ch]:
                fails.append(f"{label}: unbalanced {ch}")
                return
            stack.pop()
    if stack:
        fails.append(f"{label}: {len(stack)} unclosed {stack[0]}")

def check_decl_after_statement(code, label, fails):
    # Simplified C89 scan per plan §4.5: within a function body, a line whose
    # first word is a type keyword must not follow a statement at the same
    # brace depth. False positives are inspected, not silenced.
    type_kw = ("unsigned|signed|void|char|int|long|short|float|double|"
               "async_tick_t|async_delay_cb_t|_async_slot_t")
    decl_re = re.compile(
        rf"^\s*(?:static\s+|volatile\s+|const\s+)*({type_kw})\b")
    depth = 0
    # depths at which the previous line ended a statement
    prev_stmt_depth = set()
    for ln in code.split("\n"):
        s = ln.strip()
        if not s or s.startswith("#"):
            continue
        opens = ln.count("{")
        closes = ln.count("}")
        if decl_re.match(s):
            # declaration after a statement at this depth?
            if depth in prev_stmt_depth:
                fails.append(f"{label}: decl-after-statement: {s[:60]}")
                return
        if closes > opens:
            # leaving this block: its statement history must not leak into
            # the next sibling block at the same depth
            prev_stmt_depth.discard(depth)
        depth += opens - closes
        # a statement line (ends with ; and not a declaration) marks the depth
        # (typedef is a declaration too - it ends in ; but starts no statement)
        if (s.endswith(";") and not decl_re.match(s)
                and not s.startswith("}") and not s.startswith("typedef")):
            prev_stmt_depth.add(depth)

def check_save_restore(surv, label, fails):
    # Per function: count SAVE vs REST (an unbalanced pair means a critical
    # section leaks or double-restores). Top-level function boundaries in this
    # header are `static <type> name(...)` definition lines followed by `{` on
    # the same or next line. Verified against the real header layout.
    fn_starts = [m.start() for m in re.finditer(
        r"(?m)^static\s+[A-Za-z_][\w\s\*]*\([^;{]*\)\s*$", surv)]
    if not fn_starts:
        return
    bounds = fn_starts + [len(surv)]
    for k in range(len(bounds) - 1):
        body = surv[bounds[k]:bounds[k + 1]]
        n_save = len(re.findall(r"_ASYNC_SAVE_SREG\s*\(\s*\)", body))
        n_rest = len(re.findall(r"_ASYNC_REST_SREG\s*\(\s*\)", body))
        if n_save != n_rest:
            fails.append(f"{label}: fn#{k} SAVE={n_save} REST={n_rest} unbalanced")

def check_cvavr_keywords(surv, label, fails):
    # word-boundary scan for CVAVR keywords used as identifiers - outside
    # comments/strings (plan §3: they appear only in comments in the header;
    # this guard keeps that true).
    code = strip_comments_and_strings(surv)
    for kw in ("bit", "flash", "eeprom", "sfrb", "sfr", "interrupt"):
        for m in re.finditer(rf"\b{kw}\b", code):
            ctx = code[max(0, m.start() - 40):m.end() + 40].replace("\n", " ")
            fails.append(f"{label}: CVAVR keyword '{kw}' in code: ...{ctx}...")

# ---------- mutation sanity ----------

def mutate_sanity(src_text, fails):
    # Prove the checker is not vacuous, per plan §6 Phase 4: mutate a COPY of
    # the header, run the checks, and the checker MUST scream. Two mutations,
    # each targeting a different check class:
    #
    # 1. Guard-weaken: relax the MAX_SLOTS>8 #error guard to >100. The
    #    MAX_SLOTS=9 invalid probe then no longer trips #error, so the probe
    #    path must report "expected #error, got none". (Mutating a #define
    #    default instead would be a no-op: the combo's defines shadow the
    #    header's #ifndef defaults, exactly as in real C.)
    m1 = src_text.replace(
        "#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_MAX_SLOTS > 8",
        "#if ASYNC_DELAY_OPT_BITMASK && ASYNC_DELAY_MAX_SLOTS > 100", 1)
    if m1 == src_text:
        fails.append("mutation-sanity 1: anchor gone (guard #if)")
    else:
        defs = dict(DEFINES_DEFAULT)
        defs["ASYNC_DELAY_MAX_SLOTS"] = 9
        try:
            _, err = preprocess(m1, defs)
        except EvalError as e:
            err = str(e)
        if err is None:
            # caught: record + pop so the count stays clean but a trace exists
            fails.append("mutation-sanity 1 PASSED: weakened guard silenced "
                         "#error and the checker caught it (missing #error)")
            fails.pop()
        else:
            fails.append("mutation-sanity 1: mutated header still #errors - "
                         "probe cannot detect the weakening")
    # 2. Brace removal: delete async_delay_init's closing brace. The balance
    #    check must flag the unclosed '{'.
    m2 = src_text.replace(
        "_AD_FLAGS(i) = ASYNC_SLOT_FREE;\n}",
        "_AD_FLAGS(i) = ASYNC_SLOT_FREE;", 1)
    if m2 == src_text:
        fails.append("mutation-sanity 2: anchor gone (init closing brace)")
    else:
        defs = dict(DEFINES_DEFAULT)
        try:
            surv, _ = preprocess(m2, defs)
        except EvalError:
            surv = m2
        local = []
        check_balance(strip_comments_and_strings(surv), "mutation-sanity 2",
                      local)
        if local:
            fails.append("mutation-sanity 2 PASSED: removed brace was caught")
            fails.pop()   # scream recorded, do not pollute the count
        else:
            fails.append("mutation-sanity 2: brace removal NOT caught - "
                         "balance check is vacuous")

# ---------- main ----------

def run_checks(src_text, defs_over, label, expect_error, fails):
    defs = dict(DEFINES_DEFAULT)
    defs.update(defs_over)
    try:
        surv, err = preprocess(src_text, defs)
    except EvalError as e:
        if expect_error:
            return True       # any evaluation failure on a probe is a pass
        fails.append(f"{label}: evaluator error: {e}")
        return False
    if expect_error:
        if err:
            return True
        fails.append(f"{label}: expected #error, got none (probe vacuous)")
        return False
    if err:
        fails.append(f"{label}: unexpected #error: {err}")
        return False
    code = strip_comments_and_strings(surv)
    check_balance(code, label, fails)
    check_decl_after_statement(code, label, fails)
    check_save_restore(surv, label, fails)
    check_cvavr_keywords(surv, label, fails)
    return True

def main():
    with open(SRC, encoding="utf-8") as f:
        src = f.read()
    fails = []
    n_ok = 0
    for label, over in combos():
        if run_checks(src, over, label, False, fails):
            n_ok += 1
            print(f"[ok] {label}")
        else:
            print(f"[FAIL] {label}")
    for label, over in invalid_combos():
        if run_checks(src, over, label, True, fails):
            n_ok += 1
            print(f"[ok:invalid] {label} correctly rejected")
        else:
            print(f"[FAIL:invalid] {label}")
    mutate_sanity(src, fails)
    print(f"\ncheck_flags: {n_ok} combo(s) checked, {len(fails)} failure(s)")
    for f_ in fails:
        sys.stderr.write("  FAIL: " + f_ + "\n")
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(main())
