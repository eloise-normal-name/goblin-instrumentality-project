# Gate Frame Debug Logging To Debug-Oriented Builds

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This document follows `.agent/PLANS.md` from the repository root and must be maintained in accordance with that file.

## Purpose / Big Picture

After this change, frame-by-frame debug logging is active only in `Debug` and `RelWithDebInfo` builds. `Release` builds no longer create or write `debug_output.txt`, so production runs avoid unnecessary file I/O and diagnostic noise. You can verify this by running the app in headless mode for different configurations and checking whether `debug_output.txt` is created.

## Progress

- [x] (2026-02-19 09:31Z) Reviewed `.agent/PLANS.md`, `.github/QUICK_REFERENCE.md`, `.github/copilot-instructions.md`, and current logging call sites in `src/app.ixx`, `src/frame_debug_log.h`, `src/frame_debug_log.cpp`, `CMakeLists.txt`.
- [x] (2026-02-19 09:32Z) Added build-configuration compile definition in `CMakeLists.txt` so frame debug logging is enabled only for `Debug` and `RelWithDebInfo`.
- [x] (2026-02-19 09:32Z) Refactored `FrameDebugLog` implementation to compile to a no-op in non-debug-oriented configurations while preserving call sites in `src/app.ixx`.
- [x] (2026-02-19 09:32Z) Updated `README.md` to document configuration-limited logging behavior.
- [x] (2026-02-19 09:33Z) Built and validated `Debug` and `Release` configurations and ran headless binaries to confirm runtime gating behavior.

## Surprises & Discoveries

- Observation: `FrameDebugLog` is referenced directly in `src/app.ixx` through many `Line()` calls inside the main loop and frame synchronization methods.
  Evidence: `rg -n "frame_debug_log|Line\\(\\)" src/app.ixx` shows call sites across run loop and helper methods.

- Observation: Running explicit configure with `-A x64` failed because the existing `build/` directory was originally configured without an explicit platform value.
  Evidence: CMake error reported `generator platform: x64` does not match previous platform and advised reusing compatible flags or a different binary directory.

## Decision Log

- Decision: Keep all existing log call sites in `src/app.ixx` unchanged and make `FrameDebugLog` behavior conditional at compile time.
  Rationale: This avoids invasive changes in frame loop logic and keeps diagnostics available where intended.
  Date/Author: 2026-02-19 / Codex

- Decision: Use a CMake compile definition (`ENABLE_FRAME_DEBUG_LOG`) controlled by generator expressions for config-specific behavior.
  Rationale: CMake already controls per-config options and this avoids runtime checks for something that is purely build-config policy.
  Date/Author: 2026-02-19 / Codex

- Decision: Implement non-debug builds as an inline no-op `FrameDebugLog` class in `src/frame_debug_log.h`, with `src/frame_debug_log.cpp` compiled to active code only when `ENABLE_FRAME_DEBUG_LOG` is set.
  Rationale: This preserves the public API and existing call sites, while removing file I/O behavior from `Release` at compile time.
  Date/Author: 2026-02-19 / Codex

## Outcomes & Retrospective

The refactor is complete and matches the original purpose. `Debug` and `RelWithDebInfo` builds now receive `ENABLE_FRAME_DEBUG_LOG` from CMake, and `Release` builds compile a no-op logging implementation with unchanged call sites. Build validation passed for both `Debug` and `Release`, and runtime validation in `--headless` mode showed `debug_output.txt` created in `Debug` and absent in `Release`.

## Context and Orientation

`src/app.ixx` owns the main frame loop and currently creates `FrameDebugLog frame_debug_log{"debug_output.txt"};` as a member. The class itself is defined in `src/frame_debug_log.h` and implemented in `src/frame_debug_log.cpp`. The build graph is defined in `CMakeLists.txt`, which currently always compiles `src/frame_debug_log.cpp` and does not set a config-based logging definition. The user-visible effect today is that debug logging code is always compiled and active in all build types.

In this repository, a build configuration is the CMake/MSVC configuration such as `Debug`, `RelWithDebInfo`, or `Release`. `RelWithDebInfo` means optimized build output that still includes debug information for diagnostics.

