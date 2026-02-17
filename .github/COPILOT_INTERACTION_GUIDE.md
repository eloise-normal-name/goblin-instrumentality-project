# GitHub Copilot Interaction Guide

This guide explains how to properly interact with GitHub Copilot coding agent in issues and pull requests.

## ⚠️ Security Note: Don't Use @copilot Mentions in Comments

**DO NOT type `@copilot` in issue or PR comments.** This references a GitHub user account named "copilot" (not the AI agent), which is a significant security concern as it allows impersonation and could lead to malicious actors monitoring your repository activity.

## ✅ Correct Way to Use GitHub Copilot

### In Issues: Assign to @copilot

To have GitHub Copilot work on an issue:

1. **Open or view the issue** you want Copilot to work on
2. **Click the "Assignees" section** on the right sidebar
3. **Type and select `@copilot`** from the assignee dropdown
4. **Optionally add instructions** in the issue description for specific guidance

**Example:**
```
Issue Title: Fix memory leak in frame loop

Description:
The application leaks GPU memory on every frame submission.

@copilot Please investigate the resource cleanup in the frame loop
and ensure proper RAII patterns are followed.
```

**What happens:**
- Copilot analyzes the issue context
- Creates a new branch
- Makes code changes following repository conventions
- Opens a pull request with the fix
- You review and provide feedback as needed

### In Pull Requests: Use @copilot in Comments

Once you have an open pull request, you can:

1. **Comment on the PR** with `@copilot <task>`
2. Copilot will create a **new PR** based on the current PR's branch
3. Review and merge the new PR to incorporate changes

**Example:**
```
@copilot Please refactor this method to use RAII patterns
```

**Note:** This creates a separate PR so your original work stays intact.

## 🤖 Using Custom Agents in Comments

For specialized tasks, use the `@clp /agent` pattern in **both issues and PRs**:

### Available Agents

| Agent | Usage | Purpose |
|-------|-------|---------|
| **bugbot** | `@clp /agent bugbot` | Automated bug triage and routing |
| **check-raii** | `@clp /agent check-raii` | Verify RAII patterns and resource management |
| **review-error-handling** | `@clp /agent review-error-handling` | Check error handling with Try \| pattern |
| **review-frame-logic** | `@clp /agent review-frame-logic` | Review D3D12 frame submission logic |
| **debug-resources** | `@clp /agent debug-resources` | Diagnose D3D12 resource issues |
| **explain-nvenc** | `@clp /agent explain-nvenc` | Explain NVENC API usage |
| **refactor-extract** | `@clp /agent refactor-extract` | Help extract cohesive functionality |
| **stage-changelist** | `@clp /agent stage-changelist` | Review changes and prepare commits |

### Example Usage

**In an issue:**
```
I'm seeing a crash when submitting command lists to the queue.
The error is STATUS_INVALID_PARAMETER.

@clp /agent review-error-handling Please analyze this crash
```

**In a pull request:**
```
@clp /agent check-raii Can you verify that all resources in this
changelist follow proper RAII patterns?
```

**For automated bug triage:**
```
@clp /agent bugbot Analyze all open bugs and assign to specialists
```

## 📋 Comparison: Issue Assignment vs Comment Mentions

| Feature | Issue Assignment | Comment Mention | Custom Agents |
|---------|-----------------|-----------------|---------------|
| **Syntax** | Assignees dropdown → @copilot | `@copilot` in comment | `@clp /agent <name>` |
| **Security** | ✅ Safe (official feature) | ⚠️ **UNSAFE** (references user) | ✅ Safe (workspace pattern) |
| **Context** | Full issue + repo | Current thread + repo | Specialized knowledge |
| **Result** | Creates new PR | Creates new PR on branch | Responds in thread |
| **Best For** | Full task/feature | Scoped change on existing PR | Analysis, review, routing |

## 🎯 Best Practices

### For Bug Reports
1. Create issue with clear title and description
2. Add `bug` label (triggers BugBot automation)
3. OR assign to @copilot via Assignees dropdown
4. OR use `@clp /agent bugbot` in a comment for manual triage

### For Code Reviews
1. Use custom agents for specialized review:
   - `@clp /agent check-raii` - Resource management
   - `@clp /agent review-error-handling` - Error handling
   - `@clp /agent review-frame-logic` - D3D12 frame logic

### For Refactoring
1. Open PR with initial changes
2. Use `@clp /agent refactor-extract` for suggestions
3. OR assign issue to @copilot for automated refactoring

## 🔧 Automated Bug Triage

This repository has **BugBot** configured to automatically triage bugs:

### Automatic Triggers
- **Daily at 9 AM UTC** - Scans all open bugs
- **On issue creation** - When labeled with `bug`
- **On label change** - When `bug` label is added

### What BugBot Does
1. Analyzes issue title and description
2. Detects component (graphics, encoder, nvenc, app)
3. Routes to specialist agent based on keywords:
   - Crashes → `review-error-handling`
   - Memory leaks → `check-raii`
   - Frame issues → `review-frame-logic`
   - Resource issues → `debug-resources`
   - NVENC issues → `explain-nvenc`
4. Adds labels: component, severity, `agent:<name>`, `triage:in-progress`
5. Posts assignment comment requesting analysis

### Manual Trigger
```
@clp /agent bugbot Analyze and triage all open bugs
```

### View All Triaged Issues
**[Copilot-Assigned Issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)**

## 📚 Additional Resources

- **Bug Triage System**: `.github/BUG_TRIAGE_SYSTEM.md` - Complete setup and usage
- **BugBot Guide**: `.github/agents/BUGBOT_GUIDE.md` - Quick reference
- **Custom Agents**: `.github/prompts/README.md` - All available agents
- **Repository Instructions**: `.github/copilot-instructions.md` - Coding standards

## 🚫 What NOT to Do

❌ **Don't type @copilot in comments** - References user account, not AI agent
❌ **Don't @ mention other bot names** - Could reference unrelated users
❌ **Don't expect @copilot comments to work** - Use assignment or @clp /agent instead

✅ **Do assign issues via Assignees dropdown**
✅ **Do use @clp /agent pattern for custom agents**
✅ **Do use BugBot automation for bug triage**

## ❓ FAQ

**Q: Why can't I use @copilot in comments?**
A: It references a GitHub user named "copilot", not the AI agent. This is a security risk.

**Q: How do I get Copilot to work on an issue?**
A: Assign the issue to @copilot via the Assignees dropdown in the right sidebar.

**Q: What's the difference between @copilot and @clp /agent?**
A: @copilot (via assignment) is GitHub's official coding agent. @clp /agent runs custom workspace agents with specialized knowledge.

**Q: Can custom agents create pull requests?**
A: No, custom agents respond in comment threads with analysis and recommendations. Use issue assignment for automated PRs.

**Q: Do I need special permissions?**
A: Basic features work with default `GITHUB_TOKEN`. Enhanced functionality requires `COPILOT_MCP_GITHUB_TOKEN` secret (see `.github/BUG_TRIAGE_SYSTEM.md`).

**Q: How often does BugBot run?**
A: Daily at 9 AM UTC, plus automatically when issues are created/labeled with `bug`.

---

**Need Help?** See `.github/BUG_TRIAGE_SYSTEM.md` for comprehensive documentation or `.github/agents/BUGBOT_GUIDE.md` for quick examples.
