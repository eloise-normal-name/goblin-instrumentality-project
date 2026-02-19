# Simplify Frame Debug Logging With Compile-Gated Console Macros

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document follows `.agent/PLANS.md` from the repository root and must be maintained in accordance with that file.

## Purpose / Big Picture

After this change, debug logging in the frame loop is simpler to maintain: call sites use one macro-style API instead of the current stream-based `FrameDebugLog` class. Logs go to process console streams (`stderr` by default) in debug-oriented builds and are compiled out for `Release`. A developer can verify success by building `Debug`, running headless with stream redirection, and seeing expected frame/encoder lines in redirected output without any `debug_output.txt` file lifecycle code.

## Progress

- [x] (2026-02-19 09:47Z) Captured handoff scope, constraints, and implementation decisions from user-agent planning discussion.
- [x] (2026-02-19 09:48Z) Confirmed current logging architecture and call sites in `src/app.ixx`, `src/frame_debug_log.h`, `src/frame_debug_log.cpp`, and `CMakeLists.txt`.
- [x] (2026-02-19 09:49Z) Implemented `src/debug_log.h` and migrated frame-loop logging in `src/app.ixx` to `FRAME_LOG(...)` with local `std::chrono::steady_clock` `cpu_ms` timing.
- [x] (2026-02-19 09:49Z) Removed legacy `FrameDebugLog` source files and cleaned `CMakeLists.txt` source list.
- [x] (2026-02-19 09:49Z) Updated `README.md` to describe compile-gated `stderr` logging and `WIN32` subsystem caveat.
- [x] (2026-02-19 09:50Z) Built `Debug` and `Release` successfully via `scripts/agent-wrap.ps1` (`exit_code` 0 for both runs).
- [x] (2026-02-19 09:54Z) Verified runtime logging behavior with wrapper-captured streams: `Debug` emits frame/encoder lines on `stderr`; `Release` does not emit macro-driven debug lines.
- [x] (2026-02-19 09:57Z) Updated this ExecPlan living sections with implementation evidence, validation caveats, and final outcomes.

## Surprises & Discoveries

- Observation: the current executable is defined with `WIN32` subsystem in `CMakeLists.txt`, so console output is not automatically visible in a standalone launch.
  Evidence: `add_executable(goblin-stream WIN32 ${SOURCES})` in `CMakeLists.txt`.

- Observation: frame logging is already compile-gated by configuration (`ENABLE_FRAME_DEBUG_LOG`) and active only for `Debug` and `RelWithDebInfo`.
  Evidence: generator expression compile definition in `CMakeLists.txt` and existing no-op path in `src/frame_debug_log.h`.

- Observation: most logging usage is concentrated in `src/app.ixx`, with repeated `frame_debug_log.Line() << ...` patterns that are straightforward to replace.
  Evidence: call sites at `src/app.ixx` around run loop, wait logic, fence status, present handling, and submission logs.

- Observation: direct PowerShell `2>` redirection from this `WIN32` subsystem executable produced empty `debug_stderr.txt` and `release_stderr.txt` in this environment.
  Evidence: `Get-Item debug_stderr.txt, release_stderr.txt | Select-Object Name,Length` showed both lengths as `0` after direct headless runs.

- Observation: wrapper-captured `stderr` proves the macro logs are present in `Debug` and absent in `Release`, matching compile gating.
  Evidence: `.agent/logs/20260219-015032-137-42509/stderr.log` contains lines such as `frame=0 cpu_ms=0.000 encoder_stats ...`; `.agent/logs/20260219-015032-184-62843/stderr.log` is effectively empty for `Release`.

- Observation: `Release` initially produced `C4100` warnings because log-only parameters became unused when macros compiled out.
  Evidence: build log preview from run `20260219-014923-948-83053` listed `C4100` warnings in `src/app.ixx`; follow-up builds after adding `(void)` guards were warning-free.

## Decision Log

- Decision: replace the stream-style `FrameDebugLog` API with a single macro call style (`FRAME_LOG(...)`) at call sites.
  Rationale: this eliminates custom stream buffer/state management and makes log lines direct and obvious.
  Date/Author: 2026-02-19 / Codex

- Decision: default log sink is `stderr` (console stream), not a file.
  Rationale: user chose console-oriented logging; `stderr` is conventional for diagnostics and easy to redirect in automated runs.
  Date/Author: 2026-02-19 / Codex

