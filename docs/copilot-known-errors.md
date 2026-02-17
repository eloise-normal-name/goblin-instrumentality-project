# Known Errors and Fixes ✅

A living list of reproducible, solvable build and tooling problems that Copilot can consult so it does not suggest fixes that were already recorded as unreliable. Keep entries concise and focused on commands we actually run.

## Template
| Command | Symptom | Cause | Fix | Notes |
|---|---|---|---|---|
| `command here` | short description of what happens | root cause or likely reason | concrete steps to fix / command to run | links to docs or related issues plus verification notes |

## NVENC API Usage

| Command | Symptom | Cause | Fix | Notes | Verified |
|---|---|---|---|---|---|
| Run app with NVENC encoding | Access violation crash in `nvEncLockBitstream` | For D3D12, `NV_ENC_LOCK_BITSTREAM::outputBitstream` must point to `NV_ENC_OUTPUT_RESOURCE_D3D12` struct, not registered pointer directly | Pass `&output_resource` (pointer to `NV_ENC_OUTPUT_RESOURCE_D3D12`) instead of `output_registered_ptrs[texture_index]` | Per nvEncodeAPI.h:2801-2816: "NV_ENC_PIC_PARAMS::outputBitstream and NV_ENC_LOCK_BITSTREAM::outputBitstream must be a pointer to a struct of this type [NV_ENC_OUTPUT_RESOURCE_D3D12], when D3D12 interface is used". Fixed in PR #[number] | 2026-02-17 |

## Build System

| Command | Symptom | Cause | Fix | Notes | Verified |
|---|---|---|---|---|---|
| `cmake -G "Visual Studio 18 2026" -A x64` | CMake picks a different generator on CI | Environment or default CMake generator misconfigured | Explicitly pass the generator and architecture: `-G "Visual Studio 18 2026" -A x64`; verify with `cmake --help` | See CMake docs for generators; rerun on clean CI to confirm | N/A |

## PowerShell Command Compatibility

| Command | Symptom | Cause | Fix | Notes | Verified |
|---|---|---|---|---|---|
| `rmdir /S /Q "src\pipeline"` (PowerShell) | PowerShell reports: `Remove-Item : A positional parameter cannot be found that accepts argument '/Q'.` | `rmdir` maps to `Remove-Item` in PowerShell, which expects `-Recurse`/`-Force` switches instead of slash options | Run the command through CMD (e.g., `cmd /c rmdir /S /Q "src\pipeline"`) or use PowerShell syntax: `Remove-Item src\pipeline -Recurse -Force` | Windows 10/11/Server PowerShell reproduces this when using Unix-style switch syntax | N/A |

---

## How to contribute
- Reproduce the issue and capture the minimal commands needed.
- Add an entry using the template above, describing symptoms, root cause, and the fix that worked for you.
- Note how you confirmed the fix within the Notes column (e.g., "re-ran CMakelists on clean build").
- Open a PR updating this file with the new entry so Copilot can start recommending the documented workaround.

> Tip: Keep entries short, include exact commands, and mention the OS or shell when it matters.
