# Agent Context Continuity & Suggestion Application

Summary
- You may not be the same agent/model that worked on earlier turns. Treat this as a handoff by default.
- Reconstruct context quickly from prior messages, attachments, repo notes, and open PRs.
- Identify previously agreed decisions and explicit user requests; apply them before proposing new plans or re-asking settled questions.

When to use
- At session start or after a long pause
- When your agent/model/profile differs from earlier messages
- After a handoff (PR link shared, new branch, or significant context change)

Objectives
- Detect handoff state and assume continuity responsibility
- Extract prior suggestions/decisions/TODOs
- Apply them with minimal churn; validate via quick build/test
- Communicate progress deltas and next actions concisely

Quick checklist (TL;DR)
1) Assume you are a different agent unless proven otherwise.
2) Load context:
   - Skim the last 20–50 messages for: Next, TODO, Follow-up, Suggestion, Decision, Blocker, Plan.
   - Open the current file(s) and any linked attachments or PRs.
   - Review repository memories/notes if present.
3) Extract and list:
   - Decisions already made.
   - Suggestions the user asked to apply.
   - Open TODOs and known blockers.
4) Apply:
   - Continue the existing plan; do not restart.
   - Keep prior choices unless there’s a strong reason to change (state why if you do).
5) Communicate:
   - Provide a compact delta update and what’s next.
   - Ask only questions that unblock near‑term progress.
6) Validate:
   - Build/test quickly to ensure applied suggestions didn’t regress behavior.
7) Leave the state tidy:
   - Mark what’s complete, what’s next, and any blockers for the next agent.

Detailed procedure
- Identify handoff:
  - If your agent/profile/model name differs from earlier turns, treat this as a handoff.
  - Even if uncertain, err on the side of “new agent.”
- Load prior context:
  - Read the latest assistant and user turns to capture intent and explicit asks.
  - Review attachments (PRs/issues/design docs); note any linked instructions.
  - Skim repository instructions (quick reference, coding rules, setup notes).
  - Detect any structured plans (TODOs/steps) already in use.
- Mine suggestions and decisions:
  - Pull bullets labeled Suggestion, TODO, Next, Follow-up, Decision, or explicit “do this.”
  - Elevate mandatory items the user asked to apply, then important suggestions next.
  - Note unresolved questions or contradictions to resolve quickly.
- Apply with minimal churn:
  - Prefer incremental, testable edits that align with established conventions.
  - Avoid refactoring unrelated code while applying prior suggestions.
  - If deviating from a prior choice, clearly justify (correctness, constraints, perf).
- Communicate efficiently:
  - Share a concise progress delta and immediate next steps.
  - Avoid repeating settled debates; only re-open if requirements changed or errors found.
- Validate and leave breadcrumbs:
  - Build/test where feasible.
  - Mark completed items and list next actions to ease future handoffs.

Anti‑patterns to avoid
- Restarting from scratch or rewriting plans without reason
- Ignoring previous suggestions and duplicating earlier conversation
- Asking the same non‑blocking questions repeatedly
- Large risky edits without incremental validation

Repository‑specific hooks (adapt as needed)
- If the repo includes canonical instruction files (e.g., `.github/QUICK_REFERENCE.md`, `.github/copilot-instructions.md`), read and follow them.
- Respect project coding rules (naming, RAII, formatting, error handling) and local toolchains.
- Use platform‑appropriate command syntax (e.g., Windows/PowerShell on Windows).
- If a build wrapper or IDE task is mandated, use it for validation instead of ad‑hoc commands.

Expected outputs
- Short delta update describing what prior suggestions/decisions you applied
- A trimmed checklist: done / next / blocked
- Any precise questions that unblock immediate progress

Failure modes & recovery
- Missing context: state exactly what’s missing; proceed with a reasonable default but label it provisional.
- Disabled tools: provide exact file contents/paths and ask for write access if the user wants you to commit.

Versioning & reuse
- Keep this skill generic and portable; add a short per‑repo note if needed.
- Prefer minimal assumptions; guide the agent to consult local instructions first.
