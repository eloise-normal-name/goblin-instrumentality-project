# BugBot - Automated Bug Triage Agent

## Overview

**BugBot** is an automated bug triage specialist agent that automatically monitors issues, categorizes them by component and severity, and routes them to appropriate specialist agents for detailed analysis.

## Key Features

✅ **Automated Monitoring** - Runs daily (9 AM UTC) or triggered by issue events  
✅ **Intelligent Routing** - Routes bugs to: check-raii, review-error-handling, review-frame-logic, debug-resources, explain-nvenc  
✅ **Component Detection** - Automatically labels issues with component: graphics, encoder, nvenc, app  
✅ **Severity Classification** - Labels by severity: critical, high, medium, low  
✅ **Context Provision** - Creates detailed assignment comments with analysis requests  
✅ **Project Board Integration** - Updates issue status and labels (with enhanced permissions)  

## How BugBot Works

1. **Detection**: Monitors GitHub Issues labeled with `bug` (daily or on issue creation)
2. **Analysis**: Parses bug reports and categorizes by:
   - Component (graphics, encoder, novenc, app)
   - Severity (critical → low)
   - Issue type (crash, memory leak, performance, behavior)
3. **Routing**: Assigns to specialist agent based on bug type
4. **Assignment**: Creates comment with analysis request and context
5. **Tracking**: Updates labels for audit and status tracking

## Supported Bug Categories & Routing

| Bug Pattern | Specialist Agent | Example Issue |
|-------------|------------------|---------------|
| Memory leak, resource leak, RAII violation | `check-raii` | "GPU memory leak in frame loop" |
| Crash, exception, uninitialized value | `review-error-handling` | "App crashes on frame submission" |
| D3D12 frame, command, synchronization | `review-frame-logic` | "Frame submission fails with invalid parameter" |
| GPU resource state, barrier, descriptor | `debug-resources` | "Resource state validation error" |
| NVENC API, encoder, interop | `explain-nvenc` | "NVENC encoding fails with invalid handle" |

## Usage

### Automatic (Recommended)
BugBot runs automatically:
- **Daily schedule**: 9 AM UTC (`.github/workflows/bug-triage.yml`)
- **Event trigger**: When issues are opened or labeled `bug`

Just label your issue with `bug` and BugBot will handle the triage!

### Manual Invocation
```bash
@clp /agent bugbot Analyze and triage all open bugs
@clp /agent bugbot Determine routing for issue #42
```

### Expected Output
BugBot posts a comment like:

```markdown
## Triage Assignment

**Assigned to**: @clp /agent check-raii

**Analysis Context**: Resource leak suspected - GPU memory not freed properly

**Issue Details**:
- Title: GPU memory leak in frame loop  
- Reporter: @developer
- Created: 2/16/2026

**Please analyze**:
1. Reproduce the issue using the steps provided
2. Identify the root cause
3. Propose a fix with specific file and line references  
4. Recommend testing approach

*Auto-triaged by BugBot. See `.github/agents/bugbot.agent.md` for triage criteria.*
```

The assigned specialist agent responds to this comment with their analysis.

## Configuration

### Minimal Setup (Out of Box)
- Just add `bug` label to issues
- Default `GITHUB_TOKEN` allows:
  - Reading issues
  - Creating assignment comments
  - Adding/updating labels
  - Works immediately!

### Enhanced Setup (Optional)
Set `COPILOT_MCP_GITHUB_TOKEN` secret for:
- Full GitHub API access
- Project board automation
- Advanced permissions
- See [BUG_TRIAGE_SYSTEM.md](../BUG_TRIAGE_SYSTEM.md) for setup

## Filing a Good Bug Report

**Do:**
✅ Use clear, descriptive title  
✅ Include exact reproduction steps  
✅ Attach error messages and stack traces  
✅ Note affected component (graphics, encoder, etc.)  
✅ Add `bug` label  

**Don't:**
❌ Vague titles ("It doesn't work")  
❌ Missing repro steps  
❌ No error details  
❌ Assumptions about cause  

**Example:**
```
Title: D3D12 command queue submit fails with STATUS_INVALID_PARAMETER on RTX 4090

Steps:
1. Run app with RTX 4090
2. Wait 5-10 frames
3. Crash in queue submit

Error: 0x80070057 (STATUS_INVALID_PARAMETER)
Labels: bug, graphics
```

## Workflow Integration

### For Issue Reporters
1. Create issue with `bug` label  
2. BugBot analyzes automatically  
3. Wait for specialist agent analysis in comments  
4. Provide additional context if needed  

### For Bug Fixers
1. Review BugBot assignment comment  
2. Read specialist agent analysis  
3. Implement fix based on recommendations  
4. Test thoroughly  
5. Reference issue in PR: "Fixes #123"  

### For Code Reviewers  
1. Check BugBot triage was accurate  
2. Verify specialist agent findings made sense  
3. Ensure fix addresses root cause  
4. Approve PR  

## Troubleshooting

**Issue not triaged?**
- Add `bug` label
- Trigger workflow manually (Actions tab)
- Check workflow logs

**Wrong agent assigned?**
- Reply in issue: `@clp /agent <correct-name>`
- Add comment with additional context

**Labels not updating?**
- Configure `COPILOT_MCP_GITHUB_TOKEN` secret (see BUG_TRIAGE_SYSTEM.md)
- Or manually add labels

## Files

- **Agent Definition**: `.github/agents/bugbot.agent.md`
- **Workflow Automation**: `.github/workflows/bug-triage.yml`
- **Full Documentation**: `.github/BUG_TRIAGE_SYSTEM.md`
- **Quick Reference**: `.github/prompts/README.md` (BugBot section)

## Example Workflow

```
User Opens Issue #42: "Memory leak in encoder"
         ↓ (9 AM UTC daily or on creation)
BugBot Detects Label: bug
         ↓
BugBot Analyzes: Title mentions "memory leak"
         ↓
BugBot Routes: → @clp /agent check-raii
         ↓
BugBot Updates Labels: graphics, memory, agent:check-raii, triage:in-progress
         ↓
BugBot Creates Assignment Comment
         ↓
check-raii Agent Responds: "Found destructor missing COM release in frame loop"
         ↓
Developer Fixes & Creates PR #123
         ↓
PR Merged: "Fixes #42"
         ↓
Closer Updates Status: Issue resolved ✓
```

## Quick Links

- **Report a Bug**: Create issue, add `bug` label
- **Manual Triage**: `@clp /agent bugbot`
- **Full Guide**: [BUG_TRIAGE_SYSTEM.md](../BUG_TRIAGE_SYSTEM.md)
- **Agent Guide**: [prompts/README.md](../prompts/README.md)
- **Setup Help**: [copilot-instructions.md](../copilot-instructions.md)

