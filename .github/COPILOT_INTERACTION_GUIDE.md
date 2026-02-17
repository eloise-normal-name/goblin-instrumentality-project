# GitHub Copilot Interaction Guide

This guide explains how to properly interact with GitHub Copilot coding agent in issues and pull requests.

## ⚠️ Security Note: Don't Use @copilot Mentions in Issue Comments

**DO NOT type `@copilot` in issue comments.** In most repositories, this references a GitHub user account named "copilot" (not the AI agent), which is a significant security concern as it allows impersonation and could lead to malicious actors monitoring your repository activity.

**Note:** @copilot mentions work correctly in pull request comments (official GitHub feature), but should be avoided in issue comments for security reasons. Instead, use issue assignment (see below).

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
Please investigate the resource cleanup in the frame loop and ensure
proper RAII patterns are followed.

Steps to reproduce:
1. Run the app for 100 frames
2. Check GPU memory usage with Task Manager
3. Memory steadily increases without cleanup
```

**What happens:**
- Copilot analyzes the issue context
- Creates a new branch
- Makes code changes following repository conventions
- Opens a pull request with the fix
- You review and provide feedback as needed

### In Pull Requests: Use @copilot in Comments (Safe)

Once you have an open pull request, **@copilot mentions in PR comments are safe and officially supported**:

1. **Comment on the PR** with `@copilot <task>`
2. Copilot will create a **new PR** based on the current PR's branch
3. Review and merge the new PR to incorporate changes

**Example:**
```
@copilot Please refactor this method to use RAII patterns
```

**Note:** This creates a separate PR so your original work stays intact. This is the official GitHub Copilot feature and does not have the security concerns of issue comments.

## 📋 Comparison: Issue Assignment vs PR Comments

| Feature | Issue Assignment | PR Comment Mention |
|---------|------------------|--------------------|
| **Syntax** | Assignees dropdown → @copilot | `@copilot` in PR comment |
| **Security** | ✅ Safe (official feature) | ✅ Safe in PRs (official) |
| **Context** | Issues only | Pull requests only |
| **Result** | Creates new PR | Creates new PR on branch |
| **Best For** | Full task/feature | Scoped change on existing PR |

**⚠️ Security Warning:** `@copilot` mentions in **issue comments** are unsafe (reference user account, not the AI). Only use @copilot via assignment in issues or in PR comments.

## 🎯 Best Practices

### For Bug Reports
1. Create issue with clear title and description
2. Add `bug` label (triggers BugBot automation — see below)
3. OR assign to @copilot via Assignees dropdown for automated fix

### For Feature Requests
1. Create issue describing the feature
2. Assign to @copilot via Assignees dropdown
3. Copilot will create a PR implementing the feature

### For Code Review on PRs
1. Use @copilot mentions in PR comments for scoped changes
2. Example: `@copilot Please refactor this to use RAII patterns`

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

### View All Triaged Issues
**[Copilot-Assigned Issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)**

## 📚 Additional Resources

- **Bug Triage System**: `.github/BUG_TRIAGE_SYSTEM.md` - Complete setup and usage
- **Custom Agents**: `.github/prompts/README.md` - Workspace agents for development
- **Repository Instructions**: `.github/copilot-instructions.md` - Coding standards

## 🚫 What NOT to Do

❌ **Don't type @copilot in issue comments** - References user account, not AI agent (security risk)
❌ **Don't type @<anything> in issue comments** - Mentions users, not AI agents
✅ **@copilot in PR comments is SAFE** - Official GitHub feature

✅ **Do assign issues via Assignees dropdown**
✅ **Do use @copilot in pull request comments**
✅ **Do use BugBot automation for bug triage** (automatic)

## ❓ FAQ

**Q: Why can't I use @copilot in issue comments?**
A: In issue comments, @copilot references a GitHub user account named "copilot" (not the AI), which is a security risk. However, @copilot mentions work correctly in **pull request comments** (official feature). For issues, use the Assignees dropdown instead.

**Q: How do I get Copilot to work on an issue?**
A: Assign the issue to @copilot via the Assignees dropdown in the right sidebar. Do not type @copilot in issue comments.

**Q: What about @clp /agent or other @ mentions in issues?**
A: Any @ mention in GitHub comments references a GitHub user, not an AI agent. This is a security risk. The custom agents (check-raii, review-error-handling, etc.) are internal VS Code/Copilot workspace tools, not meant to be invoked via GitHub comments.

**Q: Can I use @copilot in pull requests?**
A: Yes! @copilot mentions in **pull request comments** are safe and officially supported. They create a new PR based on the current branch.

**Q: How do I use the custom agents (check-raii, debug-resources, etc.)?**
A: These are internal Copilot workspace agents available in VS Code. They are not invoked via GitHub comments. They assist developers working in their local environment.

**Q: Do I need special permissions?**
A: Basic features work with default `GITHUB_TOKEN`. Enhanced BugBot functionality requires `COPILOT_MCP_GITHUB_TOKEN` secret (see `.github/BUG_TRIAGE_SYSTEM.md`).

**Q: How often does BugBot run?**
A: Daily at 9 AM UTC, plus automatically when issues are created/labeled with `bug`.

---

**Need Help?** See `.github/BUG_TRIAGE_SYSTEM.md` for comprehensive documentation.
