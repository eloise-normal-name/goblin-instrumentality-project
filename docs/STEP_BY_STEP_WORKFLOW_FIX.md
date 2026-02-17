# Step-by-Step: Fix Workflow Approvals (With Screenshots Guide)

## The Problem

When Copilot or other bots create PRs, GitHub shows:

```
⚠️ Workflow(s) awaiting approval

This workflow run is waiting for approval to use Actions. 
A repository, organization, or enterprise owner must approve it.

[Approve and run]
```

You have to manually click "Approve and run" for every PR. This is annoying.

## The Solution (Takes 30 Seconds)

### Step 1: Open Repository Settings

Go to your repository's Actions settings:

**Direct link:**
```
https://github.com/eloise-normal-name/goblin-instrumentality-project/settings/actions
```

**Or navigate manually:**
1. Go to your repository: `https://github.com/eloise-normal-name/goblin-instrumentality-project`
2. Click the **"Settings"** tab (top navigation bar)
3. In the left sidebar, click **"Actions"** → **"General"**

### Step 2: Find the Right Setting

Scroll down the page until you see the section:

```
Fork pull request workflows from external contributors
```

This section controls whether workflows require approval from contributors.

You'll see a dropdown or radio buttons with these options:
- ⭕ Require approval for first-time contributors who are new to GitHub
- ⭕ Require approval for first-time contributors  
- ⭕ Require approval for all external contributors ← **You're probably on this one**

### Step 3: Change the Setting

Select one of these options:

**Option A (Recommended):**
```
⦿ Require approval for first-time contributors
```

This allows:
- ✅ Bot accounts with prior merged PRs to run workflows automatically
- ✅ Contributors who have had PRs merged before to run workflows automatically
- ⚠️ First-time contributors to your repo still need approval (security maintained)

**Option B (Most Permissive):**
```
⦿ Require approval for first-time contributors who are new to GitHub
```

This allows:
- ✅ Any GitHub account that exists to run workflows automatically (including all bots)
- ✅ Any contributor who has had **any** PR merged **anywhere** on GitHub
- ⚠️ Only brand new GitHub accounts need approval (minimal security)

**Choose Option A unless bots are still blocked, then use Option B.**

### Step 4: Save

Click the green **"Save"** button at the bottom of the page.

### Step 5: Verify It Worked

1. Have Copilot create a new PR (or push to an existing PR branch)
2. Go to the PR page on GitHub
3. Click the **"Checks"** tab
4. Workflows should start running **immediately**
5. You should **NOT** see:
   - ❌ "Workflow(s) awaiting approval" banner
   - ❌ "Approve and run" button

If workflows run automatically → **Success!** 🎉

If workflows still require approval → See "Troubleshooting" below.

---

## Troubleshooting

### Problem: Workflows still require approval after changing settings

**Cause:** The bot is truly a "first-time contributor" to your repository and you selected "Require approval for first-time contributors".

**Solution 1:** Manually approve **one** PR from the bot
- Click "Approve and run" on one PR
- Let it merge
- All future PRs from that bot will run automatically

**Solution 2:** Use the most permissive setting
- Go back to repository settings
- Select "Require approval for first-time contributors who are new to GitHub"
- This allows any existing GitHub account (including bots) to run workflows

### Problem: I don't see the "Fork pull request workflows" section

**Cause:** You might be looking at organization settings instead of repository settings.

**Solution:**
- Make sure you're in the **repository** settings, not your profile or organization settings
- URL should be: `https://github.com/eloise-normal-name/goblin-instrumentality-project/settings/actions`
- Look for the repository name "goblin-instrumentality-project" in the page header

### Problem: Setting won't save / reverts back

**Cause:** You might not have admin access to the repository.

**Solution:**
- You need to be a repository owner or admin to change this setting
- Check that you're logged in as the repository owner

### Problem: I changed the setting but existing PR still requires approval

**Cause:** The setting applies to **new workflow runs**, not existing pending ones.

**Solution:**
- Close the existing PR and reopen it
- Or push a new commit to the PR branch to trigger new workflow runs
- Or manually approve the existing run (it's already pending)

---

## What Changed?

**Before:**
```
Setting: Require approval for all external contributors
Result:  Every bot PR requires manual approval
```

**After:**
```
Setting: Require approval for first-time contributors (or more permissive)
Result:  Bot PRs run automatically (after first contribution or if not new to GitHub)
```

---

## Why This Is The Right Solution

### What Didn't Work (Don't Try These)

❌ Creating a workflow to auto-approve via GitHub API
❌ Configuring PAT tokens with special permissions  
❌ Adding bots to workflow allowlists
❌ Using MCP for programmatic approval
❌ Adding bots as repository collaborators

**Why they don't work:** GitHub's fork approval requirement is a **security setting**, not an API-level control. It cannot be bypassed programmatically by design.

### Why Changing The Setting Works

✅ You're modifying the **security policy** directly
✅ Allows trusted contributors/bots while maintaining security
✅ Takes 30 seconds and requires no code changes
✅ Official GitHub feature, not a workaround
✅ Doesn't require tokens, secrets, or workflows

---

## Security Implications

**Is this safe?**

Yes, when configured properly:

- **"Require approval for first-time contributors"**: Still requires approval for truly new contributors to your repo
- **"Require approval for first-time contributors who are new to GitHub"**: Still requires approval for brand new GitHub accounts
- You're not disabling the security feature entirely
- You're just trusting contributors/bots that have proven themselves before

**Best practice:**
- Use "Require approval for first-time contributors" (Option A)
- Review the first PR from any new bot carefully
- After it merges, future PRs from that bot run automatically
- You still maintain security for unknown contributors

---

## Quick Reference

| Setting | Who Needs Approval | Who Runs Automatically |
|---------|-------------------|------------------------|
| All external contributors | Everyone not in repo/org | Only repo members |
| First-time contributors | New to this repo | Prior contributors + bots |
| New to GitHub | New GitHub accounts | Any existing account (all bots) |

**For bot automation, use**: "First-time contributors" or "New to GitHub"

---

## Links

- **Your repository settings**: https://github.com/eloise-normal-name/goblin-instrumentality-project/settings/actions
- **Full explanation**: [docs/WORKFLOW_APPROVAL_SOLUTION.md](WORKFLOW_APPROVAL_SOLUTION.md)
- **Failed attempts**: [docs/FAILED_WORKFLOW_APPROVAL_ATTEMPTS.md](FAILED_WORKFLOW_APPROVAL_ATTEMPTS.md)
- **GitHub docs**: https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/enabling-features-for-your-repository/managing-github-actions-settings-for-a-repository

---

## Still Having Issues?

If you've followed this guide and workflows still require approval:

1. Double-check you saved the settings change
2. Try the most permissive option ("new to GitHub")
3. Manually approve one bot PR to establish it as a "contributor"
4. Verify the bot account name matches exactly (e.g., `Copilot` vs `copilot[bot]`)
5. Check if there are organization-level policies overriding repository settings

If none of this works, the repository might have unique organization policies or the bot account might be restricted. Contact GitHub support or check organization-level Actions settings.
