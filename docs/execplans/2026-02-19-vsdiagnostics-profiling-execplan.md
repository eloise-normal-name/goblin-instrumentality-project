---
status: active
owner: codex
related: user-request-2026-02-19-vsdiagnostics-profiling
---

# CLI Profiling with VSDiagnostics.exe — CPU, Memory, File I/O

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

## Purpose / Big Picture

Establish a repeatable CLI profiling workflow using `VSDiagnostics.exe` (Visual Studio Diagnostics Hub collector) so that future agents can measure CPU usage, memory allocation, and file I/O for Goblin Instrumentality, identify regressions, and guide optimization work. The headless `--headless` app mode (exits after 30 frames) provides a deterministic, bounded workload for profiling.

Deliverables:
1. **Snippet**: `.github/prompts/snippets/vsdiagnostics-guide.md` — quick command reference
2. **Skill (prompt)**: `.github/prompts/profile-performance.prompt.md` — task agent for running sessions and interpreting results
3. **Agent profile**: `.github/agents/profiler.agent.md` — specialized profiler agent for regression hunting
4. **Baseline directory**: `docs/perf-baselines/` — committed summary JSON files; `.diagsession` binary files gitignored
5. **Gitignore update**: add `*.diagsession` to `.gitignore`

## Progress

- [x] (2026-02-19) Created execplan
- [ ] Verify VSDiagnostics.exe path on local VS 2026 install
- [ ] Run CPU profiling session (headless Release build)
- [ ] Run Memory profiling session (headless Release build)
- [ ] Run FileIO profiling session (headless Release build)
- [ ] Capture baseline summary JSON from first run
- [ ] Add `*.diagsession` to `.gitignore`
- [ ] Create `docs/perf-baselines/` directory with `README.md`
- [ ] Record outcomes and update this document

## Surprises & Discoveries

_Update as work proceeds._

## Decision Log

- Decision: Use the headless `--headless` exit-after-30-frames mode as the canonical profiling workload.
  Rationale: Deterministic workload length prevents timing noise from indefinite runs; 30 frames is enough to capture steady-state behavior.
  Date/Author: 2026-02-19 / Copilot

- Decision: Store `.diagsession` binary files in `docs/perf-baselines/` but gitignore them; commit only the human-readable summary JSON derived from each session.
  Rationale: `.diagsession` files can be hundreds of MB and are binary; they carry no diff value in VCS. The JSON summary provides regression-detectable numbers.
  Date/Author: 2026-02-19 / Copilot

- Decision: Run CPU, memory, and file I/O as three separate VSDiagnostics sessions rather than one combined session.
  Rationale: VSDiagnostics `/loadConfig` accepts one agent config per invocation; running separate sessions keeps each profile clean and avoids observer effect stacking.
  Date/Author: 2026-02-19 / Copilot

## Outcomes & Retrospective

_Pending first run. Update after completing Concrete Steps._

## Context and Orientation

`VSDiagnostics.exe` is the CLI front-end to the VS Diagnostics Hub collector agent and ships with Visual Studio. It supports:
- **CPU Usage** (`CpuUsageHigh.json`) — CPU sampling at ~1 kHz; shows hot functions and call trees
- **Memory Usage** (`MemoryUsage.json`) — heap snapshot diffing; shows allocations by callsite
- **File I/O** (`FileIO.json`) — ETW-based file read/write tracking; shows I/O volume and latency per file

Session lifecycle:
```
VSDiagnostics.exe start <id> /launch:<exe> [/launchArgs:<args>] /loadConfig:<config>
# process runs; VSDiagnostics stops collecting when process exits
VSDiagnostics.exe stop <id> /output:<session.diagsession>
```

When `/launch:` is used with a process that exits on its own (as headless mode does), `stop` must still be called explicitly to flush and serialize the session file. The session id (1–255) is a local slot number.

