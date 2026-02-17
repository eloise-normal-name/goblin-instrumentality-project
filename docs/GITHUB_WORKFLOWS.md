# GitHub Workflows - Automated Build Validation

This document describes the GitHub workflow that automates build validation and metrics tracking for the Goblin Instrumentality Project.

## Overview

The repository includes GitHub Actions workflows that automate build validation:

1. **Build and Validate** - Automated build verification and metrics tracking
2. **Run App and Log Output** - Executes the app in headless mode and captures output logs
3. **Fix App Failure** - Automatically triggers when Run App workflow fails to notify and provide analysis
4. **Update Highlights Page** - Automatically updates the refactor highlights page with latest metrics
5. **Docs Index Check** - Ensures docs are indexed in README

**Note**: This repository does not use GitHub Issues for task tracking.

## Workflow

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

### Run App and Log Output (`.github/workflows/run-app.yml`)

**Triggers**: Push to main/develop, PRs, manual dispatch

**Purpose**: Executes the application in headless mode to validate runtime behavior and capture execution logs for analysis.

**Actions**:
- Configures CMake with Visual Studio 2022
- Builds MinSizeRel configuration
- Verifies binary exists
- Runs application with `--headless` flag (30 frames, auto-exit)
- Captures console output and exit code
- **Fails the workflow if app exits with non-zero code or crashes**
- Collects `debug_output.txt` file if generated
- Uploads log files as artifacts (7-day retention)
- Creates execution summary with debug output preview

**Benefits**:
- Validates app runs without crashes on Windows
- Captures runtime output for debugging
- Detects initialization or encoding errors early
- **Ensures workflow fails if app crashes or exits with errors**
- Provides downloadable logs for detailed analysis
- Uses hardware GPU adapter (not WARP) to test with actual NVENC encoder
- Log capture steps always run (`if: always()`) even if app fails

### Fix App Failure (`.github/workflows/fix-app-failure.yml`)

**Triggers**: 
- `workflow_run` event when "Run App and Log Output" workflow completes with failure
- Manual dispatch with workflow run ID

**Purpose**: Automatically detects app execution failures and provides analysis to help diagnose and fix issues.

**Actions**:
- Triggers automatically when Run App workflow fails
- Downloads execution logs if available
- Analyzes failure and creates detailed summary
- Provides step-by-step instructions for investigation
- Can be manually triggered for any failed run

**Benefits**:
- Immediate notification when app fails in CI
- Centralizes failure information for easy access
- Provides structured guidance for debugging
- Enables quick response to runtime failures
- Supports manual triggering for historical failures

**Usage**:
- Automatically runs when Run App workflow fails on main/develop
- Manually trigger from Actions tab with run ID of failed workflow
- Review the workflow summary for failure analysis
- Download logs and follow investigation steps

### Update Highlights Page (`.github/workflows/update-highlights.yml`)

**Triggers**: Push to main/develop, manual dispatch

**Purpose**: Automatically updates the `refactor-highlights.html` page with latest build metrics and project statistics.

**Actions**:
- Builds MinSizeRel configuration to get current binary
- Collects metrics: binary size, source line count (via cloc)
- Determines base commit from last highlights update
- Counts files changed since base
- Updates HTML file with current metrics:
  - Comparison range and generation date
  - Files changed count
  - Binary size
  - Source lines count
  - Build log entry with timestamp
- Commits and pushes updated highlights page to `gh-pages` branch

**Benefits**:
- Eliminates manual updates to highlights page
- Ensures metrics are always current with latest build
- Maintains historical build log automatically
- Provides automated documentation of project progress
- Runs only on main/develop to avoid noise from feature branches
- Publishes to dedicated `gh-pages` branch, keeping main branch clean

**Note**: The workflow has `contents: write` permission to commit changes to the `gh-pages` branch. It will only commit if there are actual changes to the highlights file. The `gh-pages` branch is automatically created if it doesn't exist.

### Docs Index Check (`.github/workflows/docs-index-check.yml`)

