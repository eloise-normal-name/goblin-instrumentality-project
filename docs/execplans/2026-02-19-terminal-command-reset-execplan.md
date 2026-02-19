---
status: active
owner: Nova [g53c]
related: user-request-2026-02-19-terminal-reset
---

# Terminal Command Reliability Reset (Docs, Scripts, User Settings)

This ExecPlan is a living document. See `.agent/PLANS.md` for lifecycle and hygiene rules.

## Purpose / Big Picture

Copilot terminal execution in this repo became unreliable due to drift in terminal startup state,
inconsistent command wrappers, and missing deterministic configure recovery guidance.

This reset standardizes the full terminal command path:

1. **User settings** guarantee a stable startup shell/profile/cwd
2. **Scripts** guarantee deterministic command execution and configure retry behavior
3. **Docs** guarantee agents choose the same known-good flow for configure/build/debug tasks

## Progress

- [x] (2026-02-19) Audited current terminal wrapper, docs, and user settings.
- [x] (2026-02-19) Added `scripts/configure-cmake.ps1` with automatic cache-reset retry.
- [x] (2026-02-19) Hardened `scripts/agent-wrap.ps1` with explicit working directory support.
- [x] (2026-02-19) Updated docs to use configure helper and explicit cache-reset policy.
- [x] (2026-02-19) Reset user terminal profile/cwd settings and simplified terminal command auto-approve patterns.
- [x] (2026-02-19) Validated configure flow (`scripts/configure-cmake.ps1` and `-ForceClean`) end-to-end.

## Surprises & Discoveries

- User settings had terminal UI options but lacked explicit default profile/cwd terminal boot policy,
  which made behavior dependent on ambient editor/session state.
- Command auto-approve rules had accumulated tool-specific clutter; reducing to a small allowlist
  improves predictability.

## Decision Log

- Decision: Keep `Developer PowerShell (VS 18)` as default profile and enforce
  `-SkipAutomaticLocation` in `Launch-VsDevShell.ps1` startup command.
  Rationale: this keeps MSVC environment available while preserving workspace cwd semantics.
  Date/Author: 2026-02-19, Nova [g53c]

- Decision: Add a dedicated configure helper (`scripts/configure-cmake.ps1`) instead of
  repeating one-off cache cleanup commands across docs.
  Rationale: one script reduces repeated errors and standardizes automatic retry behavior.
  Date/Author: 2026-02-19, Nova [g53c]

- Decision: Keep one automatic retry after cache cleanup (not infinite retries).
  Rationale: fixes stale-cache cases while surfacing real configuration errors quickly.
  Date/Author: 2026-02-19, Nova [g53c]

## Outcomes & Retrospective

Validation completed:

- `scripts/configure-cmake.ps1` succeeds in the normal path.
- `scripts/configure-cmake.ps1 -ForceClean` succeeds and regenerates cache/build files.
- During initial validation, a real generator mismatch was encountered; cache-reset retry behavior
  resolved configure to a valid VS 18 build tree once exit-code handling was corrected.

Terminal profile/cwd settings are now explicit in user settings, so new terminals should start in
workspace context with the VS dev shell loaded.

## Scope of Changes

- Script updates:
  - `scripts/agent-wrap.ps1`
  - `scripts/configure-cmake.ps1` (new)
- Documentation updates:
  - `README.md`
  - `AGENTS.md`
  - `.github/copilot-instructions.md`
  - `docs/copilot-known-errors.md`
- User settings update:
  - `C:\Users\Admin\AppData\Roaming\Code\User\settings.json`

## Validation and Acceptance

Acceptance criteria:

1. New terminal launches in workspace root when repo is open.
2. VS developer environment is active in default terminal profile.
3. Running `scripts/configure-cmake.ps1` succeeds on first run or succeeds after one automatic
   cache-reset retry when cache is stale.
4. Docs consistently direct agents/users to the same configure/reset flow.

## Idempotence and Recovery

- User settings changes are idempotent and safe to reapply.
- `scripts/configure-cmake.ps1 -ForceClean` is the canonical recovery path.
- If any reset behavior is undesirable, revert only terminal-related settings keys and keep
  script/doc improvements.
