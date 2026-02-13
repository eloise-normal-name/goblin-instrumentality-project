@echo off
REM Script to run code coverage using OpenCppCoverage
REM Requires OpenCppCoverage to be installed and in PATH

setlocal enabledelayedexpansion

REM Configuration
set "SOURCE_DIR=src"
set "EXECUTABLE=bin\Debug\goblin-stream.exe"
set "REPORT_DIR=coverage_report"
set "REPORT_FORMAT=html"

REM Check if OpenCppCoverage is available
where OpenCppCoverage >nul 2>&1
if %ERRORLEVEL% neq 0 (
	echo ERROR: OpenCppCoverage not found in PATH
	echo Please install OpenCppCoverage from:
	echo https://github.com/OpenCppCoverage/OpenCppCoverage/releases
	exit /b 1
)

REM Check if executable exists
if not exist "%EXECUTABLE%" (
	echo ERROR: Executable not found: %EXECUTABLE%
	echo Please build the project first with:
	echo   cmake -B build -G "Visual Studio 18 2026" -A x64
	echo   cmake --build build --config Debug
	exit /b 1
)

echo Running code coverage...
echo Source directory: %SOURCE_DIR%
echo Executable: %EXECUTABLE%
echo Report directory: %REPORT_DIR%
echo.

REM Clean previous coverage reports
if exist "%REPORT_DIR%" (
	echo Cleaning previous coverage reports...
	rmdir /s /q "%REPORT_DIR%"
)

REM Run coverage
OpenCppCoverage ^
	--sources "%SOURCE_DIR%\*" ^
	--export_type %REPORT_FORMAT%:%REPORT_DIR% ^
	--excluded_sources "*\include\*" ^
	--excluded_sources "*\build\*" ^
	-- "%EXECUTABLE%"

if %ERRORLEVEL% equ 0 (
	echo.
	echo Coverage report generated successfully!
	echo Open: %REPORT_DIR%\index.html
	echo.
	REM Try to open report in default browser
	if exist "%REPORT_DIR%\index.html" (
		start "" "%REPORT_DIR%\index.html"
	)
) else (
	echo.
	echo ERROR: Coverage run failed with error code %ERRORLEVEL%
	exit /b %ERRORLEVEL%
)

endlocal
