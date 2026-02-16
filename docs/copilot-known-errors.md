# Known Errors and Fixes ✅

A living list of reproducible, solvable build and tooling problems that Copilot can consult so it does not suggest fixes that were already recorded as unreliable. Keep entries concise and focused on commands we actually run.

## Template
| Command | Symptom | Cause | Fix | Notes |
|---|---|---|---|---|
| `command here` | short description of what happens | root cause or likely reason | concrete steps to fix / command to run | links to docs or related issues plus verification notes |

## Example
| Command | Symptom | Cause | Fix | Notes |
|---|---|---|---|---|
| `cmake -G "Visual Studio 18 2026" -A x64` | CMake picks a different generator on CI | Environment or default CMake generator misconfigured | Explicitly pass the generator and architecture: `-G "Visual Studio 18 2026" -A x64`; verify with `cmake --help` | See CMake docs for generators; rerun on clean CI to confirm |
| `rmdir /S /Q "src\pipeline"` (PowerShell) | PowerShell reports: `Remove-Item : A positional parameter cannot be found that accepts argument '/Q'.` | `rmdir` maps to `Remove-Item` in PowerShell, which expects `-Recurse`/`-Force` switches instead of slash options | Run the command through CMD (e.g., `cmd /c rmdir /S /Q "src\pipeline"`) or use PowerShell syntax: `Remove-Item src\pipeline -Recurse -Force` | Windows 10/11/Server PowerShell reproduces this when using Unix-style switch syntax |
| `git add`, `git commit`, `git checkout`, etc. | Git fails with "Unable to create '/path/to/repo/.git/index.lock': File exists. Another git process seems to be running..." or "you need to resolve your current index first" | Stale `.git/index.lock` file left behind from interrupted git operation or process crash | Remove the lock file: `rm -f .git/index.lock` (Unix/Git Bash) or `del /F .git\index.lock` (CMD), then retry the git command | This occurs when a git process is interrupted (crash, timeout, Ctrl+C). Safe to remove if no git processes are running. Verified in GitHub Actions workflows; reproduced when git operations time out or are interrupted |

---

## How to contribute
- Reproduce the issue and capture the minimal commands needed.
- Add an entry using the template above, describing symptoms, root cause, and the fix that worked for you.
- Note how you confirmed the fix within the Notes column (e.g., "re-ran CMakelists on clean build").
- Open a PR updating this file with the new entry so Copilot can start recommending the documented workaround.

> Tip: Keep entries short, include exact commands, and mention the OS or shell when it matters.
