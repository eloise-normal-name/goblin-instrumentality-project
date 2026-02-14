# PR Split Guide: Code Coverage Feature

This document explains how the code coverage feature was split into three sequential PRs for easier review and integration.

## Overview

The original large PR included:
- Code coverage infrastructure
- Headless testing with WARP adapter
- Crash diagnostics system

These have been split into three dependent PRs to make review and testing easier.

**Merge Order:** PR #1 (Headless) → PR #2 (Coverage Infrastructure) → PR #3 (Crash Diagnostics)

## PR #1: Headless Testing with WARP

**Branch:** `feature/headless-testing`
**Base:** `main`
**Goal:** Enable application to run in CI without GPU/display (prerequisite for coverage)

### Files to Include

#### Source Code Changes
- `src/graphics/d3d12_device.h` - Add use_warp parameter to constructor
- `src/graphics/d3d12_device.cpp` - Implement WARP adapter selection
- `src/main.cpp` - Add CheckHeadlessFlag() and pass to App
- `src/app.ixx` - Add RunHeadless() method and headless member

#### Documentation
- `docs/HEADLESS_MODE.md` - Complete headless mode documentation
- `README.md` - Add "Running" section with headless instructions

#### Minimal Gitignore
- `.gitignore` - Add _codeql_detected_source_root

### Testing PR #1
```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
# Test normal mode still works
bin\Debug\goblin-stream.exe
# Test headless mode
bin\Debug\goblin-stream.exe --headless
# Should exit after 10 frames
```

### Success Criteria
- ✅ Normal mode still works (window appears)
- ✅ Headless mode runs without window and exits cleanly
- ✅ No breaking changes to existing functionality
- ✅ WARP adapter properly selected in headless mode

### Why This PR Comes First

