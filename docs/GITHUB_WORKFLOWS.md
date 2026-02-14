# GitHub Workflows - Automated Validation

This document describes the GitHub workflows that automate build validation, code quality checks, and metrics tracking for the Goblin Instrumentality Project.

## Overview

The repository now includes four GitHub Actions workflows that automate processes previously done manually:

1. **Build and Validate** - Automated build verification and metrics tracking
2. **Format Check** - Code formatting validation  
3. **Code Quality** - Project-specific pattern and convention enforcement
4. **Generate Highlights** - On-demand report generation

## Workflows

### 1. Build and Validate (`.github/workflows/build-and-validate.yml`)

**Triggers**: Push to main/develop, PRs, manual dispatch

**Purpose**: Ensures the codebase builds successfully and tracks key metrics.

**Actions**:
- Configures CMake with Visual Studio 2022 (GitHub Actions runner)
- Builds Debug, Release, and MinSizeRel configurations
- Verifies MinSizeRel binary exists
- Reports binary size (used to track bloat)
- Counts source lines using `cloc`
- Uploads build artifacts (binaries available for 7 days)
- Creates build summary in GitHub Actions UI

**Benefits**:
- Catches build breaks immediately on PRs
- Tracks binary size growth over time
- Provides downloadable binaries for testing
- Automates metrics previously tracked manually in `refactor-highlights.html`

### 2. Format Check (`.github/workflows/format-check.yml`)

**Triggers**: PRs to main/develop, manual dispatch

**Purpose**: Validates all code follows `.clang-format` configuration.

**Actions**:
- Installs LLVM toolchain (includes clang-format)
- Checks all `.cpp`, `.ixx`, `.h` files in `src/` and `include/`
- Excludes `nvEncodeAPI.h` (3rd party vendor header)
- Reports formatting violations with file/line annotations
- Fails PR if any files are improperly formatted

**Benefits**:
- Enforces consistent code style automatically
- Prevents formatting debates in code reviews
- Ensures `.clang-format` rules are followed
- Provides clear error messages for violations

### 3. Code Quality (`.github/workflows/code-quality.yml`)

**Triggers**: PRs to main/develop, manual dispatch

**Purpose**: Enforces project-specific coding conventions and patterns.

**Checks**:
- **RAII violations**: Flags `Initialize`/`Shutdown` methods
- **Style violations**: Detects trailing underscores, C++ casts, namespaces
- **Error handling**: Warns about unchecked D3D12/NVENC API calls
- **Build configuration**: Verifies `/W4` warning level in CMakeLists.txt

**Benefits**:
- Automates code review for mechanical issues
- Enforces project conventions at CI time
- Catches common mistakes early
- Reduces manual code review burden

### 4. Generate Highlights (`.github/workflows/generate-highlights.yml`)

**Triggers**: Manual dispatch only (workflow_dispatch)

**Purpose**: Generates metrics report for release notes and change tracking.

**Actions**:
- Determines base commit (last highlights commit or specified)
- Counts changed files since base
- Builds MinSizeRel configuration
- Reports binary size and source line count
- Creates formatted summary in GitHub Actions UI

**Benefits**:
- Automates metrics collection for `refactor-highlights.html`
- Provides consistent reporting format
- Can be triggered on-demand for any commit range

## CI vs Local Development

| Aspect | Local Development | GitHub Actions CI |
|--------|------------------|-------------------|
| **Visual Studio** | VS 2026 (version 18) | VS 2022 (version 17) |
| **CMake Generator** | `Visual Studio 18 2026` | `Visual Studio 17 2022` |
| **Purpose** | Development & debugging | Automated validation |
| **Formatting** | Auto-format on save (VS Code) | Validate on PR |
| **Quality Checks** | Optional manual checks | Automated on every PR |

## Integration with Development Workflow

### For Contributors

1. **Before committing**: Format code locally (Ctrl+Shift+I in VS Code)
2. **Before pushing**: Build locally to catch errors early
3. **After creating PR**: Wait for CI checks to complete
4. **If checks fail**: Review GitHub Actions logs, fix issues, push updates

### For Maintainers

