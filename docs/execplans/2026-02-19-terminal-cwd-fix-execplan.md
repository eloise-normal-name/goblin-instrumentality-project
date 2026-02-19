---
status: active
owner: Nova [g53c]
related: n/a
---

# Fix: Terminal Opens in C:\Users\Admin\Source\Repos Instead of Workspace Root

This ExecPlan is a living document. See `.agent/PLANS.md` for lifecycle rules.

## Purpose / Big Picture

New VS Code integrated terminals open in `C:\Users\Admin\Source\Repos` — Visual Studio's default
source repository path — instead of the goblin-stream workspace root
(`C:\Users\Admin\goblin-stream`). This makes every new terminal session start in the wrong
directory, requiring a manual `cd` before any build, git, or script command will work correctly.
After this fix, every new terminal will open directly in the workspace root.

## Progress

- [x] (2026-02-19) Added `"terminal.integrated.cwd": "${workspaceFolder}"` to user settings.
- [x] (2026-02-19) Investigated why cwd setting was being overridden — identified root cause.
- [x] (2026-02-19) Added `-SkipAutomaticLocation` to Developer PowerShell (VS 18) profile args.
- [x] (2026-02-19) Applied `-SkipAutomaticLocation` to user settings profile args.
- [ ] Verify new terminal opens in `C:\Users\Admin\goblin-stream` after VS Code reload.

## Surprises & Discoveries

- Observation: `terminal.integrated.cwd` in user settings was not taking effect.
  Evidence: Terminals opened in `C:\Users\Admin\Source\Repos` even after setting cwd to
  `${workspaceFolder}`.

- Observation: `Launch-VsDevShell.ps1` changes the working directory to Visual Studio's default
  source root as part of its own initialization logic. This runs *after* VS Code applies
  `terminal.integrated.cwd`, so the script overwrites whatever cwd VS Code set.
  Evidence: `Get-Help Launch-VsDevShell.ps1 -Full` shows `-SkipAutomaticLocation` parameter
  exists specifically to suppress this directory change. The default behavior (without the flag)
  always cd's to the VS-managed source root, which on this machine is
  `C:\Users\Admin\Source\Repos`.

## Decision Log

- Decision: Add `-SkipAutomaticLocation` to the profile args in user settings rather than
  appending a `; Set-Location` command at the end of the `-Command` string.
  Rationale: `-SkipAutomaticLocation` is the intended, documented mechanism for this exact
  scenario. Appending `Set-Location` would also work but is more fragile (shell errors before
  that point would skip it) and harder for future agents to recognize.
  Date/Author: 2026-02-19, Nova [g53c]

- Decision: Keep `"terminal.integrated.cwd": "${workspaceFolder}"` in user settings in addition
  to the `-SkipAutomaticLocation` flag.
  Rationale: The cwd setting acts as the primary intent signal and applies to any terminal profile
  that does NOT override it with its own startup commands. Defense-in-depth.
  Date/Author: 2026-02-19, Nova [g53c]

## Outcomes & Retrospective

To be written after verification.

## Context and Orientation

The VS Code user settings file lives at:

    C:\Users\Admin\AppData\Roaming\Code\User\settings.json

The relevant section before this fix:

    "terminal.integrated.cwd": "${workspaceFolder}",
    "terminal.integrated.defaultProfile.windows": "Developer PowerShell (VS 18)",
    "terminal.integrated.profiles.windows": {
        "Developer PowerShell (VS 18)": {
            "source": "PowerShell",
            "args": [
                "-NoExit",
                "-Command",
                "& 'C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\Tools\\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64"
            ]
        }
    }

The problem is that `Launch-VsDevShell.ps1`, when invoked without `-SkipAutomaticLocation`,
changes the PowerShell session's working directory to Visual Studio's configured default source
root. On this machine that is `C:\Users\Admin\Source\Repos`. Because this happens during shell
initialization (while running the `-Command` arg), it executes *after* VS Code has already
applied `terminal.integrated.cwd`. VS Code sets the cwd, then the shell starts, then
`Launch-VsDevShell.ps1` cds away from it.

The fix is to add `-SkipAutomaticLocation` to the `Launch-VsDevShell.ps1` invocation so the
script sets up compiler environment variables without touching the working directory. The
`terminal.integrated.cwd` value then holds.

## Plan of Work

Edit the terminal profile in `C:\Users\Admin\AppData\Roaming\Code\User\settings.json`.
Find the `-Command` argument string for the Developer PowerShell (VS 18) profile and append
`-SkipAutomaticLocation` to the `Launch-VsDevShell.ps1` call.

The target line before the fix:

    "& 'C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\Tools\\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64"

After the fix:

    "& 'C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\Tools\\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation"

## Concrete Steps

1. In `C:\Users\Admin\AppData\Roaming\Code\User\settings.json`, locate the `args` array for the
   Developer PowerShell (VS 18) profile and change the last element from:

       "& '...Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64"

   to:

       "& '...Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation"

2. Save the file. Open a new VS Code terminal (Ctrl+`). The prompt should show
   `C:\Users\Admin\goblin-stream` (or the active workspace folder) as the cwd.

## Validation and Acceptance

Open a new integrated terminal in VS Code (while goblin-stream workspace is open).
Run:

    (Get-Location).Path

Expected output:

    C:\Users\Admin\goblin-stream

If the output is `C:\Users\Admin\Source\Repos` or any other path, the fix did not take effect —
check that the settings file was saved and reload the VS Code window.

## Idempotence and Recovery

The change is safe to apply multiple times; adding `-SkipAutomaticLocation` twice to the command
string would be a syntax error, so verify before saving. Reverting is simply removing the flag
from the args string.

## Artifacts and Notes

Output of `Get-Help Launch-VsDevShell.ps1 -Full | Select-Object -First 5` confirming the flag:

    Launch-VsDevShell.ps1 [-Arch <string>] [-HostArch <string>] [-SkipAutomaticLocation] ...
    Launch-VsDevShell.ps1 [-VsInstallationPath <string>] [-Arch <string>] [-HostArch <string>]
      [-SkipAutomaticLocation] ...
