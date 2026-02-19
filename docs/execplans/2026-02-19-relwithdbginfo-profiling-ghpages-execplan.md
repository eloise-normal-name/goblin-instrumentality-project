---
status: active
owner: codex
related: user-request-2026-02-19-relwithdbginfo-profiling-ghpages
---

# Add RelWithDbgInfo profiling baseline and publishable performance reporting

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

`/.agent/PLANS.md` is checked into this repository and this document is maintained in accordance with that file.

## Purpose / Big Picture

After this work, profiling no longer depends on Release-only assumptions or one-off commands. The repository gets a standard `RelWithDbgInfo` configuration (optimized with symbols), a reusable script to run profiling sessions and emit baseline JSON, and a generated `perf-highlights.html` artifact that can be published to `gh-pages` alongside existing highlights. A contributor can run one command, collect sessions, produce a baseline, and publish a report page.

## Progress

- [x] (2026-02-19 04:12Z) Inspected current build and profiling flow in `CMakeLists.txt`, `.github/prompts/profile-performance.prompt.md`, `.github/prompts/snippets/vsdiagnostics-guide.md`, `.github/agents/profiler.agent.md`, and `docs/perf-baselines/README.md`.
- [x] (2026-02-19 04:22Z) Added `RelWithDbgInfo` CMake configuration support and output directory wiring in `CMakeLists.txt`.
- [x] (2026-02-19 04:25Z) Updated profiler prompt, profiler agent, and profiler docs to use `RelWithDbgInfo` as the canonical profiling target.
- [x] (2026-02-19 04:31Z) Added reusable scripts: `scripts/profile-exe.ps1` and `scripts/render-perf-report.ps1`.
- [x] (2026-02-19 04:33Z) Added documentation updates in `README.md` and `docs/perf-baselines/README.md` to expose the reusable workflow and `perf-highlights.html` report target.
- [ ] Run profiling workflow once and commit first generated baseline JSON and report refresh.
- [ ] Validate compile/build path and update this plan to `done` once findings are captured.

## Surprises & Discoveries

- Observation: Existing profiling artifacts include only `.diagsession` binaries and no committed baseline JSON file.
  Evidence: `docs/perf-baselines/` contained `cpu_20260219_040758.diagsession`, `fileio_20260219_040919.diagsession`, and `README.md` with no `baseline_*.json`.

- Observation: There was no reusable script to run VSDiagnostics and generate a publishable report page.
  Evidence: `scripts/` initially contained only `agent-wrap.ps1` and `check-docs-index.py`.

## Decision Log

- Decision: Introduce a custom `RelWithDbgInfo` configuration in CMake while cloning flags from standard `RelWithDebInfo`.
  Rationale: User requested a standard-named RelWithDbgInfo flow, and cloning `RelWithDebInfo` preserves optimized-with-symbols behavior while adding a stable configuration name for scripts and launch profiles.
  Date/Author: 2026-02-19 / codex

- Decision: Use `scripts/profile-exe.ps1` as the primary entrypoint and keep command timing as the initial measurable CPU wall metric.
  Rationale: This creates an immediately reusable, single-command workflow with deterministic output files, even before deeper `.diagsession` metric extraction tooling is added.
  Date/Author: 2026-02-19 / codex

- Decision: Generate `perf-highlights.html` from baseline JSON files through a dedicated renderer script.
  Rationale: JSON remains machine-diffable for regression checks while HTML gives a friendly artifact suitable for gh-pages publication.
  Date/Author: 2026-02-19 / codex

## Outcomes & Retrospective

Current outcome: repository wiring, scripts, and docs are in place for repeatable `RelWithDbgInfo` profiling and HTML reporting. Remaining work is to execute the workflow once in this environment, capture a baseline JSON with measured values, and publish the resulting `perf-highlights.html` update to `gh-pages` in the normal release process.

## Context and Orientation

Build configuration is controlled in `CMakeLists.txt`. Profiling guidance and agent behavior are controlled in:
- `.github/prompts/profile-performance.prompt.md`
- `.github/prompts/snippets/vsdiagnostics-guide.md`
- `.github/agents/profiler.agent.md`
- `.github/prompts/README.md`

Performance baseline storage and reporting are rooted at:
- `docs/perf-baselines/`
- `perf-highlights.html`
- `scripts/profile-exe.ps1`
- `scripts/render-perf-report.ps1`

In this repository, “baseline JSON” means a committed summary under `docs/perf-baselines/` that captures key metrics and regression flags. “Publishable report” means a static HTML file that can be copied to the `gh-pages` branch.

## Plan of Work

Update CMake first so `RelWithDbgInfo` is a valid first-class configuration with deterministic output path `bin/RelWithDbgInfo`. Then update profiling prompts and docs so all canonical commands target that configuration. Add a reusable runner script that builds, profiles CPU/memory/file I/O, writes a new baseline JSON, and computes threshold-based regression flags against the latest prior baseline. Add a renderer script that converts baseline history into `perf-highlights.html` for publication.

## Concrete Steps

Working directory: `C:\Users\Admin\goblin-stream`

1. Build the new configuration:
   cmake --build build --config RelWithDbgInfo

2. Run reusable profiling:
   powershell -ExecutionPolicy Bypass -File scripts/profile-exe.ps1 -BuildConfig RelWithDbgInfo -Focus all -RunLabel baseline

3. Verify outputs:
   - `docs/perf-baselines/baseline_<label>_<timestamp>.json`
   - `docs/perf-baselines/*.diagsession`
   - `perf-highlights.html`

4. Publish report by copying updated `perf-highlights.html` into the `gh-pages` branch in the normal publication flow.

Expected transcript snippet:

   Baseline generated: ...\docs\perf-baselines\baseline_...json
   Report generated: ...\perf-highlights.html

## Validation and Acceptance

Acceptance is met when:

- `cmake --build build --config RelWithDbgInfo` succeeds.
- Running `scripts/profile-exe.ps1` produces non-empty session files and a new baseline JSON.
- `perf-highlights.html` is regenerated and includes at least one baseline row.
- Profiling docs and prompts consistently reference `RelWithDbgInfo` instead of `Release` for canonical profiling.

## Idempotence and Recovery

All edits are additive and idempotent. Re-running `scripts/profile-exe.ps1` creates a new timestamped baseline file and refreshes the same report output path. If profiling fails mid-run, rerun the script; existing baseline JSON files remain valid and unchanged.

## Artifacts and Notes

Pending command evidence and first generated baseline from this new workflow.

## Interfaces and Dependencies

The new reusable interface is:

- `scripts/profile-exe.ps1`
  - `-BuildConfig` (default `RelWithDbgInfo`)
  - `-Focus` (`all|cpu|memory|fileio`)
  - `-RunLabel`
  - `-AppArgs` (default `--headless`)
  - `-SkipBuild`

- `scripts/render-perf-report.ps1`
  - `-BaselineDir` (default `docs/perf-baselines`)
  - `-OutputPath` (default `perf-highlights.html`)

Revision Note (2026-02-19): Created this plan and updated it with concrete implementation files, rationale, and execution/validation commands for the RelWithDbgInfo profiling/reporting workflow.