Headless testing is the foundational capability that enables:
- Running the application in CI environments
- Code coverage execution (can't run coverage without executable running)
- Automated testing in GitHub Actions

Without headless mode, the coverage infrastructure would be incomplete and non-functional.

---

## PR #2: Code Coverage Infrastructure

**Branch:** `feature/code-coverage-infrastructure`
**Base:** `feature/headless-testing` (PR #1)
**Goal:** Set up all coverage tooling that works with headless execution

### Files to Include

#### Configuration Files
- `.opencppcoverage` - OpenCppCoverage default settings
- `.gitignore` - Add coverage_report/, coverage.xml, *.bin
- `CMakeLists.txt` - Add debug symbols configuration for Debug builds only

#### Scripts
- `run_coverage.bat` - Windows batch script for running coverage
- `run_coverage.ps1` - PowerShell script with advanced options
- `validate_coverage_setup.bat` - Verification script

#### Documentation
- `docs/CODE_COVERAGE.md` - Complete coverage usage guide
- `docs/TEST_COVERAGE_INTEGRATION.md` - Guide for future test integration
- `README.md` - Add "Code Coverage" section with quick start

#### Workflow
- `.github/workflows/coverage.yml` - Full workflow that:
  - Builds the project
  - Runs coverage with `--headless` flag
  - Uploads to Codecov
  - Archives coverage.xml artifact

### Testing PR #2
```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
# Verify debug symbols are present
dir build\Debug\*.pdb
# Run coverage locally
.\run_coverage.ps1 -Executable "bin\Debug\goblin-stream.exe"
# Verify coverage report generated
dir coverage_report\index.html
```

### Success Criteria
- ✅ Build succeeds with debug symbols
- ✅ Documentation is complete and accurate
- ✅ Coverage runs successfully with --headless flag
- ✅ Workflow generates coverage.xml
- ✅ Coverage uploaded to Codecov
- ✅ Scripts work correctly

### Why This PR Comes Second

With headless mode working (PR #1), we can now:
- Actually run the application under code coverage
- Generate real coverage data
- Upload results to Codecov
- Provide complete infrastructure for future testing

---

## PR #3: Crash Diagnostics

**Branch:** `feature/crash-diagnostics`
**Base:** `feature/code-coverage-infrastructure` (PR #2)
**Goal:** Add detailed logging to identify CI failures

### Files to Include

#### Source Code Changes
- `src/crash_diagnostics.h` - NEW: CrashDiagnostics class
- `src/main.cpp` - Integrate CrashDiagnostics, wrap in try-catch
- `src/app.ixx` - Add diagnostics parameter, log initialization steps
- `include/try.h` - Minor: already compatible, no changes needed

#### Documentation Updates
- `docs/HEADLESS_MODE.md` - Add "Crash Diagnostics" section
- `docs/CODE_COVERAGE.md` - Add "Troubleshooting" section with example crash logs

#### Workflow Updates
- `.github/workflows/coverage.yml` - Add steps:
  - Upload crash log artifact
  - Display crash log in output
  - Keep continue-on-error: true

#### Gitignore
- `.gitignore` - Add crash_log.txt

### Testing PR #3
```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
# Test crash logging
bin\Debug\goblin-stream.exe --headless
# Check that crash_log.txt was created with detailed logs
type crash_log.txt
```

### Success Criteria
- ✅ crash_log.txt created in headless mode
- ✅ Detailed step-by-step logging
- ✅ HRESULT errors logged with hex codes
- ✅ Exception context captured
- ✅ Workflow displays and uploads crash log
- ✅ No performance impact in normal mode

---

## Merge Order

1. Merge PR #1 (Headless Testing) to `main` first
2. Merge PR #2 (Coverage Infrastructure) to `main` (will include PR #1 changes)
3. Merge PR #3 (Crash Diagnostics) to `main` (will include PR #1 + PR #2 changes)

## Benefits of This Split

### PR #1 Benefits (Headless Testing)
- Core functionality needed for everything else
- Clear value: application can run in CI
- Easy to test: does it run headlessly?
- Small, focused code change
- No dependencies on other features

### PR #2 Benefits (Coverage Infrastructure)
- Builds on working headless mode
- Can immediately generate real coverage data
- Pure infrastructure addition
- Can be reviewed by documentation/DevOps specialists
- Provides value for entire team

### PR #3 Benefits (Crash Diagnostics)
- Debugging aid, not core functionality
- Can be deferred if needed
- Easy to review: just logging additions
- Clear value proposition: better error messages
- Minimal performance impact

## Creating the Branches

```bash
# Start from main
git checkout main
git pull origin main

# Create PR #1 branch (Headless Testing)
git checkout -b feature/headless-testing
# Cherry-pick relevant commits or manually apply changes
# Commit and push

# Create PR #2 branch from PR #1 (Coverage Infrastructure)
git checkout -b feature/code-coverage-infrastructure feature/headless-testing
# Add coverage infrastructure
# Commit and push

# Create PR #3 branch from PR #2 (Crash Diagnostics)
git checkout -b feature/crash-diagnostics feature/code-coverage-infrastructure
# Add crash diagnostics
# Commit and push
```

## Alternative: Squash and Split

If you prefer clean history, squash current PR into themed commits:

```bash
git checkout copilot/add-code-coverage-testing
git reset --soft d7d4209  # Before any changes
git status  # See all changes

# Create themed commits IN ORDER
# First: Headless testing
git add src/graphics/ src/main.cpp src/app.ixx docs/HEADLESS_MODE.md README.md .gitignore
git commit -m "Implement headless testing with WARP"

# Second: Coverage infrastructure
git add .opencppcoverage CMakeLists.txt docs/CODE_COVERAGE.md docs/TEST_COVERAGE_INTEGRATION.md
git add run_coverage.* validate_coverage_setup.bat .github/workflows/coverage.yml
git add -u .gitignore README.md  # Updates to these files
git commit -m "Add code coverage infrastructure"

# Third: Crash diagnostics
git add src/crash_diagnostics.h include/try.h
git add -u src/  # Updated source files
git add -u docs/  # Updated docs
git add -u .github/workflows/  # Updated workflow
git add -u .gitignore  # Final gitignore update
git commit -m "Add crash diagnostics system"
```

Then split into branches as shown above.

## Updating Instructions

The `.github/copilot-instructions.md` file has been updated to reference this split approach for future similar situations.
