# GitHub Workflows - Automated Build Validation

This document describes the GitHub workflow that automates build validation and metrics tracking for the Goblin Instrumentality Project.

## Overview

The repository includes GitHub Actions workflows that automate build validation and issue monitoring:

1. **Build and Validate** - Automated build verification and metrics tracking
2. **Documentation Linting** - Validates markdown documentation for link correctness and consistency
3. **Monitor Assigned Issues** - Tracks updates to issues assigned to bots/agents
4. **Auto-Approve Bot Workflow Runs** - Automatically approves workflow runs from trusted bot accounts

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

### Documentation Linting (`.github/workflows/docs-lint.yml`)

**Triggers**: Push to main, PRs, manual dispatch

**Purpose**: Ensures documentation maintains quality standards and all links are valid.

**Actions**:
- **Markdown Link Check**: Validates internal links in markdown files
  - **Internal markdown links**: Validates relative paths to other `.md` files (e.g., `[text](./other-doc.md)`, `[text](../folder/file.md)`)
  - **Anchor links**: Validates section references within documents (e.g., `[text](#section-name)`)
  - **External URLs ignored**: HTTP/HTTPS links are not checked to focus on internal documentation structure
- **Markdown Linting**: Enforces consistent markdown syntax
  - ATX-style headers (using `#`)
  - Consistent list indentation
  - Allows HTML elements where needed (br, details, summary)
  - Flexible line length for readability

**Benefits**:
- **Enables agent efficiency**: Ensures agents can navigate documentation via internal links without encountering broken references
- **Catches broken internal links early**: Validates internal file references and anchors before they reach production
- **Maintains documentation quality**: Enforces consistent markdown formatting across all docs
- **Prevents link rot**: Detects when files are moved/renamed without updating cross-references

**Configuration**:
- Link checker config: `.github/markdown-link-check-config.json`
- Markdown lint rules: `.markdownlint.json`

**Scanned Files**:
- All `.md` files in the repository
- `docs/*.md` - Project documentation
- `.github/prompts/*.md` - Custom agent prompts
- `.github/copilot-instructions.md` - Development guidelines

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

**Labels Used**:
- `needs-bot-attention`: Automatically added when user comments or updates are detected on bot-assigned issues. This label helps maintainers and bots identify which issues require action.

**Note**: The workflow will attempt to create the label if it doesn't exist. If label creation fails, it will log a warning but continue processing.

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

### Documentation Linting Workflow

**Link Check Failures:**
If the link checker reports broken links:
- **Internal markdown links**: Verify the referenced file exists at the specified path; update paths if files were moved/renamed
- **Anchor links**: Ensure the target section heading exists in the referenced document; check for correct heading format
- Add patterns to `.github/markdown-link-check-config.json` to ignore specific link patterns if needed

**Markdown Lint Failures:**
If markdown linting reports issues:
- Review the specific rule that failed (e.g., MD003, MD007)
- Fix formatting to match the rule requirements
- Update `.markdownlint.json` to adjust or disable specific rules if needed
- Run `markdownlint-cli2` locally to test changes before pushing

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

### Monitor Assigned Issues Workflow

**Label Creation Failures:**
If the workflow fails to add the `needs-bot-attention` label:
- Ensure the workflow has `issues: write` permission (already configured)
- Check if the label exists in the repository (workflow will attempt to create it)
- Review workflow logs for permission or API errors

**Workflow Not Triggering:**
If the workflow doesn't run when expected:
- Check if the issue is assigned to one of the monitored bots (copilot, copilot-swe-agent[bot], github-actions[bot])
- Verify the issue event type matches the triggers (assignment, edits, comments)
- Use manual dispatch to test the scheduled run functionality

**Label Not Applied:**
If user comments don't result in labeling:
- Verify the comments are from non-bot users
- Check if the bot has interacted with the issue previously
- Review workflow run logs to see comment detection logic

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
