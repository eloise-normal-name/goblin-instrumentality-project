# Bug Triage System Setup & Usage

The Goblin Instrumentality Project now includes **BugBot**, an automated bug triage and assignment agent that monitors issues, routes them to specialized agents, and keeps your project board updated.

**[View Copilot-Assigned Issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)** - All issues currently assigned to copilot agents

## Automated Crash Detection

**NEW**: The system now automatically detects crashes in build verification and creates issues for agent analysis!

When the "Run App and Log Output" workflow fails:
1. **Crash Detection Workflow** (`create-crash-issue.yml`) triggers automatically
2. Downloads crash logs and extracts crash details (access violations, HRESULT errors, exceptions)
3. Creates a new bug issue with crash type, logs, and reproduction steps
4. Labels the issue: `bug`, `critical`, `automated`, and appropriate component label
5. **BugBot** automatically triages the new issue and assigns it to a specialist agent

**No manual intervention needed** - agents can now discover and analyze crashes without local testing!

## Quick Start

### For Users Reporting Bugs
1. Create an issue describing the problem
2. Add the `bug` label to your issue
3. BugBot will automatically:
   - Analyze your bug report
   - Route it to the appropriate specialist agent
   - Update issue labels with component and severity
   - Post a comment requesting detailed analysis

### For Developers Triaging Bugs
1. **[View all copilot-assigned issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)** - See all issues currently being analyzed
2. Let BugBot run automatically (daily at 9 AM UTC)
3. Or manually trigger with: `@clp /agent bugbot`
4. Review issue assignments in comments
5. Wait for specialized agent analysis
6. Implement fixes based on agent recommendations

