# Task: Add Code Formatting Validation Workflow

**Status**: Pending  
**Priority**: High  
**Estimated Effort**: Small  
**Related PR**: #2

## Summary

Create a GitHub Actions workflow to automatically validate code formatting using clang-format on all pull requests. This ensures consistent code style across the project and catches formatting violations early in the development process.

## Background

This workflow was originally implemented as part of PR #2 (commit `68bda0194b5b25a56979c838ec3af73e3e3e45c8`) but was removed to simplify the PR scope. The workflow should be re-implemented in a separate PR to focus on code formatting validation specifically.

PR #2 initially included multiple workflows:
- ✅ build-and-validate.yml (merged)
- ⏳ format-check.yml (this task)
- ⏳ code-quality.yml (separate task)
- ⏳ generate-highlights.yml (separate task)

## Acceptance Criteria

### Required
- [ ] Create `.github/workflows/format-check.yml` workflow file
- [ ] Workflow validates formatting for all C++ source files in `src/` and `include/` directories
- [ ] Uses project's `.clang-format` configuration file explicitly (`-style=file`)
- [ ] Workflow runs automatically on pull requests
- [ ] Provides clear error messages indicating which files need formatting

### Documentation
- [ ] Update `docs/GITHUB_WORKFLOWS.md` with format-check workflow section
- [ ] Update `.github/copilot-instructions.md` to reference format checks in CI section
- [ ] Include troubleshooting guide for format check failures

### Testing
- [ ] Verify workflow runs on PR creation
- [ ] Test with intentionally misformatted code to ensure detection
- [ ] Confirm workflow passes with properly formatted code
- [ ] Validate error messages are clear and actionable

## Technical Details

### Workflow File Location
`.github/workflows/format-check.yml`

### Key Requirements
1. **Explicit Style File**: Use `clang-format -style=file` to ensure the repository's `.clang-format` is used
2. **Verification**: Check that `.clang-format` exists before running validation
3. **Dry Run**: Use `--dry-run --Werror` to check without modifying files
4. **Debugging**: Display first 5 lines of `.clang-format` in workflow logs

### Files to Check
- `src/**/*.cpp`
- `src/**/*.ixx`
- `src/**/*.h`
- `include/**/*.h`
- `include/**/*.hpp`

### Exclusions
- Binary files
- Build artifacts (`build/`, `out/`)
- Third-party vendor files (if applicable)

### Error Reporting
- List all files that need formatting
- Show diff preview for easy identification
- Provide command to fix locally: `clang-format -i -style=file <file>`

## Implementation References

### Original Implementation
- **Commit**: `68bda0194b5b25a56979c838ec3af73e3e3e45c8`
- **Improvements**: `4e4e902c11b2f6231ba7dbdb01f94fe2cb1f6325`
- **PR**: https://github.com/eloise-normal-name/goblin-instrumentality-project/pull/2

### Example Workflow Structure
```yaml
name: Format Check
on:
  pull_request:
    paths:
      - 'src/**'
      - 'include/**'
      - '.clang-format'

jobs:
  format-check:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Verify .clang-format exists
        shell: pwsh
        run: |
          if (-not (Test-Path .clang-format)) {
            Write-Error ".clang-format not found"
            exit 1
          }
          
      - name: Show .clang-format config
        shell: pwsh
        run: Get-Content .clang-format -Head 5
      
      - name: Check formatting
        shell: pwsh
        run: |
          $files = Get-ChildItem -Recurse -Include *.cpp,*.ixx,*.h,*.hpp -Path src,include
          $issues = @()
          foreach ($file in $files) {
            $result = clang-format -style=file --dry-run --Werror $file.FullName 2>&1
            if ($LASTEXITCODE -ne 0) {
              $issues += $file.FullName
            }
          }
          if ($issues.Count -gt 0) {
            Write-Error "Files need formatting:"
            $issues | ForEach-Object { Write-Error "  $_" }
            exit 1
          }
```

## Related Files

- `.github/workflows/format-check.yml` (to be created)
- `.clang-format` (existing configuration)
- `docs/GITHUB_WORKFLOWS.md` (to be updated)
- `.github/copilot-instructions.md` (to be updated)

## Dependencies

- clang-format (available in GitHub Actions windows-latest runner)
- Project's `.clang-format` configuration file

## Success Metrics

- Workflow successfully detects formatting violations
- Zero false positives
- Clear, actionable error messages
- Fast execution time (< 2 minutes)
- Comprehensive documentation

## Follow-up Tasks

After this workflow is implemented:
- Monitor for false positives in first few PRs
- Consider adding auto-fix option (create commit with formatting fixes)
- Evaluate pre-commit hooks for local development

## Notes

- The workflow should match the patterns established in `build-and-validate.yml`
- Use PowerShell for consistency with other workflows
- Consider caching clang-format binary if execution time becomes an issue
- May need multiple passes of clang-format for complex files (see nvEncodeAPI.h history)

## Labels

`workflow`, `automation`, `code-quality`, `formatting`
