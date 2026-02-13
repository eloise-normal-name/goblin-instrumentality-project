# Code Coverage Testing

This document describes how to measure code coverage for the Goblin Instrumentality Project.

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

Once tests are available, run coverage using:

```cmd
OpenCppCoverage --sources "src\*" --export_type html:coverage_report -- bin\Debug\goblin-stream.exe
```

**Options explained:**
- `--sources "src\*"` - Only measure coverage for source files in src/ directory
- `--export_type html:coverage_report` - Generate HTML report in coverage_report/ folder
- `-- bin\Debug\goblin-stream.exe` - The executable to run (adjust path as needed)

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

When GitHub Actions workflows are added, coverage can be automated using:

1. Windows runner with OpenCppCoverage installed
2. Generate Cobertura XML format
3. Upload to coverage services (Codecov, Coveralls, etc.)

Example GitHub Actions step:
```yaml
- name: Run Coverage
  run: |
    OpenCppCoverage --sources "src\*" --export_type cobertura:coverage.xml -- bin\Debug\goblin-stream.exe
```

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

## Future Enhancements

- Automated coverage reporting in CI/CD pipeline
- Coverage trending over time
- Integration with pull request checks
- Coverage badges in README
