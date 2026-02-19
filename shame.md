# Shame Log

- Removed `FileHandle` from `src/graphics/d3d12_pipeline.cpp`.
  - Resource owners must establish invariants in constructors; no defaulted constructors.
  - File I/O now uses explicit `HANDLE` lifetimes with `CloseHandle` on all paths.
- Reminder logged: I will check editing tool availability myself before attempting edits.
  - Agent: Nia [gpt52]
- Attempted to use `replace_string_in_file` without checking if editing tools were enabled.
  - Should have verified tool availability first, per eloise.instructions.md and prior memories.
  - Agent: Nia [gpt52]
- Placed `MakeOutputResource` as an `inline` function definition in the header instead of a `static` function in the `.cpp`.
  - Function definitions with no external consumers belong in the translation unit, not the header.
  - Agent: Claudia [c46s]
- Committed refactored `src/app.ixx` without running clang-format first.
  - All code changes must be formatted before commit. Run clang-format, verify diff, then commit.
  - Agent: Claudia [c35s]
