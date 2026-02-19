# Performance Baselines

This directory stores committed performance summary JSON files for the Goblin Instrumentality Project.

## What lives here

| File pattern | Contents |
|---|---|
| `baseline_<label>_<date>.json` | Human-readable metric summary committed to VCS |
| `cpu_<timestamp>.diagsession` | **Gitignored** binary CPU profiling session (open in VS) |
| `memory_<timestamp>.diagsession` | **Gitignored** binary memory profiling session |
| `fileio_<timestamp>.diagsession` | **Gitignored** binary file I/O profiling session |
| `../../perf-highlights.html` | Generated performance highlights report for `gh-pages` publication |

## How to run

Use the profiler agent:
```
@workspace /agent profiler Run full profiling suite and compare against latest baseline
```

Or follow `.github/prompts/snippets/vsdiagnostics-guide.md` for manual CLI steps.

Reusable script workflow:
```
powershell -ExecutionPolicy Bypass -File scripts/profile-exe.ps1 -BuildConfig RelWithDbgInfo -Focus all -RunLabel baseline
```

## Regression thresholds

- CPU wall time: **>10%** increase vs previous baseline
- Peak private memory: **>5%** increase vs previous baseline
- File I/O write calls: any increase per 30 frames vs previous baseline

## JSON schema

```json
{
  "run_label": "post-<change>_YYYY-MM-DD",
  "build_config": "RelWithDbgInfo",
  "app_args": "--headless",
  "frames": 30,
  "session_files": {
    "cpu":    "docs/perf-baselines/cpu_TIMESTAMP.diagsession",
    "memory": "docs/perf-baselines/memory_TIMESTAMP.diagsession",
    "fileio": "docs/perf-baselines/fileio_TIMESTAMP.diagsession"
  },
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
  },
  "regression_flags": []
}
```
