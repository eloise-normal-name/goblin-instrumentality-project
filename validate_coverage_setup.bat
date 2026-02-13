@echo off
REM Validation script for code coverage setup
REM Checks that all required files and tools are in place

echo ===================================
echo Code Coverage Setup Validation
echo ===================================
echo.

set "ERROR_COUNT=0"

REM Check 1: Documentation exists
echo [1/6] Checking documentation...
if exist "docs\CODE_COVERAGE.md" (
	echo   [OK] docs\CODE_COVERAGE.md found
) else (
	echo   [ERROR] docs\CODE_COVERAGE.md not found
	set /a ERROR_COUNT+=1
)

REM Check 2: Coverage scripts exist
echo [2/6] Checking coverage scripts...
if exist "run_coverage.bat" (
	echo   [OK] run_coverage.bat found
) else (
	echo   [ERROR] run_coverage.bat not found
	set /a ERROR_COUNT+=1
)

if exist "run_coverage.ps1" (
	echo   [OK] run_coverage.ps1 found
) else (
	echo   [ERROR] run_coverage.ps1 not found
	set /a ERROR_COUNT+=1
)

REM Check 3: Configuration file exists
echo [3/6] Checking configuration...
if exist ".opencppcoverage" (
	echo   [OK] .opencppcoverage found
) else (
	echo   [ERROR] .opencppcoverage not found
	set /a ERROR_COUNT+=1
)

REM Check 4: GitHub Actions workflow exists
echo [4/6] Checking GitHub Actions workflow...
if exist ".github\workflows\coverage.yml" (
	echo   [OK] .github\workflows\coverage.yml found
) else (
	echo   [ERROR] .github\workflows\coverage.yml not found
	set /a ERROR_COUNT+=1
)

REM Check 5: OpenCppCoverage availability
echo [5/6] Checking OpenCppCoverage...
where OpenCppCoverage >nul 2>&1
if %ERRORLEVEL% equ 0 (
	echo   [OK] OpenCppCoverage found in PATH
	for /f "delims=" %%i in ('where OpenCppCoverage') do echo   Location: %%i
) else (
	echo   [WARNING] OpenCppCoverage not found in PATH
	echo   Install from: https://github.com/OpenCppCoverage/OpenCppCoverage/releases
	echo   This is optional for development but required to run coverage
)

REM Check 6: CMakeLists.txt has debug symbols configuration
echo [6/6] Checking CMakeLists.txt...
findstr /C:"Debug symbols for coverage" CMakeLists.txt >nul 2>&1
if %ERRORLEVEL% equ 0 (
	echo   [OK] CMakeLists.txt has coverage configuration
) else (
	echo   [WARNING] CMakeLists.txt may not have coverage configuration
)

echo.
echo ===================================
if %ERROR_COUNT% equ 0 (
	echo Result: All checks passed!
	echo Code coverage infrastructure is ready.
) else (
	echo Result: %ERROR_COUNT% error(s) found
	echo Please fix the errors above.
)
echo ===================================
exit /b %ERROR_COUNT%
