---
status: draft
owner: Nova [g53c]
related: PR #85 (refactor(app): simplify Run() into orchestration, remove log wrapper noise)
---

# Refactor `App` into a cleaner coordinator of graphics and encoding systems

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `.agent/PLANS.md` from the repository root and must be maintained in accordance with it.

## Purpose / Big Picture

After this change, `src/app.ixx` will read as an orchestration layer that coordinates frame lifecycle, wait dispatch, and shutdown behavior, instead of owning dense low-level D3D12 recording and frame-resource mechanics directly. A new contributor should be able to open `App::Run()` and understand the system flow quickly: wait for signals, process completions, submit/present/encode, and drain on exit. The behavior should remain equivalent to current runtime behavior, including startup pre-recorded command lists and headless bounded execution.

## Progress

- [x] (2026-02-19 00:00Z) Captured current `App` responsibility map and extraction seams from `src/app.ixx` and collaborators in `src/graphics/*` and `src/encoder/*`.
- [x] (2026-02-19 00:00Z) Confirmed refactor direction with user: deep refactor, hybrid file split, preserve startup pre-recorded command lists, keep App-owned defaults.
- [x] (2026-02-19 00:00Z) Authored this draft ExecPlan only; no implementation edits have been executed under this plan.
- [ ] Implement graphics helper extraction for frame resource ownership and cleanup.
- [ ] Implement command recording extraction while preserving startup pre-record semantics.
- [ ] Simplify `App::Run()` into coordinator-first orchestration and keep wait/completion semantics equivalent.
- [ ] Build and run validation (Debug/Release build and headless smoke run) and record evidence.

## Surprises & Discoveries

- Observation: `App` currently blends orchestration with low-level frame resource ownership and command list recording details.
  Evidence: `src/app.ixx` includes `FrameResources` internals, `RecordCommandList(...)`, frame loop orchestration, and shutdown drain in one class.

- Observation: Existing helpers in `src/graphics/d3d12_commands.*` and `src/graphics/d3d12_resources.*` provide a natural landing area for extraction, but they are not currently used by `App` for command recording.
  Evidence: File inspection of `src/graphics/d3d12_commands.cpp/.h` and `src/app.ixx` call paths.

- Observation: There are non-obvious synchronization and encoder lifecycle invariants that must be preserved during cleanup.
  Evidence: `FrameEncoder` fixed-size pending ring behavior and completion handling paths in `src/encoder/frame_encoder.cpp`; wait and drain behavior in `src/app.ixx`.

## Decision Log

- Decision: Target a deep refactor with a hybrid split where high-level orchestration stays in `src/app.ixx` and selected low-level concerns move to focused helpers.
  Rationale: Maximizes readability gains while keeping module churn bounded.
  Date/Author: 2026-02-19 / Nova [g53c]

- Decision: Preserve startup pre-recorded command lists exactly in this refactor.
  Rationale: Reduces risk of introducing frame-time behavior drift while improving structure.
  Date/Author: 2026-02-19 / Nova [g53c]

- Decision: Keep App-owned defaults (output file and encoder defaults) unchanged in this pass.
  Rationale: Keeps this plan focused on architecture/readability rather than configuration redesign.
  Date/Author: 2026-02-19 / Nova [g53c]

## Outcomes & Retrospective

This section will be completed after implementation and validation. Success means `App` reads as a coordinator with preserved behavior, and verification evidence shows build and runtime parity.

## Context and Orientation

The primary target is `src/app.ixx`, which currently owns both high-level coordination and significant low-level operational detail. In this repository, `App` coordinates these systems:

- graphics device and presentation (`src/graphics/d3d12_device.*`, `src/graphics/d3d12_swap_chain.*`)
- rendering pipeline and mesh setup (`src/graphics/d3d12_pipeline.*`, `src/graphics/d3d12_mesh.*`)
- encoding submission and completion (`src/encoder/frame_encoder.*`, `src/encoder/nvenc_session.*`, `src/encoder/nvenc_config.*`)
- encoded output writing (`src/encoder/bitstream_file_writer.*`)

A waitable component is a Win32 event handle plus callback action that executes when signaled. `App` uses this model to process frame latency, writer completion, and encoder completion signals. The refactor must preserve these semantics and keep shutdown drain behavior reliable.

Repository rules that matter for this work include: PascalCase methods, snake_case variables, RAII resource management, no namespaces, no smart pointers from the standard library, and preserving intentional WRL `ComPtr` address-taking patterns where they are required.

## Plan of Work

First, isolate frame-resource ownership concerns currently embedded as `App::FrameResources` into a graphics-side helper type under `src/graphics/` with matching header and source files. The helper must encapsulate resource/event lifetime and expose only the operations `App` needs for orchestration.

Second, extract command recording responsibilities from `App::RecordCommandList(...)` into a focused helper entry point in graphics code. `App` should supply orchestration inputs, while the helper owns low-level barrier/state/setup commands. The extraction must preserve existing startup-only pre-recording behavior.

Third, reduce `App` to coordinator behavior by simplifying `Run()` and constructor flow so they read as stage orchestration. Keep signal dispatch, present/encode ordering, and drain semantics equivalent. Keep App-owned defaults as-is.

Fourth, run validation and record concise evidence directly in this plan so a novice can confirm behavior and know what success looks like.

## Milestones

### Milestone 1: Extract frame resource ownership

At the end of this milestone, low-level frame resource/event lifecycle code is no longer embedded as a heavy nested ownership block in `App`.

### Milestone 2: Extract command recording mechanics

At the end of this milestone, low-level graphics command list recording details are handled by a dedicated graphics helper while preserving startup pre-recorded command list behavior.

### Milestone 3: Finalize `App` as coordinator

At the end of this milestone, `App` orchestration flow is concise and behavior-preserving, with clear wait/dispatch/submit/drain stages.

### Milestone 4: Validate and capture evidence

At the end of this milestone, build and runtime checks confirm no regressions and this document is updated with final outcomes.

## Concrete Steps

Working directory for all commands:

    c:\Users\Admin\goblin-stream

Planned validation commands to run only during implementation phase:

    cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
    cmake --build build --config Debug
    cmake --build build --config Release
    .\bin\Debug\goblin-stream.exe --headless

Expected outcomes to verify during implementation:

- Builds complete successfully.
- Headless run exits normally after bounded frame submission.
- `output.h264` exists and is non-empty.

## Validation and Acceptance

Acceptance is behavior-based. After implementation, a reviewer should be able to read `src/app.ixx` and see mostly orchestration rather than low-level graphics mechanics. Runtime behavior must remain equivalent for frame submission, signal-driven completion handling, and shutdown drain. Build and headless execution must succeed and produce output.

## Idempotence and Recovery

This plan is intended for incremental, safe refactoring. Steps can be applied in small commits and validated after each extraction. If an extraction introduces compile or runtime regression, revert only that extraction step and keep earlier proven structural improvements. Validation commands are safe to rerun.

## Artifacts and Notes

Implementation-phase artifacts to capture here when executed:

- Build success excerpts.
- Headless run exit confirmation.
- Small, focused diff summaries describing what moved out of `App` and why.

At present, this plan is intentionally draft-only and unexecuted.

## Interfaces and Dependencies

No external dependencies may be added. Keep the existing stack and repository conventions.

The resulting shape should keep `App` as the primary lifecycle coordinator while extracting low-level helpers into graphics-side files. Expected preserved interfaces include wait coordination, frame submission orchestration, encode submission, and drain processing contracts already used by `App`.

Revision note (2026-02-19): Created this new draft ExecPlan at user request. It documents intended refactor scope and verification strategy without executing any implementation changes yet.
