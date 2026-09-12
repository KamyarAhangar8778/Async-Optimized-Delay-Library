---
name: project-audit
description: Audit project health and documentation quality.
---

# Project Audit

Assess a project's structural health, config consistency, documentation quality, and AI-agent readability. Unlike `improve` (read-only, plan-focused), this skill **directly fixes issues** after presenting findings.

## When to Use

- "Review the project structure"
- "Is this project well-organized?"
- "Are the docs up to date?"
- "Make the docs LLM-friendly"
- "Why isn't this a 10/10?"
- New agent onboarding to an existing codebase

## Phase 1 — Structure Assessment

Use `codebase-memory-mcp` tools (NOT grep/glob):

```
1. get_architecture(aspects=["overview","clusters","hotspots","boundaries","languages","file_tree"])
2. index_status() — verify project is indexed
3. If not indexed: index_repository() first
```

### Scoring Dimensions

| Dimension | Weight | What to check |
|---|---|---|
| **Modularity** | 25% | Feature-based folders, cohesion scores, no circular deps |
| **Separation of Concerns** | 20% | Core/hooks/views split, business logic vs UI |
| **AI-Agent Readability** | 20% | AGENTS.md exists, MCP indexed, typed interfaces, JSDoc |
| **Documentation** | 15% | Root docs current, ARCHITECTURE.md accurate, DESIGN.md complete |
| **Config Consistency** | 10% | Config files don't contradict project architecture |
| **Testing** | 10% | Tests colocated, coverage of core utils, test commands documented |

### Cluster Analysis

From `get_architecture` clusters, check:
- **High cohesion (>=0.8)** = well-bounded module
- **Low cohesion (<0.6)** = mixed concerns, needs refactoring
- **fan-in=0, fan-out=0** = orphan module (not wired into the app)
- **Circular boundaries** = architectural debt

## Phase 2 — Config Consistency Audit

Check every config file against the project's actual architecture:

| Config | Common Drift |
|---|---|
| `package.json` | Name doesn't match project (scaffolded name left in) |
| `.env.example` | References APIs/services not used by the project |
| `metadata.json` | Claims capabilities contradicted by architecture |
| `components.json` | RTL flag, style, or aliases don't match actual setup |
| `tsconfig.json` | Strict mode not enabled when AGENTS.md requires it |
| `next.config.ts` | Missing required headers (COOP/COEP for FFmpeg) |

**Rule:** If a config file's values contradict the project's documented architecture, that is a finding.

## Phase 3 — Documentation LLM-Readability

### What "LLM-Friendly" Means

| Property | Bad | Good |
|---|---|---|
| **Length** | 8,000+ chars per doc | 2,000-5,000 chars |
| **Format** | Long prose paragraphs | Tables, bullet lists, code blocks |
| **Structure** | Flat text | Clear H2/H3 hierarchy |
| **Actionability** | "We follow best practices" | Specific rules with examples |
| **Scannability** | Wall of text | Token-efficient, high signal-density |

### Documentation Checklist

- `AGENTS.md` — role, constraints, file conventions, testing commands
- `ARCHITECTURE.md` — data flow, subsystems, directory tree, invariants
- `DESIGN.md` — visual identity, material system, RTL rules, color tokens
- `README.md` — features, tech stack, getting started, project structure
- No outdated references (e.g., mentioning services/APIs not in the project)
- Tables used for structured data (not prose descriptions)
- Code examples are copy-pasteable and correct

### Rewrite Principles

1. **Delete first, write second** — remove outdated/verbose content before adding
2. **Tables over prose** — for any structured information (configs, rules, dependencies)
3. **Code blocks for patterns** — show the pattern, don't describe it
4. **No filler** — every sentence must carry information
5. **Cross-reference, don't duplicate** — docs should complement, not repeat each other

## Phase 4 — Fix and Verify

After presenting findings:

1. **Fix config drift** — update metadata.json, .env.example, components.json, package.json
2. **Rewrite docs** — apply LLM-readability principles
3. **Re-index** — `index_repository()` after all changes
4. **Verify** — run test, lint, and build commands

### Verification Commands (by framework)

| Framework | Test | Lint | Build |
|---|---|---|---|
| Next.js | `npm run test` | `npm run lint` | `npm run build` |
| Python | `pytest` | `ruff check .` | — |
| Go | `go test ./...` | `golangci-lint run` | `go build ./...` |

## Pitfalls

1. **Graph tree can hide nested files** — `get_architecture` file_tree may show directories as empty when they contain deeply nested files. Always verify with `search_files(target="files")` before concluding a folder is empty.

2. **Do not over-rewrite** — if docs are already concise and accurate, do not rewrite for the sake of it. The goal is signal density, not different text.

3. **Config files may be external tool artifacts** — `metadata.json` from Jules/AI Studio, `components.json` from shadcn CLI. Updating them is fine but understand their source so you don't break the tool that generated them.

4. **Re-index is mandatory** — after changing any source files, re-index via codebase-memory MCP so the graph stays accurate. Skip this and future sessions will have stale data.

5. **Honest scoring** — when rating a project, give specific reasons for deductions. "8.5/10 with these 5 issues" is more useful than vague praise. Users ask "why not 10?" and deserve a concrete answer.
