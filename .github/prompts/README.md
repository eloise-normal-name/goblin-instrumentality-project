# Custom Agent Usage Guide

This directory contains specialized custom agents designed for the Goblin Instrumentality Project. These agents are experts in specific aspects of the codebase and can help with various development tasks.

**[📋 View All Copilot-Assigned Issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)** - See issues currently assigned to copilot agents by BugBot

## Available Agents

### Bug Triage & Orchestration

#### BugBot (`bugbot.agent.md`)
**When to use:**
- Manually trigger with `@clp /agent bugbot Analyze and triage new bugs`
- When you need intelligent routing of complex issues

**What it does:**
- Queries open bugs from GitHub Issues
- Classifies bugs by component and severity
- Routes issues to appropriate specialized agents:
  - Memory/RAII issues → `check-raii`
  - Error handling/crashes → `review-error-handling`
  - D3D12 frame/command issues → `review-frame-logic`
  - GPU resource problems → `debug-resources`
  - NVENC integration issues → `explain-nvenc`
- Updates issue labels with component and severity
- Creates detailed assignment comments with analysis context

**Automation Status:**
GitHub Actions workflows are temporarily removed and will be refactored back in eventually. Use manual BugBot invocation until automation returns.

**Manual Usage:**
```
@clp /agent bugbot Analyze issue #42 and determine which agent should investigate
@clp /agent bugbot Create triage assignments for all open bugs in the graphics component
```

---

### Code Quality & Standards

#### Check RAII (`check-raii.prompt.md`)
**When to use:**
- After adding new classes that manage resources (GPU objects, HANDLEs, COM interfaces)
- Before committing code that allocates memory or system resources
- When refactoring resource management code
- During code review to verify proper cleanup

**Common scenarios:**
- Created a new D3D12 resource wrapper class
- Added Windows API HANDLE management
- Implemented COM interface lifetime handling
- Suspicious of resource leaks or double-frees

**Example usage:**
```
@workspace /agent check-raii Check D3D12CommandAllocators class for RAII compliance
```

#### Review Error Handling (`review-error-handling.prompt.md`)
**When to use:**
- After adding new D3D12 or NVENC API calls
- When debugging crashes or unexpected behavior
- Before submitting PR with API integration code
- Reviewing existing error handling patterns

**Common scenarios:**
- Added new encoder initialization code
- Implemented graphics pipeline setup
- Integrated new NVENC features
- Seeing silent failures in production

**Example usage:**
```
@workspace /agent review-error-handling Check all HRESULT returns in nvenc_session.cpp
```

### D3D12 & Graphics

#### Review Frame Logic (`review-frame-logic.prompt.md`)
**When to use:**
- After modifying frame submission or present logic
- When experiencing frame drops or stuttering
- Debugging synchronization issues between CPU and GPU
- Optimizing rendering performance

**Common scenarios:**
- Changed triple-buffering implementation
- Modified fence wait patterns
- Updated resource barrier placement
- Investigating GPU stalls

**Example usage:**
```
@workspace /agent review-frame-logic Analyze synchronization in App::RunFrame
```

#### Debug D3D12 Resources (`debug-resources.prompt.md`)
**When to use:**
- Investigating D3D12 debug layer warnings or errors
- Resource state mismatch errors
- Descriptor heap corruption issues
- Texture format or usage problems

**Common scenarios:**
- Debug layer reports invalid resource state
- Render target views not displaying correctly
- Resource barrier validation failures
- Descriptor heap exhaustion

**Example usage:**
```
@workspace /agent debug-resources Trace resource states in render target creation
```

### NVENC Integration

#### Explain NVENC Usage (`explain-nvenc.prompt.md`)
**When to use:**
- Learning NVENC API patterns
- Understanding resource registration flow
- Debugging encoding pipeline issues
- Reviewing D3D12-NVENC integration

**Common scenarios:**
- First time working with NVENC APIs
- Unclear about registration vs. mapping lifecycle
- Setting up new encode sessions
- Investigating encoding artifacts or errors

**Example usage:**
```
@workspace /agent explain-nvenc Explain the resource registration and mapping flow in frame_encoder.cpp (RegisterTexture and MapInputTexture)
```

### Performance Profiling

#### Profile Performance (`profile-performance.prompt.md`)
**When to use:**
- After significant code changes to the encode loop, frame submission, or I/O path
- Before merging a performance-sensitive PR
- When investigating CPU time, memory growth, or unexpected file write volume
- Establishing or updating the stored baseline in `docs/perf-baselines/`

**What it does:**
- Locates `VSDiagnostics.exe` in the local VS 2026 install
- Builds the RelWithDbgInfo binary via `cmake --build`
- Runs three separate VSDiagnostics sessions (CPU, memory, file I/O) against the `--headless` 30-frame workload
- Compares new measurements against the latest baseline JSON in `docs/perf-baselines/`
- Flags CPU wall time regression (>10%), memory regression (>5%), and any new file write calls
- Routes regressions to `review-frame-logic`, `check-raii`, or `debug-resources` as appropriate
- Writes a new dated baseline JSON file for future comparisons
- Regenerates `perf-highlights.html` for publishing findings to `gh-pages`

**Reference snippet**: `.github/prompts/snippets/vsdiagnostics-guide.md`

**Example usage:**
```
@workspace /agent profile-performance all post-encoder-refactor
@workspace /agent profile-performance cpu only, label before-merge
@workspace /agent profile-performance Has memory usage regressed since last baseline?
```

---

### Refactoring & Maintenance

#### Refactor Extract (`refactor-extract.prompt.md`)
**When to use:**
- Classes growing too large (>300 lines)
- Clear cohesive functionality can be separated
- Planning major code reorganization
- Improving code modularity

