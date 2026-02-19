# Goblin Instrumentality Project - Instructions

## 🚨 READ THIS FIRST 🚨
**[QUICK REFERENCE GUIDE](QUICK_REFERENCE.md)** - Scan this < 50 line guide before writing any code. Critical rules, examples, and CI enforcement status.

**NEVER COMMIT CODEQL ARTIFACTS** - Before using `codeql_checker`, read the full warning in the **"CRITICAL: DO NOT COMMIT CODEQL ARTIFACTS"** section below. Files like `_codeql_detected_source_root` are temporary scan artifacts and must never be committed. If you ignore this warning, include 🤡 in your commit message.

## Quick Links (Table of Contents)

- **[Quick Reference Guide](QUICK_REFERENCE.md)** - Critical coding rules (< 50 lines, scan first)

- Project overview: `README.md`
- **[Copilot-Assigned Issues](https://github.com/eloise-normal-name/goblin-instrumentality-project/issues?q=is%3Aissue+is%3Aopen+label%3Atriage%3Ain-progress)** - Issues triaged and assigned to copilot agents
- Known errors & fixes: `docs/copilot-known-errors.md`
- Bug triage system: `.github/BUG_TRIAGE_SYSTEM.md` (BugBot issue routing)
- Copilot setup validation: `.github/COPILOT_SETUP_VALIDATION.md`
- Custom agent guide: `.github/prompts/README.md`
- NVENC integration guide: `.github/prompts/snippets/nvenc-guide.md`
- Refactor highlights: `refactor-highlights.html` (published to `gh-pages` branch)

## Technology Stack

> Formal project name: Goblin Instrumentality Project (formerly Goblin Stream)
- **Language**: C++23 (latest standard)
- **Build System**: CMake 3.28+ with MSVC compiler (Visual Studio 2026 Community, aka version 18)
  - ⚠️ **VS 2022 is NOT installed on this machine.** Do not construct or guess paths containing `\2022\`. The install root is `C:\Program Files\Microsoft Visual Studio\18\Community\`.
  - Key tool paths (use these verbatim, do not guess):
    - `clang-format`: `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-format.exe`
    - `Launch-VsDevShell.ps1`: `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\Launch-VsDevShell.ps1`
    - `vcvars64.bat`: `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat`
- **Linking**: Static linking (`/MT` runtime)
- **Target Platform**: Windows x64
- **Subsystem**: Windows (no console window)

### Dependencies
- **Allowed**: C++ Standard Library and Windows APIs only
- **Prohibited**: External third-party libraries
- **Preference**: Use Windows API directly instead of STL equivalents when straightforward

### Code Quality Standards
- **Documentation**: Self-documenting code with meaningful names (NO comment-based documentation)
- **Naming**: 
  - Methods and public APIs use PascalCase (e.g., `WaitForGpu`, `BeginFrame`)
  - Local and member variables use snake_case without trailing underscores (e.g., `create_device`, `fence_event`)
  - Constants use CAPS_CASE (SCREAMING_SNAKE_CASE)
- **Resource Management**: Full RAII with constructor/destructor pairs (no Initialize/Shutdown methods; **✅ CI enforced**):
  - All resource allocation in constructors; all cleanup in destructors
  - No two-phase initialization; construction must fully succeed or throw
  - GPU resources receive config/device pointers in constructor parameters
  - HANDLEs, COM objects, DLL modules wrapped with proper RAII cleanup
  - **Do not use defaulted constructors (`= default`) on resource-owning types** unless the default state is valid
- **ComPtr Usage** (**❌ NOT CI-enforced**):
  - Use `ComPtr` for RAII ownership in members: `ComPtr<ID3D12Device> device;`
  - Pass raw pointers to functions: `void CreateBuffer(ID3D12Device* device)`
  - Avoids unnecessary AddRef/Release cycles and clarifies ownership intent
- **Command Lists**: Command lists and allocators must be generated once and reused efficiently (via Reset); do not recreate them every frame
- **Style**: All code must conform to `.clang-format` configuration (Tabs, 4-wide, 100-column limit)
  - When providing code in chat, format it as if `.clang-format` has been applied to avoid bad formatting being written
  - Do not format `include/nvenc/nvEncodeAPI.h` since it is a 3rd party vendor header
  - Single-statement conditionals (`if`, `for`, `while`) should omit braces
  - Multi-statement blocks require braces
  - **Formatting is enforced** on all changes; run the formatter before finalizing edits
- **Warnings**: Compile with `/W4` (validated by CI; treat all warnings as errors in future)
- **Type Deduction**: Prefer `auto` when it avoids writing the type (e.g., function returns, lambdas). Do not add `*` or `&` to `auto` declarations unless required for correctness. For struct initialization where you must write the type anyway, use explicit type: `Type var{.field = val};`
- **Smart Pointers**: Do NOT use `std::unique_ptr`, `std::shared_ptr`, or `std::make_unique` in this codebase. Use RAII with inline members or raw pointers managed in constructors/destructors instead.
- **Member Initialization** (**❌ NOT CI-enforced**):
  - Initialize members inline in class body: `Member member{...};` (keeps data and initialization together)
  - Constructor initializer list: Only for parameter wiring (`width(width)`) and simple constructor calls
  - **Do not refactor inline initializers into constructor lists** during unrelated changes
  - Example (✓): `NvencConfig nvenc_config{&nvenc_session, ENCODER_CONFIG};` in member declaration
  - Example (✗): Moving complex designated-initializer objects into constructor initializer list
- **Casts**: Use C-style casts `(Type)value` instead of C++ casts (`static_cast`, `const_cast`, `dynamic_cast`, `reinterpret_cast`); should rarely be necessary in this project (**CI enforced**)
- **Error Handling**: Use `Try |` pattern from `include/try.h` for D3D12 and NVENC API calls that return error statuses (**CI warns on unchecked calls**):
  - Chain consecutive error-checked operations with single `Try` and multiple `|` operators:
    ```cpp
    Try | function1()
        | function2()
        | function3();
    ```
  - Throws empty exception on first failure; all error codes must be checked
  - For HANDLE-returning calls (e.g., `GetFrameLatencyWaitableObject`), use direct assignment and null check instead:
    ```cpp
    handle = api_call();
    if (!handle)
        throw;
    ```
  - This pattern is preferred for brevity; do not wrap HANDLE validation in Try pattern or helpers
- **Struct Initialization** (**❌ NOT CI-enforced - CRITICAL**):
  - Use designated initializers (`.field = value`) at initialization time, **never initialize-then-assign**
  - ✓ Good: `Type var{.field = val, .other = val2};`
  - ✗ Bad: `Type var{}; var.field = val; var.other = val2;`
  - **Why**: Init-then-assign pattern caused NVENC crash (wrong pointer type assigned to struct field)
  - Set only required non-default fields; avoid verbose restatements of defaults
- **Style Discipline**:
  - Do not submit messy or verbose code; keep changes concise, follow repo conventions, and run clang-format before finalizing edits
- **Process Discipline**:
  - Before editing, re-check these instructions and ensure struct initialization and RAII rules are followed
- **Code Simplification**: Remove trivial wrapper functions and stubs when making changes
  - If a function becomes a simple pass-through or constant return after refactoring, inline it directly at call sites
  - Example: Remove `static Type Select(bool) { return Type::Value; }` and use `Type::Value` directly
  - Keep code minimal and direct; avoid unnecessary indirection

## Prohibited Patterns
These patterns are prohibited and should be avoided:
- **snake_case Methods**: All methods must use PascalCase, not snake_case (CI enforced)
- **C++ Casts**: Use C-style casts `(Type)value` instead of `static_cast`, `const_cast`, `dynamic_cast`, `reinterpret_cast` (CI enforced)
- **Namespaces**: Do not use namespaces; keep all code in the global namespace (CI enforced)
- **Trailing Underscores**: Do not use trailing underscores for member variables (use plain snake_case) (CI enforced)
- **Initialize/Shutdown Methods**: Violates RAII principles (CI enforced)
- **Threading**: No multi-threaded code or concurrency primitives
- **Comments**: Use self-documenting code instead of comments
- **External Libraries**: Stick to Windows APIs and C++ standard library
- **NVENC**: Follow the condensed D3D12-only guide at `.github/prompts/snippets/nvenc-guide.md` and the official programming guide at https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/nvenc-video-encoder-api-prog-guide/index.html.

## 🚨 CRITICAL: DO NOT COMMIT CODEQL ARTIFACTS 🚨

**⚠️ STOP AND READ THIS BEFORE RUNNING `codeql_checker` OR ANY SECURITY SCANNING TOOL ⚠️**

The `codeql_checker` tool creates temporary artifact files during security scanning. These files are **AUTOMATICALLY GENERATED** and must **NEVER** be committed to version control.

### CodeQL Artifacts (DO NOT COMMIT):
- `_codeql_detected_source_root` - Symlink marker file pointing to repository root
- `_codeql_build_dir/` - Directory containing CodeQL build artifacts
- Any file or directory starting with `_codeql`
- Any `.sarif` files (CodeQL scan results)

### Why This Matters:
These artifacts are **temporary scan markers** created by the CodeQL security scanner. They serve no purpose in version control and pollute the repository history. They are already listed in `.gitignore` and should remain untracked.

### Instructions for Agents:
1. **BEFORE** running `codeql_checker`: Verify `.gitignore` contains these entries
2. **AFTER** running `codeql_checker`: Run `git status` and ensure NO `_codeql*` or `*.sarif` files are staged
3. **NEVER** use `git add .` or `git add -A` without reviewing what files are being staged
4. **IF** you accidentally stage CodeQL artifacts: Use `git restore --staged <file>` to unstage them immediately

### Self-Awareness Test:
If you commit any CodeQL artifact to version control despite reading this warning, you must:
1. Include the clown emoji (🤡) in your commit message
2. Immediately revert the commit
3. Add an apology note explaining what you did wrong

**READ THIS SECTION AGAIN. THEN READ IT ONE MORE TIME. DO NOT COMMIT CODEQL ARTIFACTS.**

## Build Process
- **Generator**: Use `-G "Visual Studio 18 2026" -A x64` (local development)
  - "Visual Studio 18 2026" is the CMake generator name. Version 18 = VS 2026. Do not use `"Visual Studio 17 2022"` — VS 2022 is not installed.
- **Intermediate Files**: Located in `build/` (git-ignored)
- **Configure helper**: Prefer `powershell -ExecutionPolicy Bypass -File scripts/configure-cmake.ps1` for resilient configure with one automatic cache-reset retry.
- **Cache reset policy**: If configure errors mention stale cache, generator mismatch, or invalid source/build paths, clear `build/CMakeCache.txt` and `build/CMakeFiles/` (or run `scripts/configure-cmake.ps1 -ForceClean`) and reconfigure.
- **Automation note**: GitHub Actions workflows are temporarily removed and will be refactored back in eventually. Rely on local builds for validation.

## VS Code Workflow
- **Extension**: Use CMake Tools extension
- **Build**: F7 or Status Bar "Build" button
- **Run/Debug**: F5 or Ctrl+F5
  - Debug configurations available in `.vscode/launch.json`:
    - **Debug (Normal)**: Run Debug build with window visible
    - **Debug (Headless)**: Run Debug build with `--headless` flag (no window, exits after 30 frames)
    - **Release (Normal)**: Run Release build with window visible
    - **Release (Headless)**: Run Release build with `--headless` flag
- **Format**: Ctrl+Shift+I (format document on save is automatic; CI validates on PR)
- **Terminal**: The default terminal profile is **Developer PowerShell (VS 18)**, configured in the **user settings** (`%APPDATA%\Code\User\settings.json`). This profile runs `Launch-VsDevShell.ps1` on startup, which loads the full MSVC environment — `cmake`, `clang-format`, `cl.exe`, etc. are all available in PATH. Do not change this default; it is required for terminal-based builds and formatting.
- **Workspace Settings** (`.vscode/settings.json`): Intentionally minimal and **not tracked in git**. Do not add terminal profiles, environment config, or personal preferences here — those belong in user settings. Do not edit this file during development tasks.
- **Agent access to user settings**: Agents with write-capable file tools are allowed to read and edit `C:\Users\Admin\AppData\Roaming\Code\User\settings.json` for local VS Code environment setup (terminal profile, terminal cwd, editor/tool preferences). Prefer user settings for these changes instead of `.vscode/settings.json`.

## Command Syntax
- **Always use Windows/PowerShell native commands**, never Unix syntax
  - ✅ `Get-Item`, `dir`, `Get-ChildItem` (Windows/PowerShell)
  - ❌ `ls -la`, `ls -l` (Unix/Linux syntax)
  - ❌ `cat`, `grep`, `find`, `head`, `tail` (Unix tools — do not use `head` or `tail`)
- **Use quoted paths** when they contain spaces: `"C:\Program Files\..."`
- **CMake commands**: Always specify generator explicitly: `-G "Visual Studio 18 2026" -A x64`

## Agent Command Execution
For potentially long-running or resource-intensive commands, use the agent terminal wrapper (`scripts/agent-wrap.ps1`) for structured logging and timeout protection:

- **Wrapper syntax**: `powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "<your-command>" -TimeoutSec <seconds>`
- **Returns**: Single JSON line with command status, timing, exit code, log paths, and output preview
- **Logs**: Saved to `.agent/logs/<run-id>/` with separate stdout, stderr, and combined logs

**When to use the wrapper:**
- ✅ Build commands (`cmake --build`) — MSVC env is available because the default terminal is Developer PowerShell (VS 18); however **prefer `Build_CMakeTools` for builds** as it provides better IDE integration
- ✅ Test runs (`ctest`)
- ✅ Code analysis tools (CodeQL, `clang-format`) — `clang-format` is in PATH via Developer PowerShell
- ✅ Application test runs (`./bin/Debug/goblin-stream.exe --headless`)
- ✅ Any command that might hang or take >30 seconds

**When NOT to use:**
- ❌ Quick file operations (`dir`, `Get-ChildItem`, `Get-Content`)
- ❌ Git commands (`git status`, `git diff`)
- ❌ Simple checks (`Test-Path`)

**Examples:**
```powershell
# Build with 5-minute timeout
powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Debug" -TimeoutSec 300

# Run tests with output preview
powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "ctest --test-dir build -C Debug --verbose" -TimeoutSec 180 -PrintOutput
```

**Benefits**: Guaranteed termination (no stuck processes), structured output for automation, per-run logs for debugging

## Continuous Integration
- **Status**: GitHub Actions workflows are temporarily removed and will be refactored back in eventually.
- **Validation**: Use local builds and manual checks until automation returns.

## Git Merge and PR Management
- **Review Changes Before Merging**: Always carefully review git changes when merging a PR
  - Use `git --no-pager status` and `git --no-pager diff` to inspect pending changes
  - Consider the impact of changes on existing code and functionality
  - Check for merge conflicts and resolve them thoughtfully
- **Integration Considerations**: When merging changes from main into a feature branch:
  - Review the commit history to understand what changed
  - Test the integrated code after merging
  - Verify builds succeed locally after integration
- **Code Review**: Before finalizing a PR merge:
  - Ensure local builds and checks pass
  - Review the cumulative diff of all changes
  - Verify changes align with project conventions and standards

## Documentation
- **README.md**: Create/update as needed; must reflect current architecture, module structure, and data flow
- **Update README** when adding, removing, or restructuring modules
- **Architecture diagrams**: Keep ASCII diagrams in README current with implementation
- **Known errors**: Add reproducible, solvable command/build errors to `docs/copilot-known-errors.md`. Follow the template (Command, Symptom, Cause, Fix, Notes, Verified), include exact commands and minimal environment notes (OS, shell), verify the fix on a clean environment, and open a PR to add or update entries.
- **Docs checks policy**: Run markdown link checks and markdown lint only when explicitly requested. Do not treat them as default required gates.

## Custom Agents
- **Agent Profiles** (`.github/agents/`): GitHub Copilot agent profiles (`.agent.md` files) that appear in the Copilot agent dropdown for code review and assistance
  - `gobbo` - Elegant C++ code reviewer focused on concise, readable code
  - `profiler` - CLI performance profiling agent; runs VSDiagnostics sessions, diffs baselines, routes regressions
- **Prompt Agents** (`.github/prompts/`): Specialized task agents (`.prompt.md` files) for specific development workflows
  - `check-raii` - Verify RAII patterns and resource management
  - `review-error-handling` - Check Try | usage and error handling
  - `review-frame-logic` - Review D3D12 frame submission and synchronization
  - `debug-resources` - Diagnose D3D12 resource state issues
  - `explain-nvenc` - Explain NVENC API usage and integration
  - `refactor-extract` - Assist with extracting cohesive functionality
  - `stage-changelist` - Review changes and prepare commits with highlights
  - `profile-performance` - Run VSDiagnostics CPU/memory/file I/O sessions; compare against baselines in `docs/perf-baselines/`
- **Snippets** (`.github/prompts/snippets/`): Reference materials for agents
  - `nvenc-guide.md` - NVENC SDK 13.0 D3D12 quick reference
  - `vsdiagnostics-guide.md` - VSDiagnostics.exe CLI profiling quick reference
- **Performance Baselines**: `docs/perf-baselines/` — committed JSON summaries; `.diagsession` binary files gitignored
- **Usage Guide**: See `.github/prompts/README.md` for complete usage recommendations
- **When to Use**: Before committing new resource-managing classes, after API integration, when debugging rendering issues, or preparing for code review
- **Best Practice**: Run `check-raii` and `review-error-handling` on all new code before submission; run `profile-performance` after encoder or frame-loop changes

## Future Considerations
- Evaluate external libraries only if Windows API doesn't provide equivalent functionality
- If threading becomes necessary, this policy will be revisited and documented
- Maintain static linking preference for portable deployment
- TODO: Consider daily scheduled documentation hygiene checks (markdown links + markdown lint) once CI workflow automation returns.
