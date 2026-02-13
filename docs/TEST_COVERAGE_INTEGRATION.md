# Example Test Integration with Code Coverage

This document provides examples of how to integrate unit tests with the code coverage infrastructure once tests are added to the project.

## Test Framework Integration

When adding tests to this project, consider these approaches:

### Option 1: CTest with CMake

Add to CMakeLists.txt:
```cmake
enable_testing()

add_executable(goblin-stream-tests
    tests/test_main.cpp
    tests/test_d3d12_device.cpp
    # Add more test files
)

target_link_libraries(goblin-stream-tests PRIVATE
    # Link against your main library or sources
    d3d12 dxgi dxguid
)

add_test(NAME AllTests COMMAND goblin-stream-tests)
```

Then run coverage on tests:
```cmd
OpenCppCoverage --sources "src\*" -- build\Debug\goblin-stream-tests.exe
```

### Option 2: Manual Test Executable

Create a separate test executable that exercises the main application code:

1. Build test executable with shared code
2. Run coverage on test executable
3. Coverage measures which production code was executed

Example:
```cmd
cmake --build build --config Debug --target goblin-stream-tests
OpenCppCoverage --sources "src\*" --export_type html:coverage -- build\Debug\goblin-stream-tests.exe
```

## Coverage for Different Test Types

### Unit Tests

For isolated component tests:
```cmd
OpenCppCoverage --sources "src\graphics\*" -- tests\graphics_unit_tests.exe
```

### Integration Tests

For end-to-end tests:
```cmd
OpenCppCoverage --sources "src\*" --export_type cobertura:integration_coverage.xml -- tests\integration_tests.exe
```

### Combined Coverage

Merge multiple coverage runs:
```cmd
REM Run first test suite
OpenCppCoverage --sources "src\*" --export_type binary:unit.cov -- tests\unit_tests.exe

REM Run second test suite and merge
OpenCppCoverage --sources "src\*" --input_coverage=unit.cov --export_type html:combined_coverage -- tests\integration_tests.exe
```

## Best Practices for Test Coverage

### 1. Structure Tests for Coverage

- Organize tests by component (graphics, encoder, etc.)
- Each test file should correspond to a source file
- Use descriptive test names that indicate what's being covered

### 2. Measure Coverage Incrementally

Start with critical components:
```cmd
REM Cover graphics subsystem first
OpenCppCoverage --sources "src\graphics\*" -- tests\graphics_tests.exe

REM Add encoder coverage
OpenCppCoverage --sources "src\encoder\*" -- tests\encoder_tests.exe
```

### 3. Set Coverage Targets

- Aim for >80% line coverage on core components
- 100% coverage on critical paths (initialization, cleanup)
- Lower coverage acceptable for error handling paths

### 4. Review Uncovered Code

Use HTML reports to identify:
- Uncovered branches (missing test cases)
- Uncovered functions (untested features)
- Uncovered error paths (add negative tests)

## Example Test Structure

```
goblin-stream/
├── src/
│   ├── graphics/
│   │   ├── d3d12_device.cpp
│   │   └── d3d12_device.h
│   └── encoder/
│       ├── nvenc_session.cpp
│       └── nvenc_session.h
├── tests/
│   ├── test_main.cpp
│   ├── graphics/
│   │   └── test_d3d12_device.cpp
│   └── encoder/
│       └── test_nvenc_session.cpp
└── CMakeLists.txt
```

## Running Tests with Coverage in CI/CD

The GitHub Actions workflow (.github/workflows/coverage.yml) will automatically:

1. Build the project
2. Run tests with coverage
3. Generate coverage reports
4. Upload to Codecov (if configured)
5. Archive reports as artifacts

## Viewing Coverage Reports

### Local Development

After running `run_coverage.bat` or `run_coverage.ps1`:
- HTML report opens automatically in browser
- Navigate through files to see line-by-line coverage
- Red lines = uncovered
- Green lines = covered

### CI/CD

- View artifacts in GitHub Actions run
- Download coverage.xml for detailed analysis
- Use Codecov dashboard for trend analysis

## Coverage Exclusions

To exclude specific code from coverage (use sparingly):

### OpenCppCoverage Exclusions

Add to .opencppcoverage:
```
--excluded_line_regex ".*NOCOVERAGE.*"
--excluded_sources "*\tests\*"
```

Then in code:
```cpp
// NOCOVERAGE - Platform-specific code
#ifdef SPECIAL_PLATFORM
    SpecialPlatformCode();
#endif
```

## Troubleshooting

### Coverage shows 0% despite tests running

- Ensure Debug build (/Zi flag enabled)
- Check --sources pattern matches your source files
- Verify executable is being run correctly

### Missing symbols in coverage report

- Rebuild with Debug configuration
- Ensure PDB files are generated
- Check CMakeLists.txt has /Zi and /DEBUG flags

### Tests pass but coverage fails

- Coverage tool may crash on some code patterns
- Use --continue_after_cpp_exception flag
- Check OpenCppCoverage verbose output for errors
