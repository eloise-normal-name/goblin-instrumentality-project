param(
	[Parameter(Mandatory = $true)]
	[string]$Command,

	[string]$WorkingDirectory = ".",

	[int]$TimeoutSec = 120,

	[string]$LogDir = ".agent/logs",

	[int]$PreviewChars = 4000,

	[switch]$PrintOutput
)

$ErrorActionPreference = "Stop"

if ($TimeoutSec -lt 1) {
	throw "TimeoutSec must be >= 1."
}

if ($PreviewChars -lt 0) {
	throw "PreviewChars must be >= 0."
}

if ([string]::IsNullOrWhiteSpace($Command)) {
	throw "Command must not be empty."
}

$resolved_working_directory = (Resolve-Path $WorkingDirectory -ErrorAction Stop).Path

$run_id = (Get-Date -Format "yyyyMMdd-HHmmss-fff") + "-" + (Get-Random -Maximum 100000).ToString("D5")
$repo_root = Get-Location
$resolved_log_dir = Join-Path $resolved_working_directory $LogDir
$run_dir = Join-Path $resolved_log_dir $run_id
$stdout_path = Join-Path $run_dir "stdout.log"
$stderr_path = Join-Path $run_dir "stderr.log"
$combined_path = Join-Path $run_dir "combined.log"

New-Item -ItemType Directory -Path $run_dir -Force | Out-Null

$start = Get-Date
$start_info = New-Object System.Diagnostics.ProcessStartInfo
$shell_executable = if ($PSVersionTable.PSEdition -eq "Core") { "pwsh.exe" } else { "powershell.exe" }
$start_info.FileName = $shell_executable
$wrapped_command = "`$ProgressPreference='SilentlyContinue'; $Command"
$encoded_command = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($wrapped_command))
$start_info.Arguments = "-NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand $encoded_command"
$start_info.WorkingDirectory = $resolved_working_directory
$start_info.RedirectStandardOutput = $true
$start_info.RedirectStandardError = $true
$start_info.UseShellExecute = $false
$start_info.CreateNoWindow = $true

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $start_info
$null = $process.Start()

$timed_out = -not $process.WaitForExit($TimeoutSec * 1000)
if ($timed_out) {
	taskkill /PID $process.Id /T /F | Out-Null
}

$stdout_text = $process.StandardOutput.ReadToEnd()
$stderr_text = $process.StandardError.ReadToEnd()

Set-Content -Path $stdout_path -Value $stdout_text
Set-Content -Path $stderr_path -Value $stderr_text

$end = Get-Date
$duration_ms = [int][Math]::Round(($end - $start).TotalMilliseconds)
$exit_code = if ($timed_out) {
	124
} elseif ($null -eq $process.ExitCode) {
	0
} else {
	[int]$process.ExitCode
}


$combined_lines = @()
if ($stdout_text.Length -gt 0) {
	$combined_lines += "=== STDOUT ==="
	$combined_lines += $stdout_text.TrimEnd("`r", "`n")
}
if ($stderr_text.Length -gt 0) {
	if ($combined_lines.Count -gt 0) {
		$combined_lines += ""
	}
	$combined_lines += "=== STDERR ==="
	$combined_lines += $stderr_text.TrimEnd("`r", "`n")
}
Set-Content -Path $combined_path -Value ($combined_lines -join "`r`n")

if ($PrintOutput) {
	if ($stdout_text.Length -gt 0) {
		Write-Output $stdout_text.TrimEnd("`r", "`n")
	}
	if ($stderr_text.Length -gt 0) {
		Write-Output $stderr_text.TrimEnd("`r", "`n")
	}
}

$preview_text = [string](Get-Content -Raw -Path $combined_path)
if ($PreviewChars -gt 0 -and $preview_text.Length -gt $PreviewChars) {
	$preview_text = $preview_text.Substring(0, $PreviewChars)
}
if ($PreviewChars -eq 0) {
	$preview_text = ""
}

$result = [ordered]@{
	command = $Command
	start_time_utc = $start.ToUniversalTime().ToString("o")
	end_time_utc = $end.ToUniversalTime().ToString("o")
	duration_ms = $duration_ms
	timeout_sec = $TimeoutSec
	timed_out = $timed_out
	exit_code = $exit_code
	run_id = $run_id
	working_directory = $resolved_working_directory
	log_dir = $run_dir
	stdout_path = $stdout_path
	stderr_path = $stderr_path
	combined_path = $combined_path
	output_preview = $preview_text
}

$json = $result | ConvertTo-Json -Depth 4 -Compress
Write-Output $json

exit $exit_code
