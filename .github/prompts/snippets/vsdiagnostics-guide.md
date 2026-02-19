VSDIAGNOSTICS GUIDE — CLI PROFILING, GOBLIN INSTRUMENTALITY

TOOL
  VSDiagnostics.exe — Diagnostics Hub CLI collector, ships with Visual Studio.

LOCATE (run once to find path)
  Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Filter "VSDiagnostics.exe" | Select-Object -First 1 -ExpandProperty FullName

EXPECTED PATHS (VS 2026 Community / internal version 18)
  Exe:     C:\Program Files\Microsoft Visual Studio\18\Community\Team Tools\DiagnosticsHub\Collector\VSDiagnostics.exe
  Configs: C:\Program Files\Microsoft Visual Studio\18\Community\Team Tools\DiagnosticsHub\Collector\AgentConfigs\

AVAILABLE AGENT CONFIGS (VS 18 Community, verified)
  CpuUsageHigh.json   — CPU sampling at ~1 kHz; yields hot function list and call tree
  CpuUsageLow.json    — CPU sampling at lower frequency; lower overhead for longer runs
  FileIOBase.json     — ETW file read/write tracing; shows I/O volume and per-file breakdown
  NOTE: MemoryUsage.json does NOT exist in VS 18 Community. Memory profiling requires opening
        a .diagsession in VS Performance Profiler UI, or use DotNetObjectAllocBase.json (.NET only).

SESSION LIFECYCLE
  # Step 1: start session (launches process and attaches collector)
  VSDiagnostics.exe start <id> /launch:<exe> /launchArgs:<args> /loadConfig:<config>

  # Step 2: process exits naturally (--headless exits after 30 frames)

  # Step 3: flush and save (must be called even after process exits)
  VSDiagnostics.exe stop <id> /output:<path\session.diagsession>

  <id> is 1–255 (local slot number; reuse after stop)

CANONICAL HEADLESS PROFILING SEQUENCE
  $vsdiag  = "C:\Program Files\Microsoft Visual Studio\18\Community\Team Tools\DiagnosticsHub\Collector\VSDiagnostics.exe"
  $configs = "C:\Program Files\Microsoft Visual Studio\18\Community\Team Tools\DiagnosticsHub\Collector\AgentConfigs"
  $stamp   = Get-Date -Format 'yyyyMMdd_HHmmss'
  $out     = "docs\perf-baselines"

  # NOTE: 'start' is NON-BLOCKING — it launches the process and returns immediately.
  # Wait for the process to finish before calling stop.

  # CPU
  & $vsdiag start 1 /launch:"bin\RelWithDbgInfo\goblin-stream.exe" /launchArgs:"--headless" /loadConfig:"$configs\CpuUsageHigh.json"
  Start-Sleep -Seconds 20   # wait for 30-frame headless run to complete
  & $vsdiag stop  1 /output:"$out\cpu_$stamp.diagsession"

  # Memory — MemoryUsage.json not available in VS 18 Community; skip or use VS UI

  # File I/O (config is FileIOBase.json, not FileIO.json)
  & $vsdiag start 1 /launch:"bin\RelWithDbgInfo\goblin-stream.exe" /launchArgs:"--headless" /loadConfig:"$configs\FileIOBase.json"
  Start-Sleep -Seconds 20
  & $vsdiag stop  1 /output:"$out\fileio_$stamp.diagsession"

REUSABLE AUTOMATION
  powershell -ExecutionPolicy Bypass -File scripts\profile-exe.ps1 -BuildConfig RelWithDbgInfo -Focus all -RunLabel baseline

OPENING RESULTS
  In Visual Studio: File → Open → <session.diagsession>
  Opens in Performance Profiler report tab with flamegraph / allocation table / I/O table.

BASELINE SUMMARY JSON (commit to docs/perf-baselines/)
  {
    "run_label": "",          // e.g. "post-encoder-refactor_2026-02-19"
    "build_config": "RelWithDbgInfo",
    "app_args": "--headless",
    "frames": 30,
    "cpu":    { "wall_time_ms": 0, "avg_cpu_pct": 0.0, "peak_cpu_pct": 0.0, "top_hot_functions": [] },
    "memory": { "peak_private_bytes": 0, "total_heap_allocs": 0, "top_alloc_sites": [] },
    "fileio": { "total_read_bytes": 0, "total_write_bytes": 0, "write_calls": 0, "top_files": [] }
  }

REGRESSION THRESHOLDS
  CPU wall time    >10% increase vs previous baseline → investigate with review-frame-logic
  Peak memory      >5%  increase vs previous baseline → investigate with debug-resources / check-raii
  File I/O writes  any new write call count per 30 frames → investigate BitstreamFileWriter

GITIGNORE
  *.diagsession   (binary, can be hundreds of MB — never commit)
  docs/perf-baselines/*.diagsession  (in case glob order matters)

TROUBLESHOOTING
  "Access denied"            — run terminal as Administrator
  "Session already exists"   — VSDiagnostics.exe stop <id>; then retry start
  Config not found           — verify AgentConfigs path matches installed VS edition/version
  Stop returns before flush  — add Start-Sleep 2 between app exit and stop call if session file is empty
