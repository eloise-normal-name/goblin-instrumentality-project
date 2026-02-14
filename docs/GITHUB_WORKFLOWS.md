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

- Run `clang-format -i <file>` locally
- Commit formatted files
- Ensure `.clang-format` config is up to date

### Code Quality Check Fails

- Review specific pattern violations in workflow log
- Consult `.github/copilot-instructions.md` for project conventions
- Fix violations or discuss exceptions if necessary

## References

- GitHub Actions Documentation: https://docs.github.com/en/actions
- Project Instructions: `.github/copilot-instructions.md`
- Refactor Highlights: `refactor-highlights.html`
