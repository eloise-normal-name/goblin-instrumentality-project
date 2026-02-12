# Goblin Stream

Windows C++ application for GPU-accelerated frame rendering and video encoding. Uses Direct3D 12 for rendering and NVIDIA NVENC for hardware H.264/HEVC encoding to produce streamable video output.

## Project Structure

```
goblin-stream/
├── .github/           # Copilot instructions and project standards
├── .vscode/           # VS Code configuration
├── bin/               # Build outputs (Debug/Release)
├── include/           # Third-party headers only (do not format)
│   └── nvenc/         # Vendored NVENC 13 SDK headers
├── src/
│   ├── graphics/      # D3D12 rendering (device, swap chain, targets, commands)
│   ├── encoder/       # NVENC encoding (session, config, D3D12 interop)
│   ├── try.h          # Error handling helper (Try | pattern)
│   └── main.cpp       # Application entry point
└── CMakeLists.txt     # CMake build configuration
```

## Architecture

```
┌─────────────────────────────────┐
│         Main Application        │
│           (main.cpp)            │
└───────────────┬─────────────────┘
                ▼
┌─────────────────────────────────┐
│          D3D12 Graphics         │
│  ┌────────────────────────────┐ │
│  │ d3d12_device                │ │
│  │ d3d12_swap_chain            │ │
│  │ d3d12_resources             │ │
│  │ d3d12_commands              │ │
│  └────────────────────────────┘ │
└───────────────┬─────────────────┘
                ▼
┌────────────────────────┐
│    GPU Render Work     │
│   (command queue)      │
└────────────────────────┘
```

### Data Flow

1. **D3D12 Swap Chain** provisions back buffers for present
2. **App Offscreen Targets** provisions per-buffer render textures used as render + encode inputs
3. **D3D12 Commands** are pre-recorded once at startup (design requirement) and reused each frame
4. **GPU Render Work** clears offscreen render targets, copies them to swap chain buffers, and presents

### Command Recording Model

The application uses pre-recorded command lists by design. Command lists are generated once during
startup and executed repeatedly each frame without per-frame reset/re-record.

## Build

### Using VS Code

1. Install **C/C++** and **CMake Tools** extensions.
2. Open the folder in VS Code.
3. When prompted, select a Kit (e.g., `Visual Studio Community 2026 Release - amd64`).
4. **Build**: Click "Build" in the status bar or press `F7`.
5. **Debug**: Press `Ctrl+F5` (Run) or `F5` (Debug).

### Using Command Line (CMake)

> **Note:** Ensure you are using the **Visual Studio Community 2026 Preview - amd64** environment.

```powershell
# 1. Generate Build System
cmake -B build -G "Visual Studio 18 2026" -A x64

# 2. Build Debug
cmake --build build --config Debug

# 3. Build Release
cmake --build build --config Release

# 4. Run
.\bin\Debug\goblin-stream.exe
```

## Development

See `.github/copilot-instructions.md` for detailed code standards, conventions, and development practices.

Key points:
- **C++23** standard with self-documenting code
- Windows API preferred; static linking
- No multi-threading; GPU synchronization only
- Format with clang-format before committing
- Do not format the include/ directory (third-party headers only)

## References

- [Direct3D 12 Documentation](https://docs.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics)
- [NVIDIA NVENC Programming Guide](https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/nvenc-video-encoder-api-prog-guide/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
