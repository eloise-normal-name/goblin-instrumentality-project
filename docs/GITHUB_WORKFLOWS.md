# GitHub Workflows - Automated Build Validation

This document describes the GitHub workflow that automates build validation and metrics tracking for the Goblin Instrumentality Project.

## Overview

The repository includes GitHub Actions workflows that automate build validation:

1. **Build and Validate** - Automated build verification and metrics tracking
2. **Run App and Log Output** - Executes the app in headless mode and captures output logs
3. **Update Highlights Page** - Automatically updates the refactor highlights page with latest metrics
4. **Monitor Assigned Issues** - Tracks updates to issues assigned to bots/agents
5. **Auto-Approve Bot Workflow Runs** - Automatically approves workflow runs from trusted bot accounts
6. **Docs Index Check** - Ensures docs are indexed in README

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
- Runs application with `--headless` flag (300 frames, auto-exit)
- Captures console output and exit code
- Collects `debug_output.txt` file if generated
- Uploads log files as artifacts (7-day retention)
- Creates execution summary with debug output preview

**Benefits**:
- Validates app runs without crashes on Windows
- Captures runtime output for debugging
- Detects initialization or encoding errors early
- Provides downloadable logs for detailed analysis
- Uses hardware GPU adapter (not WARP) to test with actual NVENC encoder
- Continues on error to ensure logs are always captured

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
- Commits and pushes updated highlights page

**Benefits**:
- Eliminates manual updates to highlights page
- Ensures metrics are always current with latest build
- Maintains historical build log automatically
- Provides automated documentation of project progress
- Runs only on main/develop to avoid noise from feature branches

**Note**: The workflow has `contents: write` permission to commit changes back to the repository. It will only commit if there are actual changes to the highlights file.

### Auto-Approve Bot Workflow Runs (`.github/workflows/auto-approve-bot-workflows.yml`)

**Triggers**: 
- `workflow_run` event when "Build and Validate" workflow is requested
- Only for pull request events

**Purpose**: Enables trusted bot accounts to trigger PR review workflows without manual approval.

**Actions**:
- Detects when a workflow run is triggered by a pull request
- Checks if the actor is a trusted bot (copilot, copilot-swe-agent[bot], github-actions[bot])
- Automatically approves the workflow run using GitHub API
- Logs approval status for auditing

**Benefits**:
- Allows bot-created PRs to run automated checks immediately
- Eliminates manual approval requirement for trusted bots
- Improves automation workflow efficiency
- Maintains security by restricting auto-approval to trusted bots only

**Security**:
- Only trusted bot accounts are approved (explicitly listed)
- Uses `actions: write` permission to approve runs
- Logs all approval actions for audit trail
- Gracefully handles approval errors (e.g., already approved)

**Trusted Bots**:
- `copilot` - GitHub Copilot agent
- `copilot-swe-agent[bot]` - Copilot SWE agent
- `github-actions[bot]` - GitHub Actions bot

**Maintenance**:
- When adding new workflows that run on PRs, update the `workflows:` list in this workflow file
- When adding trusted bot accounts, update the `trustedBots` array in the workflow script
- Review workflow logs periodically to ensure approvals are working as expected

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
- Verify app runs long enough to generate output (should complete 300 frames)
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

**Metric Extraction Failures:**
If metrics are incorrect or missing:
- Verify `cloc` installation succeeded in the workflow
- Check that the MinSizeRel binary exists at expected path
- Review regex patterns in the PowerShell script for HTML updates
- Ensure git history is available (workflow uses `fetch-depth: 0`)

**Commit/Push Failures:**
If the workflow fails to commit changes:
- Verify git configuration is set correctly (user.name, user.email)
- Check for permission errors in workflow logs
- Ensure no protected branch rules prevent bot commits
- Review if there are merge conflicts (shouldn't happen on main)

### Auto-Approve Bot Workflow Runs

**Adding New Workflows:**
If a new workflow that runs on PRs is added:
- Update `.github/workflows/auto-approve-bot-workflows.yml` to include the new workflow name in the `workflows:` list
- This ensures trusted bots can trigger the new workflow without manual approval

**Approval Not Working:**
If bot PRs still require manual approval:
- Verify the bot account is in the `trustedBots` list in the workflow
- Check that the workflow has `actions: write` permission
- Review workflow logs for API errors or permission issues
- Ensure the actor login matches exactly (e.g., `copilot` vs `copilot[bot]`)

**Debugging:**
- Check the workflow run logs in the Actions tab
- Look for "Check if run is from trusted bot" step output
- Verify the actor and fork status are correctly detected

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
