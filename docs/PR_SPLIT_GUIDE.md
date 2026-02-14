# PR Split Guide: Code Coverage Feature

This document explains how the code coverage feature was split into three sequential PRs for easier review and integration.

## Overview

The original large PR included:
- Code coverage infrastructure
- Headless testing with WARP adapter
- Crash diagnostics system

These have been split into three dependent PRs to make review and testing easier.

## PR #1: Code Coverage Infrastructure

**Branch:** `feature/code-coverage-infrastructure`
**Base:** `main`
**Goal:** Set up all coverage tooling without requiring code execution

### Files to Include

#### Configuration Files
- `.opencppcoverage` - OpenCppCoverage default settings
- `.gitignore` - Add coverage_report/, coverage.xml, *.bin, _codeql_detected_source_root
- `CMakeLists.txt` - Add debug symbols configuration for Debug builds only

#### Scripts
- `run_coverage.bat` - Windows batch script for running coverage
- `run_coverage.ps1` - PowerShell script with advanced options
- `validate_coverage_setup.bat` - Verification script

#### Documentation
- `docs/CODE_COVERAGE.md` - Complete coverage usage guide
- `docs/TEST_COVERAGE_INTEGRATION.md` - Guide for future test integration
- `README.md` - Add "Code Coverage" section with quick start

#### Workflow (Partial)
- `.github/workflows/coverage.yml` - Basic workflow that:
  - Builds the project
  - Has placeholder "Run Coverage" step that just echoes "Waiting for headless mode"
  - Does NOT upload coverage yet (no coverage.xml generated)

### Testing PR #1
```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
# Verify debug symbols are present
dir build\Debug\*.pdb
# Verify scripts exist
.\validate_coverage_setup.bat
```

### Success Criteria
- ✅ Build succeeds with debug symbols
- ✅ Documentation is complete and accurate
- ✅ Scripts are present (even if they can't run yet)
- ✅ Workflow runs without errors (even though coverage isn't collected)

---

## PR #2: Headless Testing with WARP

**Branch:** `feature/headless-testing`
**Base:** `feature/code-coverage-infrastructure` (PR #1)
**Goal:** Enable application to run in CI without GPU/display

### Files to Include

#### Source Code Changes
- `src/graphics/d3d12_device.h` - Add use_warp parameter to constructor
- `src/graphics/d3d12_device.cpp` - Implement WARP adapter selection
- `src/main.cpp` - Add CheckHeadlessFlag() and pass to App
- `src/app.ixx` - Add RunHeadless() method and headless member

#### Documentation
- `docs/HEADLESS_MODE.md` - Complete headless mode documentation
- `README.md` - Add "Running" section with headless instructions
- `docs/CODE_COVERAGE.md` - Update with headless mode examples

#### Workflow Update
- `.github/workflows/coverage.yml` - Update "Run Coverage" step to:
  ```yaml
  - name: Run Coverage
    run: |
      OpenCppCoverage --sources "src\*" --export_type cobertura:coverage.xml -- bin\Debug\goblin-stream.exe --headless
    continue-on-error: true
  ```
  - Enable Codecov upload
  - Enable coverage artifact archival

### Testing PR #2
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
- ✅ Workflow generates coverage.xml
- ✅ Coverage uploaded to Codecov
- ✅ No breaking changes to existing functionality

---

## PR #3: Crash Diagnostics

**Branch:** `feature/crash-diagnostics`
**Base:** `feature/headless-testing` (PR #2)
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

1. Merge PR #1 to `main` first
2. Merge PR #2 to `main` (will include PR #1 changes)
3. Merge PR #3 to `main` (will include PR #1 + PR #2 changes)

## Benefits of This Split

### PR #1 Benefits
- Pure infrastructure, zero risk to application
- Can be reviewed by documentation/DevOps specialists
- Sets up tooling for entire team immediately
- No code execution needed for testing

### PR #2 Benefits
- Focused on one feature: headless execution
- Clear before/after testing: does it run headlessly?
- Smaller code surface area for review
- Can verify coverage actually works

### PR #3 Benefits
- Debugging aid, not core functionality
- Can be deferred if needed
- Easy to review: just logging additions
- Clear value proposition: better error messages

## Creating the Branches

```bash
# Start from main
git checkout main
git pull origin main

# Create PR #1 branch
git checkout -b feature/code-coverage-infrastructure
# Cherry-pick relevant commits or manually apply changes
# Commit and push

# Create PR #2 branch from PR #1
git checkout -b feature/headless-testing feature/code-coverage-infrastructure
# Add headless changes
# Commit and push

# Create PR #3 branch from PR #2
git checkout -b feature/crash-diagnostics feature/headless-testing
# Add crash diagnostics
# Commit and push
```

## Alternative: Squash and Split

If you prefer clean history, squash current PR into themed commits:

```bash
git checkout copilot/add-code-coverage-testing
git reset --soft d7d4209  # Before any changes
git status  # See all changes

# Create themed commits
git add .opencppcoverage .gitignore CMakeLists.txt docs/ run_coverage.* validate_coverage_setup.bat README.md .github/workflows/coverage.yml
git commit -m "Add code coverage infrastructure"

git add src/graphics/ src/main.cpp src/app.ixx docs/HEADLESS_MODE.md
git commit -m "Implement headless testing with WARP"

git add src/crash_diagnostics.h include/try.h
git add -u src/  # Updated files
git commit -m "Add crash diagnostics system"
```

Then split into branches as above.

## Updating Instructions

The `.github/copilot-instructions.md` file has been updated to reference this split approach for future similar situations.
