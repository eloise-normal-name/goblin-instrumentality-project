# Goblin Stream - Project Instructions

## Technology Stack
- **Language**: C++23 (latest standard)
- **Build System**: CMake 3.28+ with MSVC compiler (Visual Studio 2026 Community, aka version 18)
- **Linking**: Static linking (`/MT` runtime)
- **Target Platform**: Windows x64
- **Subsystem**: Windows (no console window)

## Code Organization

### Dependencies
- **Allowed**: C++ Standard Library and Windows APIs only
- **Prohibited**: External third-party libraries
- **Preference**: Use Windows API directly instead of STL equivalents when straightforward

### Code Quality Standards
- **Documentation**: Self-documenting code with meaningful names (NO comment-based documentation)
- **Naming**: 
  - Methods and public APIs use PascalCase (e.g., `WaitForGpu`, `BeginFrame`)
  - Local variables and private helper functions use snake_case (e.g., `create_device`, `fence_event`)
  - Member variables use snake_case without trailing underscores
  - Constants use CAPS_CASE (SCREAMING_SNAKE_CASE)
- **Resource Management**: Full RAII with constructor/destructor pairs (no Initialize/Shutdown methods):
  - All resource allocation in constructors (called once at object creation)
  - All resource cleanup in destructors (called at object destruction)
  - No two-phase initialization; construction must either fully succeed or throw
  - For GPU resources receiving parameters, constructors accept all necessary config/device pointers
  - HANDLEs, COM objects, and DLL modules wrapped with proper cleanup in destructors
- **Command Lists**: Command lists and allocators must be generated once and reused efficiently (via Reset); do not recreate them every frame
- **Style**: All code must conform to `.clang-format` configuration (Tabs, 4-wide, 100-column limit)
  - Single-statement conditionals (`if`, `for`, `while`) should omit braces
  - Multi-statement blocks require braces
- **Warnings**: Compile with `/W4` (treat all warnings as errors in future)
- **Type Deduction**: Prefer `auto` when it avoids writing the type (e.g., function returns, smart pointer declarations from `make_unique`, lambdas). Do not add `*` or `&` to `auto` declarations unless required for correctness. For struct initialization where you must write the type anyway, use explicit type: `Type var{.field = val};`
- **Casts**: Use C-style casts `(Type)value` instead of C++ casts (`static_cast`, `const_cast`, `dynamic_cast`, `reinterpret_cast`); should rarely be necessary in this project
- **Error Handling**: Use `Try |` pattern from `include/try.h` for all D3D12 and NVENC API calls
  - Chain consecutive error-checked operations with single `Try` and multiple `|` operators:
    ```cpp
    Try | function1()
        | function2()
        | function3();
    ```
  - Throws empty exception on first failure; all error codes must be checked
- **Struct Initialization**:
  - Use designated initializers (`.field = value`) at initialization time, never initialize-then-assign
  - Prefer brace initialization: `Type var{.field = val};` (no equals sign)
  - Example (✓ correct): `WNDCLASSEXW wc{.cbSize = sizeof(WNDCLASSEXW), .style = CS_HREDRAW};`
  - Example (✗ wrong): `WNDCLASSEXW wc{}; wc.cbSize = ...; wc.style = ...;`
  - Avoid re-assigning default values (0, nullptr, false) unless critical for clarity

## Prohibited Patterns
- **Threading**: No multi-threaded code or concurrency primitives
- **Comments**: Use self-documenting code instead of comments
- **External Libraries**: Stick to Windows APIs and C++ standard library
- **Namespaces**: Do not use namespaces; keep all code in the global namespace
- **Trailing Underscores**: Do not use trailing underscores for member variables (use plain snake_case)
- **NVENC**: Follow the [NVENC Programming Guide](https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/nvenc-video-encoder-api-prog-guide/index.html).
  - Use D3D12 Readback Heaps for bitstream output (Client-allocated).
  - Use D3D12 Textures for input.

## Git Workflow
- **Branching**: Use feature branches to organize changes
- **Branch Naming**: 
  - `feature/description` for new features
  - `bugfix/description` for bug fixes
  - `refactor/description` for refactoring
- **Master Branch**: Stable, buildable code only

## Build Process
- **Environment**: Must use **Visual Studio Community 2026 Preview - amd64** environment/kit
- **Generator**: Use `-G "Visual Studio 18 2026" -A x64`
- **Debug Target**: Outputs to `bin/Debug/goblin-stream.exe` with full debug info (`/Zi`)
- **Release Target**: Outputs to `bin/Release/goblin-stream.exe` with optimizations (`/O2`)
- **Intermediate Files**: Located in `build/` (git-ignored)

## VS Code Workflow
- **Extension**: Use CMake Tools extension
- **Build**: F7 or Status Bar "Build" button
- **Run/Debug**: F5 or Ctrl+F5
- **Format**: Ctrl+Shift+I (format document on save is automatic)
- **Terminal**: Configure to use Command Prompt (`cmd`) instead of PowerShell for better compatibility with CMake and build tools
  - Command Palette → "Terminal: Select Default Shell" → Choose "Command Prompt"
  - Or add to `.vscode/settings.json`: `"terminal.integrated.defaultProfile.windows": "Command Prompt"`

## Command Syntax
- **Always use Windows/PowerShell native commands**, never Unix syntax
  - ✅ `Get-Item`, `dir`, `Get-ChildItem` (Windows/PowerShell)
  - ❌ `ls -la`, `ls -l` (Unix/Linux syntax)
  - ❌ `cat`, `grep`, `find` (Unix tools)
- **Use quoted paths** when they contain spaces: `"C:\Program Files\..."`
- **CMake commands**: Always specify generator explicitly: `-G "Visual Studio 18 2026" -A x64`

## Documentation
- **README.md**: Must reflect current architecture, module structure, and data flow
- **Update README** when adding, removing, or restructuring modules
- **Architecture diagrams**: Keep ASCII diagrams in README current with implementation

## Future Considerations
- Evaluate external libraries only if Windows API doesn't provide equivalent functionality
- If threading becomes necessary, this policy will be revisited and documented
- Maintain static linking preference for portable deployment
