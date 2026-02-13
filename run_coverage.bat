@echo off
REM Script to run code coverage using OpenCppCoverage
REM Requires OpenCppCoverage to be installed and in PATH
REM NOTE: This script is ready but requires tests to be implemented first.
REM      The main executable (goblin-stream.exe) is a DirectX 12 GUI app
REM      and cannot be used for automated coverage without a test harness.
REM      See docs/TEST_COVERAGE_INTEGRATION.md for guidance.

setlocal enabledelayedexpansion

echo.
echo ========================================
echo   Code Coverage - Configuration Check
echo ========================================
echo.
echo WARNING: No tests are implemented yet.
echo.
echo The coverage infrastructure is ready, but this project currently
echo has no test executable. The main application (goblin-stream.exe)
echo is a DirectX 12 GUI application that requires a GPU.
echo.
echo To use code coverage:
echo   1. Implement unit tests or a test harness
echo   2. Remove the exit statement below (line 30)
echo   3. Update the EXECUTABLE variable to point to your test executable
echo   4. Run this script again
echo.
echo See docs/TEST_COVERAGE_INTEGRATION.md for detailed guidance.
echo.
pause
REM TODO: Remove this exit statement when tests are added
exit /b 1

REM ============================================================
REM The code below is ready to use once tests are implemented
REM ============================================================

REM Configuration
REM TODO: Update this to point to your test executable when tests are added
set "SOURCE_DIR=src"
set "EXECUTABLE=bin\Debug\goblin-stream-tests.exe"
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