- Decision: keep compile-time gating with `ENABLE_FRAME_DEBUG_LOG`; do not add runtime log-level plumbing.
  Rationale: current policy is build-configuration based and already integrated; preserving it minimizes scope.
  Date/Author: 2026-02-19 / Codex

- Decision: do not change subsystem to `CONSOLE` in this task.
  Rationale: subsystem change alters runtime UX by adding a console window; this plan focuses only on simplifying logging internals and call sites.
  Date/Author: 2026-02-19 / Codex

- Decision: keep runtime validation evidence primarily from wrapper-captured `stderr` logs rather than direct shell redirection files.
  Rationale: direct redirection files were empty in this environment for the `WIN32` subsystem executable, while wrapper piping captured expected `Debug` logs reliably and `Release` silence.
  Date/Author: 2026-02-19 / Codex

- Decision: explicitly cast log-only parameters to `(void)` inside helper methods when `ENABLE_FRAME_DEBUG_LOG` is disabled.
  Rationale: this preserves compile-time log stripping while keeping `Release` builds clean under `/W4` without widening scope.
  Date/Author: 2026-02-19 / Codex

## Outcomes & Retrospective

The migration is implemented end-to-end. `FrameDebugLog` file-backed stream logging was removed, call sites now use `FRAME_LOG(...)`, and logging is still compile-gated by `ENABLE_FRAME_DEBUG_LOG`. `Debug` and `Release` builds both succeed. Validation shows `Debug` emits frame and encoder log lines to `stderr` under wrapper capture, while `Release` emits no macro-driven debug lines. A remaining environment caveat is that direct `2>` redirection to files from this `WIN32` target yielded empty files in this shell setup, so wrapper logs are the primary runtime proof artifact.

## Context and Orientation

`src/app.ixx` owns the main frame loop and currently stores `FrameDebugLog frame_debug_log{"debug_output.txt"};` as a member. Logging calls use stream chaining via `frame_debug_log.Line() << ... << "\n";`, and per-frame timing prefixing is implemented inside `src/frame_debug_log.cpp`. The logging class interface and a no-op implementation branch live in `src/frame_debug_log.h`, and file-backed implementation lives in `src/frame_debug_log.cpp`.

The build defines `ENABLE_FRAME_DEBUG_LOG` only for `Debug` and `RelWithDebInfo` in `CMakeLists.txt`. This means logging code can stay compile-time eliminated for `Release`.

In this plan, "macro-based logging" means a preprocessor macro that expands to `std::fprintf` in enabled builds and to a no-op in disabled builds. "Compile-gated" means the compiler removes logging paths entirely when `ENABLE_FRAME_DEBUG_LOG` is not defined.

## Plan of Work

Create a new header `src/debug_log.h` that defines logging macros for enabled and disabled builds. In enabled builds, `FRAME_LOG(format, ...)` writes to `stderr` and appends a newline. In disabled builds, the macro expands to a no-op expression that does not evaluate arguments.

Edit `src/app.ixx` to include `debug_log.h`, remove the `FrameDebugLog` member, and replace all `frame_debug_log.Line()` stream expressions with `FRAME_LOG` calls that encode the same information as plain text fields. Keep the current frame/encoder instrumentation points intact so behavioral visibility remains equivalent.

Replace class-owned frame timing with a local timing helper in `App::Run()` (either QPC or `std::chrono::steady_clock`). Compute `cpu_ms` once per loop iteration and include it explicitly in each log line that needs frame context.

Delete `src/frame_debug_log.h` and `src/frame_debug_log.cpp` after call-site migration, then remove `src/frame_debug_log.cpp` from `SOURCES` in `CMakeLists.txt`.

Update `README.md` where repository structure describes logging. Replace `frame_debug_log.*` references with the new macro helper and document that console logs are visible when running from a terminal or when redirecting streams, because the app remains a `WIN32` subsystem executable.

## Concrete Steps

From `c:\Users\Admin\goblin-stream`:

1. Create `src/debug_log.h` with compile-gated macros.
2. Edit `src/app.ixx`:
    - include `debug_log.h`
    - remove `#include "frame_debug_log.h"`
    - remove `FrameDebugLog frame_debug_log{"debug_output.txt"};`
    - migrate every log call to `FRAME_LOG(...)`
    - implement local frame timing for `cpu_ms`
