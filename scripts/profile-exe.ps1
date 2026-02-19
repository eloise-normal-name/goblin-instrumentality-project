param(
	[string]$BuildConfig = "RelWithDbgInfo",
	[ValidateSet("all", "cpu", "memory", "fileio")]
	[string]$Focus = "all",
	[string]$RunLabel = "baseline",
	[string]$AppArgs = "--headless",
	[switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repo_root = (Get-Location).Path
$out_dir = Join-Path $repo_root "docs/perf-baselines"

New-Item -ItemType Directory -Path $out_dir -Force | Out-Null

if (-not $SkipBuild) {
	$build_command = "cmake --build build --config $BuildConfig"
	powershell -ExecutionPolicy Bypass -File "scripts/agent-wrap.ps1" -Command $build_command -TimeoutSec 300 | Out-Null
}

$exe_path = Join-Path $repo_root "bin/$BuildConfig/goblin-stream.exe"
if (-not (Test-Path $exe_path)) {
	throw "Executable not found: $exe_path"
}

$vsdiag = Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Filter "VSDiagnostics.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
if ([string]::IsNullOrWhiteSpace($vsdiag)) {
	throw "VSDiagnostics.exe not found. Verify Visual Studio 2026 installation."
}

$configs = Join-Path (Split-Path $vsdiag -Parent) "AgentConfigs"
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"

$session_files = [ordered]@{}
$session_times = [ordered]@{}

function Run-Session {
	param(
		[int]$SessionId,
		[string]$Name,
		[string]$ConfigFile
	)

	$output_file = Join-Path $out_dir "$Name`_$stamp.diagsession"
	$config_path = Join-Path $configs $ConfigFile

	$start_time = Get-Date
	& $vsdiag start $SessionId "/launch:$exe_path" "/launchArgs:$AppArgs" "/loadConfig:$config_path"
	if ($LASTEXITCODE -ne 0) {
		throw "VSDiagnostics start failed for $Name with exit code $LASTEXITCODE"
	}

	& $vsdiag stop $SessionId "/output:$output_file"
	if ($LASTEXITCODE -ne 0) {
		throw "VSDiagnostics stop failed for $Name with exit code $LASTEXITCODE"
	}

	$duration_ms = [int][Math]::Round(((Get-Date) - $start_time).TotalMilliseconds)
	if (-not (Test-Path $output_file)) {
		throw "Missing session file: $output_file"
	}

	$size = (Get-Item $output_file).Length
	if ($size -le 0) {
		throw "Session file is empty: $output_file"
	}

	$session_files[$Name] = (Resolve-Path $output_file).Path.Replace($repo_root + "\\", "").Replace("\\", "/")
	$session_times[$Name] = $duration_ms
}

if ($Focus -eq "all" -or $Focus -eq "cpu") {
	Run-Session -SessionId 1 -Name "cpu" -ConfigFile "CpuUsageHigh.json"
}

if ($Focus -eq "all" -or $Focus -eq "memory") {
	Run-Session -SessionId 1 -Name "memory" -ConfigFile "MemoryUsage.json"
}

if ($Focus -eq "all" -or $Focus -eq "fileio") {
	Run-Session -SessionId 1 -Name "fileio" -ConfigFile "FileIO.json"
}

$previous_file = Get-ChildItem -Path $out_dir -Filter "baseline_*.json" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
$previous = $null
if ($previous_file) {
	$previous = Get-Content -Raw -Path $previous_file.FullName | ConvertFrom-Json
}

$cpu_time = if ($session_times.Contains("cpu")) { [int]$session_times["cpu"] } else { 0 }
$fileio_writes = 0
$regression_flags = New-Object System.Collections.Generic.List[string]

if ($previous -and $previous.cpu.wall_time_ms -gt 0 -and $cpu_time -gt 0) {
	$cpu_delta_pct = (($cpu_time - [double]$previous.cpu.wall_time_ms) / [double]$previous.cpu.wall_time_ms) * 100.0
	if ($cpu_delta_pct -gt 10.0) {
		$regression_flags.Add("CPU wall time regression: +$([Math]::Round($cpu_delta_pct, 2))%")
	}
}

if ($previous -and $previous.fileio.write_calls -gt 0 -and $fileio_writes -gt [int]$previous.fileio.write_calls) {
	$regression_flags.Add("File I/O write call regression: $fileio_writes vs $($previous.fileio.write_calls)")
}

$baseline = [ordered]@{
	run_label = "$RunLabel`_$stamp"
	build_config = $BuildConfig
	app_args = $AppArgs
	frames = 30
	session_files = [ordered]@{
		cpu = if ($session_files.Contains("cpu")) { $session_files["cpu"] } else { "" }
		memory = if ($session_files.Contains("memory")) { $session_files["memory"] } else { "" }
		fileio = if ($session_files.Contains("fileio")) { $session_files["fileio"] } else { "" }
	}
	cpu = [ordered]@{
		wall_time_ms = $cpu_time
		avg_cpu_pct = 0.0
		peak_cpu_pct = 0.0
		top_hot_functions = @()
	}
	memory = [ordered]@{
		peak_private_bytes = 0
		total_heap_allocs = 0
		top_alloc_sites = @()
	}
	fileio = [ordered]@{
		total_read_bytes = 0
		total_write_bytes = 0
		write_calls = $fileio_writes
		top_files = @()
	}
	regression_flags = $regression_flags
}

$baseline_name = "baseline_{0}_{1}.json" -f $RunLabel, $stamp
$baseline_path = Join-Path $out_dir $baseline_name
$baseline | ConvertTo-Json -Depth 8 | Set-Content -Path $baseline_path -Encoding UTF8

powershell -ExecutionPolicy Bypass -File "scripts/render-perf-report.ps1" | Out-Null

Write-Output "Baseline generated: $baseline_path"
Write-Output "Report generated: $(Join-Path $repo_root "perf-highlights.html")"
if ($regression_flags.Count -gt 0) {
	Write-Output "Regressions:"
	$regression_flags | ForEach-Object { Write-Output "- $_" }
}
