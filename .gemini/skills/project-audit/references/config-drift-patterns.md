# Config Drift Patterns

Real examples of config files contradicting project architecture.

## Pattern 1: Scaffolding artifacts left in

**File:** `package.json`
**Issue:** `"name": "ai-studio-applet"` — scaffolded name from Jules/AI Studio, not the actual project name.
**Fix:** `"name": "offline-media-converter"`

**File:** `.env.example`
**Issue:** Contains `GEMINI_API_KEY` and `APP_URL` for server-side Gemini API. Project is fully client-side offline-first.
**Fix:** Replace with comment explaining no env vars are needed.

**File:** `metadata.json`
**Issue:** `"majorCapabilities": ["MAJOR_CAPABILITY_SERVER_SIDE_GEMINI_API"]` — contradicts offline-first mandate.
**Fix:** Remove capability claims, keep only name/description.

## Pattern 2: Feature flags not updated

**File:** `components.json` (shadcn CLI config)
**Issue:** `"rtl": false` — but the entire project is RTL-first with `dir="rtl"` on root HTML.
**Fix:** `"rtl": true`

## Detection heuristic

When auditing, check:
1. Does `package.json` name match the project's actual purpose?
2. Do `.env.example` entries reference services the project actually uses?
3. Do feature flags (RTL, strict mode, etc.) match documented architecture?
4. Do metadata/capability files claim things the project doesn't do?