## Plan of Work

First, edit `CMakeLists.txt` to define `ENABLE_FRAME_DEBUG_LOG` only when the active configuration is `Debug` or `RelWithDebInfo`. The definition will be absent in `Release` builds.

Second, refactor `src/frame_debug_log.h` so it exposes the same `FrameDebugLog` API in all configurations, but implements a no-op version when `ENABLE_FRAME_DEBUG_LOG` is not defined. This keeps `src/app.ixx` unchanged and avoids branching call sites.

Third, update `src/frame_debug_log.cpp` so its current file-writing implementation only compiles when `ENABLE_FRAME_DEBUG_LOG` is enabled. In disabled configurations, the translation unit will compile with no symbols, matching the inline no-op header behavior.

Finally, update `README.md` in the repository structure section to state that `frame_debug_log.*` is only active in `Debug` and `RelWithDebInfo` builds.

## Concrete Steps

From `c:\Users\Admin\goblin-stream`:

1. Edit `CMakeLists.txt` and add:
    - `target_compile_definitions(goblin-stream PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ENABLE_FRAME_DEBUG_LOG>")`
2. Edit `src/frame_debug_log.h` to add compile-time split:
    - enabled path keeps current members and methods.
    - disabled path defines `FrameDebugLog` with constructor that accepts `const char*`, `BeginFrame(uint64_t)`, and `Line()` returning a no-op stream.
3. Edit `src/frame_debug_log.cpp` so the file-writing implementation is wrapped by `#ifdef ENABLE_FRAME_DEBUG_LOG`.
4. Edit `README.md` to document active configurations for logging.
5. Run:
    - `powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake -G \"Visual Studio 18 2026\" -A x64 -S . -B build" -TimeoutSec 180`
    - `powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Debug" -TimeoutSec 300`
    - `powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Release" -TimeoutSec 300`

Expected command outcomes:
    - Configure returns success JSON with `exitCode` 0.
    - Both builds return success JSON with `exitCode` 0.

## Validation and Acceptance

Acceptance criteria are:

1. `Debug` build compiles and `FrameDebugLog` file-writing implementation is present.
2. `Release` build compiles with the same app call sites untouched, using no-op logging implementation.
3. `README.md` explicitly states logging is for `Debug` and `RelWithDebInfo`.

If headless run validation is executed:
    - `Debug` headless run should create or update `debug_output.txt`.
    - `Release` headless run should not create new frame log entries because logging is compiled out.

## Idempotence and Recovery

All edits are text changes and can be safely re-applied. Re-running configure/build commands is idempotent in the same `build/` directory. If a build fails after partial edits, fix compile errors and re-run the same build command; no destructive cleanup is required.

## Artifacts and Notes

Key artifact snippets after implementation:
    - `CMakeLists.txt`: `"$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ENABLE_FRAME_DEBUG_LOG>"`
    - Build wrapper result: `cmake --build build --config Debug` returned `exit_code: 0`.
    - Build wrapper result: `cmake --build build --config Release` returned `exit_code: 0`.
    - Runtime validation: `.\\bin\\Debug\\goblin-stream.exe --headless` created `debug_output.txt` (length 15701 bytes in this run).
    - Runtime validation: `.\\bin\\Release\\goblin-stream.exe --headless` left `debug_output.txt` absent.

## Interfaces and Dependencies

The public interface of `FrameDebugLog` must remain:

    class FrameDebugLog {
      public:
        FrameDebugLog(const char* path);
        void BeginFrame(uint64_t frame_index);
        std::ostream& Line();
    };

`src/app.ixx` must continue to instantiate and call this interface without configuration-specific call-site changes.

Dependency and control point:
    - `CMakeLists.txt` sets `ENABLE_FRAME_DEBUG_LOG` by configuration.
    - `src/frame_debug_log.h` and `src/frame_debug_log.cpp` consume this definition.

Revision Note (2026-02-19): Initial ExecPlan created to implement build-configuration gating for frame debug logging with minimal code churn and explicit validation steps.
Revision Note (2026-02-19): Updated all living sections after implementation and validation, including build/run evidence and the final design decision for no-op behavior in non-debug-oriented builds.
