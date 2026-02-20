---
status: done
owner: codex
related: none
---

# Add headless native coverage script and an intentionally uncovered C++ source

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document is maintained in accordance with `.agent/PLANS.md`.

## Purpose / Big Picture

After this change, a contributor can run one PowerShell script that builds the app, runs `goblin-stream.exe --headless`, and emits native C++ coverage artifacts (`.coverage` and Cobertura XML) that CI can publish. The change also adds one deliberately unexecuted C++ translation unit so the coverage report shows known uncovered lines, proving the tool is wired correctly.

## Progress

- [x] (2026-02-20 00:05Z) Created a dedicated git worktree at `..\goblin-stream-coverage-headless` on branch `feat/coverage-headless-script`.
- [x] (2026-02-20 00:10Z) Added `scripts/run-headless-coverage.ps1` to build and collect native coverage by launching the headless app.
- [x] (2026-02-20 00:11Z) Added `scripts/coverage.runsettings` with native instrumentation enabled.
- [x] (2026-02-20 00:12Z) Added `src/coverage_dummy.cpp` and linked it in `CMakeLists.txt`.
- [x] (2026-02-20 00:13Z) Updated `README.md` with headless coverage usage.
- [x] (2026-02-20 00:22Z) Validated `scripts/run-headless-coverage.ps1 -BuildConfig Debug -SkipBuild`; produced `.coverage` and Cobertura files and confirmed `src/coverage_dummy.cpp` reports `line-rate="0"`.
- [x] (2026-02-20 00:24Z) Validated full script path including wrapped build (`-BuildTimeoutSec 120`) and confirmed successful artifact generation.
- [x] (2026-02-20 00:34Z) Added human-readable coverage outputs (`.summary.txt` and `.report.html`) and validated generation in the worktree.

## Surprises & Discoveries

- Observation: This repository has no CTest/test target wiring in `CMakeLists.txt`, so "test" must be the executable run itself for now.
  Evidence: `CMakeLists.txt` defines only `add_executable(goblin-stream WIN32 ${SOURCES})` and no `enable_testing()` or `add_test(...)`.

- Observation: The installed `Microsoft.CodeCoverage.Console` version does not support a `report` command.
  Evidence: `Microsoft.CodeCoverage.Console -h` lists `collect`, `connect`, `merge`, `shutdown`, `snapshot`, `instrument`, and `uninstrument` only.

- Observation: Coverage settings files must use `<Configuration>` as the XML root for this tool path; `<RunSettings>` is rejected.
  Evidence: `collect --settings <file>` emitted `"is not a valid Coverage settings file"` for `<RunSettings>`, and accepted the same content under `<Configuration>`.

- Observation: `python scripts/check-docs-index.py` currently fails due multiple pre-existing unindexed docs in `docs/execplans/` and `docs/perf-baselines/README.md`.
  Evidence: The command reported a list of missing references that includes many historical files not touched by this plan.

## Decision Log

- Decision: Implement coverage collection with `Microsoft.CodeCoverage.Console.exe` instead of `dotnet test --collect`.
  Rationale: The project is native C++ and does not currently have a managed test runner path.
  Date/Author: 2026-02-20 / codex

- Decision: Keep coverage scope focused to modules matching `goblin-stream` in `scripts/coverage.runsettings`.
  Rationale: This reduces noise from system modules and keeps CI output actionable.
  Date/Author: 2026-02-20 / codex

- Decision: Add a separate dummy source file (`src/coverage_dummy.cpp`) that is compiled but never executed.
  Rationale: This creates a deterministic uncovered region to validate that native coverage reporting is functioning.
  Date/Author: 2026-02-20 / codex

- Decision: Lower wrapped build timeout in `scripts/run-headless-coverage.ps1` from 300 seconds to 120 seconds (configurable via `-BuildTimeoutSec`).
  Rationale: The project build completes far faster in normal conditions, and the lower default catches hangs sooner.
  Date/Author: 2026-02-20 / codex

- Decision: Convert `.coverage` to Cobertura with `merge --output-format cobertura` instead of `report`.
  Rationale: `report` is not available in this installed tool version; `merge` provides a supported conversion path.
  Date/Author: 2026-02-20 / codex

- Decision: Generate human-readable artifacts directly in `scripts/run-headless-coverage.ps1` by parsing Cobertura XML into text and HTML summaries.
  Rationale: CI artifacts are easier to review quickly without opening raw XML or `.coverage` binaries.
  Date/Author: 2026-02-20 / codex

