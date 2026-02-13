# Goblin Instrumentality Project

A high-performance Windows DirectX 12 graphics application with NVENC hardware encoding support.

## Technology Stack

- **Language**: C++23
- **Build System**: CMake 3.28+ with MSVC (Visual Studio 2026 Community)
- **Target Platform**: Windows x64
- **Graphics API**: DirectX 12
- **Encoder**: NVIDIA NVENC

## Building

### Prerequisites

- Visual Studio 2026 Community (or later)
- CMake 3.28+
- Windows 10/11 SDK
- NVIDIA GPU with NVENC support (for encoding features)

### Build Instructions

```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
```

The executable will be located in `bin\Debug\goblin-stream.exe`

For Release builds:
```cmd
cmake --build build --config Release
```

## Code Coverage

This project supports code coverage testing using OpenCppCoverage.

### Quick Start

1. Install OpenCppCoverage from https://github.com/OpenCppCoverage/OpenCppCoverage/releases
2. Build the project in Debug mode
3. Run coverage:
   ```cmd
   .\run_coverage.bat
   ```
   Or with PowerShell:
   ```powershell
   .\run_coverage.ps1
   ```

For detailed coverage documentation, see [docs/CODE_COVERAGE.md](docs/CODE_COVERAGE.md)

### CI/CD Coverage

Code coverage is automatically run on pull requests and pushes via GitHub Actions. Coverage reports are uploaded as artifacts.

## Project Structure

```
goblin-instrumentality-project/
├── src/
│   ├── graphics/          # DirectX 12 rendering components
│   ├── encoder/           # NVENC encoding components
│   └── main.cpp           # Application entry point
├── include/
│   ├── nvenc/            # NVENC SDK headers
│   └── try.h             # Error handling utilities
├── docs/                  # Documentation
├── .github/
│   └── workflows/        # GitHub Actions workflows
└── CMakeLists.txt        # Build configuration
```

## Development

### Code Style

- Follow C++23 best practices
- Use RAII for resource management
- PascalCase for public APIs and methods
- snake_case for local and member variables
- CAPS_CASE for constants
- Format code with `.clang-format` (Ctrl+Shift+I in VS Code)

### Testing

Code coverage testing infrastructure is available. Tests should be added to validate core functionality.

For information on integrating tests with coverage, see [docs/TEST_COVERAGE_INTEGRATION.md](docs/TEST_COVERAGE_INTEGRATION.md)

## License

See LICENSE file for details.
