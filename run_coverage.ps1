# PowerShell script to run code coverage using OpenCppCoverage
# Requires OpenCppCoverage to be installed and in PATH
# NOTE: This script is ready but requires tests to be implemented first.
#       See docs/TEST_COVERAGE_INTEGRATION.md for guidance.

param(
	[string]$SourceDir = "src",
	[string]$Executable = "bin\Debug\goblin-stream-tests.exe",
	[string]$ReportDir = "coverage_report",
	[string]$ReportFormat = "html",
	[switch]$Cobertura,
	[switch]$Help,
	[switch]$Force
)

function Show-Help {
	Write-Host @"
Usage: .\run_coverage.ps1 [options]

NOTE: No tests are implemented yet. The coverage infrastructure is configured
      but requires a test executable. See docs/TEST_COVERAGE_INTEGRATION.md.

Options:
  -SourceDir <path>     Source directory to measure coverage (default: src)
  -Executable <path>    Path to test executable (default: bin\Debug\goblin-stream-tests.exe)
  -ReportDir <path>     Output directory for reports (default: coverage_report)
  -ReportFormat <type>  Report format: html, cobertura, or binary (default: html)
  -Cobertura           Shortcut to generate Cobertura XML format
  -Force               Skip the warning about missing tests
  -Help                Show this help message

Examples (when tests are added):
  .\run_coverage.ps1 -Executable "bin\Debug\my-tests.exe"
  .\run_coverage.ps1 -Cobertura -Executable "bin\Debug\my-tests.exe"
"@
	exit 0
}

if ($Help) {
	Show-Help
}

# Warn about missing tests unless -Force is used
if (-not $Force -and $Executable -eq "bin\Debug\goblin-stream-tests.exe") {
	Write-Host ""
	Write-Host "========================================" -ForegroundColor Yellow
	Write-Host "  Code Coverage - Configuration Check" -ForegroundColor Yellow
	Write-Host "========================================" -ForegroundColor Yellow
	Write-Host ""
	Write-Host "WARNING: No tests are implemented yet." -ForegroundColor Yellow
	Write-Host ""
	Write-Host "The coverage infrastructure is ready, but this project currently"
	Write-Host "has no test executable. The default test executable path is:"
	Write-Host "  $Executable" -ForegroundColor Cyan
	Write-Host ""
	Write-Host "To use code coverage:"
	Write-Host "  1. Implement unit tests or a test harness"
	Write-Host "  2. Build your test executable"
	Write-Host "  3. Run: .\run_coverage.ps1 -Executable 'path\to\tests.exe'"
	Write-Host ""
	Write-Host "See docs/TEST_COVERAGE_INTEGRATION.md for detailed guidance." -ForegroundColor Cyan
	Write-Host ""
	Write-Host "To skip this warning and run anyway, use: -Force" -ForegroundColor DarkGray
	Write-Host ""
	exit 1
}

if ($Cobertura) {
	$ReportFormat = "cobertura"
	$ReportDir = "coverage.xml"
}

Write-Host "=== Code Coverage Runner ===" -ForegroundColor Cyan
Write-Host ""

# Check if OpenCppCoverage is available
$opencpp = Get-Command OpenCppCoverage -ErrorAction SilentlyContinue
if (-not $opencpp) {
	Write-Host "ERROR: OpenCppCoverage not found in PATH" -ForegroundColor Red
	Write-Host "Please install OpenCppCoverage from:"
	Write-Host "https://github.com/OpenCppCoverage/OpenCppCoverage/releases"
	exit 1
}

# Check if executable exists
if (-not (Test-Path $Executable)) {
	Write-Host "ERROR: Executable not found: $Executable" -ForegroundColor Red
	Write-Host ""
	Write-Host "Please build the project first with:"
	Write-Host '  cmake -B build -G "Visual Studio 18 2026" -A x64'
	Write-Host "  cmake --build build --config Debug"
	exit 1
}

Write-Host "Configuration:" -ForegroundColor Green
Write-Host "  Source directory: $SourceDir"
Write-Host "  Executable: $Executable"
Write-Host "  Report format: $ReportFormat"
Write-Host "  Report output: $ReportDir"
Write-Host ""

# Clean previous coverage reports (only for directory-based reports)
if ($ReportFormat -eq "html" -and (Test-Path $ReportDir)) {
	Write-Host "Cleaning previous coverage reports..." -ForegroundColor Yellow
	Remove-Item -Path $ReportDir -Recurse -Force
}

# Build coverage command
$coverageArgs = @(
	"--sources", "$SourceDir\*",
	"--export_type", "${ReportFormat}:${ReportDir}",
	"--excluded_sources", "*\include\*",
	"--excluded_sources", "*\build\*",
	"--", $Executable
)

Write-Host "Running coverage analysis..." -ForegroundColor Cyan
Write-Host ""

# Run coverage
$process = Start-Process -FilePath "OpenCppCoverage" -ArgumentList $coverageArgs -NoNewWindow -Wait -PassThru

if ($process.ExitCode -eq 0) {
	Write-Host ""
	Write-Host "Coverage report generated successfully!" -ForegroundColor Green
	
	if ($ReportFormat -eq "html" -and (Test-Path "$ReportDir\index.html")) {
		Write-Host "Opening: $ReportDir\index.html" -ForegroundColor Cyan
		Start-Process "$ReportDir\index.html"
	} elseif ($ReportFormat -eq "cobertura") {
		Write-Host "Cobertura XML saved to: $ReportDir" -ForegroundColor Cyan
	}
} else {
	Write-Host ""
	Write-Host "ERROR: Coverage run failed with exit code $($process.ExitCode)" -ForegroundColor Red
	exit $process.ExitCode
}
