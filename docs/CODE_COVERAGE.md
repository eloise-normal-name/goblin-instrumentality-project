# Code Coverage Testing

This document describes how to measure code coverage for the Goblin Instrumentality Project.

## Important Note

**The coverage infrastructure is configured and ready, but no tests exist yet.** This project is a DirectX 12 GUI application that requires a GPU and cannot be run headlessly in CI environments without tests.

Once unit tests or test harnesses are implemented, code coverage will work automatically. See [TEST_COVERAGE_INTEGRATION.md](TEST_COVERAGE_INTEGRATION.md) for guidance on adding tests.

## Overview

Code coverage measures which parts of the codebase are executed during testing. This helps identify untested code paths and improve test quality.

## Prerequisites

### OpenCppCoverage (Recommended for Windows)

OpenCppCoverage is a free, open-source code coverage tool for C++ on Windows.

**Installation:**
1. Download from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases
2. Install to default location or custom path
3. Add to PATH: `C:\Program Files\OpenCppCoverage` (or your installation directory)

**Verify installation:**
```cmd
OpenCppCoverage --help
```

## Running Coverage Locally

### With OpenCppCoverage

**When tests are available**, run coverage using:

```cmd
OpenCppCoverage --sources "src\*" --export_type html:coverage_report -- bin\Debug\goblin-stream-tests.exe
```

**Note:** Replace `goblin-stream-tests.exe` with your test executable name. The main `goblin-stream.exe` is a GUI application and cannot be used for automated coverage without tests.

**Options explained:**
- `--sources "src\*"` - Only measure coverage for source files in src/ directory
- `--export_type html:coverage_report` - Generate HTML report in coverage_report/ folder
- `-- bin\Debug\goblin-stream-tests.exe` - The test executable to run

### Additional Export Formats

**Cobertura XML (for CI/CD):**
```cmd
OpenCppCoverage --sources "src\*" --export_type cobertura:coverage.xml -- bin\Debug\goblin-stream.exe
```

**Binary format (fastest):**
```cmd
OpenCppCoverage --sources "src\*" --export_type binary:coverage.bin -- bin\Debug\goblin-stream.exe
```

## Coverage Build Configuration

For accurate coverage results, build with Debug configuration:

```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
```

## Coverage Reports

### HTML Report

The HTML report (`coverage_report/index.html`) provides:
- Line-by-line coverage visualization
- Function coverage statistics
- Branch coverage information
- Summary by module

### Reading Coverage Metrics

- **Line Coverage**: Percentage of code lines executed
- **Branch Coverage**: Percentage of conditional branches taken
- **Function Coverage**: Percentage of functions called

## Integration with CI/CD

A GitHub Actions workflow (`.github/workflows/coverage.yml`) is configured and ready to run coverage automatically.

**Current Status:** The workflow is configured but skips coverage execution until tests are added. Once tests are implemented, the workflow will automatically run coverage on every push and pull request.

To enable automated coverage:
1. Add test executable to the project
2. Update the "Run Coverage" step in `.github/workflows/coverage.yml`
3. Replace the placeholder with: `OpenCppCoverage --sources "src\*" --export_type cobertura:coverage.xml -- bin\Debug\your-test-executable.exe`

The workflow will then:
- Build the project in Debug mode
- Run tests with coverage
- Generate Cobertura XML reports
- Upload results to Codecov (if configured)
- Archive coverage reports as artifacts

## Best Practices

1. **Run coverage regularly** - Integrate into development workflow
2. **Set coverage targets** - Aim for >80% line coverage
3. **Focus on critical paths** - Prioritize coverage for core functionality
4. **Review uncovered code** - Understand why certain code paths aren't tested
5. **Don't game metrics** - Write meaningful tests, not just coverage-boosting tests

## Limitations

- Coverage measures execution, not test quality
- 100% coverage doesn't guarantee bug-free code
- Some code paths (error handling, platform-specific) may be hard to cover
- Windows-only tooling (OpenCppCoverage) limits cross-platform coverage

## Alternative Tools

### Visual Studio Code Coverage (Enterprise only)

Visual Studio Enterprise includes built-in code coverage:

1. Test → Analyze Code Coverage
2. Results appear in Code Coverage Results window
3. Export as XML or binary format

Not recommended due to licensing requirements.

## Troubleshooting

### Application Crashes During Coverage

If the application crashes while running under code coverage, check the crash diagnostics:

1. **crash_log.txt**: Created automatically in headless mode
   - Contains timestamped log of initialization steps
   - Shows exactly where the crash occurred
   - Includes HRESULT error codes for D3D12 failures

2. **In CI/CD**: 
   - Crash log automatically displayed in workflow output
   - Also archived as "crash-diagnostics" artifact for download

3. **Common Issues**:
   - DirectX 12 initialization failures → Check WARP adapter setup
   - NVENC initialization → May not be available on all CI runners
   - Resource creation failures → Check for out-of-memory conditions

Example crash log:
```
[2026-02-14 00:13:51.545] Crash diagnostics initialized
[2026-02-14 00:13:51.546] >>> Parsing command-line arguments
[2026-02-14 00:13:51.546] Headless mode: ENABLED
[2026-02-14 00:13:51.547] >>> Creating application window
[2026-02-14 00:13:51.548] Window created successfully
[2026-02-14 00:13:51.549] >>> Initializing application
[2026-02-14 00:13:51.550] >>> App constructor - initializing graphics
[2026-02-14 00:13:51.551] InitializeGraphics: Getting source size
[2026-02-14 00:13:51.552] ERROR in InitializeGraphics: HRESULT = 0x887A0005 (-2005270523)
[2026-02-14 00:13:51.553] EXCEPTION in App constructor - InitializeGraphics
```

## Future Enhancements

- Automated coverage reporting in CI/CD pipeline
- Coverage trending over time
- Integration with pull request checks
- Coverage badges in README
