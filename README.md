# Goblin Stream

Windows C++ application for GPU-accelerated frame rendering and video encoding. Uses Direct3D 12 for rendering and NVIDIA NVENC for hardware H.264/HEVC encoding to produce streamable video output.

## Project Structure

```
goblin-stream/
├── .github/           # Copilot instructions and project standards
├── .vscode/           # VS Code configuration
├── bin/               # Build outputs (Debug/Release)
├── include/nvenc/     # Vendored NVENC 13 SDK headers
├── src/
│   ├── graphics/      # D3D12 rendering (device, resources, commands)
│   ├── encoder/       # NVENC encoding (session, config, D3D12 interop)
│   ├── pipeline/      # Frame coordination (render-to-encode sync)
│   └── main.cpp       # Application entry point
└── CMakeLists.txt     # CMake build configuration
```

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Main Application                        │
│                           (main.cpp)                            │
└─────────────────────────────┬───────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Frame Coordinator                          │
│              (synchronizes render → encode flow)                │
└───────────────┬─────────────────────────────┬───────────────────┘
                │                             │
                ▼                             ▼
┌───────────────────────────┐   ┌───────────────────────────────┐
│     D3D12 Graphics        │   │        NVENC Encoder          │
│  ┌─────────────────────┐  │   │  ┌─────────────────────────┐  │
│  │ d3d12_device        │  │   │  │ nvenc_session           │  │
│  │ (device, swapchain) │  │   │  │ (DLL loading, session)  │  │
│  ├─────────────────────┤  │   │  ├─────────────────────────┤  │
│  │ d3d12_resources     │  │   │  │ nvenc_d3d12             │  │
│  │ (textures, heaps)   │──┼───┼─►│ (texture registration)  │  │
│  ├─────────────────────┤  │   │  ├─────────────────────────┤  │
│  │ d3d12_commands      │  │   │  │ nvenc_config            │  │
│  │ (command lists)     │  │   │  │ (H.264/HEVC settings)   │  │
│  └─────────────────────┘  │   │  └─────────────────────────┘  │
└───────────────────────────┘   └───────────────────────────────┘
                │                             │
                ▼                             ▼
┌───────────────────────────┐   ┌───────────────────────────────┐
│      GPU Render Work      │   │    GPU Encode Work            │
│   (D3D12 command queue)   │   │  (nvEncodeAPI64.dll)          │
└───────────────────────────┘   └───────────────────────────────┘
```

### Data Flow

1. **D3D12 Device** creates render targets as shared GPU resources
2. **D3D12 Commands** records draw calls and executes on GPU
3. **Frame Coordinator** waits for D3D12 fence signal (render complete)
4. **NVENC D3D12** submits the rendered texture to hardware encoder
5. **NVENC Session** retrieves H.264/HEVC bitstream for streaming

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
cmake -S . -B build

# 2. Build Debug
cmake --build build --config Debug

# 3. Build Release
cmake --build build --config Release

# 4. Run
.\bin\Debug\goblin-stream.exe
```

## Code Standards

All code must follow the standards defined in `.github/copilot-instructions.md`:

- **C++23** standard with self-documenting code
- **Google C++ style** enforced by `.clang-format`
- **Windows API** preferred over STL when straightforward
- **Static linking** (MultiThreaded runtime `/MT`)
- **Vendored headers only** (NVENC headers included; no external library linking)
- **No multi-threading** (single-threaded with GPU synchronization)

## Development

1. Create a feature branch: `git checkout -b feature/your-feature`
2. Make changes and keep code self-documenting
3. Format code: Automatic on save in VS Code (or manually with clang-format)
4. Build and test: Use CMake Tools in VS Code
5. Commit and push your branch

## Resources

- [Windows API Documentation](https://docs.microsoft.com/en-us/windows/win32/api/)
- [Direct3D 12 Programming Guide](https://docs.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics)
- [NVIDIA Video Codec SDK](https://developer.nvidia.com/video-codec-sdk)
- [NVENC Programming Guide](https://docs.nvidia.com/video-technologies/video-codec-sdk/nvenc-video-encoder-api-prog-guide/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [MSVC Documentation](https://docs.microsoft.com/en-us/cpp/)
