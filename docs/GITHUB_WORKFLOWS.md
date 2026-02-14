# GitHub Workflows - Automated Build Validation

This document describes the GitHub workflow that automates build validation and metrics tracking for the Goblin Instrumentality Project.

## Overview

The repository includes GitHub Actions workflows that automate build validation and issue monitoring:

1. **Build and Validate** - Automated build verification and metrics tracking
2. **Monitor Assigned Issues** - Tracks updates to issues assigned to bots/agents

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

### Monitor Assigned Issues (`.github/workflows/monitor-assigned-issues.yml`)

**Triggers**: 
- Scheduled every 6 hours
- Manual dispatch
- Issue assignment, edits, labeling
- Issue comments

**Purpose**: Ensures bot-assigned issues receive timely attention when users provide updates.

**Actions**:
- Monitors issues assigned to bot accounts (copilot, copilot-swe-agent[bot], github-actions[bot])
- Detects new user comments since last bot interaction
- Checks for issue updates (edits, labels, status changes)
- Adds `needs-bot-attention` label when user input is detected
- Runs scheduled checks to catch any missed updates

**Benefits**:
- Prevents issues from going stale when users provide additional information
- Ensures bot-assigned work items are prioritized when users engage
- Provides visibility into which issues need bot attention via labels
- Automated monitoring reduces manual tracking overhead

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
