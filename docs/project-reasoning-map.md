# Project Reasoning Map

This file is a quick mental model for debugging and feature work in `goblin-stream`.

## Runtime Pipeline
1. `src/main.cpp` creates the window and starts `App::Run`.
2. `src/app.ixx` owns core systems (D3D12 + NVENC) and runs the frame loop.
3. Per frame:
   - wait for frame latency or completed write event
   - process completed encodes
   - execute pre-recorded command list for the current frame slot
   - present and signal fence
   - submit frame to NVENC
4. Bitstream is drained asynchronously and written by `BitstreamFileWriter`.

## Ownership and Lifetimes
- `App` is the top-level owner.
- `FrameResources` in `src/app.ixx` owns:
  - per-slot command list
  - fence + event
  - offscreen render target
- `FrameEncoder` manages NVENC resource registration and encode queue state.
- `BitstreamFileWriter` owns overlapped file handle + slot buffers/events.

## Where to Investigate by Symptom
- Crash in encode/bitstream lock/unlock:
  - `src/encoder/frame_encoder.cpp`
  - `docs/nvenc-d3d12-workflow.md`
  - `docs/nvenc-crash-fix-summary.md`
- Stalls/hitches during encoding:
  - `src/encoder/frame_encoder.cpp` completion/drain path
  - `src/encoder/bitstream_file_writer.cpp` pending slot handling
- Present returns `DXGI_ERROR_WAS_STILL_DRAWING` frequently:
  - `src/app.ixx` (`HandlePresentResult`, fence readiness logic)
- Output file empty or truncated:
  - `src/encoder/bitstream_file_writer.cpp` (`WriteFrame`, `DrainCompleted`, destructor flush)
- Render output wrong but app runs:
  - `src/graphics/d3d12_pipeline.*`
  - `src/graphics/d3d12_mesh.*`
  - `src/shaders/mesh_vs.hlsl`, `src/shaders/mesh_ps.hlsl`

## Key Invariants
- One command list/fence/render target per frame slot (`BUFFER_COUNT`).
- Encode input uses per-slot texture registrations (`RegisterTexture` in app startup).
- NVENC D3D12 output handoff uses `NV_ENC_OUTPUT_RESOURCE_D3D12`.
- Frame loop processes completed frames both before and after submission to reduce backpressure.

## Minimal Debug Loop
1. Build debug: `cmake --build build --config Debug`
2. Run `--headless` to reproduce quickly (30 frames).
3. Inspect:
   - `debug_output.txt` for frame/encoder timing
   - `output.h264` for output existence/size changes
4. If requested, validate docs index:
   - `python scripts/check-docs-index.py`
