# Headless Mode

## Overview

Headless mode enables the Goblin Instrumentality Project to run in CI/CD environments without requiring a physical GPU or display. This is accomplished using DirectX 12's WARP (Windows Advanced Rasterization Platform) software renderer.

## Usage

Run the application with the `--headless` flag:

```cmd
bin\Debug\goblin-stream.exe --headless
```

## Implementation Details

### Command-Line Parsing
- `main.cpp`: Added `CheckHeadlessFlag()` function to parse command-line arguments
- Supports `--headless` flag
- Flag is passed through the application initialization chain

### WARP Adapter Selection
- `D3D12Device`: Modified constructor to accept `use_warp` parameter
- When `use_warp=true`, uses `IDXGIFactory7::EnumWarpAdapter()` instead of hardware adapter
- WARP provides a fully conformant DirectX 12 implementation in software

### Window Handling
- In headless mode, window is created but hidden using `SW_HIDE` flag
- Window still exists (required for swap chain creation) but not displayed
- No message pump processing in headless mode

### Execution Flow
- `App::RunHeadless()`: Simplified render loop for testing
- Executes 10 test frames to exercise the rendering pipeline
- Uses synchronous fence-based synchronization
- No presentation or message processing
- Exits cleanly after all frames complete

### GitHub Actions Integration
- Coverage workflow updated to run with `--headless` flag
- OpenCppCoverage now executes the application and generates coverage reports
- Coverage data uploaded to Codecov and archived as artifacts

## Testing

The headless mode can be tested locally on Windows:

```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
bin\Debug\goblin-stream.exe --headless
```

Expected behavior:
- Application runs without displaying a window
- Executes 10 rendering frames using WARP
- Returns exit code 0 on success
- Returns exit code 1 on failure

## Code Coverage

With headless mode enabled, the CI workflow now:

1. Builds the application in Debug mode
2. Runs with OpenCppCoverage: `OpenCppCoverage --sources "src\*" --export_type cobertura:coverage.xml -- bin\Debug\goblin-stream.exe --headless`
3. Generates `coverage.xml` in Cobertura format
4. Uploads coverage to Codecov
5. Archives coverage report as build artifact

## Limitations

- WARP is significantly slower than hardware rendering
- Only basic rendering pipeline is exercised (10 frames)
- NVENC encoding is initialized but not fully tested in headless mode
- No interactive testing (no user input, display output, or message pump)

## Future Enhancements

Possible improvements for headless testing:

1. Add frame count parameter: `--headless --frames=100`
2. Validate rendered output (checksum/hash comparison)
3. Add automated correctness tests
4. Extend headless mode to support more test scenarios
5. Add performance benchmarking in WARP mode
