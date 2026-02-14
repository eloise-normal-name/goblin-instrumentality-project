# Task: Add Code Quality Checks Workflow

**Status**: Pending  
**Priority**: High  
**Estimated Effort**: Medium  
**Related PR**: #2

## Summary

Create a GitHub Actions workflow to enforce project-specific code quality standards including RAII patterns, prohibited C++ casts, namespace usage, and error handling patterns. This automates checks that would otherwise require manual code review.

## Background

This workflow was originally implemented as part of PR #2 (commit `68bda0194b5b25a56979c838ec3af73e3e3e45c8`) but was removed to simplify the PR scope. The workflow should be re-implemented in a separate PR to focus on automated code quality enforcement.

The Goblin Instrumentality Project has specific coding standards that differ from typical C++ projects:
- **RAII-only**: No Initialize/Shutdown methods allowed (CI enforced)
- **C-style casts**: C++ casts (`static_cast`, etc.) are prohibited
- **Global namespace**: No namespace declarations allowed
- **No trailing underscores**: Member variables use plain snake_case
- **Error handling**: All D3D12/NVENC calls must use `Try |` pattern

## Acceptance Criteria

### Required
- [ ] Create `.github/workflows/code-quality.yml` workflow file
- [ ] Check for prohibited Initialize/Shutdown methods (RAII violation)
- [ ] Check for prohibited C++ casts (`static_cast`, `const_cast`, `dynamic_cast`, `reinterpret_cast`)
- [ ] Check for prohibited namespace usage
- [ ] Check for prohibited trailing underscores in variable names
- [ ] Warn about unchecked D3D12 and NVENC API calls (should use `Try |` pattern)
- [ ] Workflow runs automatically on pull requests

### Error Reporting
- [ ] Provide clear error messages with file locations and line numbers
- [ ] Differentiate between errors (fail build) and warnings (informational)
- [ ] Include code snippets showing violations
- [ ] Suggest fixes when applicable

### Documentation
- [ ] Update `docs/GITHUB_WORKFLOWS.md` with code-quality workflow section
- [ ] Update `.github/copilot-instructions.md` to reference code quality checks
- [ ] Include troubleshooting guide for each violation type
- [ ] Document how to run checks locally

### Testing
- [ ] Verify workflow runs on PR creation
- [ ] Test with code that violates each rule (RAII, casts, namespaces, underscores)
- [ ] Confirm clear error reporting with file/line information
- [ ] Validate warnings for unchecked API calls
- [ ] Ensure clean code passes all checks

## Technical Details

### Workflow File Location
`.github/workflows/code-quality.yml`

### Files to Check
- `src/**/*.cpp` - C++ source files
- `src/**/*.ixx` - C++ module files

**Note**: Header files are not checked to avoid false positives in third-party code.

### Check Patterns

#### 1. RAII Violations (ERROR)
**Pattern**: `\bInitialize\(|\bShutdown\(`  
**Reason**: All resources must be managed in constructors/destructors  
**Message**: "RAII violation: Initialize/Shutdown methods are prohibited. Use constructors/destructors."

#### 2. C++ Casts (ERROR)
**Pattern**: `static_cast<|const_cast<|dynamic_cast<|reinterpret_cast<`  
**Reason**: Project uses C-style casts for simplicity  
**Message**: "C++ cast found. Use C-style casts: (Type)value"

#### 3. Namespaces (ERROR)
**Pattern**: `^namespace\s+\w+|^using\s+namespace\s+`  
**Reason**: Project keeps all code in global namespace  
**Message**: "Namespace usage prohibited. Keep code in global namespace."

#### 4. Trailing Underscores (ERROR)
**Pattern**: `\b\w+_\s*[;=]`  
**Reason**: Member variables use plain snake_case, no trailing underscores  
**Message**: "Trailing underscore found. Use plain snake_case for member variables."