1. **Merge requirements**: All workflow checks must pass
2. **Binary size**: Monitor trends in build summaries
3. **Metrics tracking**: Use "Generate Highlights" workflow for release notes
4. **Quality enforcement**: Use workflow checks as conversation starters, not blockers

## Workflow Configuration

All workflows use:
- **Runner**: `windows-latest` (Windows Server with VS 2022)
- **Shell**: PowerShell (`pwsh`) for scripts
- **Retention**: 7 days for build artifacts
- **Concurrency**: Runs automatically on PR updates

## Future Enhancements

Potential additions to CI workflows:
- Static analysis (if external tools are approved)
- Performance benchmarking (track frame time)
- Automated testing (when test infrastructure is added)
- Documentation generation
- Release automation

## Troubleshooting

### Workflow Fails but Builds Locally

- Check Visual Studio version (2026 local vs 2022 CI)
- Verify all source files are committed
- Check for platform-specific code paths

### Format Check Fails

The Format Check workflow validates that all code follows the `.clang-format` configuration. When it fails:

**Step 1: Identify which files need formatting**
Check the workflow logs to see the list of files that need formatting.

**Step 2: Format the files locally**
```bash
# Format all source files (run from repository root)
# Use -style=file to ensure .clang-format from repository root is used
clang-format -i -style=file src/**/*.cpp src/**/*.h src/**/*.ixx include/*.h

# Or format specific files listed in the error
clang-format -i -style=file src/encoder/nvenc_config.cpp
clang-format -i -style=file src/graphics/d3d12_device.cpp
# ... etc

# Or use a loop for multiple files
for file in src/**/*.cpp src/**/*.h src/**/*.ixx include/*.h; do clang-format -i -style=file "$file"; done
```

**Step 3: Verify formatting**
```bash
# Check that formatting matches (use -style=file)
clang-format -style=file src/main.cpp | diff src/main.cpp -
```

**Step 4: Commit and push**
```bash
git add .
git commit -m "Format code according to .clang-format"
git push
```

**Note:** `nvEncodeAPI.h` is excluded from formatting checks as it's a 3rd party vendor header.

### Code Quality Check Fails

Review the workflow logs for specific violations and fix them according to project conventions:

**RAII Violations:**
- Error: Found `Initialize()` or `Shutdown()` methods
- Fix: Move resource allocation to constructors and cleanup to destructors
- Example:
  ```cpp
  // ✗ Wrong
  class Resource {
      void Initialize() { /* allocate */ }
      void Shutdown() { /* cleanup */ }
  };
  
  // ✓ Correct
  class Resource {
      Resource() { /* allocate */ }
      ~Resource() { /* cleanup */ }
  };
  ```

**C++ Cast Violations:**
- Error: Found `static_cast`, `dynamic_cast`, `reinterpret_cast`, or `const_cast`
- Fix: Replace with C-style casts: `(Type)value`
- Example:
  ```cpp
  // ✗ Wrong
  auto ptr = static_cast<ID3D12Device*>(device);
  
  // ✓ Correct
  auto ptr = (ID3D12Device*)device;
  ```

**Namespace Violations:**
- Error: Found `namespace` declarations
- Fix: Remove namespaces, use global namespace only
- Example:
  ```cpp
  // ✗ Wrong
  namespace MyNamespace {
      void Function() {}
  }
  
  // ✓ Correct
  void Function() {}
  ```

**Unchecked API Calls:**
- Warning: D3D12/NVENC calls without `Try |` pattern
- Fix: Use `Try |` for all API calls that return HRESULT
- Example:
  ```cpp
  // ✗ Wrong
  device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
  
  // ✓ Correct
  Try | device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
  ```

### Build Failures

Check the build logs and ensure:
- All source files are listed in `CMakeLists.txt`
- Code compiles with `/W4` warning level
- No missing headers or undefined references
- Static linking flags are correct (`/MT` runtime)

### Accessing Workflow Logs

1. Go to the PR on GitHub
2. Scroll to the bottom and click "Details" next to the failed check
3. Click on the failed job to see detailed logs
4. Review error messages and file paths

## References

- GitHub Actions Documentation: https://docs.github.com/en/actions
- Project Instructions: `.github/copilot-instructions.md`
- Refactor Highlights: `refactor-highlights.html`
