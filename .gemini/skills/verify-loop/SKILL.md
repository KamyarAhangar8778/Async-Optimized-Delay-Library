---
name: verify-loop
description: Close the loop on recent changes. Use after any edit/edit-batch, before claiming "done", or when switching tasks mid-stream. Re-checks unverified edits (build/test/lint/trace), flushes memory-worthy learnings to the ledger, and reports what is (and isn't) actually verified. Global — works in any project.
---

# Verify Loop

The loop that turns "I wrote it" into "it's verified". PostToolUse can't inject reminders
(see [[learnings]]), so this is the explicit close-the-loop pass.

## When to run

- After any batch of edits, before saying "done".
- When switching context and you want a clean handoff.
- On request ("verify what we changed").

## The pass (in order, stop early if any fails)

1. **Diff** — `git status --short` + `git diff --stat` (or MCP `detect_changes`). What did we actually touch?
2. **Check** — smallest runnable proof the change works:
   - JS/TS: `npm run test` or `npm run lint` or `npm run build` (pick the one the change touches)
   - Python: run the file's `__main__`/self-check, or one pytest
   - Shell/hook: feed it a sample input, assert output shape
   - If a check is skipped, say so explicitly — "not verified" is a real status.
3. **Harvest** — did any fix expose a reusable pattern? If yes, `/learn record` it to the ledger (global).
4. **Report** — one line per change: verified ✔ / skipped ✖ (and why). Never claim done on a ✖.

## Guardrails

- If a check fails, STOP and report the failure — do not paper over it.
- Keep it lazy: the smallest check that could fail, not a full suite.