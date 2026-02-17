# MCP Configuration for GitHub Workflow Approval

This directory contains an MCP (Model Context Protocol) configuration file that can be used as an alternative to the GitHub Actions workflow for approving bot PR workflows.

## Files

- **`mcp-config.json`**: MCP server configuration for GitHub API integration with the `COPILOT_MCP_GITHUB_TOKEN` Personal Access Token

## Usage

### Option 1: GitHub Actions Workflow (Recommended)

The repository includes `.github/workflows/auto-approve-bot-workflows.yml` which automatically approves workflow runs from trusted bots using the GitHub API.

**Setup:**
1. Create a GitHub Personal Access Token (PAT):
   - **Classic PAT**: Enable `repo` scope (full repository access)
   - **Fine-grained PAT**: Grant Actions `write` permission level
2. Add as repository secret:
   - Go to: Settings → Secrets and variables → Actions → New repository secret
   - Name: `COPILOT_MCP_GITHUB_TOKEN`
   - Value: Your PAT

**How it works:**
- Triggers when specified workflows are requested (Build and Validate, Run App, Code Quality, Docs Index)
- Checks if the actor is a trusted bot (Copilot, github-actions[bot], etc.)
- Validates the `COPILOT_MCP_GITHUB_TOKEN` secret
- Automatically approves the workflow run via GitHub API

### Option 2: MCP Configuration (Alternative)

If you prefer to use the Model Context Protocol directly, use the `mcp-config.json` configuration.

**Setup:**
1. Install the MCP GitHub server:
   ```bash
   npm install -g @modelcontextprotocol/server-github
   ```

2. Export your GitHub PAT as an environment variable:
   ```bash
   # Windows (PowerShell)
   $env:COPILOT_MCP_GITHUB_TOKEN="your_pat_here"
   
   # Windows (Command Prompt)
   set COPILOT_MCP_GITHUB_TOKEN=your_pat_here
   
   # Linux/macOS
   export COPILOT_MCP_GITHUB_TOKEN=your_pat_here
   ```

3. Use the MCP configuration with your MCP-compatible client:
   ```bash
   mcp-client --config .github/mcp-config.json
   ```

**Available MCP Tools:**
The GitHub MCP server provides tools for:
- Listing and managing workflow runs
- Approving workflow runs
- Querying repository information
- Managing issues and pull requests
- And more...

## Token Requirements

Regardless of which option you choose, the `COPILOT_MCP_GITHUB_TOKEN` must have appropriate permissions:

### Classic Personal Access Token
- Required scope: `repo` (Full control of private repositories)
  - Includes: `repo:status`, `repo_deployment`, `public_repo`, `repo:invite`, `security_events`

### Fine-grained Personal Access Token
- Required permissions:
  - **Actions**: Read and write access
  - **Contents**: Read access (for repository information)
  - **Metadata**: Read access (automatically included)

### Creating a Personal Access Token

**Classic PAT:**
1. Go to: https://github.com/settings/tokens
2. Click "Generate new token" → "Generate new token (classic)"
3. Select scopes: `repo`
4. Click "Generate token"
5. Copy the token immediately (it won't be shown again)

**Fine-grained PAT:**
1. Go to: https://github.com/settings/tokens?type=beta
2. Click "Generate new token"
3. Configure:
   - Token name: "Goblin Instrumentality MCP Token"
   - Expiration: Your preference
   - Repository access: Select this repository
   - Permissions: Actions (Read and write)
4. Click "Generate token"
5. Copy the token immediately

## Security Considerations

- **Never commit your PAT to version control**
- Store the token securely as a GitHub secret or environment variable
- Use fine-grained PATs when possible for better security (principle of least privilege)
- Regularly rotate your tokens
- Revoke tokens that are no longer needed

## Troubleshooting

### Workflow approval fails with 403 error
- Verify the `COPILOT_MCP_GITHUB_TOKEN` secret is configured
- Check that the token has the correct permissions (see Token Requirements above)
- Ensure the token hasn't expired
- Verify the repository access if using a fine-grained PAT

### MCP server cannot authenticate
- Verify the `COPILOT_MCP_GITHUB_TOKEN` environment variable is set
- Check that the token is valid and not expired
- Ensure Node.js and npm are properly installed
- Try reinstalling the MCP GitHub server package

### Workflow runs are not being approved
- Check that the actor is in the trusted bots list (see workflow file)
- Verify the workflow is triggered by a `pull_request` or `push` event
- Review the workflow run logs in the Actions tab for detailed error messages
- Ensure the workflow names in the trigger match exactly

## Documentation

For more information, see:
- [GitHub Workflows Documentation](../../docs/GITHUB_WORKFLOWS.md) - Complete workflow documentation including auto-approve workflow
- [GitHub PAT Documentation](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token)
- [Model Context Protocol](https://modelcontextprotocol.io/) - MCP specification and documentation
- [GitHub Actions Documentation](https://docs.github.com/en/actions) - GitHub Actions reference
