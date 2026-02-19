param(
	[string]$SourceDir = ".",
	[string]$BuildDir = "build",
	[string]$Generator = "Visual Studio 18 2026",
	[string]$Architecture = "x64",
	[switch]$ForceClean,
	[switch]$NoRetry
)

$ErrorActionPreference = "Stop"

$resolved_source = (Resolve-Path $SourceDir -ErrorAction Stop).Path
$resolved_build = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
	$BuildDir
} else {
	Join-Path (Get-Location).Path $BuildDir
}

function Clear-CmakeCache {
	param(
		[string]$BuildPath
	)

	$cache_file = Join-Path $BuildPath "CMakeCache.txt"
	$cache_dir = Join-Path $BuildPath "CMakeFiles"

	if (Test-Path $cache_file) {
		Remove-Item $cache_file -Force
	}

	if (Test-Path $cache_dir) {
		Remove-Item $cache_dir -Recurse -Force
	}
}

function Invoke-Configure {
	param(
		[string]$SourcePath,
		[string]$BuildPath,
		[string]$SelectedGenerator,
		[string]$SelectedArchitecture
	)

	& cmake -G $SelectedGenerator -A $SelectedArchitecture -S $SourcePath -B $BuildPath
	$script:last_cmake_exit_code = [int]$LASTEXITCODE
}

if ($ForceClean) {
	Clear-CmakeCache -BuildPath $resolved_build
}

Invoke-Configure -SourcePath $resolved_source -BuildPath $resolved_build -SelectedGenerator $Generator -SelectedArchitecture $Architecture
$exit_code = $script:last_cmake_exit_code
if ($exit_code -eq 0) {
	Write-Output "CMake configure succeeded."
	exit 0
}

if ($NoRetry) {
	throw "CMake configure failed with exit code $exit_code"
}

Write-Output "CMake configure failed with exit code $exit_code. Clearing CMake cache and retrying once."
Clear-CmakeCache -BuildPath $resolved_build

Invoke-Configure -SourcePath $resolved_source -BuildPath $resolved_build -SelectedGenerator $Generator -SelectedArchitecture $Architecture
$retry_exit_code = $script:last_cmake_exit_code
if ($retry_exit_code -ne 0) {
	throw "CMake configure failed after cache reset with exit code $retry_exit_code"
}

Write-Output "CMake configure succeeded after cache reset."
