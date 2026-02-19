---
status: active
owner: codex
related: user-request-2026-02-19-frame-encoder-app-run-perf
---

# Optimize FrameEncoder Encode Path and App Run-Loop Completion Drains

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

`/.agent/PLANS.md` is checked into this repository and this document is maintained in accordance with that file.

## Purpose / Big Picture

This change reduces avoidable CPU-side work in frame submission and encode completion handling. After this work, `FrameEncoder::EncodeFrame` reuses per-texture NVENC setup instead of rebuilding full submission structures every frame, input textures stay mapped for their registered lifetime, pending completion bookkeeping avoids extra per-frame payload copies, and `App::Run` avoids duplicate completion-drain calls inside the steady-state loop. The behavior remains the same from a user perspective: encoded frames are still written, headless mode still exits after 30 submitted frames, and final draining still completes before shutdown.

## Progress

- [x] (2026-02-19 10:34Z) Reviewed startup docs and code paths: `README.md`, `.github/QUICK_REFERENCE.md`, `src/encoder/frame_encoder.cpp`, `src/encoder/frame_encoder.h`, and `src/app.ixx`.
- [x] (2026-02-19 10:36Z) Reviewed NVENC D3D12 workflow notes in `docs/nvenc-d3d12-workflow.md` and crash constraints in `docs/nvenc-crash-fix-summary.md`.
- [ ] Implement per-texture cached encode submission state in `src/encoder/frame_encoder.h` and `src/encoder/frame_encoder.cpp`.
- [ ] Switch input mapping lifecycle from per-frame map/unmap to map-on-register and unmap-on-unregister in `src/encoder/frame_encoder.cpp`.
- [ ] Compact pending output tracking to reserved minimal metadata in `src/encoder/frame_encoder.h` and `src/encoder/frame_encoder.cpp`.
- [ ] Remove redundant per-frame completion drain call in `src/app.ixx` while keeping equivalent end-to-end behavior.
- [ ] Build and validate with `cmake --build build --config Debug` via `scripts/agent-wrap.ps1`.
- [ ] Record observed outcomes, decisions, and final evidence.

## Surprises & Discoveries

- Observation: Existing plan files in this repository currently live under `docs/` root, while `.agent/PLANS.md` now specifies `docs/execplans/` for active plans.
  Evidence: `Get-ChildItem docs` showed `*-execplan.md` files directly in `docs/` and no `docs/execplans/` directory before this run.

## Decision Log

- Decision: Create this active ExecPlan in `docs/execplans/` using the canonical naming and lifecycle policy from `.agent/PLANS.md` instead of adding another root-level plan file.
  Rationale: Follow current repository policy and avoid adding to historical plan location drift.
  Date/Author: 2026-02-19 / codex

- Decision: Keep the final shutdown drains (`ProcessCompletedFrames(..., true)` and fence wait) unchanged and only remove redundant steady-state per-frame drain work.
  Rationale: The request targets duplicated work inside `App::Run` frame iteration; end-of-run drains are correctness critical for complete bitstream flush.
  Date/Author: 2026-02-19 / codex

- Decision: All steady-state waits must go through `MsgWaitForMultipleObjects` in `App::WaitForFrame`; `FrameEncoder` and `BitstreamFileWriter` both expose `HasPending*()`/`Next*Event()` so `WaitForFrame` can include their events in a single unified wait. Polling drain calls inside the frame loop are removed in favour of event-driven wakeups (`EncoderDone`/`WriteDone` return values).
  Rationale: A single wait point ensures Windows messages, present readiness, write completions, and encode completions are all handled without spin-polling. Polling drains outside the wait introduced latency jitter and wasted CPU checking queues that hadn't signalled. Init and shutdown paths (`wait_for_all=true`) are exempt because they intentionally block until all work is flushed.
  Date/Author: 2026-02-19 / Claudia

## Outcomes & Retrospective

Pending implementation. This section will be updated with measured/observed outcomes after code edits and validation run.

## Context and Orientation

