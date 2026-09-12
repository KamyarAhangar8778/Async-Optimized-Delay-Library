---
name: learnings
description: Cross-project learning ledger. Log a pattern, fix, or architecture decision learned in one project so it carries to the next. Use whenever you solve something reusable (a non-obvious FFmpeg arg, a WebSocket throttle, a filter idiom) or when starting work in a project and want to check what patterns already apply. This skill is global — it persists across all Kaveh's projects, not just this repo.
---

# Cross-Project Learning Ledger

Patterns learned in one codebase that work in another shouldn't be re-derived from scratch.
This ledger records them once (global, project-agnostic) and surfaces them on demand.

## Where it lives

- Ledger file: `~/.claude/skills/learnings/ledger.md` (one table, append-only, dedup by pattern name).
- This `SKILL.md` is the handler that reads/updates it.

## Use when

- **Record** — you just fixed something non-obvious (a bug, a perf trap, an idiom) that would help in any codebase. Call `/learnings record`.
- **Recall** — starting work and want prior patterns that apply. Ask "check the learnings ledger for X" or `/learnings list`.

## Format (keep it lean)

Each entry is ONE line in `ledger.md`, pipe-separated:

```
| pattern | what it is / why non-obvious | project(s) it came from | applies-to hint |
```

Rules:
- **One polyline per fact.** No essays. If it needs more, that's a real doc — not a ledger row.
- **Reusable, not reportage.** Drop it only if it generalizes ("websocket backpressure", "COOP/COEP for wasm threading") — not project trivia ("renamed file X").
- **Dedupe first.** If the pattern already exists, update its "from" column (append project) instead of adding a row.
- Prefer a concrete trap name ("SSE `data: ` strip", "python3-store-stub") over abstractions.

## Commands

| Command | Action |
|---|---|
| `/learn record` | Append a new reusable pattern (dedupe first). |
| `/learn list` | Print the ledger, optionally filtered by `to` hint. |
| `/learn check <pattern>` | Report whether it exists; summarize if yes. |

## Guardrails

- Only *cross-project-reusable* facts go here. Project-specific (file paths, local constraints) stay in project memory — never in the ledger.
- Editing is additive only. Never delete a row unless the pattern is proven wrong by Kaveh.