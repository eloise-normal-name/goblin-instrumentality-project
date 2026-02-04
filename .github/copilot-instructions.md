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
- **Naming**: Use clear, descriptive identifiers in snake_case; no trailing underscores on member variables; constants use CAPS_CASE (SCREAMING_SNAKE_CASE)
- **Resource Management**: RAII patterns for all resource handling (no manual delete/close patterns)
- **Style**: All code must conform to `.clang-format` configuration (Tabs, 4-wide, 100-column limit)
- **Warnings**: Compile with `/W4` (treat all warnings as errors in future)

## Prohibited Patterns
- **Threading**: No multi-threaded code or concurrency primitives
- **Comments**: Use self-documenting code instead of comments
- **External Libraries**: Stick to Windows APIs and C++ standard library
- **Namespaces**: Do not use namespaces; keep all code in the global namespace
- **Trailing Underscores**: Do not use trailing underscores for member variables (use plain snake_case)

## Git Workflow
- **Branching**: Use feature branches to organize changes
- **Branch Naming**: 
  - `feature/description` for new features
  - `bugfix/description` for bug fixes
  - `refactor/description` for refactoring
- **Master Branch**: Stable, buildable code only

## Build Process
- **Debug Target**: Outputs to `bin/Debug/goblin-stream.exe` with full debug info (`/Zi`)
- **Release Target**: Outputs to `bin/Release/goblin-stream.exe` with optimizations (`/O2`)
- **Intermediate Files**: Located in `build/` (git-ignored)

## VS Code Workflow
- **Extension**: Use CMake Tools extension
- **Build**: F7 or Status Bar "Build" button
- **Run/Debug**: F5 or Ctrl+F5
- **Format**: Ctrl+Shift+I (format document on save is automatic)

## Documentation
- **README.md**: Must reflect current architecture, module structure, and data flow
- **Update README** when adding, removing, or restructuring modules
- **Architecture diagrams**: Keep ASCII diagrams in README current with implementation

## Future Considerations
- Evaluate external libraries only if Windows API doesn't provide equivalent functionality
- If threading becomes necessary, this policy will be revisited and documented
- Maintain static linking preference for portable deployment
