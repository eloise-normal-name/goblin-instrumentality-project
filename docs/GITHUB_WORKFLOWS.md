# GitHub Workflows - Automated Build Validation

This document describes the GitHub workflows that automate build validation, code formatting, and metrics tracking for the Goblin Instrumentality Project.

## Overview

The repository includes the following GitHub Actions workflows:

1. **Build and Validate** - Automated build verification and metrics tracking
2. **Format Check** - Code formatting validation using clang-format

## Workflows

### Build and Validate (`.github/workflows/build-and-validate.yml`)

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
- Determines base commit (last highlights commit or specified)
- Counts changed files since base
- Builds MinSizeRel configuration
- Reports binary size and source line count

### Format Check (`.github/workflows/format-check.yml`)

**Triggers**: PRs modifying source files, .clang-format, or the workflow itself; manual dispatch

**Purpose**: Ensures all C++ code follows the project's formatting standards defined in `.clang-format`.

**Actions**:
- Verifies `.clang-format` exists in repository root
- Displays first 5 lines of `.clang-format` for debugging
- Checks all C++ files (`.cpp`, `.ixx`, `.h`, `.hpp`) in `src/` and `include/` directories
- Excludes vendor files: `include/nvenc/nvEncodeAPI.h`
- Uses `clang-format -style=file --dry-run --Werror` to validate formatting
- Reports files needing formatting with clear fix instructions

**Benefits**:
- Enforces consistent code style across the project
- Catches formatting violations before code review
- Provides actionable error messages with fix commands
- Prevents formatting debates during PR reviews
- Ensures `.clang-format` configuration is respected

**Exclusions**:
- `include/nvenc/nvEncodeAPI.h` - Third-party NVIDIA vendor header

## CI vs Local Development

| Aspect | Local Development | GitHub Actions CI |
|--------|------------------|-------------------|
| **Visual Studio** | VS 2026 (version 18) | VS 2022 (version 17) |
| **CMake Generator** | `Visual Studio 18 2026` | `Visual Studio 17 2022` |
| **Purpose** | Development & debugging | Automated validation |

## Integration with Development Workflow

### For Contributors

1. **Before committing**: Build locally to catch errors early
2. **Before pushing**: Ensure code compiles with `/W4` warning level and is properly formatted
3. **After creating PR**: Wait for CI checks to complete (build and format validation)
4. **If checks fail**: Review GitHub Actions logs, fix issues, push updates

### For Maintainers

1. **Merge requirements**: Both build and format-check workflows must pass
2. **Binary size**: Monitor trends in build summaries
3. **Metrics tracking**: Use workflow outputs to track project size over time

## Workflow Configuration

The workflow uses:
- **Runner**: `windows-latest` (Windows Server with VS 2022)
- **Shell**: PowerShell (`pwsh`) for scripts
- **Retention**: 7 days for build artifacts
- **Concurrency**: Runs automatically on PR updates

## Future Enhancements

Potential additions to CI workflows:
- Code quality checks (RAII, casts, namespaces)
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

### Build Failures

Check the build logs and ensure:
- All source files are listed in `CMakeLists.txt`
- Code compiles with `/W4` warning level
- No missing headers or undefined references
- Static linking flags are correct (`/MT` runtime)

### Format Check Failures

If the format-check workflow reports files need formatting:

1. **Fix a single file**:
   ```powershell
   clang-format -i -style=file path/to/file.cpp
   ```

2. **Fix all files at once**:
   ```powershell
   Get-ChildItem -Recurse -Include *.cpp,*.ixx,*.h,*.hpp -Path src,include | ForEach-Object { clang-format -i -style=file $_.FullName }
   ```

3. **Verify formatting locally** (before committing):
   ```powershell
   clang-format -style=file --dry-run --Werror path/to/file.cpp
   ```

4. **Commit and push** the formatting changes

**Notes**:
- The workflow uses the `.clang-format` file from the repository root
- `include/nvenc/nvEncodeAPI.h` is automatically excluded (vendor file)
- Format your code before pushing to avoid CI failures
- VS Code formats on save if configured correctly (Ctrl+Shift+I)

### Accessing Workflow Logs

1. Go to the PR on GitHub
2. Scroll to the bottom and click "Details" next to the failed check
3. Click on the failed job to see detailed logs
4. Review error messages and file paths

## References

- GitHub Actions Documentation: https://docs.github.com/en/actions
- Project Instructions: `.github/copilot-instructions.md`
- Refactor Highlights: `refactor-highlights.html`
