param(
	[string]$BaselineDir = "docs/perf-baselines",
	[string]$OutputPath = "perf-highlights.html"
)

$ErrorActionPreference = "Stop"

$repo_root = (Get-Location).Path
$baseline_path = Join-Path $repo_root $BaselineDir
$output_file = Join-Path $repo_root $OutputPath

if (-not (Test-Path $baseline_path)) {
	throw "Baseline directory not found: $baseline_path"
}

$baseline_files = Get-ChildItem -Path $baseline_path -Filter "baseline_*.json" | Sort-Object LastWriteTime

$rows = @()
$latest = $null

foreach ($file in $baseline_files) {
	$data = Get-Content -Raw -Path $file.FullName | ConvertFrom-Json
	$flags = ""
	if ($data.regression_flags) {
		$flags = ($data.regression_flags -join "; ")
	}

	$rows += [ordered]@{
		file = $file.Name
		run_label = [string]$data.run_label
		build_config = [string]$data.build_config
		cpu_wall_time_ms = [string]$data.cpu.wall_time_ms
		write_calls = [string]$data.fileio.write_calls
		regression_flags = $flags
	}

	$latest = $data
}

$total_runs = $rows.Count
$latest_label = if ($latest) { [string]$latest.run_label } else { "No baselines yet" }
$latest_config = if ($latest) { [string]$latest.build_config } else { "N/A" }
$latest_cpu = if ($latest) { [string]$latest.cpu.wall_time_ms } else { "N/A" }
$latest_writes = if ($latest) { [string]$latest.fileio.write_calls } else { "N/A" }
$generated = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")

$table_rows = ""
foreach ($row in ($rows | Sort-Object file -Descending)) {
	$table_rows += "<tr>"
	$table_rows += "<td>$($row.file)</td>"
	$table_rows += "<td>$($row.run_label)</td>"
	$table_rows += "<td>$($row.build_config)</td>"
	$table_rows += "<td>$($row.cpu_wall_time_ms)</td>"
	$table_rows += "<td>$($row.write_calls)</td>"
	$table_rows += "<td>$($row.regression_flags)</td>"
	$table_rows += "</tr>`n"
}

if ([string]::IsNullOrWhiteSpace($table_rows)) {
	$table_rows = "<tr><td colspan='6'>No baseline JSON files found.</td></tr>"
}

$html = @"
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Goblin Instrumentality - Performance Highlights</title>
	<style>
		body { font-family: Segoe UI, sans-serif; margin: 0; background: #0f172a; color: #e2e8f0; }
		main { max-width: 1100px; margin: 0 auto; padding: 24px; }
		h1 { margin: 0 0 6px; }
		.subtitle { color: #94a3b8; margin-bottom: 20px; }
		.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 10px; margin-bottom: 20px; }
		.card { border: 1px solid #334155; border-radius: 10px; padding: 12px; background: #111827; }
		.k { text-transform: uppercase; font-size: 0.8rem; color: #94a3b8; }
		.v { margin-top: 6px; font-size: 1.2rem; font-weight: 700; }
		table { width: 100%; border-collapse: collapse; background: #111827; border: 1px solid #334155; border-radius: 10px; overflow: hidden; }
		th, td { padding: 10px; border-bottom: 1px solid #1e293b; text-align: left; font-size: 0.9rem; }
		th { background: #0b1120; }
		.note { margin-top: 14px; color: #94a3b8; font-size: 0.9rem; }
	</style>
</head>
<body>
	<main>
		<h1>Performance Highlights</h1>
		<p class="subtitle">Generated $generated from docs/perf-baselines/baseline_*.json</p>

		<div class="grid">
			<div class="card"><div class="k">Total Baselines</div><div class="v">$total_runs</div></div>
			<div class="card"><div class="k">Latest Label</div><div class="v">$latest_label</div></div>
			<div class="card"><div class="k">Latest Build Config</div><div class="v">$latest_config</div></div>
			<div class="card"><div class="k">Latest CPU Wall Time</div><div class="v">$latest_cpu ms</div></div>
			<div class="card"><div class="k">Latest File Write Calls</div><div class="v">$latest_writes</div></div>
		</div>

		<table>
			<thead>
				<tr>
					<th>Baseline File</th>
					<th>Run Label</th>
					<th>Build</th>
					<th>CPU Wall Time (ms)</th>
					<th>File Write Calls</th>
					<th>Regression Flags</th>
				</tr>
			</thead>
			<tbody>
				$table_rows
			</tbody>
		</table>

		<p class="note">Publish this file to the gh-pages branch to share performance findings.</p>
	</main>
</body>
</html>
"@

Set-Content -Path $output_file -Value $html -Encoding UTF8
Write-Output "Report generated: $output_file"