### For Automated Crash Issues
Crashes detected in CI automatically create issues labeled `automated`:
- Check the **[Run App and Log Output](https://github.com/eloise-normal-name/goblin-instrumentality-project/actions/workflows/run-app.yml)** workflow for crash history
- Download `app-execution-logs` artifact from failed runs for detailed crash logs
- Automated issues are triaged just like manual issues

## System Architecture

```
         User Creates Issue          OR      App Crashes in CI
                 ↓                                    ↓
           [bug] label added              Crash Detection Workflow
                 ↓                                    ↓
                 ↓                        Creates Bug Issue (automated)
                 ↓                                    ↓
                 └──────────────┬─────────────────────┘
                                ↓
                   BugBot Monitors (Daily 9 AM UTC or on issue create)
                                ↓
                   Categorizes Bug by:
                   - Component (graphics, encoder, nvenc, app)
                   - Severity (critical, high, medium, low)
                   - Type (crash, wrong behavior, performance, leak, etc.)
                                ↓
                   Routes to Specialist Agent:
                   - check-raii (resource/memory issues)
                   - review-error-handling (crashes, exceptions)
                   - review-frame-logic (D3D12 frame issues)
                   - debug-resources (GPU state issues)
                   - explain-nvenc (encoder integration)
                                ↓
                   Specialist Agent Analyzes:
                   - Reproduces issue (or uses crash logs)
                   - Identifies root cause
                   - Suggests specific fix
                   - Recommends testing approach
                                ↓
                   Developer Implements Fix:
                   - Reviews agent analysis
                   - Makes targeted changes
                   - Tests thoroughly
                   - Creates PR with findings
```

## Bug Labels & Categories

### Component Labels
Use these to help BugBot route issues correctly:
- `graphics` - D3D12 device, swap chain, commands, resources
- `encoder` - NVENC session, encoding, bitstream output
- `nvenc` - NVENC-D3D12 interop and GPU surface handling
- `app` - Main app logic and orchestration
- `docs` - Documentation and examples

### Severity Labels
- `critical` - Crashes, data loss, security issues, blocks testing
- `high` - Wrong behavior, serious performance issues
- `medium` - Edge case failures, performance degradation
- `low` - Cosmetic issues, minor improvements

### Triage Status Labels
BugBot adds these automatically:
- `agent:<agent-name>` - Indicates which agent is assigned (informational)
- `triage:in-progress` - Bug has been triaged and assigned
- `automated` - Issue was created automatically by crash detection system

## Configuration

### GitHub Actions Setup

The automated bug triage system includes these workflows:

#### 1. Run App and Log Output (`.github/workflows/run-app.yml`)
- **Trigger**: On push to main/develop, PRs, or manual dispatch
- **Purpose**: Runs the app in headless mode to detect crashes
- **Detection**: Validates exit code and detects null (crash) or non-zero (error) codes
- **Output**: Uploads execution logs as artifacts

#### 2. Create Crash Issue (`.github/workflows/create-crash-issue.yml`)
- **Trigger**: When "Run App and Log Output" workflow fails
- **Purpose**: Automatically creates bug issues for detected crashes
- **Features**:
  - Downloads crash logs from failed workflow
  - Extracts crash type (Access Violation, HRESULT Error, Exception)
  - Creates or updates issue with crash details and reproduction steps
  - Adds labels: `bug`, `critical`, `automated`, and component label
  - Checks for existing open crash issues to avoid duplicates

#### 3. Bug Triage & Assignment (`.github/workflows/bug-triage.yml`)
- **Trigger**: Daily at 9 AM UTC, when issues are opened/labeled, or manual dispatch
- **Purpose**: Routes bugs to appropriate specialist agents
- **Routing Logic**:
  - `explain-nvenc`: NVENC, encoder, bitstream issues
  - `review-error-handling`: Crashes, exceptions, critical errors
  - `check-raii`: Memory leaks, resource management
  - `review-frame-logic`: D3D12 frames, command lists, graphics
  - `debug-resources`: GPU resource states, diagnostics

### Permissions Required

**For Basic Operation (default `GITHUB_TOKEN`):**
- Read issues and issue comments
- Create issue comments
- Add/remove issue labels
- Works with public repositories

**For Enhanced Functionality (with `COPILOT_MCP_GITHUB_TOKEN`):**
Set the `COPILOT_MCP_GITHUB_TOKEN` secret in repository settings:
- Move cards on GitHub Project boards
- Update issue milestones
- Assign reviewers with elevated permissions
- Full GitHub API access

**To configure the token:**
1. Go to repository **Settings** → **Secrets and variables** → **Actions**
2. Add new repository secret: `COPILOT_MCP_GITHUB_TOKEN`
3. Use a GitHub Personal Access Token (PAT) with appropriate scopes:
   - Classic PAT: Select `repo` scope
   - Fine-grained PAT: Select `issues`, `pull_requests`, `contents`, `projects` scopes

## How Bug Routing Works

BugBot uses intelligent keyword matching to route issues:

### Memory/Resource Leaks → `check-raii`
Triggers on keywords: leak, memory, RAII, destructor, cleanup
Example: "GPU memory leak in frame loop"

### Crashes & Exceptions → `review-error-handling`
Triggers on keywords: crash, exception, STATUS_, HRESULT, error code
Example: "App crashes on frame submission with STATUS_ACCESS_VIOLATION"

### D3D12 Frame Issues → `review-frame-logic`
Triggers on keywords: frame, command, d3d12, submission, present, sync
Example: "Frame drops when command list submits large buffers"

### GPU Resource Problems → `debug-resources`
Triggers on keywords: debug, resource, state, barrier, descriptor
Example: "Resource state validation failure in debug layer"

### NVENC Integration Issues → `explain-nvenc`
Triggers on keywords: nvenc, encode, encoder, bitstream
Example: "NVENC encoding fails with invalid surface handle"

## Assignment Comment Details

When BugBot assigns a bug, it posts a comment like:

```markdown
## Triage Assignment

**Assigned to**: @clp /agent review-error-handling

**Analysis Context**: Critical crash or exception detected

**Issue Details**:
- Title: App crashes on frame submission
- Reporter: @username
- Created: 2/16/2026

**Please analyze**:
1. Reproduce the issue using the steps provided in the issue description
2. Identify the root cause
3. Propose a fix with specific file and line references
4. Recommend testing approach

*Auto-triaged by BugBot. See `.github/agents/bugbot.agent.md` for triage criteria.*
```

The specialist agent will respond to this comment with their analysis.

## Manual Bug Triaging

If you want to manually route a specific issue:

```bash
# Trigger BugBot to analyze all open bugs
@clp /agent bugbot

# Ask BugBot to specifically analyze one issue
@clp /agent bugbot Analyze issue #42 and suggest which agent should investigate
```

## Best Practices

### When Reporting a Bug
1. **Use clear titles** - "D3D12 device creation fails on AMD" is better than "Doesn't work"
2. **Provide reproduction steps** - Exact steps to reproduce the bug
3. **Include symptom details**:
   - Exact error message or crash address
   - System configuration (GPU model, driver version)
   - When it started (changed files, OS update, etc.)
4. **Add first label** - Add `bug` label so BugBot knows to process it
5. **Describe impact** - Does it block testing? Is it a full crash? Performance only?

### Example Good Bug Report
```
Title: Frame submission crashes with STATUS_INVALID_PARAMETER on RTX 4090

Description:
When running with NVIDIA RTX 4090, the app crashes during frame submission.

**Steps to reproduce:**
1. Run app with RTX 4090 GPU
2. Let it run for 5-10 frames
3. Crash occurs in D3D12 command queue submit

**Error:**
Status code: 0x80070057 (STATUS_INVALID_PARAMETER)
Stack: ...

**System:**
- OS: Windows 11 23H2
- GPU: NVIDIA RTX 4090
- Driver: 552.12
```

### When Implementing a Fix
1. **Review the agent analysis comment** - Understand the root cause
2. **Make minimal changes** - Fix only the identified issue
3. **Add error handling** - Prevent similar issues
4. **Test thoroughly** - Verify the fix and check for regressions
5. **Reference the issue** - Link to the bug report in your PR

## Troubleshooting

### BugBot Didn't Assign My Issue
**Possible causes:**
- Issue not labeled with `bug`
- Didn't create the issue yet (wait for next daily run)
- Workflow is disabled

**Solutions:**
- Add `bug` label to the issue
- Manually trigger workflow: Go to **Actions** → **Bug Triage & Assignment** → **Run workflow**
- Check workflow logs for errors

### Labels Not Being Updated
**Possible causes:**
- Default `GITHUB_TOKEN` limitations (no `COPILOT_MCP_GITHUB_TOKEN` configured)
- Permissions not set correctly

**Solutions:**
- Configure `COPILOT_MCP_GITHUB_TOKEN` for full permissions (see Configuration section)
- Check workflow run logs for permission errors
- Manually add labels if needed: `agent:<name>`, `triage:in-progress`

### Agent Not Responding to Assignment Comment
**Possible causes:**
- Agent not available or mentioned incorrectly
- Issue doesn't have enough context

**Solutions:**
- Check agent spelling: `check-raii`, `review-error-handling`, `review-frame-logic`, `debug-resources`, `explain-nvenc`
- Reply with more context: error messages, code snippets, reproduction steps
- Manually invoke agent: `@clp /agent <name> <your question>`

## Integration with Your Workflow

### Pre-Merge Checklist
Before merging a bug fix PR:
1. [ ] Agent analysis comment reviewed and understood
2. [ ] Fix addresses root cause identified by agent
3. [ ] No new issues introduced (consider running relevant agent again)
4. [ ] Close associated bug issue with "Fixed by #PR_NUMBER"

### Issue Closure
When a fix is merged:
```markdown
Fixed by #123

This implements the recommendations from the specialist agent analysis.
Closes #<original_bug_number>
```

## Additional Resources

- **BugBot Agent**: `.github/agents/bugbot.agent.md` - Full agent instructions
- **Workflow**: `.github/workflows/bug-triage.yml` - Automation configuration
- **Agent Guide**: `.github/prompts/README.md` - Specialized agent documentation
- **Project Instructions**: `.github/copilot-instructions.md` - Coding standards and patterns

## FAQ

**Q: Does this replace code review?**
A: No. BugBot performs initial triage and routing. Developer review and code review processes are unchanged.

**Q: Can I disable automatic triaging?**
A: Yes. Disable the workflow in GitHub Actions settings, or remove the `bug` label from issues.

**Q: What if BugBot routes to the wrong agent?**
A: Reply in the issue mentioning the correct agent: `@clp /agent <correct-agent-name>`

**Q: How often does BugBot run?**
A: Daily at 9 AM UTC, plus whenever an issue is opened or labeled with `bug`.

**Q: Can I use this for feature requests?**
A: BugBot is specifically for bug triage. Use standard issue labels for feature requests and enhancements.

