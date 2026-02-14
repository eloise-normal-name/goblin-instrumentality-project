# Task: Add Metrics Report Generation Workflow

**Status**: Pending  
**Priority**: Medium  
**Estimated Effort**: Medium-Large  
**Related PR**: #2

## Summary

Create a GitHub Actions workflow to automatically generate and update the project metrics report (`refactor-highlights.html`). This provides automatic tracking of code size, build metrics, and project statistics.

## Background

This workflow was originally planned as part of PR #2 but was removed to simplify the PR scope. The workflow should be re-implemented in a separate PR to provide automated metrics tracking.

The project maintains a `refactor-highlights.html` file that showcases project metrics including:
- Binary sizes across configurations (Debug, Release, MinSizeRel)
- Source line counts by language
- Build configuration details
- Project statistics and trends

Currently, this file is manually updated. This workflow would automate the process.

## Acceptance Criteria

### Required
- [ ] Create `.github/workflows/generate-highlights.yml` workflow file
- [ ] Generate metrics report with current statistics
- [ ] Update `refactor-highlights.html` with latest metrics
- [ ] Include binary size tracking (Debug, Release, MinSizeRel)
- [ ] Include source line counts using cloc or similar tool
- [ ] Include build configuration details
- [ ] Workflow runs on appropriate trigger (push to main, schedule, or manual)

### Automation
- [ ] Commit updated metrics back to repository automatically
- [ ] Use bot credentials for automated commits
- [ ] Include meaningful commit message with metrics summary
- [ ] Prevent workflow from triggering itself (avoid infinite loop)

### Documentation
- [ ] Update `docs/GITHUB_WORKFLOWS.md` with generate-highlights workflow section
- [ ] Update `.github/copilot-instructions.md` to reference metrics generation
- [ ] Document how to trigger workflow manually
- [ ] Include troubleshooting guide

### Testing
- [ ] Verify workflow runs after PR merge to main
- [ ] Confirm metrics report is updated with accurate data
- [ ] Validate HTML formatting and structure
- [ ] Check automated commit attribution
- [ ] Test manual trigger if available

## Technical Details

### Workflow File Location
`.github/workflows/generate-highlights.yml`

### Trigger Strategy

**Option 1: On Push to Main** (Recommended)
```yaml
on:
  push:
    branches: [main]
```
Pros: Metrics stay current with each merge  
Cons: Creates commit for every merge

**Option 2: Scheduled**
```yaml
on:
  schedule:
    - cron: '0 0 * * 0'  # Weekly on Sunday
```
Pros: Cleaner commit history  
Cons: Metrics may be outdated

**Option 3: Manual Trigger**
```yaml
on:
  workflow_dispatch:
```
Pros: Full control  
Cons: Requires manual action

**Recommended**: Combination of push to main + manual trigger

### Metrics to Collect

#### 1. Binary Sizes
- **Source**: Build artifacts from build-and-validate workflow
- **Files**: 
  - `build/Debug/goblin-instrumentality-project.exe`
  - `build/Release/goblin-instrumentality-project.exe`
  - `build/MinSizeRel/goblin-instrumentality-project.exe`
- **Format**: Display in KB and percentage difference from baseline

#### 2. Source Line Counts
- **Tool**: `cloc` (Count Lines of Code)
- **Targets**: 
  - C++ source files (`src/*.cpp`, `src/*.ixx`)
  - C++ headers (`src/*.h`, `include/*.h`)
  - Separate count for vendor code vs project code
- **Format**: Total lines, code lines, comment lines, blank lines

#### 3. File Counts
- **Categories**:
  - Total source files
  - Total header files
  - Total module files (.ixx)
  - Workflow files

#### 4. Build Information
- **Details**:
  - Compiler version (MSVC)
  - CMake version
  - Last build timestamp
  - Last update timestamp

### HTML Report Structure

The `refactor-highlights.html` file should include:
- Header with project name and last update timestamp
- Summary section with key metrics
- Detailed sections for each metric category
- Charts/graphs (if applicable)
- Historical trend data (optional future enhancement)

### Workflow Implementation Strategy

