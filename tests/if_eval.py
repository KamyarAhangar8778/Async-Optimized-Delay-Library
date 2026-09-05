#!/usr/bin/env python3
"""
if_eval.py - directive-only C preprocessor evaluator + C text utilities.

Shared by check_flags.py (plan 006 §4.5). No C parsing: #if/#ifdef/#ifndef/
#elif/#else/#endif nesting is evaluated against a dict of integer defines;
#error lines that survive are reported. Text helpers blank out comments and
strings while preserving line structure (line numbers stay meaningful).
"""

import re


class EvalError(Exception):
    pass


def _c_eval(expr, defs):
    # Expression evaluator for the header's #if forms: ints, !, &&, ||,
    # ==, !=, >=, <=, >, <, defined(X) / defined X, and macro substitution
    # (undefined macros -> 0, per C rules).
    e = expr.strip()
    e = re.sub(r"/\*.*?\*/", " ", e)
    e = re.sub(r"//[^\n]*", " ", e)
    def dl(m):
        return "1" if m.group(1) in defs else "0"
    e = re.sub(r"defined\s*\(\s*([A-Za-z_]\w*)\s*\)", dl, e)
    e = re.sub(r"defined\s+([A-Za-z_]\w*)", dl, e)
    # macro substitution - repeat until stable (macros referencing macros)
    for _ in range(16):
        new = re.sub(r"\b([A-Za-z_]\w*)\b",
                     lambda m: str(defs[m.group(1)]) if m.group(1) in defs else "0",
                     e)
        if new == e:
            break
        e = new
    if not e.strip():
        raise EvalError("empty #if expression")
    # Replace '!' only when NOT part of '!=' - a bare regex '!' would turn
    # '!=' into ' not =' and break every #if that uses inequality.
    py = e.replace("&&", " and ").replace("||", " or ")
    py = re.sub(r"!(?!=)", " not ", py)
    try:
        v = eval(py, {"__builtins__": {}}, {})   # tokens are ints/comparisons only
    except Exception as ex:
        raise EvalError(f"cannot evaluate #{expr!r}: {ex}")
    return bool(v)


def preprocess(text, defs):
    """Directive-only #if evaluator. Returns (surviving_text, hit_error_msg).

    hit_error_msg is not None when an #error line survived (its text).
    Mutates nothing; defs may gain entries from simple integer #defines
    inside the text (C semantics: an #ifndef default resolves the value)."""
    out = []
    # stack entries: [parent_active, this_branch_taken_ever, this_branch_active]
    stack = []
    error_msg = None
    lines = text.split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        # join line continuations for directive lines
        while line.rstrip().endswith("\\") and i + 1 < len(lines):
            i += 1
            line = line.rstrip()[:-1] + " " + lines[i]
        m = re.match(r"^\s*#\s*(\w+)(.*)$", line)
        if m:
            d, rest = m.group(1), m.group(2)
            active = all(s[2] for s in stack)
            if d in ("if", "ifdef", "ifndef"):
                if d == "ifdef":
                    mm = re.match(r"^\s*([A-Za-z_]\w*)", rest)
                    cond = bool(mm) and mm.group(1) in defs
                elif d == "ifndef":
                    mm = re.match(r"^\s*([A-Za-z_]\w*)", rest)
                    cond = bool(mm) and mm.group(1) not in defs
                else:
                    cond = _c_eval(rest, defs) if active else False
                stack.append([active, bool(cond) if active else False,
                              bool(cond) if active else False])
            elif d == "elif":
                if not stack:
                    raise EvalError("#elif without #if")
                parent, taken, _ = stack[-1]
                cond = (not taken) and parent and _c_eval(rest, defs)
                stack[-1] = [parent, taken or (cond if parent else False),
                             bool(cond)]
            elif d == "else":
                if not stack:
                    raise EvalError("#else without #if")
                parent, taken, _ = stack[-1]
                stack[-1] = [parent, True, parent and not taken]
            elif d == "endif":
                if not stack:
                    raise EvalError("#endif without #if")
                stack.pop()
            elif d == "error":
                if active and error_msg is None:
                    error_msg = rest.strip()
            elif d == "define":
                # Track only object-like INTEGER defines (async_tick_t etc.
                # would otherwise be substituted as 0 and eval warns). The
                # header's #if expressions only ever reference integer config
                # macros, so this is sufficient.
                if active:
                    dm = re.match(r"^\s*([A-Za-z_]\w*)\s+([+-]?\d+|0[xX][0-9a-fA-F]+)\s*$", rest)
                    if dm:
                        defs = dict(defs)
                        defs[dm.group(1)] = int(dm.group(2), 0)
            # unknown directives (e.g. #asm - not a C directive) never fatal
            i += 1
            continue
        if all(s[2] for s in stack):
            out.append(line)
        i += 1
    if stack:
        raise EvalError("unterminated #if block(s)")
    return "\n".join(out), error_msg


def strip_comments_and_strings(text):
    # keep line structure (newlines) so line numbers stay meaningful
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("".join(ch if ch == "\n" else " " for ch in text[i:j]))
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                if text[j] == "\\":
                    j += 1
                j += 1
            out.append(" " + " " * (j - i))
            i = j + 1
        elif c == "'":
            j = i + 1
            while j < n and text[j] != "'":
                if text[j] == "\\":
                    j += 1
                j += 1
            out.append(" " + " " * (j - i))
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out)
