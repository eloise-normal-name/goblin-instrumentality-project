---
name: 💡 How to Use GitHub Copilot
about: Quick reference for interacting with GitHub Copilot
title: '[INFO] How to Use GitHub Copilot in This Repository'
labels: documentation
---

# 🤖 How to Use GitHub Copilot in This Repository

## ⚠️ Security Warning

**DO NOT type @mentions in issue comments!** Any @mention (like @copilot, @clp, etc.) references a GitHub user account, NOT an AI agent. This is a security risk.

**Note:** @copilot mentions work correctly in **pull request comments** (official GitHub feature).

## ✅ Correct Ways to Use Copilot

### Option 1: Assign Issue to @copilot (Automated PR)

1. Click **"Assignees"** in the right sidebar
2. Select **@copilot** from the dropdown
3. Copilot will automatically:
   - Analyze the issue
   - Create a branch with changes
   - Open a pull request for review

### Option 2: Use @copilot in PR Comments

Once you have a pull request open, you can comment with:
```
@copilot Please refactor this method to use RAII patterns
```

Copilot will create a new PR based on the current branch with the requested changes.

### Option 3: Use BugBot Automation (For Bugs)

1. Add `bug` label to your issue
2. BugBot automatically runs daily at 9 AM UTC
3. Routes bug to appropriate specialist  
4. Adds component labels and triage status

## 📚 Full Documentation

See **[`.github/COPILOT_INTERACTION_GUIDE.md`](../COPILOT_INTERACTION_GUIDE.md)** for complete details.

---

**Questions?** Check the comprehensive guide or ask in the issue thread.