**Common scenarios:**
- App class has too many responsibilities
- Extracting swap chain management from device code
- Separating encoder configuration from session
- Creating reusable utility classes

**Example usage:**
```
@workspace /agent refactor-extract Suggest extracting descriptor heap management from D3D12Device
```

#### Stage Changelist (`stage-changelist.prompt.md`)
**When to use:**
- Ready to commit after significant refactoring
- Need to review and organize multiple changed files
- Want to generate formatted change summary
- Creating release highlights

**Common scenarios:**
- Completed feature implementation with many file changes
- Major refactoring across multiple modules
- Ready to create PR with comprehensive summary
- Generating project status report

**Example usage:**
```
@workspace /agent stage-changelist Review all changes since last commit and prepare staging
```

## Workflow Recommendations

### Bug Triage (Manual, Temporary)
BugBot can be run manually:
1. Queries all open issues labeled `bug`
2. Categorizes each bug by component and severity
3. Routes to appropriate specialized agent
4. Updates issue labels and assignments
5. Creates comment with analysis request

**If you open a bug report:**
- Label it `bug`
- Provide clear reproduction steps
- Note the affected component (graphics, encoder, nvenc, app)
- BugBot will automatically route to the right agent

**To manually triage a complex issue:**
```
@clp /agent bugbot Analyze and categorize issue #42 for routing
```

### New Feature Development
1. Start: Use `explain-nvenc` or similar to understand existing patterns
2. Implement: Follow project conventions from copilot-instructions.md
3. Validate: Run `check-raii` and `review-error-handling` on new code
4. Optimize: Use `review-frame-logic` if touching rendering path
5. Finalize: Use `stage-changelist` to organize and commit

### Bug Investigation
1. Identify: Use `debug-resources` for D3D12 issues
2. Analyze: Use `review-frame-logic` for sync problems
3. Trace: Use `review-error-handling` to verify all paths checked
4. Fix: Make minimal changes following project patterns
5. Verify: Re-run relevant agents on fixed code

### Code Review Preparation
1. Self-review: Run `check-raii` on all resource-managing classes
2. Error check: Run `review-error-handling` on API-heavy files
3. Performance: Run `review-frame-logic` if frame code changed
4. Documentation: Ensure code is self-documenting per project standards
5. Stage: Use `stage-changelist` to prepare comprehensive PR

### Performance Profiling
1. Baseline: Run `/agent profiler` or `profile-performance` to capture CPU/memory/file I/O baseline
2. Change: Make the target code change
3. Measure: Re-run `profile-performance` with a descriptive label
4. Compare: Agent auto-diffs against prior baseline and flags regressions
5. Fix: Route CPU regressions to `review-frame-logic`; memory to `check-raii`/`debug-resources`
6. Verify: Re-run profiling to confirm regression resolved

### Refactoring Sessions
1. Plan: Use `refactor-extract` to identify extraction opportunities
2. Extract: Create new classes following RAII patterns
3. Validate: Run `check-raii` on newly extracted classes
4. Verify: Run `review-error-handling` to ensure no broken error paths
5. Review: Use `review-frame-logic` if frame code was touched
6. Stage: Use `stage-changelist` to document refactoring

## Best Practices

### Agent Usage Tips
- **Be specific**: Provide file names and line ranges when relevant
- **Chain agents**: Use multiple agents sequentially for comprehensive review
- **Iterate**: Re-run agents after addressing feedback
- **Context matters**: Explain what you changed or what problem you're investigating

### Common Combinations
- **BugBot** → `check-raii` / `review-error-handling` / `review-frame-logic`: Automated bug routing and analysis
- `check-raii` + `review-error-handling`: New class implementation
- `review-frame-logic` + `debug-resources`: Rendering issues
- `explain-nvenc` + `review-error-handling`: Encoder integration
- `refactor-extract` + `check-raii`: Code reorganization
- `profile-performance` → `review-frame-logic`: CPU regression investigation
- `profile-performance` → `check-raii` + `debug-resources`: Memory regression investigation
- **Profiler agent** → `review-frame-logic` / `check-raii`: Automated regression routing after measurement

### Anti-patterns to Avoid
- Running agents without specific questions or focus areas
- Ignoring agent feedback without understanding it
- Using agents as a substitute for reading documentation
- Not verifying agent suggestions match project conventions

## Integration with Development Workflow

### VS Code Integration
Custom agents can be invoked through GitHub Copilot Chat:
1. Open Copilot Chat panel (Ctrl+Alt+I)
2. Type `@workspace /agent <agent-name> <your question>`
3. Review suggestions and apply fixes
4. Re-run agent to verify fixes

### Pre-commit Checklist
Before committing code touching:
- Resource management → Run `check-raii`
- D3D12/NVENC APIs → Run `review-error-handling`
- Frame submission → Run `review-frame-logic`
- Multiple files → Consider `stage-changelist`

### Code Review Checklist
When reviewing PRs:
- Large changes → Suggest `refactor-extract` for complex additions
- Resource handling → Verify with `check-raii`
- Error handling → Cross-check with `review-error-handling`
- Frame logic changes → Validate with `review-frame-logic`

## Additional Resources

- Project conventions: `.github/copilot-instructions.md`
- NVENC guide: `.github/prompts/snippets/nvenc-guide.md`
- VSDiagnostics CLI guide: `.github/prompts/snippets/vsdiagnostics-guide.md`
- Performance baselines: `docs/perf-baselines/`
- Known errors: `docs/copilot-known-errors.md`

## Contributing New Agents

If you identify a recurring pattern or specialized knowledge area that would benefit from a custom agent:

1. Create a new `.prompt.md` file in this directory
2. Follow the existing format (frontmatter + instructions)
3. Include specific project context and examples
4. Test with real scenarios from the codebase
5. Update this README with usage suggestions