#### 5. Unchecked API Calls (WARNING)
**Patterns**:
- D3D12: `\b(CreateDevice|CreateCommandQueue|CreateSwapChain|CreateCommandAllocator|CreateCommandList|CreateFence|CreateResource)\(`
- NVENC: `\b(NvEncCreateInstance|NvEncOpenEncodeSession|NvEncInitializeEncoder|NvEncCreateInputBuffer|NvEncCreateBitstreamBuffer)\(`
  
**Reason**: All API calls should use `Try |` error handling pattern  
**Message**: "API call may need error checking. Use Try | pattern from include/try.h"

### Implementation Strategy

Use PowerShell `Select-String` for pattern matching:

```powershell
# Example: Check for RAII violations
$files = Get-ChildItem -Recurse -Include *.cpp,*.ixx -Path src
$violations = $files | Select-String -Pattern '\bInitialize\(|\bShutdown\(' -AllMatches

if ($violations.Count -gt 0) {
    Write-Error "RAII violations found:"
    foreach ($v in $violations) {
        Write-Error "$($v.Path):$($v.LineNumber): $($v.Line.Trim())"
    }
    exit 1
}
```

### Workflow Structure

```yaml
name: Code Quality

on:
  pull_request:
    paths:
      - 'src/**/*.cpp'
      - 'src/**/*.ixx'

jobs:
  code-quality:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Check RAII violations
        shell: pwsh
        run: |
          # Check pattern and report violations
          
      - name: Check C++ casts
        shell: pwsh
        run: |
          # Check pattern and report violations
          
      - name: Check namespaces
        shell: pwsh
        run: |
          # Check pattern and report violations
          
      - name: Check trailing underscores
        shell: pwsh
        run: |
          # Check pattern and report violations
          
      - name: Warn about unchecked API calls
        shell: pwsh
        continue-on-error: true
        run: |
          # Check pattern and report warnings
```

## Implementation References

### Original Implementation
- **Commit**: `68bda0194b5b25a56979c838ec3af73e3e3e45c8`
- **PR**: https://github.com/eloise-normal-name/goblin-instrumentality-project/pull/2

### Related Code Standards
- **RAII**: `.github/copilot-instructions.md` - "Resource Management" section
- **Casts**: `.github/copilot-instructions.md` - "Casts" section
- **Error Handling**: `.github/copilot-instructions.md` - "Error Handling" section
- **Try Pattern**: `include/try.h` - Error handling macro

## Related Files

- `.github/workflows/code-quality.yml` (to be created)
- `src/**/*.cpp` and `src/**/*.ixx` (files to check)
- `docs/GITHUB_WORKFLOWS.md` (to be updated)
- `.github/copilot-instructions.md` (to be updated)
- `include/try.h` (error handling reference)

## Dependencies

- PowerShell (available in GitHub Actions windows-latest runner)
- No external tools required

## Success Metrics

- Workflow successfully detects all violation types
- Zero false positives
- Clear, actionable error messages with file/line numbers
- Fast execution time (< 1 minute)
- Warnings don't fail the build but are visible

## Potential Issues & Solutions

### Issue: False Positives in Comments
**Solution**: Add filter to exclude matches in comments
```powershell
$violations = $violations | Where-Object { 
    $line = $_.Line.Trim()
    -not ($line -match '^//') -and -not ($line -match '^\*') 
}
```

### Issue: String Literals with Pattern
**Solution**: May need more sophisticated regex to exclude strings
**Alternative**: Accept minor false positives and allow manual override via comment

### Issue: Third-party Code
**Solution**: Only check `src/` directory, skip `include/` where vendor headers live

## Follow-up Tasks

After this workflow is implemented:
- Monitor for false positives in first few PRs
- Refine regex patterns based on real-world usage
- Consider adding auto-fix suggestions for common violations
- Evaluate extending to header files with vendor exclusions

## Notes

- Warnings (unchecked API calls) should use `continue-on-error: true` to not fail builds
- Each check should be a separate workflow step for clear error reporting
- Consider adding a summary step that counts total violations
- Pattern matching should be case-sensitive to avoid false positives

## Labels

`workflow`, `automation`, `code-quality`, `standards-enforcement`