Baseline summary JSON format (written by agent after opening `.diagsession` in VS or via ETW parse):
```json
{
  "run_label": "baseline_2026-02-19",
  "build_config": "Release",
  "app_args": "--headless",
  "frames": 30,
  "cpu": {
    "wall_time_ms": 0,
    "avg_cpu_pct": 0.0,
    "peak_cpu_pct": 0.0,
    "top_hot_functions": []
  },
  "memory": {
    "peak_private_bytes": 0,
    "total_heap_allocs": 0,
    "top_alloc_sites": []
  },
  "fileio": {
    "total_read_bytes": 0,
    "total_write_bytes": 0,
    "write_calls": 0,
    "top_files": []
  }
}
```

## Plan of Work

### Step 1 — Locate VSDiagnostics.exe

```powershell
$vsdiag = Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Filter "VSDiagnostics.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
$vsdiag.FullName
```

Expected path (VS 2026 Community):
```
C:\Program Files\Microsoft Visual Studio\2026\Community\Team Tools\DiagnosticsHub\Collector\VSDiagnostics.exe
```

Agent configs directory (same base):
```
...\Team Tools\DiagnosticsHub\Collector\AgentConfigs\
```

### Step 2 — Build Release Binary

```powershell
powershell -ExecutionPolicy Bypass -File scripts/agent-wrap.ps1 -Command "cmake --build build --config Release" -TimeoutSec 300
```

### Step 3 — CPU Profiling Session

```powershell
$vsdiag  = "C:\Program Files\Microsoft Visual Studio\2026\Community\Team Tools\DiagnosticsHub\Collector\VSDiagnostics.exe"
$configs = "C:\Program Files\Microsoft Visual Studio\2026\Community\Team Tools\DiagnosticsHub\Collector\AgentConfigs"
$out     = "docs\perf-baselines"

New-Item -ItemType Directory -Force -Path $out | Out-Null

& $vsdiag start 1 /launch:"bin\Release\goblin-stream.exe" /launchArgs:"--headless" /loadConfig:"$configs\CpuUsageHigh.json"
# wait for process to exit naturally (30 frames), then:
& $vsdiag stop 1 /output:"$out\cpu_$(Get-Date -Format 'yyyyMMdd_HHmmss').diagsession"
```

### Step 4 — Memory Profiling Session

```powershell
& $vsdiag start 1 /launch:"bin\Release\goblin-stream.exe" /launchArgs:"--headless" /loadConfig:"$configs\MemoryUsage.json"
& $vsdiag stop 1 /output:"$out\memory_$(Get-Date -Format 'yyyyMMdd_HHmmss').diagsession"
```

### Step 5 — File I/O Profiling Session

```powershell
& $vsdiag start 1 /launch:"bin\Release\goblin-stream.exe" /launchArgs:"--headless" /loadConfig:"$configs\FileIO.json"
& $vsdiag stop 1 /output:"$out\fileio_$(Get-Date -Format 'yyyyMMdd_HHmmss').diagsession"
```

### Step 6 — Baseline JSON

Open each `.diagsession` in Visual Studio (File → Open) under the Performance Profiler report tab. Extract key metrics and write to:
```
docs/perf-baselines/baseline_YYYYMMDD.json
```

### Step 7 — Gitignore

Add to `.gitignore`:
```
# VSDiagnostics profiling sessions (binary, large)
*.diagsession
```

### Step 8 — Update Progress, Surprises, Outcomes

Fill in all remaining sections in this document after steps complete.

## Regression Detection Workflow

After each significant code change, a future agent should:

1. Rebuild Release binary
2. Re-run the three VSDiagnostics sessions in Steps 3–5
3. Write a new dated baseline JSON under `docs/perf-baselines/`
4. Diff the new JSON against the previous baseline:
   - CPU wall time regression: >10% increase warrants investigation
   - Peak memory regression: >5% increase warrants investigation
   - File I/O write bytes regression: any increase in write calls per 30 frames warrants investigation
5. If regression found: route to `review-frame-logic` (CPU/timing) or `debug-resources` (memory) agents

## Concrete Steps — Evidence

_Paste command output and `.diagsession` path here after each step._
