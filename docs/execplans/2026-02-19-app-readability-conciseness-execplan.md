---
status: draft
owner: codex
related: none
---

# Refactor App for Readability and Concise Frame-Loop Flow

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `.agent/PLANS.md` from the repository root and must be maintained in accordance with it.

## Purpose / Big Picture

After this change, `src/app.ixx` will read as a small orchestration layer instead of a monolith that mixes initialization details, wait dispatch, frame submission, and logging helpers in one place. A contributor should be able to scan the file top-to-bottom and understand frame lifecycle in minutes: wait, process signals, submit frame, drain on shutdown. The runtime behavior must remain the same, and this will be demonstrated by a successful build plus a headless run that still exits after 30 submitted frames and writes `output.h264`.

## Progress

- [x] (2026-02-19 00:00Z) Captured baseline orientation and constraints from `src/app.ixx`, repository quick-reference guidance, and prior wait-coordinator refactor plan.
- [x] (2026-02-19 00:00Z) Drafted this ExecPlan in `docs/execplans/2026-02-19-app-readability-conciseness-execplan.md`.
- [ ] Implementation not started yet by request.
- [ ] Refactor `Run()` into concise orchestration helpers while preserving behavior.
- [ ] Reduce low-value indirection/log wrapper noise and keep event-driven waiting path explicit.
- [ ] Convert `FrameResources` ownership plumbing to clearer RAII structure where safe.
- [ ] Validate with configure/build and headless run; record evidence and outcomes.

## Surprises & Discoveries

- Observation: `App` currently mixes multiple responsibilities that are logically distinct: startup resource wiring, frame wait orchestration, render/submit flow, and shutdown drain.
  Evidence: `src/app.ixx` contains constructor setup loops, main loop control, wait dispatch, command recording, and multiple logging utilities in one class body.

- Observation: This repository intentionally uses `*&com_ptr` in selected paths because WRL `ComPtr` overloads unary `operator&` and address-taking semantics are non-obvious.
  Evidence: `.github/QUICK_REFERENCE.md` guidance under “ComPtr & Smart Pointers” and WRL `ComPtr` API notes on `operator&` versus `GetAddressOf`.

## Decision Log

- Decision: Keep this plan scoped to readability/structure refactoring in `src/app.ixx` first, with no API changes to encoder or swap-chain modules unless required for clarity.
  Rationale: This reduces risk and keeps behavior-preservation validation straightforward.
  Date/Author: 2026-02-19 / codex

- Decision: Prefer extraction of small, named helper methods over introducing new files during first pass.
  Rationale: The shortest path to better readability is reducing local complexity while preserving current module boundaries.
  Date/Author: 2026-02-19 / codex

- Decision: Preserve existing `*&ComPtr` conventions unless a specific call site is proven safe and clearer with a repository-approved alternative.
  Rationale: Prevent accidental semantic drift in COM pointer/address operations.
  Date/Author: 2026-02-19 / codex

## Outcomes & Retrospective

This section will be completed after implementation and validation. Expected outcome is a shorter, more narrative `App` control flow with unchanged runtime behavior and cleaner ownership semantics in frame resources.

## Context and Orientation

`src/app.ixx` defines the exported `App` class that owns D3D12 device/swap-chain setup, NVENC frame encoding integration, frame wait dispatch, command submission, and shutdown draining. The current `Run()` loop already follows an event-driven model through `FrameWaitCoordinator`, but the method still interleaves timing logs, wait handling, message pumping, readiness checks, present handling, encoding, and frame bookkeeping in one long block.

A “waitable component” in this repository means a pair of values: a Win32 `HANDLE` and an `App` member callback invoked when that handle is signaled by `MsgWaitForMultipleObjects`. The code already has this concept in `FrameWaitCoordinator::WaitableComponent`.

`FrameResources` currently stores vectors of raw COM pointers and raw Win32 events with a manual destructor that releases/clears each collection. This is functional but verbose and easy to misread during future modifications.

Key files relevant to this plan:

- `src/app.ixx` (primary refactor target)
- `src/encoder/frame_encoder.h` and `src/encoder/frame_encoder.cpp` (event/query APIs consumed by `App`)
- `src/encoder/bitstream_file_writer.h` and `src/encoder/bitstream_file_writer.cpp` (writer event/query APIs consumed by `App`)
- `.github/QUICK_REFERENCE.md` (repository rules including `ComPtr` address semantics)