3. Delete legacy files:
    - `src/frame_debug_log.h`
    - `src/frame_debug_log.cpp`
4. Edit `CMakeLists.txt` and remove `src/frame_debug_log.cpp` from `SOURCES`.
5. Edit `README.md` logging notes.
6. Build and validate using wrapper:
    - `powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Debug" -TimeoutSec 300`
    - `powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Release" -TimeoutSec 300`
7. Runtime verification with redirection:
    - `.\bin\Debug\goblin-stream.exe --headless 2> debug_stderr.txt`
    - `.\bin\Release\goblin-stream.exe --headless 2> release_stderr.txt`

Expected command outcomes:
    - Both builds return success with `exitCode` 0.
    - `debug_stderr.txt` contains frame/encoder log lines.
    - `release_stderr.txt` contains no macro-driven debug lines.

Observed outcomes during implementation:
    - Build runs succeeded:
      - `20260219-014955-644-92417` (`Debug`, `exit_code: 0`)
      - `20260219-014955-644-78667` (`Release`, `exit_code: 0`)
    - Direct `2>` files were empty in this environment (`debug_stderr.txt` and `release_stderr.txt` length `0`).
    - Wrapper run `20260219-015032-137-42509` captured `Debug` log lines in `.agent/logs/.../stderr.log`.
    - Wrapper run `20260219-015032-184-62843` captured no debug log lines in `Release` stderr log.

## Validation and Acceptance

Acceptance is met when all of the following are true:

1. `Debug` build succeeds after removing `FrameDebugLog` files and using macro logging.
2. `Release` build succeeds with logging compiled out and no unresolved logging symbols.
3. `src/app.ixx` has no `frame_debug_log` usage and logs still appear at the same lifecycle points.
4. Headless `Debug` run with redirected `stderr` shows lines for:
    - encoder stats
    - wait results
    - fence/present status
    - frame submitted values
5. Headless `Release` run with redirected `stderr` has no comparable debug lines.
6. `README.md` documents the new logging mechanism and the `WIN32` console visibility caveat.

## Idempotence and Recovery

These edits are text-only and safe to reapply. Re-running build and runtime validation commands is idempotent. If migration is interrupted, restore a compiling state by temporarily keeping `frame_debug_log` includes and member until all call sites in `src/app.ixx` are converted, then remove legacy files in one final step. If validation output is unclear, delete `debug_stderr.txt` and `release_stderr.txt` and re-run the commands to collect clean artifacts.

## Artifacts and Notes

Expected `src/debug_log.h` shape:

    #pragma once
    #include <cstdio>
    #ifdef ENABLE_FRAME_DEBUG_LOG
    #define FRAME_LOG(fmt, ...) do { std::fprintf(stderr, fmt "\n", __VA_ARGS__); } while (0)
    #else
    #define FRAME_LOG(...) do { } while (0)
    #endif

Notes:
    - Implementer should handle zero-variadic-argument use safely if needed (for example by requiring at least one format argument or using a helper macro variant).
    - If any log message previously relied on stream formatting state (`std::fixed`, precision), encode format explicitly in the `printf` format string.

## Interfaces and Dependencies

Define in `src/debug_log.h`:

    #ifdef ENABLE_FRAME_DEBUG_LOG
    #define FRAME_LOG(format, ...) ...
    #else
    #define FRAME_LOG(format, ...) ...
    #endif

Usage contract in `src/app.ixx`:

    FRAME_LOG("frame=%llu cpu_ms=%.3f ...", frame_index, cpu_ms, ...);

Build dependency contract:

    - `ENABLE_FRAME_DEBUG_LOG` remains defined only for `Debug` and `RelWithDebInfo` via `CMakeLists.txt`.
    - No new third-party library is introduced.
    - Target remains `WIN32` subsystem unless a separate product decision explicitly requests `CONSOLE`.

Revision Note (2026-02-19): Created this ExecPlan as a handoff artifact for a new agent to implement macro-based console debug logging with compile-time gating and no subsystem change.
Revision Note (2026-02-19 09:57Z): Updated this living ExecPlan after implementation to reflect completed milestones, build/run evidence, release-warning mitigation, and the environment-specific stderr redirection caveat.
