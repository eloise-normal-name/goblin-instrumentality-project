# Remaining Workflow Tasks from PR #2

This document tracks the unfinished workflow automation work that was originally planned in PR #2 but split into separate tasks for easier review and implementation.

## Background

PR #2 (https://github.com/eloise-normal-name/goblin-instrumentality-project/pull/2) initially included multiple GitHub workflow automations but was scoped down to only include the `build-and-validate.yml` workflow. The following workflows were removed and need to be implemented in separate PRs:

- Code formatting validation (clang-format)
- Code quality checks (RAII, casts, namespaces, error handling)
- Metrics report generation

Each workflow was originally implemented in commit `68bda0194b5b25a56979c838ec3af73e3e3e45c8` and should be referenced when creating these tasks.

---

## Task 1: Add Code Formatting Validation Workflow

### Description
Create a GitHub Actions workflow to automatically validate code formatting using clang-format on all pull requests. This ensures consistent code style across the project and catches formatting violations early in the development process.

### Acceptance Criteria
- [ ] Create `.github/workflows/format-check.yml` workflow file
- [ ] Workflow validates formatting for all C++ source files in `src/` and `include/` directories
- [ ] Uses project's `.clang-format` configuration file explicitly (`-style=file`)
- [ ] Workflow runs automatically on pull requests
- [ ] Provides clear error messages indicating which files need formatting
- [ ] Excludes binary files and build artifacts
- [ ] Updates `docs/GITHUB_WORKFLOWS.md` with format-check documentation
- [ ] Updates `.github/copilot-instructions.md` to reference format checks in CI section

### Implementation Notes
- Reference commit `68bda0194b5b25a56979c838ec3af73e3e3e45c8` for initial implementation
- Reference commit `4e4e902c11b2f6231ba7dbdb01f94fe2cb1f6325` for improvements to ensure correct .clang-format usage
- The workflow should verify `.clang-format` exists before running checks
- Use `clang-format --dry-run --Werror` to check formatting without modifying files
- Display first 5 lines of `.clang-format` in logs for debugging

### Related Files
- `.github/workflows/format-check.yml` (to be created)
- `.clang-format` (existing configuration)
- `docs/GITHUB_WORKFLOWS.md` (to be updated)
- `.github/copilot-instructions.md` (to be updated)

### Testing
- Verify workflow runs on PR creation
- Test with intentionally misformatted code to ensure detection
- Confirm workflow passes with properly formatted code
- Validate error messages are clear and actionable

---

## Task 2: Add Code Quality Checks Workflow

### Description
Create a GitHub Actions workflow to enforce project-specific code quality standards including RAII patterns, prohibited C++ casts, namespace usage, and error handling patterns. This automates checks that would otherwise require manual code review.

### Acceptance Criteria
- [ ] Create `.github/workflows/code-quality.yml` workflow file
- [ ] Check for prohibited Initialize/Shutdown methods (RAII violation)
- [ ] Check for prohibited C++ casts (`static_cast`, `const_cast`, `dynamic_cast`, `reinterpret_cast`)
- [ ] Check for prohibited namespace usage (project uses global namespace)
- [ ] Check for prohibited trailing underscores in variable names
- [ ] Warn about unchecked D3D12 and NVENC API calls (should use `Try |` pattern)
- [ ] Workflow runs automatically on pull requests
- [ ] Provides clear error messages with file locations and line numbers
- [ ] Only checks C++ source files (`src/*.cpp`, `src/*.ixx`)
- [ ] Updates `docs/GITHUB_WORKFLOWS.md` with code-quality documentation
- [ ] Updates `.github/copilot-instructions.md` to reference code quality checks

### Implementation Notes
- Reference commit `68bda0194b5b25a56979c838ec3af73e3e3e45c8` for initial implementation
- Use PowerShell Select-String for pattern matching
- Search patterns:
  - RAII violations: `\bInitialize\(|\bShutdown\(`
  - C++ casts: `static_cast<|const_cast<|dynamic_cast<|reinterpret_cast<`
  - Namespaces: `^namespace\s+\w+|^using\s+namespace\s+`
  - Trailing underscores: `\b\w+_\s*[;=]`
  - Unchecked calls: `\b(CreateDevice|CreateCommandQueue|CreateSwapChain|NvEncCreateInstance)\(` (warnings only)
- Exclude vendor files if needed
- Return non-zero exit code if violations found

### Related Files
- `.github/workflows/code-quality.yml` (to be created)
- `src/*.cpp` and `src/*.ixx` (files to check)
- `docs/GITHUB_WORKFLOWS.md` (to be updated)
- `.github/copilot-instructions.md` (to be updated)

### Testing
- Verify workflow runs on PR creation
- Test with code that violates each rule
- Confirm clear error reporting with file/line information
- Validate warnings for unchecked API calls

---

## Task 3: Add Metrics Report Generation Workflow

### Description
Create a GitHub Actions workflow to automatically generate and update the project metrics report (refactor-highlights.html). This provides automatic tracking of code size, build metrics, and project statistics.

### Acceptance Criteria
- [ ] Create `.github/workflows/generate-highlights.yml` workflow file
- [ ] Generate metrics report with current statistics
- [ ] Update `refactor-highlights.html` with latest metrics
- [ ] Include binary size tracking (Debug, Release, MinSizeRel)
- [ ] Include source line counts using cloc
- [ ] Include build configuration details
- [ ] Workflow runs on pushes to main branch (after merges)
- [ ] Commit updated metrics back to repository
- [ ] Updates `docs/GITHUB_WORKFLOWS.md` with generate-highlights documentation
- [ ] Updates `.github/copilot-instructions.md` to reference metrics generation

### Implementation Notes
- Reference commit `68bda0194b5b25a56979c838ec3af73e3e3e45c8` for initial implementation
- May need to extract data from build-and-validate.yml artifacts
- Consider whether this should run on schedule or on-demand
- Requires write permissions to commit changes back to repository
- Should use automated commit with bot credentials

### Related Files
- `.github/workflows/generate-highlights.yml` (to be created)
- `refactor-highlights.html` (to be updated automatically)
- `docs/GITHUB_WORKFLOWS.md` (to be updated)
- `.github/copilot-instructions.md` (to be updated)

### Testing
- Verify workflow runs after PR merge
- Confirm metrics report is updated with accurate data
- Validate HTML formatting and structure
- Check automated commit attribution

---

## Implementation Order

Recommended order for implementing these workflows:

1. **Format Check** - Most straightforward, provides immediate value
2. **Code Quality** - More complex pattern matching, builds on format-check experience  
3. **Metrics Generation** - Most complex, requires repository write access

---

## Notes

- All workflows were originally created together but separated for easier review
- The `build-and-validate.yml` workflow is already implemented and merged via PR #2
- Each workflow should follow the same patterns established in `build-and-validate.yml`
- All workflows use Visual Studio 2022 in CI (GitHub Actions) while local development uses VS 2026
- Documentation should be comprehensive with troubleshooting guides for each workflow

---

## References

- PR #2: https://github.com/eloise-normal-name/goblin-instrumentality-project/pull/2
- Original implementation commit: `68bda0194b5b25a56979c838ec3af73e3e3e45c8`
- Format-check improvements: `4e4e902c11b2f6231ba7dbdb01f94fe2cb1f6325`
- Troubleshooting documentation: `1f91a42265ff9d5842fb97cc5ae5ddb8bc73841f`
- Existing workflow: `.github/workflows/build-and-validate.yml`
