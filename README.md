# Goblin Instrumentality Project

## Overview

The Goblin Instrumentality Project is a Windows x64 C++23 application that uses Direct3D 12 for graphics and NVIDIA NVENC for GPU video encoding. It targets the Windows subsystem (no console window) and is built with CMake + MSVC.

## Quick Links

- **[View Copilot-Assigned Issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)** - Issues triaged and assigned to copilot agents
- **[Automated Crash Detection](/.github/workflows/create-crash-issue.yml)** - CI automatically creates issues for crashes with logs and agent assignment

## Repository Structure

- `src/` - Application sources
  - `app.ixx`, `main.cpp` - App entry points and orchestration
  - `try.h` - Error handling via `Try |` pattern
  - `frame_debug_log.*` - Per-frame timing and debug log output
  - `graphics/` - D3D12 device, swap chain, command allocators, command lists, and resource management
  - `encoder/` - NVENC configuration, D3D12 interop, and session management
- `include/` - Vendor headers (`nvenc/nvEncodeAPI.h`)
- `scripts/` - CI helper scripts (docs index validation)
- `docs/` - Project documentation and workflow notes
- `.github/` - Copilot instructions, workflow configuration, and custom agent prompts

## High-Level Data Flow

1. The app initializes D3D12 core objects (device, swap chain, command allocators/commands, and resources).
2. Each frame records and executes D3D12 command lists and presents via the swap chain.
3. The encoder path configures and runs an NVENC session, using D3D12 interop for GPU-backed encoding.

## Build (Windows)

- Configure: `cmake -G "Visual Studio 18 2026" -A x64 -S . -B build`
- Build (Debug): `cmake --build build --config Debug`

## Documentation Index

- Project instructions: [.github/copilot-instructions.md](.github/copilot-instructions.md)
- Bug triage system: [.github/BUG_TRIAGE_SYSTEM.md](.github/BUG_TRIAGE_SYSTEM.md) (automated crash detection & agent routing)
- CI workflows & troubleshooting: [docs/GITHUB_WORKFLOWS.md](docs/GITHUB_WORKFLOWS.md)
- Known errors & fixes: [docs/copilot-known-errors.md](docs/copilot-known-errors.md)
- NVENC crash fix summary: [docs/nvenc-crash-fix-summary.md](docs/nvenc-crash-fix-summary.md)
- Mesh rendering & PBR plan: [docs/mesh-rendering-pbr-plan.md](docs/mesh-rendering-pbr-plan.md)
- NVENC D3D12 workflow: [docs/nvenc-d3d12-workflow.md](docs/nvenc-d3d12-workflow.md)
- Custom agent guide: [.github/prompts/README.md](.github/prompts/README.md)
- Refactor highlights: [refactor-highlights.html](refactor-highlights.html) (published to `gh-pages` branch)
