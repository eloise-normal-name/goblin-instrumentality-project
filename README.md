# Goblin Instrumentality Project

## Overview

The Goblin Instrumentality Project is a Windows x64 C++23 application that uses Direct3D 12 for graphics and NVIDIA NVENC for GPU video encoding. It targets the Windows subsystem (no console window) and is built with CMake + MSVC.

## Quick Links

- **[View Copilot-Assigned Issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)** - Issues triaged and assigned to copilot agents
- `perf-highlights.html` - Performance findings report (publishable to `gh-pages`)

## Repository Structure

- `src/` - Application sources
  - `app.ixx`, `main.cpp` - App entry points and orchestration
  - `try.h` - Error handling via `Try |` pattern
  - `debug_log.h` - Compile-gated `FRAME_LOG(...)` macro output to `stderr` (enabled only in `Debug`, `RelWithDebInfo`, and `RelWithDbgInfo`; redirect streams or run from a terminal because the app uses `WIN32` subsystem)
  - `graphics/` - D3D12 device, swap chain, command allocators, command lists, and resource management
  - `encoder/` - NVENC configuration, D3D12 interop, and session management
- `include/` - Vendor headers (`nvenc/nvEncodeAPI.h`)
- `scripts/` - CI helper scripts (docs index validation)
  - `agent-wrap.ps1` - Runs a PowerShell command with timeout and writes per-run logs plus JSON metadata
- `docs/` - Project documentation
- `.github/` - Copilot instructions and custom agent prompts

## High-Level Data Flow

1. The app initializes D3D12 core objects (device, swap chain, command allocators/commands, and resources).
2. Each frame records and executes D3D12 command lists and presents via the swap chain.
3. The encoder path configures and runs an NVENC session, using D3D12 interop for GPU-backed encoding.

## Build (Windows)

- Configure: `cmake -G "Visual Studio 18 2026" -A x64 -S . -B build`
- Configure (auto retry with cache reset): `powershell -ExecutionPolicy Bypass -File scripts/configure-cmake.ps1`
- Build (Debug): `cmake --build build --config Debug`
- Build (RelWithDbgInfo): `cmake --build build --config RelWithDbgInfo`

If configure fails after branch switches or toolchain updates, clear cache and retry:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/configure-cmake.ps1 -ForceClean
```

## Reusable Profiling Workflow

Run profiling and regenerate the publishable performance report:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/profile-exe.ps1 -BuildConfig RelWithDbgInfo -Focus all -RunLabel baseline
```

This produces:
- `docs/perf-baselines/baseline_<label>_<timestamp>.json`
- `.diagsession` files in `docs/perf-baselines/` (gitignored)
- `perf-highlights.html` (for `gh-pages` publication)

## Native C++ Coverage (Headless Workload)

Collect code coverage by running the app in deterministic headless mode (`--headless` exits after 30 frames):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run-headless-coverage.ps1 -BuildConfig Debug
```

Scenario-driven examples:

```powershell
# Force shader cache miss to exercise CompileShader/WriteFileBytes path
powershell -ExecutionPolicy Bypass -File scripts/run-headless-coverage.ps1 -BuildConfig Debug -Scenario shader-cache-miss

# Use custom app arguments
powershell -ExecutionPolicy Bypass -File scripts/run-headless-coverage.ps1 -BuildConfig Debug -Scenario custom -AppArgs "--headless"
```

This produces:
- `artifacts/coverage/<scenario>_<timestamp>.coverage`
- `artifacts/coverage/<scenario>_<timestamp>.cobertura.xml`
- `artifacts/coverage/<scenario>_<timestamp>.summary.txt`
- `artifacts/coverage/<scenario>_<timestamp>.report.html`

The coverage script uses `scripts/coverage.runsettings` and instruments native C++ modules that match `goblin-stream`.

## Agent Terminal Wrapper

For automation and long-running commands, use `scripts/agent-wrap.ps1` to get structured output with timeout protection:

**Basic usage:**
```powershell
powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "<command>" -TimeoutSec <seconds>
```

Run in explicit working directory:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "<command>" -WorkingDirectory "C:\Users\Admin\goblin-stream"
```

**Features:**
- Guaranteed termination after timeout (default: 120 seconds)
- Structured JSON output with status, timing, exit code, and log paths
- Separate logs for stdout, stderr, and combined output in `.agent/logs/<run-id>/`
- Output preview (first 4000 chars by default)

**When to use:**
- ✅ Build commands, test runs, code analysis tools, validation runs
- ❌ Quick file operations, git commands, simple checks

**Examples:**
```powershell
# Build with 5-minute timeout
powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Debug" -TimeoutSec 300

# Run tests with console output
powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "ctest --test-dir build -C Debug" -TimeoutSec 180 -PrintOutput
```

See [.github/copilot-instructions.md](.github/copilot-instructions.md) for full usage guidelines.

## Documentation Index

- Project instructions: `.github/copilot-instructions.md`
- Bug triage system: [.github/BUG_TRIAGE_SYSTEM.md](.github/BUG_TRIAGE_SYSTEM.md) (manual and agent-assisted routing)
- Known errors & fixes: [docs/copilot-known-errors.md](docs/copilot-known-errors.md)
- Coverage triage and cleanup plan: [docs/coverage-cleanup-plan.md](docs/coverage-cleanup-plan.md)
- Mesh rendering & PBR plan: [docs/mesh-rendering-pbr-plan.md](docs/mesh-rendering-pbr-plan.md)
- NVENC D3D12 workflow: [docs/nvenc-d3d12-workflow.md](docs/nvenc-d3d12-workflow.md)
- NVENC crash fix summary: [docs/nvenc-crash-fix-summary.md](docs/nvenc-crash-fix-summary.md)
- Project reasoning map: [docs/project-reasoning-map.md](docs/project-reasoning-map.md)
- Custom agent guide: `.github/prompts/README.md`
- Refactor highlights: `refactor-highlights.html` (published to `gh-pages` branch)

**Note**: GitHub Actions workflows are temporarily removed and will be refactored back in eventually.
