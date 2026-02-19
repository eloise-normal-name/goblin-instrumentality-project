---
status: done
owner: codex
related: none
---

# Refactor App Frame Waiting Into Waitable Component Grouping

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `.agent/PLANS.md` from the repository root.

## Purpose / Big Picture

After this change, the frame wait path in `src/app.ixx` is organized around a single waitable-component grouping, where each component contributes a wait handle and a completion method. This makes adding future waitable components straightforward and reduces duplicated branching in the frame loop. The runtime behavior is preserved: frame-latency wakeups proceed rendering, encoder/write wakeups process completion work and continue the loop.

## Progress

- [x] (2026-02-19T11:21:00Z) Baseline inspected in `src/app.ixx`, `src/encoder/frame_encoder.*`, and `src/encoder/bitstream_file_writer.*`.
- [x] (2026-02-19T11:23:00Z) Created ExecPlan with implementation and validation steps.
- [x] (2026-02-19T11:22:10Z) Refactored `src/app.ixx` to replace `FrameWaitResult`/`WaitForFrame` with grouped waitable components plus completion method dispatch.
- [x] (2026-02-19T11:22:32Z) Configured worktree and built Debug successfully.
- [x] (2026-02-19T11:22:32Z) Finalized plan status and retrospective.
- [x] (2026-02-19T11:33:47Z) Applied feedback: removed steady-state polling calls to `ProcessCompletedFrames` in `App::Run`; completion processing now runs via encoder event callback (`OnEncoderReady`) and explicit shutdown drains only.
- [x] (2026-02-19T11:34:09Z) Rebuilt Debug after feedback update; build succeeded (`goblin-stream.vcxproj -> ...\\bin\\Debug\\goblin-stream.exe`).

## Surprises & Discoveries

- Observation: Existing wait handling already depended on three signaling sources, but dispatch logic was coupled to manually tracked handle indices.
  Evidence: Original `WaitForFrame` in `src/app.ixx` matched `WAIT_OBJECT_0 + index` branches.

- Observation: Running configure and build in parallel can race because `build/` may not exist when build starts.
  Evidence: `cmake --build build --config Debug` initially failed with `build is not a directory` when launched concurrently with configure.

## Decision Log

- Decision: Keep the refactor local to `src/app.ixx` without changing `FrameEncoder` or `BitstreamFileWriter` APIs.
  Rationale: Those classes already expose the required primitives (`HasPending*`, `Next*Event`, and completion methods); the extensibility issue was App-side orchestration.
  Date/Author: 2026-02-19 / codex

- Decision: Use a `FrameWaitCoordinator::WaitableComponent` record of `{HANDLE, App completion method}` and central dispatch through one wait function.
  Rationale: This directly maps each waitable source to its completion behavior, so adding a new component is additive and localized.
  Date/Author: 2026-02-19 / codex

- Decision: Keep the existing post-submit `frame_encoder.ProcessCompletedFrames(bitstream_writer)` call in `Run()` after `EncodeFrame`.
  Rationale: The request was structural grouping/refactor, not scheduling changes; preserving this call avoids accidental behavior drift in completion pacing.
  Date/Author: 2026-02-19 / codex

- Decision: During steady-state main-loop execution, call `ProcessCompletedFrames` only when the encoder completion event signals (`OnEncoderReady` path), not as a polling step.
  Rationale: Completion handling is already event-driven through `MsgWaitForMultipleObjects`; polling `ProcessCompletedFrames` in the main loop adds avoidable wakeups and CPU churn.
  Date/Author: 2026-02-19 / codex

- Decision: Remove both steady-state polling call sites in `App::Run` (top-of-loop drain and post-submit drain) to match the event-driven rule.
  Rationale: Keeping either call site would still introduce polling semantics and dilute the waitable-component architecture objective from this plan.
  Date/Author: 2026-02-19 / codex

## Outcomes & Retrospective

The App frame wait path now uses `FrameWaitCoordinator` and `FrameLoopAction` to drive one wait-and-dispatch flow. `Run()` no longer carries separate writer/encoder wait branches tied to `FrameWaitResult` values. In steady-state loop execution, encoder completion processing is event-driven (via `OnEncoderReady`) rather than polled from `Run()`.

What remains: runtime profiling and functional playback checks were out of scope for this request.

## Context and Orientation

`src/app.ixx` owns the frame loop and previously used `FrameWaitResult` plus `WaitForFrame` to wait on swap-chain frame latency, bitstream writer completion, and frame encoder completion. The old version built a mixed handle array and dispatched by index checks.

A waitable component here means a runtime record containing:

- a Windows waitable handle (`HANDLE`) for `MsgWaitForMultipleObjects`, and
- a completion method on `App` to call when that handle is signaled.

The refactor moved this mapping into a grouped structure so future components can be added without introducing new enum variants and parallel branching logic.

## Plan of Work

In `src/app.ixx`:

- Replace `FrameWaitResult` with `FrameLoopAction`.
- Add nested `FrameWaitCoordinator` with `WaitableComponent` and `Wait`.
- Add `App` completion methods `OnFrameLatencyReady`, `OnWriterReady`, and `OnEncoderReady`.
- Add `WaitForSignal` to perform `MsgWaitForMultipleObjects` and dispatch by selected component.
- Update `Run()` to use `frame_wait_coordinator.Wait(*this, ...)` and keep current message-pump-on-continue behavior.

## Concrete Steps

From `c:\Users\Admin\goblin-stream-wt-wait-refactor`:

1. Configure build files:

    cmake -G "Visual Studio 18 2026" -A x64 -S . -B build

2. Build Debug:

    cmake --build build --config Debug

Observed transcript excerpt:

    goblin-stream.vcxproj -> C:\Users\Admin\goblin-stream-wt-wait-refactor\bin\Debug\goblin-stream.exe

## Validation and Acceptance

Acceptance criteria and observed result:

- Debug build succeeds after refactor: met.
- `App::Run` no longer uses `FrameWaitResult` and branch-by-index wait dispatch: met.
- Waiting behavior equivalence retained by completion methods:
  - frame latency -> proceed frame path,
  - writer event -> `DrainCompleted()` then continue loop,
  - encoder event -> `ProcessCompletedFrames()` then continue loop (event-driven only; no steady-state polling calls in `Run()`),
  - message queue wakeups -> continue to message pump.

## Idempotence and Recovery

Edits are source-only and repeatable in the worktree. Re-running configure/build is safe. If a compile issue appears, only `src/app.ixx` changes need adjustment; no migrations or generated-source changes are involved.

## Artifacts and Notes

Key changed files:

- `src/app.ixx`
- `docs/execplans/2026-02-19-app-waitable-components-execplan.md`

## Interfaces and Dependencies

`src/app.ixx` now defines and uses:

- `enum class FrameLoopAction { Continue, Proceed };`
- `class FrameWaitCoordinator` with:
  - `struct WaitableComponent { HANDLE handle; FrameLoopAction (App::*on_complete)(); };`
  - `FrameLoopAction Wait(App& app, uint32_t frames_submitted, double cpu_ms);`
- `App` completion methods:
  - `FrameLoopAction OnFrameLatencyReady();`
  - `FrameLoopAction OnWriterReady();`
  - `FrameLoopAction OnEncoderReady();`
- `App::WaitForSignal(...)` for centralized wait and dispatch.

No external dependencies were added.

Revision note: Updated plan from `active` to `done` after implementing and validating the App wait-coordinator refactor, then revised to enforce and implement event-driven-only steady-state `ProcessCompletedFrames` calls in `Run()` and revalidated with a Debug build.
