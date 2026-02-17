# Failed Workflow Approval Attempts - DO NOT RETRY

This document lists all the approaches that have been tried and **confirmed to NOT work** for automating workflow approvals on bot PRs.

## ❌ Failed Attempt #1: Auto-Approve Workflow with GitHub API

**What was tried:**
- Created `.github/workflows/auto-approve-bot-workflows.yml`
- Used `workflow_run` trigger to detect when workflows need approval
- Called `github.rest.actions.approveWorkflowRun()` API endpoint
- Configured `COPILOT_MCP_GITHUB_TOKEN` with `repo` scope

**Why it failed:**
- The `approveWorkflowRun` API is for approving **pending workflow runs**, not bypassing the fork contributor security policy
- GitHub's fork contributor approval requirement is a **repository security setting** that cannot be programmatically bypassed
- The workflow triggers **after** the security check blocks the run, so it can't preemptively allow it
- This is by design - GitHub intentionally prevents automated bypass of this security feature

**Evidence:**
- Workflow runs successfully and calls the API
- API returns success (or 422 "already approved")
- But the target workflows still show "Workflow awaiting approval"
- Logs show: "Successfully approved workflow run" but nothing changes

**Status:** Confirmed failed - DO NOT RETRY

---

## ❌ Failed Attempt #2: Personal Access Token (PAT) Configuration

**What was tried:**
- Created GitHub Personal Access Token (Classic PAT)
- Granted `repo` scope (full repository access)
- Added as repository secret `COPILOT_MCP_GITHUB_TOKEN`
- Used token in auto-approve workflow
- Tried both Classic PAT and Fine-grained PAT with Actions write permission

**Why it failed:**
- PAT tokens provide elevated **API permissions**, not security policy overrides
- The fork contributor approval setting applies **regardless of token permissions**
- GitHub's security model doesn't allow tokens to bypass repository security policies
- The token works fine for API calls, but cannot override the approval requirement

**Evidence:**
- Token validation succeeds
- API calls authenticate successfully
- Workflow logs show token has proper permissions
- But workflows still require manual approval

**Status:** Confirmed failed - DO NOT RETRY

---

## ❌ Failed Attempt #3: Trusted Bot Allowlist in Workflow

**What was tried:**
- Created array of trusted bot usernames in workflow
- Added: `Copilot`, `copilot[bot]`, `copilot-swe-agent[bot]`, `github-actions[bot]`
- Checked `github.event.workflow_run.actor.login` against allowlist
- Only approved runs from trusted bots

**Why it failed:**
- The approval requirement happens **before** workflows run
- Workflow-level checks cannot affect the pre-workflow security policy
- GitHub's security check is at the repository settings level, not workflow level
- Workflows can't see or affect runs that haven't been approved yet

**Evidence:**
- Workflow correctly identifies bot actors
- Logs show "Actor is a trusted bot"
- But the workflow itself only runs **after** manual approval is given
- Can't approve workflows that are blocked before any workflows run

**Status:** Confirmed failed - DO NOT RETRY

---

## ❌ Failed Attempt #4: MCP (Model Context Protocol) Configuration

**What was tried:**
- Configured `.github/mcp-config.json` for GitHub API integration
- Used MCP server to programmatically call GitHub API
- Attempted to approve workflows via MCP

**Why it failed:**
- MCP is just another way to call the GitHub API
- Suffers from the same limitations as Failed Attempt #1
- Cannot bypass repository security settings
- MCP doesn't have special privileges that regular API calls don't have

**Status:** Confirmed failed - DO NOT RETRY

---

## ❌ Failed Attempt #5: Workflow Event Filtering

**What was tried:**
- Modified auto-approve workflow to accept both `pull_request` and `push` events
- Added job-level `if` condition: `github.event.workflow_run.event == 'pull_request' || github.event.workflow_run.event == 'push'`
- Logic: Bot PRs push commits to branches, might trigger as `push` events

**Why it failed:**
- While the filtering is technically correct, it doesn't solve the approval problem
- The workflow still runs **after** the security check blocks the target workflow
- Event filtering doesn't affect when or how the approval requirement is enforced
- This was a red herring - the event type isn't the problem

**Status:** Confirmed failed (but the event filtering logic is correct for other purposes) - DO NOT RETRY as a solution

---

## ❌ Failed Attempt #6: Adding Bots to Repository Collaborators

**What was tried:**
- Attempted to add bot accounts as repository collaborators
- Logic: Collaborators bypass fork approval requirements

**Why it failed:**
- GitHub doesn't allow adding bot accounts as direct collaborators
- Bot accounts like `copilot[bot]`, `github-actions[bot]` can't be invited
- These are service accounts, not user accounts
- Even `Copilot` (the SWE agent account) may not be invitable

**Status:** Technically impossible - DO NOT RETRY

---

## ❌ Failed Attempt #7: Organization-Level Configuration

**What was tried:**
- Attempted to configure organization-level Actions settings
- Logic: Organization settings override repository settings

**Why it failed:**
- The repository is not part of an organization (owned by personal account)
- Organization settings don't apply to personal account repositories
- Even if it were in an org, this is still a security setting that must be explicitly configured

**Status:** Not applicable - DO NOT RETRY

---

## ✅ What Actually Works

**Repository Settings Configuration:**

1. Go to: `https://github.com/eloise-normal-name/goblin-instrumentality-project/settings/actions`
2. Scroll to: **"Fork pull request workflows from external contributors"**
3. Change to: **"Require approval for first-time contributors"** or **"Require approval for first-time contributors who are new to GitHub"**
4. Click **Save**

This is a **30-second fix** that actually works because:
- It modifies the **repository security policy** directly
- Bot accounts with prior contributions bypass approval automatically
- Bot accounts that exist on GitHub are not "new to GitHub"
- This is the **only** way to allow automated workflow runs on bot PRs

---

## Key Lessons Learned

1. **GitHub's security model is intentional**: Fork approval requirements cannot be bypassed programmatically
2. **API permissions ≠ Security overrides**: Even full-access PAT tokens can't override security settings
3. **Workflows run too late**: By the time workflows run, the approval requirement has already blocked target workflows
4. **Repository settings > Workflow automation**: Some problems require settings changes, not code solutions
5. **GitHub's documentation is unclear**: The fork approval feature is poorly documented and easy to misunderstand

---

## If Someone Suggests These Again

**Response:** "All of these approaches have been tried and confirmed to fail. See `docs/FAILED_WORKFLOW_APPROVAL_ATTEMPTS.md` for detailed explanations of why each doesn't work. The only working solution is to configure repository settings as documented in `docs/WORKFLOW_APPROVAL_SOLUTION.md`."

---

## References

- Working solution: [docs/WORKFLOW_APPROVAL_SOLUTION.md](WORKFLOW_APPROVAL_SOLUTION.md)
- Quick fix guide: [docs/QUICK_FIX_WORKFLOW_APPROVAL.md](QUICK_FIX_WORKFLOW_APPROVAL.md)
- GitHub Docs: [Managing GitHub Actions settings](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/enabling-features-for-your-repository/managing-github-actions-settings-for-a-repository)
