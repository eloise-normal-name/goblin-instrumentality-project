---
name: 💡 How to Use GitHub Copilot
about: Quick reference for interacting with GitHub Copilot and custom agents
title: '[INFO] How to Use GitHub Copilot in This Repository'
labels: documentation
---

# 🤖 How to Use GitHub Copilot in This Repository

## ⚠️ Security Warning

**DO NOT type `@copilot` in issue comments!** This references a GitHub user account (not the AI), which is a security risk.

**Note:** @copilot mentions work correctly in **pull request comments** (official GitHub feature).

## ✅ Correct Ways to Use Copilot

### Option 1: Assign Issue to @copilot (Automated PR)

1. Click **"Assignees"** in the right sidebar
2. Select **@copilot** from the dropdown
3. Copilot will automatically:
   - Analyze the issue
   - Create a branch with changes
   - Open a pull request for review

### Option 2: Use Custom Agents (Analysis & Review)

Comment with `@clp /agent <agent-name>` pattern:

| Agent | When to Use |
|-------|-------------|
| `@clp /agent bugbot` | Automated bug triage and routing |
| `@clp /agent check-raii` | Verify resource management patterns |
| `@clp /agent review-error-handling` | Check error handling |
| `@clp /agent review-frame-logic` | Review D3D12 frame logic |
| `@clp /agent debug-resources` | Diagnose GPU resource issues |
| `@clp /agent explain-nvenc` | Explain NVENC API usage |

### Option 3: Use BugBot Automation (For Bugs)

1. Add `bug` label to your issue
2. BugBot automatically runs daily at 9 AM UTC
3. Routes bug to appropriate specialist agent
4. Adds component labels and triage status

## 📚 Full Documentation

See **[`.github/COPILOT_INTERACTION_GUIDE.md`](../COPILOT_INTERACTION_GUIDE.md)** for complete details.

---

**Questions?** Check the comprehensive guide or ask in the issue thread.
