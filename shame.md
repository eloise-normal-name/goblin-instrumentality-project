# Shame Log

- Removed `FileHandle` from `src/graphics/d3d12_pipeline.cpp`.
  - Resource owners must establish invariants in constructors; no defaulted constructors.
  - File I/O now uses explicit `HANDLE` lifetimes with `CloseHandle` on all paths.
- Reminder logged: I will check editing tool availability myself before attempting edits.
  - Agent: Nia [gpt52]