## Outcomes & Retrospective

The script, settings, source wiring, and README updates are complete and validated in the dedicated worktree. Running `scripts/run-headless-coverage.ps1 -BuildConfig Debug -SkipBuild` produced:

- `artifacts/coverage/headless_20260219_162308.coverage`
- `artifacts/coverage/headless_20260219_162308.cobertura.xml`

The Cobertura file includes `src/coverage_dummy.cpp` with `line-rate="0"` and both dummy methods at `line-rate="0"`, proving the intentionally unexecuted file is flagged as uncovered.

The full default path with build also succeeds:

- `powershell -ExecutionPolicy Bypass -File scripts/run-headless-coverage.ps1 -BuildConfig Debug -BuildTimeoutSec 120`

The script now also emits:

- `artifacts/coverage/headless_<timestamp>.summary.txt`
- `artifacts/coverage/headless_<timestamp>.report.html`

Process lesson: when an ExecPlan specifies a dedicated worktree, all commands and edits must remain in that worktree until explicit user approval to switch contexts.

## Context and Orientation

`CMakeLists.txt` is the single source list for the app executable; adding a file there guarantees it is compiled into `goblin-stream.exe`. `scripts/agent-wrap.ps1` is the project’s safe command wrapper for long-running tasks like builds. `scripts/run-headless-coverage.ps1` is introduced as the new entry point for coverage collection. `scripts/coverage.runsettings` holds code-coverage collector settings for native instrumentation. `src/coverage_dummy.cpp` contains intentionally unexecuted code paths used to verify low-coverage detection.

## Plan of Work

Add one new script (`scripts/run-headless-coverage.ps1`) that optionally builds the selected config, resolves `vswhere.exe`, resolves `Microsoft.CodeCoverage.Console.exe`, runs `collect` against `bin/<config>/goblin-stream.exe --headless`, then runs `report --format cobertura`. Add `scripts/coverage.runsettings` with native static and dynamic instrumentation enabled and module filtering for `goblin-stream`. Add `src/coverage_dummy.cpp` with callable but never-invoked functions. Wire this file into `CMakeLists.txt` so it is included in the build. Add a README usage section with the exact command and expected artifact paths.

## Concrete Steps

From repository root (`c:\Users\Admin\goblin-stream-coverage-headless`):

    powershell -ExecutionPolicy Bypass -File scripts/configure-cmake.ps1
    powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Debug" -TimeoutSec 120 -PrintOutput
    powershell -ExecutionPolicy Bypass -File scripts/run-headless-coverage.ps1 -BuildConfig Debug -BuildTimeoutSec 120

Expected high-signal output includes:

    Coverage file: <repo>\artifacts\coverage\headless_<timestamp>.coverage
    Cobertura file: <repo>\artifacts\coverage\headless_<timestamp>.cobertura.xml

## Validation and Acceptance

Acceptance is met when:

1. `scripts/run-headless-coverage.ps1 -BuildConfig Debug` exits with code 0.
2. Both coverage artifacts exist under `artifacts/coverage/`.
3. The Cobertura XML contains entries for `src/coverage_dummy.cpp` and shows uncovered lines for functions in that file.

This proves native coverage is collected from the headless workload and that uncovered application code is visible in reports.

## Idempotence and Recovery

All steps are idempotent. Re-running the script creates timestamped artifacts, so no destructive overwrite is required. If configure fails due to stale cache/generator mismatch, run:

    powershell -ExecutionPolicy Bypass -File scripts/configure-cmake.ps1 -ForceClean

If coverage tool discovery fails, verify Visual Studio 2026 Community installation and rerun.

## Artifacts and Notes

Key files introduced or changed:

- `scripts/run-headless-coverage.ps1`
- `scripts/coverage.runsettings`
- `src/coverage_dummy.cpp`
- `CMakeLists.txt`
- `README.md`

## Interfaces and Dependencies

The new script depends on:

- `scripts/agent-wrap.ps1` for build invocation.
- `C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe` for Visual Studio discovery.
- `Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe` inside the discovered Visual Studio installation.

No external third-party libraries are introduced.

Revision note: Added explicit process lesson that worktree-scoped ExecPlans must not switch execution context mid-session without explicit user approval.