`src/app.ixx` owns the frame loop, render submission, and encode handoff. Each successful present path currently calls `FrameEncoder::EncodeFrame(...)` and then immediately calls `FrameEncoder::ProcessCompletedFrames(...)`, while another completion drain already happens at the top of each loop iteration.

`src/encoder/frame_encoder.cpp` owns texture registration, map/unmap flow, encode submission, and completion processing. It currently maps and unmaps input resources every frame, rebuilds full input/output NVENC structures per frame in `EncodeFrame`, and enqueues a `PendingOutput` payload that stores a full `NV_ENC_OUTPUT_RESOURCE_D3D12` copy per submission in a `std::deque`.

For this repository, a "mapped input" means NVENC has returned an `NV_ENC_INPUT_PTR` for a registered D3D12 input resource via `nvEncMapInputResource`. A "pending output" means an encode submission has been queued and awaits its output fence value before bitstream lock/unlock and file write.

## Plan of Work

First, update `src/encoder/frame_encoder.h` to define per-texture cached encode structs and compact pending metadata, and adjust private members from `std::deque` to a pre-reserved `std::vector` plus head index tracking.

Next, update `src/encoder/frame_encoder.cpp` constructor and registration paths to reserve cache/pending capacity and map each texture once during registration. Build cached per-texture NVENC input/output/fence/pic parameter templates that are patched only for dynamic frame values before `nvEncEncodePicture`.

Then, update completion processing to consume compact pending entries, reconstruct only the required D3D12 output-resource wrapper for lock/unlock, and compact the pending vector when consumed entries accumulate.

After encoder changes, edit `src/app.ixx` to remove the second steady-state completion drain call inside the per-frame success path, preserving top-of-loop drains and shutdown drains.

Finally, run a Debug build through `scripts/agent-wrap.ps1`, then update this ExecPlan sections (`Progress`, `Surprises & Discoveries`, `Decision Log`, `Outcomes & Retrospective`, and `Concrete Steps` evidence) to reflect actual results.

## Concrete Steps

Working directory: `C:\Users\Admin\goblin-stream`

1. Edit `src/encoder/frame_encoder.h` and `src/encoder/frame_encoder.cpp` to add cached per-texture encode state, persistent mapping, and compact pending output metadata.
2. Edit `src/app.ixx` to remove duplicate per-frame completion draining.
3. Run:
   powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Debug" -TimeoutSec 300
4. Confirm build success and record output evidence below.

Expected validation transcript snippet after Step 3:

   status: "ok"
   exitCode: 0

## Validation and Acceptance

Acceptance is met when all points below hold:

- `FrameEncoder::EncodeFrame` no longer maps and unmaps input textures every call.
- Per-texture NVENC input/output setup is cached and only per-frame dynamic values (input wait fence value, output signal fence value, and timestamp) are patched at submission time.
- Pending completion data uses reserved storage and minimal metadata without storing full per-frame `NV_ENC_OUTPUT_RESOURCE_D3D12` payload copies.
- `App::Run` does not call `ProcessCompletedFrames` twice in the same steady-state frame path.
- Debug build succeeds.

## Idempotence and Recovery

All edits are source-level and idempotent. Re-running the same patch sequence should produce no further changes once applied. If a build fails after edits, rerun the same wrapper build command after fixing compile errors; no migration or destructive data operation is involved.

## Artifacts and Notes

Pending implementation and build output capture.

## Interfaces and Dependencies

The plan keeps existing external interfaces unchanged:

- `FrameEncoder::EncodeFrame(uint32_t texture_index, uint64_t fence_wait_value, uint32_t frame_index)` remains callable from `App::Run` with the same signature.
- `FrameEncoder::ProcessCompletedFrames(BitstreamFileWriter& writer, bool wait_for_all)` remains the completion drain API.
- NVENC D3D12 requirements from `include/nvenc/nvEncodeAPI.h` remain enforced: encode, lock, and unlock still operate through `NV_ENC_INPUT_RESOURCE_D3D12` and `NV_ENC_OUTPUT_RESOURCE_D3D12` wrapper structures.

Revision Note (2026-02-19): Initial plan created to execute requested performance-focused changes with a self-contained implementation and validation path.
