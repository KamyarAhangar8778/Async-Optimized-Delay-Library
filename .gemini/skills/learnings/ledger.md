# Cross-Project Learning Ledger

Patterns learned in one codebase that apply to another. One line per fact. Additive only.

| pattern | what it is / why non-obvious | source | applies-to hint |
|---|---|---|---|
| python3-store-stub | On this machine `python3` is a Microsoft Store stub that fails silently; use `py -3` instead | offline-converter | any bash hook/script on Windows |
| git-bash-pwd-normalize | `$PWD` in Git Bash is `/c/...`; normalize to `C:/...` before matching repo prefixes | offline-converter | any hook mapping cwd→project |
| sse-data-strip | codebase-memory MCP over HTTP returns `data: {json}` SSE lines; strip prefix before parsing | offline-converter | any MCP-via-curl hook |
| graph-row-is-array | `query_graph` rows are arrays (not dicts); index `r[0]`/`r[1]` not `r.get(...)` | offline-converter | any graph-query consumer |
| posttooluse-no-inject | PostToolUse output is never injected into model context — reminders must go via UserPromptSubmit/PreCompact | offline-converter | hook design, any project |
| copytree-skip-git | Plugin caches contain locked `.git` dirs; skip them in backup copytrees or `PermissionError` aborts | harness-backup | backup/restore, any project |
| cvavr-entry-spill | CodeVisionAVR spills register locals at function ENTRY before any branch, so a hot function's cheap early-exit path pays it too; keep the fast path local-free and push locals into a callee | AsyncDelay | any embedded hot path with an early return |
| cvavr-bit-keyword | `bit` (also `flash`/`eeprom`/`sfrb`/`sfrw`/`interrupt`/`funcused`) is a CodeVisionAVR type specifier; naming a variable `bit` yields "invalid combination of type specifiers" plus bogus "must declare first in block" and "undefined symbol" errors downstream | AsyncDelay | CodeVisionAVR / vendor-C compilers |
| avr-runtime-shift-call | On AVR, `1 << i` with a RUNTIME `i` compiles to a call to a bit-at-a-time shift loop (`__LSLW12`), and `arr[i]` to a `MUL` + pointer-load helper; unrolling with compile-time indices folds all three into `SBRS`/`LDS` (~72 → ~13 cycles/slot) | AsyncDelay | AVR/8-bit bitmask scans |
| mask-cannot-hold-3-states | A one-bit-per-slot "busy" mask cannot distinguish FREE from EXPIRED-awaiting-poll, so an allocator reading it hands out slots whose owner has not collected the result; use a second ALLOCATED mask and assert `active & ~used == 0` | AsyncDelay | slot/handle allocators, any language |
| isr-mask-rmw-race | `mask \|= bit` is load-modify-store on 8-bit MCUs, so an ISR bit-clear landing mid-sequence is lost and leaves a dead entry live; guard main-context RMW and save/restore the status register instead of a bare `sei`, which would enable interrupts under a caller that had them off | AsyncDelay | any ISR-shared bitmask |
| model-the-algorithm-in-python | When the real toolchain is unavailable (no license/hardware), port the algorithm to a throwaway Python model plus a `#if` evaluator to run traces and catch C89/keyword/decl errors before the one build you get; include an inverted test that reproduces the bug with the fix flag off, to prove the test measures something | AsyncDelay | embedded, cross-compiled, or license-gated builds |