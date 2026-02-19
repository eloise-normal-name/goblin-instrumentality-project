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

## Windows Overlapped I/O

| Command | Symptom | Cause | Fix | Notes | Verified |
|---|---|---|---|---|---|
| Overlapped `WriteFile` with auto-reset event + `GetOverlappedResult(..., TRUE)` | App hangs indefinitely in `GetOverlappedResult` after `MsgWaitForMultipleObjects` fires | `MsgWaitForMultipleObjects` (and all wait functions) **consume** auto-reset events when they return, clearing the signal. `GetOverlappedResult(bWait=TRUE)` then waits on the same now-nonsignaled event forever. | Use **manual-reset events** (`bManualReset=TRUE` in `CreateEvent`). Add a `ResetEvent` call before submitting each new operation. | Per OVERLAPPED docs: *"you should use a manual-reset event; if you use an auto-reset event, your application can stop responding if you wait for the operation to complete and then call GetOverlappedResult with the bWait parameter set to TRUE."* `WriteFile` itself clears `hEvent` before issuing I/O, so an explicit `ResetEvent` before `WriteFile` is redundant but harmless. | 2026-02-18 |
| Overlapped `WriteFile` with `hEvent = NULL` and multiple concurrent ops on same file | No way to distinguish which operation completed; race conditions | If `hEvent` is NULL, the OS signals the file handle itself on completion, which is ambiguous when multiple overlapped ops are in flight | Always set `OVERLAPPED::hEvent` to a dedicated per-operation event; never rely on the file handle signal for overlapped I/O | Per `GetOverlappedResult` docs: *"Use of file, named pipe, or communications-device handles for this purpose is discouraged."* | N/A |

---

## How to contribute
- Reproduce the issue and capture the minimal commands needed.
- Add an entry using the template above, describing symptoms, root cause, and the fix that worked for you.
- Note how you confirmed the fix within the Notes column (e.g., "re-ran CMakelists on clean build").
- Open a PR updating this file with the new entry so Copilot can start recommending the documented workaround.

> Tip: Keep entries short, include exact commands, and mention the OS or shell when it matters.
