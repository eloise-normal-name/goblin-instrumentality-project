# Coverage Triage and Cleanup Plan (2026-02-20)

This document explains the low-coverage files from the latest headless coverage run and defines a staged cleanup + scenario expansion path.

Primary artifacts reviewed:

- `artifacts/coverage/headless_20260220_000750.summary.txt`
- `artifacts/coverage/headless_20260220_000750.cobertura.xml`
- `artifacts/coverage/headless_20260220_000750.report.html`

## Implementation Status

- ✅ Stage 1 cleanup implemented:
  - Removed unreferenced graphics wrappers/utilities:
    - `src/graphics/command_allocators.h/.cpp`
    - `src/graphics/resources.h/.cpp`
    - Unused `D3D12Commands` wrapper surface from `src/graphics/commands.h/.cpp`
  - Kept active API: `RecordFrameCommandList(...)` in `src/graphics/commands.*`
  - Updated `CMakeLists.txt` source list accordingly.
- ✅ Build validation passed after cleanup.
- ✅ Post-cleanup coverage run completed:
  - `artifacts/coverage/headless_20260220_002439.summary.txt`
  - Overall line coverage now **81.22% (679/836)**.
  - Removed 0%-coverage files no longer appear; `src/graphics/commands.cpp` now reports 100% in this run.
- ✅ Stage 2 tooling update implemented:
  - `scripts/run-headless-coverage.ps1` now supports `-Scenario headless|shader-cache-miss|custom`.
  - `shader-cache-miss` scenario clears cached `.cso` shader files before collection.
  - Artifact names are now scenario-prefixed (`<scenario>_<timestamp>.*`).
  - Build phase now fails fast on non-zero wrapper exit codes (prevents silent stale-binary runs).
- ⚠️ Current workspace compile blockers (outside this coverage script change) prevent full build-backed validation in this session:
  - `src/graphics/frame_resources.cpp`: `RenderTextureArray` missing `resize/clear` members.
  - `src/app.ixx`: reference-initialization error (`Canvas::frame_resources`).
  - Scenario logic validated successfully with `-SkipBuild` while these unrelated compile issues remain.

## Baseline

- Overall line coverage: **75.28% (874/1161)**
- Lowest-coverage files in summary:
  - `src/graphics/command_allocators.cpp` (0%)
  - `src/graphics/commands.cpp` (0%)
  - `src/graphics/commands.h` (0%)
  - `src/graphics/resources.cpp` (0%)
  - `src/encoder/nvenc_session.cpp` (52.14%)
  - `src/graphics/pipeline.cpp` (56.52%)
  - `src/encoder/frame_encoder.cpp` (58.33%)

## Why These Files Are Low

### 0% files are currently unreferenced wrappers/utilities

- `src/graphics/command_allocators.cpp`
  - `D3D12CommandAllocators` has no call sites outside its own header/cpp.
  - Current app path creates a single allocator directly in `App`.

- `src/graphics/commands.cpp` and `src/graphics/commands.h`
  - `D3D12Commands` wrapper methods have no external usages.
  - Runtime uses pre-recorded command lists via `RecordFrameCommandList(...)` and direct queue execution.

- `src/graphics/resources.cpp`
  - `ReadbackBuffer` has no call sites outside its own definition.
  - `CreateTransitionBarrier(...)` is only used inside the currently-unreferenced `D3D12Commands::TransitionResource(...)`.

Conclusion: these files are not "hard to test"; they are strong cleanup candidates because they appear detached from active runtime paths.

### Scenario-gated files (not dead, but partially unexercised)

- `src/encoder/nvenc_session.cpp` (~52%)
  - `ConfigureHEVC(...)` has 0 hits in Cobertura.
  - `QueryCapabilities(...)` and `IsCodecSupported(...)` have 0 hits and no active call sites in current startup path.
  - Current default config in `App` uses H264.

- `src/graphics/pipeline.cpp` (~56%)
  - `CompileShader(...)` and `WriteFileBytes(...)` are 0-hit.
  - `LoadOrCompileShader(...)` likely takes "load cached `.cso`" path during normal runs.

- `src/encoder/frame_encoder.cpp` (~58%)
  - `RegisterBitstreamBuffer(...)` and `UnregisterBitstreamBuffer(...)` are 0-hit.
  - Main encode path is exercised, but optional/legacy buffer APIs are not.

- `src/main.cpp` (~78%, not top bucket but relevant)
  - `WindowProc` branches for `WM_CLOSE` and `WM_KEYDOWN` are low/zero hit.
  - Headless mode avoids normal interactive message behavior.

## Headless Bias and Report Interpretation

- Coverage script runs `--headless` and exits after ~30 frames.
- This is deterministic and useful for CI, but it systematically misses:
  - interactive window/message paths,
  - non-default codec/mode branches,
  - error and fallback branches that require induced failures.

Also note: the summary reports lowest file percentages from parsed Cobertura class entries, so percentages can be skewed by file/class grouping and helper symbols.

## Cleanup and Coverage Execution Plan

### Stage 1: Prove and remove dead wrappers (low risk, high clarity)

Targets:

- `src/graphics/command_allocators.h`
- `src/graphics/command_allocators.cpp`
- `src/graphics/commands.h`
- `src/graphics/commands.cpp`
- `src/graphics/resources.h`
- `src/graphics/resources.cpp` (or split/remove only truly dead pieces)

Acceptance criteria:

- No external references remain to removed symbols.
- Build passes unchanged behavior in headless mode.
- Coverage report no longer includes orphaned 0% files.

### Stage 2: Add scenario matrix (do not force unit-test architecture first)

Run matrix:

1. Headless default (current baseline)
2. Non-headless smoke (exercise message loop, `WindowProc` branches)
3. Codec variant (HEVC path when device support allows)
4. Shader cache-miss run (force compile/write path in `pipeline.cpp`)

Acceptance criteria:

- `main.cpp` interactive branches increase from current baseline.
- `nvenc_session.cpp` HEVC/config code receives coverage where supported.
- `pipeline.cpp` compile and write branches are observed in at least one run.

### Stage 3: Decide keep-vs-prune for optional APIs

Candidates:

- `FrameEncoder::RegisterBitstreamBuffer(...)`
- `FrameEncoder::UnregisterBitstreamBuffer(...)`
- `NvencSession::QueryCapabilities(...)`
- `NvencSession::IsCodecSupported(...)`

Rules:

- Keep if there is a near-term feature owner and planned call path.
- Otherwise remove or isolate behind clearly-owned integration points.

## Immediate Next PR Breakdown

1. **PR A (cleanup):** remove unreferenced graphics wrappers/utilities.
2. **PR B (coverage scenarios):** extend coverage script/options for non-headless and variant runs.
3. **PR C (API audit):** remove or wire optional NVENC/frame-encoder methods.

This sequencing improves code clarity first, then improves coverage signal quality, then resolves remaining optional-path ambiguity.