**Triggers**: Push, PRs, manual dispatch

**Purpose**: Prevents orphaned documentation by ensuring every file in `docs/` is linked from `README.md`.

**Actions**:
- Scans `docs/` for markdown files
- Parses `README.md` for markdown links
- Fails if any `docs/*.md` file is not referenced in the README documentation index

**Benefits**:
- Keeps documentation easy to discover
- Flags unused or forgotten docs early in CI
- Encourages a single, maintained index of docs

## CI vs Local Development

| Aspect | Local Development | GitHub Actions CI |
|--------|------------------|-------------------|
| **Visual Studio** | VS 2026 (version 18) | VS 2022 (version 17) |
| **CMake Generator** | `Visual Studio 18 2026` | `Visual Studio 17 2022` |
| **Purpose** | Development & debugging | Automated validation |

## Integration with Development Workflow

### For Contributors

1. **Before committing**: Build locally to catch errors early
2. **Before pushing**: Ensure code compiles with `/W4` warning level
3. **After creating PR**: Wait for CI checks to complete
4. **If checks fail**: Review GitHub Actions logs, fix issues, push updates

### For Maintainers

1. **Merge requirements**: Build workflow check must pass
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
- Code formatting validation
- Code quality checks (RAII, casts, namespaces)
- Static analysis (if external tools are approved)
- Performance benchmarking (track frame time)
- Automated testing (when test infrastructure is added)
- Documentation generation
- Release automation

## Troubleshooting

### Run App and Log Output

**App Crashes or Exits with Error:**
If the app fails to run in the workflow:
- Check the "Run app in headless mode" step output for exit codes
- Review uploaded `debug_output.txt` artifact for error messages
- Verify the app builds successfully (check "Build MinSizeRel" step)
- Ensure GitHub Actions runner has hardware GPU support (uses Hardware adapter, not WARP)
- Check for DirectX 12 or NVENC initialization failures in logs
- Verify NVIDIA GPU drivers are available on the runner

**Missing debug_output.txt:**
If the debug output file is not created:
- Verify app runs long enough to generate output (should complete 30 frames)
- Check console output for file I/O errors
- Review app source code to ensure file is being written
- Ensure working directory is correct when app executes

**Accessing App Logs:**
To review detailed execution logs:
1. Go to the workflow run in the Actions tab
2. Click on the "run-app" job
3. Review "Run app in headless mode" step for console output
4. Download "app-execution-logs" artifact for `debug_output.txt`
5. Check execution summary at bottom of workflow run page

### Update Highlights Page

**Highlights Not Updating:**
If the highlights page doesn't update after a push to main/develop:
- Check the workflow run logs in the Actions tab
- Verify the workflow has `contents: write` permission
- Ensure the build completed successfully (workflow builds MinSizeRel)
- Check if there are actual changes to commit (workflow skips commit if no changes)
- Verify the `gh-pages` branch exists and is accessible

**Metric Extraction Failures:**
If metrics are incorrect or missing:
- Verify `cloc` installation succeeded in the workflow
- Check that the MinSizeRel binary exists at expected path
- Review regex patterns in the PowerShell script for HTML updates
- Ensure git history is available (workflow uses `fetch-depth: 0`)

**Commit/Push Failures:**
If the workflow fails to commit changes to `gh-pages`:
- Verify git configuration is set correctly (user.name, user.email)
- Check for permission errors in workflow logs
- Ensure no protected branch rules prevent bot commits to `gh-pages`
- Confirm the `gh-pages` branch was created successfully (first run creates it)
- Review if there are merge conflicts (shouldn't happen on main)

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

### Accessing Workflow Logs

1. Go to the PR on GitHub
2. Scroll to the bottom and click "Details" next to the failed check
3. Click on the failed job to see detailed logs
4. Review error messages and file paths

## References

- GitHub Actions Documentation: https://docs.github.com/en/actions
- Project Instructions: `.github/copilot-instructions.md`
- Refactor Highlights: `refactor-highlights.html`
