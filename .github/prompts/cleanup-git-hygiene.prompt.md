```prompt
---
name: "Cleanup Git Hygiene"
description: "Find and clean stale branches/worktrees with a required threshold argument and dry-run-first safety."
tools: ['vscode', 'execute', 'read', 'agent', 'edit', 'search', 'todo']
argument-hint: "threshold-days=<int> [apply] [remote] [protect=a,b,c]. Example: threshold-days=2"
---

# Cleanup stale branches and worktrees

Use this workflow to clean stale local branches and stale worktree entries safely.

## Arguments

- `threshold-days=<int>` (required): stale age threshold in days.
- `apply` (optional): execute deletions. If omitted, run dry-run only.
- `remote` (optional): include remote branch deletion (`origin`) for eligible stale branches.
- `protect=a,b,c` (optional): extra protected branches in addition to `main,gh-pages`.

If `threshold-days` is missing or invalid, stop and ask for a valid integer.

## Behavior

1. Parse arguments.
1. Compute effective protected branch list: `main`, `gh-pages`, plus `protect=...` values.
1. Run `scripts/cleanup-git-stale.ps1` in dry-run mode first and present candidate branches/worktrees.
1. If `apply` was not provided, stop after dry-run and ask whether to proceed.
1. If `apply` was provided, run apply mode after showing dry-run output in the same session.

## Command construction

Build a PowerShell command with:

- `-ThresholdDays <value>` from `threshold-days=...`
- `-ProtectedBranches <array>` from effective protected list
- add `-IncludeRemoteDeletes` only if `remote` was requested
- add `-Apply` only for apply execution

Always report the exact effective threshold and protected list before running.

## Safety

- Never delete `main`, `gh-pages`, current branch, or default branch.
- Keep dry-run output visible before destructive actions.
- If apply mode encounters locked/dirty worktrees, do not force; report skips.

```
