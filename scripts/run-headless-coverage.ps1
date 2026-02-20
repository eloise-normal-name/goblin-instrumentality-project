param(
	[string]$BuildConfig = "Debug",
	[ValidateSet("headless", "shader-cache-miss", "custom")]
	[string]$Scenario = "headless",
	[string]$AppArgs = "",
	[string]$OutputDir = "artifacts/coverage",
	[int]$BuildTimeoutSec = 120,
	[switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repo_root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$output_path = Join-Path $repo_root $OutputDir
New-Item -ItemType Directory -Path $output_path -Force | Out-Null

function Clear-ShaderCache {
	param(
		[string]$RepoRoot,
		[string]$Config
	)

	$shader_files = @("mesh_vs.cso", "mesh_ps.cso")
	$cache_dirs = @(
		(Join-Path $RepoRoot "src/shaders"),
		(Join-Path $RepoRoot "bin/$Config/shaders")
	)

	foreach ($cache_dir in $cache_dirs) {
		foreach ($shader_file in $shader_files) {
			$cache_path = Join-Path $cache_dir $shader_file
			if (Test-Path $cache_path) {
				Remove-Item $cache_path -Force
			}
		}
	}
}

if (-not $SkipBuild) {
	$build_command = "cmake --build build --config $BuildConfig"
	$build_result_json = powershell -ExecutionPolicy Bypass -File (Join-Path $repo_root "scripts/agent-wrap.ps1") -Command $build_command -TimeoutSec $BuildTimeoutSec -WorkingDirectory $repo_root
	if ($LASTEXITCODE -ne 0) {
		throw "Build failed with exit code $LASTEXITCODE while running: $build_command"
	}

	$build_result = $null
	try {
		$build_result = ($build_result_json | Select-Object -Last 1 | ConvertFrom-Json)
	} catch {
		throw "Failed to parse build wrapper output as JSON. Raw output: $build_result_json"
	}

	if ($null -eq $build_result -or $build_result.exit_code -ne 0) {
		throw "Build wrapper reported failure. Output preview: $($build_result.output_preview)"
	}
}

$exe_path = Join-Path $repo_root "bin/$BuildConfig/goblin-stream.exe"
if (-not (Test-Path $exe_path)) {
	throw "Executable not found: $exe_path"
}

$vswhere_candidates = @(
	(Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"),
	(Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe")
)
$vswhere_path = $vswhere_candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($vswhere_path)) {
	throw "vswhere.exe not found in standard install locations."
}

$vs_install_path = & $vswhere_path -latest -products * -prerelease -property installationPath | Select-Object -First 1
if (-not [string]::IsNullOrWhiteSpace($vs_install_path)) {
	$vs_install_path = $vs_install_path.Trim()
}
if ([string]::IsNullOrWhiteSpace($vs_install_path) -or -not (Test-Path $vs_install_path)) {
	throw "Visual Studio installation path not found via vswhere."
}

$coverage_console = Join-Path $vs_install_path "Common7\IDE\Extensions\Microsoft\CodeCoverage.Console\Microsoft.CodeCoverage.Console.exe"
if (-not (Test-Path $coverage_console)) {
	throw "Coverage console not found: $coverage_console"
}

$runsettings_path = Join-Path $repo_root "scripts/coverage.runsettings"
if (-not (Test-Path $runsettings_path)) {
	throw "Runsettings file not found: $runsettings_path"
}

$effective_app_args = $AppArgs
switch ($Scenario) {
	"headless" {
		if ([string]::IsNullOrWhiteSpace($effective_app_args)) {
			$effective_app_args = "--headless"
		}
	}
	"shader-cache-miss" {
		if ([string]::IsNullOrWhiteSpace($effective_app_args)) {
			$effective_app_args = "--headless"
		}
		Clear-ShaderCache -RepoRoot $repo_root -Config $BuildConfig
	}
	"custom" {
	}
}

$scenario_slug = ($Scenario.ToLowerInvariant() -replace "[^a-z0-9]+", "-").Trim("-")
if ([string]::IsNullOrWhiteSpace($scenario_slug)) {
	$scenario_slug = "coverage"
}

$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$coverage_file = Join-Path $output_path "$scenario_slug`_$stamp.coverage"
$cobertura_file = Join-Path $output_path "$scenario_slug`_$stamp.cobertura.xml"
$summary_file = Join-Path $output_path "$scenario_slug`_$stamp.summary.txt"
$html_file = Join-Path $output_path "$scenario_slug`_$stamp.report.html"

$target = if ([string]::IsNullOrWhiteSpace($effective_app_args)) {
	"`"$exe_path`""
} else {
	"`"$exe_path`" $effective_app_args"
}

& $coverage_console collect --settings $runsettings_path --output $coverage_file --output-format coverage $target
if ($LASTEXITCODE -ne 0) {
	throw "CodeCoverage collect failed with exit code $LASTEXITCODE"
}

& $coverage_console merge $coverage_file --output $cobertura_file --output-format cobertura
if ($LASTEXITCODE -ne 0) {
	throw "CodeCoverage merge failed with exit code $LASTEXITCODE"
}

[xml]$coverage_xml = Get-Content -Raw -Path $cobertura_file
$overall_line_rate = [double]$coverage_xml.coverage.'line-rate'
$overall_line_pct = [Math]::Round($overall_line_rate * 100.0, 2)
$lines_covered = [int]$coverage_xml.coverage.'lines-covered'
$lines_valid = [int]$coverage_xml.coverage.'lines-valid'

$class_nodes = @($coverage_xml.coverage.packages.package.classes.class)
$class_rows = @()
foreach ($class_node in $class_nodes) {
	$filename = [string]$class_node.filename
	$line_rate = [double]$class_node.'line-rate'
	$line_pct = [Math]::Round($line_rate * 100.0, 2)
	if ($filename.StartsWith($repo_root, [System.StringComparison]::OrdinalIgnoreCase)) {
		$relative_path = $filename.Replace($repo_root + "\", "").Replace("\", "/")
		$class_rows += [PSCustomObject]@{
			Path = $relative_path
			LinePct = $line_pct
		}
	}
}

$uncovered_rows = $class_rows |
	Group-Object Path |
	ForEach-Object {
		[PSCustomObject]@{
			Path = $_.Name
			LinePct = ($_.Group | Measure-Object -Property LinePct -Minimum).Minimum
		}
	} |
	Sort-Object LinePct, Path |
	Select-Object -First 25
$summary_lines = @(
	"$Scenario Coverage Summary",
	"Generated: $(Get-Date -Format o)",
	"Scenario: $Scenario",
	"App args: $effective_app_args",
	"Coverage file: $coverage_file",
	"Cobertura file: $cobertura_file",
	"",
	"Overall line coverage: $overall_line_pct% ($lines_covered/$lines_valid)",
	"",
	"Lowest-coverage files (top 25):"
)
foreach ($row in $uncovered_rows) {
	$summary_lines += ("- {0,6}%  {1}" -f $row.LinePct, $row.Path)
}
Set-Content -Path $summary_file -Value $summary_lines -Encoding UTF8

$table_rows = $uncovered_rows | ForEach-Object {
	"<tr><td>$($_.LinePct)%</td><td>$($_.Path)</td></tr>"
}
$html = @"
<!doctype html>
<html lang=""en"">
<head>
<meta charset=""utf-8"" />
<title>$Scenario Coverage Report</title>
<style>
body { font-family: Segoe UI, Arial, sans-serif; margin: 24px; color: #1f2937; background: #f8fafc; }
h1 { margin-bottom: 8px; }
.meta { color: #4b5563; margin-bottom: 12px; }
.card { background: #ffffff; border: 1px solid #d1d5db; border-radius: 8px; padding: 16px; margin-bottom: 16px; }
table { border-collapse: collapse; width: 100%; background: #ffffff; }
th, td { border: 1px solid #d1d5db; padding: 8px; text-align: left; }
th { background: #eef2ff; }
</style>
</head>
<body>
<h1>$Scenario Coverage Report</h1>
<div class=""meta"">Generated: $(Get-Date -Format o)</div>
<div class=""card"">
<div><strong>Overall line coverage:</strong> $overall_line_pct% ($lines_covered/$lines_valid)</div>
<div><strong>Scenario:</strong> $Scenario</div>
<div><strong>App args:</strong> $effective_app_args</div>
<div><strong>Coverage file:</strong> $coverage_file</div>
<div><strong>Cobertura file:</strong> $cobertura_file</div>
</div>
<div class=""card"">
<h2>Lowest-Coverage Files (Top 25)</h2>
<table>
<thead><tr><th>Line Coverage</th><th>File</th></tr></thead>
<tbody>
$(($table_rows -join "`n"))
</tbody>
</table>
</div>
</body>
</html>
"@
Set-Content -Path $html_file -Value $html -Encoding UTF8

Write-Output "Coverage file: $coverage_file"
Write-Output "Cobertura file: $cobertura_file"
Write-Output "Summary file: $summary_file"
Write-Output "HTML report: $html_file"
