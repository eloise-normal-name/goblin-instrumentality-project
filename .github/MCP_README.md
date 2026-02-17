# MCP Configuration for GitHub Automation

This directory contains an MCP (Model Context Protocol) configuration file that can be used for GitHub API automation tasks.

## Files

- **`mcp-config.json`**: MCP server configuration for GitHub API integration with the `COPILOT_MCP_GITHUB_TOKEN` Personal Access Token

## Usage

### MCP Configuration

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
- Querying repository information
- Managing issues and pull requests
- Searching and listing repository data
- And more...

## Token Requirements

Regardless of which option you choose, the `COPILOT_MCP_GITHUB_TOKEN` must have appropriate permissions:

### Classic Personal Access Token
- Required scope: `repo` (Full control of private repositories)
  - Includes: `repo:status`, `repo_deployment`, `public_repo`, `repo:invite`, `security_events`

### Fine-grained Personal Access Token
- Required permissions:
   - **Contents**: Read access (for repository information)
   - **Issues**: Read and write access
   - **Pull requests**: Read and write access
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
   - Token name: "GitHub Workflow Auto-Approve Token"
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

### MCP server cannot authenticate
- Verify the `COPILOT_MCP_GITHUB_TOKEN` environment variable is set
- Check that the token is valid and not expired
- Ensure Node.js and npm are properly installed
- Try reinstalling the MCP GitHub server package

## Documentation

For more information, see:
- [GitHub PAT Documentation](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token)
- [Model Context Protocol](https://modelcontextprotocol.io/) - MCP specification and documentation

**Note**: GitHub Actions workflows are temporarily removed and will be refactored back in eventually.
