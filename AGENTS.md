# Codex Startup Guide for `goblin-stream`

## Purpose
Use this file as the first-stop orientation for future coding sessions. It links to the right docs and points to source files that matter for common tasks.

## First 5 Minutes
1. Read `README.md` for build/run and module layout.
2. Scan `.github/QUICK_REFERENCE.md` for non-negotiable code rules.
3. If changing encoder flow, read:
   - `docs/nvenc-d3d12-workflow.md`
   - `docs/nvenc-crash-fix-summary.md`
4. Open `src/app.ixx` to understand frame loop, render submission, and encode handoff.
5. Run a local build:
   - `cmake -G "Visual Studio 18 2026" -A x64 -S . -B build`
   - `cmake --build build --config Debug`

## High-Value Source Map
- Entry + runtime:
  - `src/main.cpp`
  - `src/app.ixx`
- Encoding:
  - `src/encoder/frame_encoder.cpp`
  - `src/encoder/nvenc_config.cpp`
  - `src/encoder/nvenc_session.cpp`
  - `src/encoder/bitstream_file_writer.cpp`
- Graphics:
  - `src/graphics/d3d12_device.cpp`
  - `src/graphics/d3d12_swap_chain.cpp`
  - `src/graphics/d3d12_pipeline.cpp`
  - `src/graphics/d3d12_mesh.cpp`
  - `src/graphics/d3d12_resources.cpp`
  - `src/graphics/d3d12_commands.cpp`
  - `src/graphics/d3d12_command_allocators.cpp`

## Common Change Entry Points
- Change encoder settings/default codec/preset:
  - `src/app.ixx` (`EncoderConfig`)
  - `src/encoder/nvenc_config.*`
- Change encode submission/completion behavior:
  - `src/encoder/frame_encoder.cpp`
  - `src/encoder/bitstream_file_writer.cpp`
- Change render loop timing/fence behavior:
  - `src/app.ixx` (`WaitForFrame`, `PresentAndSignal`, run loop)
- Change draw pipeline/shaders:
  - `src/graphics/d3d12_pipeline.*`
  - `src/shaders/*.hlsl`

## Known Pitfalls
- D3D12 NVENC output must use `NV_ENC_OUTPUT_RESOURCE_D3D12` structure pointers for lock/unlock, not raw registered resource pointers.
- `BitstreamFileWriter` is overlapped I/O with a ring of slots; reason carefully about `head`, `pending_count`, event signaling, and `GetOverlappedResult`.
- Headless mode exits after 30 submitted frames; useful for quick checks (`--headless`).

## Fast Validation
- Build debug:
  - `cmake --build build --config Debug`
  - Or with wrapper: `powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Debug" -TimeoutSec 300`
- Run headless from VS Code launch config:
  - `Debug (Headless)`
- Check docs index after adding docs:
  - `python scripts/check-docs-index.py`

## Agent Command Execution
Use `scripts/agent-wrap.ps1` for long-running or critical commands:
- Provides timeout protection, structured JSON output, and per-run logs
- Use for: builds, tests, code analysis, validation runs
- Don't use for: file operations, git commands, quick checks
- See `.github/copilot-instructions.md` for detailed usage

## Working Agreements
- Follow `.github/copilot-instructions.md` for style and policy.
- Prefer minimal, targeted edits over broad refactors.
- Update docs when behavior changes, especially NVENC flow and frame lifecycle.
- If agent write tools are available, local VS Code environment settings belong in `C:\Users\Admin\AppData\Roaming\Code\User\settings.json` (not `.vscode/settings.json`).


# ExecPlans

When writing complex features or significant refactors, use an ExecPlan (as described in .agent/PLANS.md) from design to implementation.

ExecPlan documentation hygiene policy (file placement, naming, lifecycle, and archiving) is canonical in `.agent/PLANS.md` and must be followed for all new plan docs.