## Plan of Work

First, reshape `App::Run` into a compact orchestration method by extracting named helpers that each represent one frame-loop phase. The body of `Run` should mostly read as: gather frame timing, wait/dispatch, handle messages if needed, attempt frame submit, update counters, and stop on headless limit.

Second, simplify local helper surface area by merging or removing logging wrappers that only forward parameters and add little semantic value. Keep logging behavior itself intact; only reduce indirection where it does not clarify intent.

Third, improve `FrameResources` readability by replacing manual release-heavy patterns with clearer RAII ownership expression where safe. If a complete ownership conversion is too invasive for one pass, perform a partial conversion that reduces destructor noise while preserving exact behavior.

Fourth, keep wait-dispatch flow explicit and data-driven. Preserve `FrameWaitCoordinator` and ensure it remains the only steady-state path for signal-driven completion processing in the frame loop.

Finally, validate with configure/build and a headless run. Capture concise evidence in this plan and update all living sections.

## Milestones

### Milestone 1: Make the main loop read as orchestration

At the end of this milestone, `App::Run` is shorter and organized around named helper methods that represent frame-loop stages. No behavior changes are intended. A reader should be able to identify control flow from method names without parsing every branch.

### Milestone 2: Reduce structural noise around waits and logs

At the end of this milestone, wait handling remains event-driven and centralized, and low-value wrappers are reduced or merged. The result should be less branch and parameter plumbing while preserving log output points and behavior.

### Milestone 3: Clarify frame resource ownership

At the end of this milestone, `FrameResources` ownership and cleanup semantics are easier to reason about than the current manual release loops. The destructor path should be smaller or simpler, with equivalent runtime behavior.

### Milestone 4: Validate behavior and record evidence

At the end of this milestone, the Debug build succeeds, the app runs headless, and drain behavior remains intact. This plan file is updated with outcomes, discoveries, and final decision notes.

## Concrete Steps

Working directory for all commands:

    c:\Users\Admin\goblin-stream-wt-wait-refactor

Validation commands to run after implementation work is complete:

    cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
    cmake --build build --config Debug
    .\bin\Debug\goblin-stream.exe --headless

Expected high-level outcomes:

- Configure and build complete successfully.
- Headless run exits normally after bounded frame submission.
- `output.h264` is produced or updated.

If a command fails, rerun only the failed command after fixing the local issue; these steps are idempotent and safe to repeat.

## Validation and Acceptance

Acceptance is behavior-based:

- `src/app.ixx` is measurably easier to navigate: `Run()` is shorter and primarily orchestration.
- Wait-driven completion handling remains explicit and functional.
- Build succeeds in Debug configuration.
- Headless execution still produces expected bounded-run behavior and encode output artifact.

A reviewer can verify by reading `Run()` first, then following extracted helper names in order, without needing to jump through unrelated setup or cleanup details.

## Idempotence and Recovery

This work is source-only and should be safe to apply incrementally. Configure/build commands can be run repeatedly. If a refactor step introduces a compile break, revert only the local helper extraction for that step and re-run build before continuing. If ownership conversion in `FrameResources` proves too risky in one pass, keep existing behavior and land only non-invasive readability changes first.

## Artifacts and Notes

Artifacts to capture during implementation phase:

- Short build success excerpt showing `goblin-stream.vcxproj -> ...\bin\Debug\goblin-stream.exe`.
- Short headless run confirmation excerpt.
- Brief diff notes summarizing how `Run()` was decomposed and which helper wrappers were removed.

No implementation has been started yet in this draft.

## Interfaces and Dependencies

No new external dependencies are allowed. Keep current stack: C++23, Windows APIs, D3D12, NVENC integration, existing `Try |` error style, and repository naming/style constraints.

Target interfaces after refactor should still include the existing wait coordinator surface:

- `App::FrameWaitCoordinator`
- `App::FrameWaitCoordinator::WaitableComponent`
- `App::FrameWaitCoordinator::Wait(...)`

Any new helper methods introduced in `App` should be private, narrowly scoped, and named for user-visible frame-loop intent rather than low-level mechanics.

Revision note (2026-02-19): Initial draft created at user request to provide a refactor blueprint only, with implementation intentionally deferred.