```yaml
name: Generate Metrics Report

on:
  push:
    branches: [main]
  workflow_dispatch:

jobs:
  generate-metrics:
    runs-on: windows-latest
    
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0  # Need history for trends
      
      - name: Setup CMake and MSVC
        # Same setup as build-and-validate
      
      - name: Build all configurations
        # Build Debug, Release, MinSizeRel
      
      - name: Collect binary sizes
        shell: pwsh
        run: |
          $sizes = @{}
          foreach ($config in @('Debug', 'Release', 'MinSizeRel')) {
            $exe = "build/$config/goblin-instrumentality-project.exe"
            if (Test-Path $exe) {
              $sizes[$config] = (Get-Item $exe).Length
            }
          }
          # Export to JSON or environment variables
      
      - name: Install cloc
        shell: pwsh
        run: choco install cloc -y
      
      - name: Count source lines
        shell: pwsh
        run: |
          $stats = cloc src include --json
          # Parse and format results
      
      - name: Generate HTML report
        shell: pwsh
        run: |
          # Read template or existing HTML
          # Update with new metrics
          # Write to refactor-highlights.html
      
      - name: Commit changes
        shell: pwsh
        run: |
          git config user.name "github-actions[bot]"
          git config user.email "github-actions[bot]@users.noreply.github.com"
          git add refactor-highlights.html
          git diff --staged --quiet || git commit -m "Update metrics report [skip ci]"
          git push
```

### Preventing Infinite Loops

**Important**: The commit message must include `[skip ci]` to prevent triggering workflows on the automated commit.

Alternative: Use a different actor for the commit and exclude that actor from workflow triggers:
```yaml
on:
  push:
    branches: [main]
    paths-ignore:
      - 'refactor-highlights.html'
```

## Implementation References

### Original Planning
- **PR**: #2 mentioned this as a "Future Enhancement"
- **Related Workflow**: `build-and-validate.yml` for build metrics collection

### Similar Examples
- Look at `build-and-validate.yml` for build process
- Reference `refactor-highlights.html` for current structure

## Related Files

- `.github/workflows/generate-highlights.yml` (to be created)
- `refactor-highlights.html` (to be updated automatically)
- `.github/workflows/build-and-validate.yml` (reference for build process)
- `docs/GITHUB_WORKFLOWS.md` (to be updated)
- `.github/copilot-instructions.md` (to be updated)

## Dependencies

- CMake and MSVC (for building)
- cloc (for line counting) - installed via Chocolatey
- PowerShell (for scripting)
- Git (for committing changes)
- Write permissions to repository

## Permissions Required

The workflow needs write permissions to commit changes:
```yaml
permissions:
  contents: write
```

## Success Metrics

- Metrics automatically update after merges
- HTML report is valid and well-formatted
- All metrics are accurate
- Workflow execution time < 10 minutes
- No infinite loop triggers
- Clean commit history

## Potential Issues & Solutions

### Issue: Build Time Too Long
**Solution**: Consider downloading artifacts from build-and-validate workflow instead of rebuilding

### Issue: Merge Conflicts
**Solution**: Keep HTML structure simple, update only specific sections with clear delimiters

### Issue: Workflow Permissions
**Solution**: Ensure repository settings allow GitHub Actions to create commits

### Issue: Large Commit History
**Solution**: Consider squashing metrics commits periodically or using a separate branch

## Alternatives Considered

### Alternative 1: Separate Metrics Branch
Store metrics in a `gh-pages` or `metrics` branch instead of main.  
**Pros**: Cleaner main branch history  
**Cons**: More complex setup, metrics not visible in main

### Alternative 2: GitHub Releases
Attach metrics report to each release.  
**Pros**: Tied to versions  
**Cons**: Only updates on releases, not continuous

### Alternative 3: External Service
Use a third-party metrics service.  
**Pros**: Professional features, historical tracking  
**Cons**: Dependency on external service, may cost money

**Decision**: Stick with in-repo HTML for simplicity and self-contained project

## Follow-up Tasks

After this workflow is implemented:
- Add historical trend charts (compare to previous versions)
- Consider GitHub Pages deployment for better visualization
- Add more detailed metrics (test coverage when available)
- Create dashboard view with multiple metrics pages

## Advanced Features (Future)

- **Trend Analysis**: Track metrics over time, show graphs
- **Threshold Alerts**: Fail if binary size increases beyond threshold
- **Comparison**: Compare to previous version or baseline
- **Export**: Generate JSON/CSV for external analysis
- **Performance Metrics**: Include runtime benchmarks if tests exist

## Notes

- This is the most complex of the three workflows from PR #2
- Requires careful testing to avoid commit loops
- Consider implementing after format-check and code-quality are stable
- May want to create template/generator script for HTML to keep workflow clean
- Should be resilient to build failures (don't fail if one config doesn't build)

## Labels

`workflow`, `automation`, `metrics`, `reporting`, `enhancement`
