# Quick Fix: Stop Manual Workflow Approvals for Bot PRs

## The 3-Step Solution (Takes 30 seconds)

1. **Open repository settings:**
   ```
   https://github.com/eloise-normal-name/goblin-instrumentality-project/settings/actions
   ```

2. **Scroll down to:** "Fork pull request workflows from external contributors"

3. **Change the setting to one of these:**
   - ✅ **"Require approval for first-time contributors"** (recommended)
   - ✅ **"Require approval for first-time contributors who are new to GitHub"** (most permissive)

4. **Click "Save"**

## That's It!

Bot PRs from Copilot and github-actions[bot] will now run workflows automatically without manual approval.

## Why This Works

- GitHub's approval requirement is a **repository setting**, not a workflow problem
- Bot accounts (Copilot, github-actions[bot]) have already contributed to GitHub projects, so they're not "new to GitHub"
- If they've had PRs merged in your repo before, they're not "first-time contributors" either
- Changing this setting allows these trusted bots to run workflows immediately

## What Doesn't Work

❌ Creating a workflow to auto-approve (tried this - API can't bypass security settings)
❌ Configuring PAT tokens with special permissions (tokens can't override this setting)
❌ Adding bots to a trusted list in workflows (security check happens before workflows run)
❌ Using `workflow_run` triggers (triggers after the approval block, not before)

## Verify It Worked

1. Have Copilot create a new PR or push to an existing PR branch
2. Go to the PR on GitHub
3. Check the "Checks" tab
4. Workflows should start running immediately with **no "Approve and run" button**

## If It Still Requires Approval

This can happen if the bot is truly a "first-time contributor" to your repo:

**Option A:** Manually approve one PR from that bot
- After the first approval and merge, future PRs will run automatically

**Option B:** Use the most permissive setting
- Change to "Require approval for first-time contributors who are new to GitHub"
- This allows any bot account that exists on GitHub to run workflows

## Full Documentation

For detailed explanation of why previous attempts failed and why this works:
- See [docs/WORKFLOW_APPROVAL_SOLUTION.md](WORKFLOW_APPROVAL_SOLUTION.md)
