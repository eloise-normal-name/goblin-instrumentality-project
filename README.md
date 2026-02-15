# Goblin Instrumentality Project

## Overview

The Goblin Instrumentality Project is a Windows x64 C++23 application that uses Direct3D 12 for graphics and NVIDIA NVENC for GPU video encoding. It targets the Windows subsystem (no console window) and is built with CMake + MSVC.

## Repository Structure

- `src/` - Application sources
  - `app.ixx`, `main.cpp` - App entry points and orchestration
  - `graphics/` - D3D12 device, swap chain, command allocators, command lists, and resource management
  - `encoder/` - NVENC configuration, D3D12 interop, and session management
- `include/` - Shared headers (including `try.h` for error handling)
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

- Project instructions: `.github/copilot-instructions.md`
- CI workflows & troubleshooting: `docs/GITHUB_WORKFLOWS.md`
- Known errors & fixes: `docs/copilot-known-errors.md`
- Custom agent guide: `.github/prompts/README.md`
- Refactor highlights: `refactor-highlights.html`
