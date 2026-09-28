@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM  build_and_run.bat - Build MPI bucket sort on Windows
REM  Uses 8.3 DOS short names to avoid space issues
REM ============================================================

echo ==========================================
echo  159.735 Assignment 2 - Parallel Bucket Sort
echo  Windows Build
echo ==========================================
echo.

REM --- Check for g++ ---
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: g++ not found
    echo Install MSYS2 and add C:\msys64\mingw64\bin to PATH
    pause
    exit /b 1
)

echo Compiler: g++ (MSYS2/MinGW64)
echo.

REM --- Use 8.3 short names for MS-MPI paths ---
set "MSMPI_INC=C:\PROGRA~2\MICROS~2\MPI\Include"
set "MSMPI_LIB=C:\PROGRA~2\MICROS~2\MPI\Lib\x64"

REM --- Verify paths exist ---
if not exist "!MSMPI_INC!" (
    echo ERROR: MS-MPI Include not found
    echo Reinstall: msmpisdk.msi from
    echo   https://github.com/microsoft/Microsoft-MPI/releases/latest
    pause
    exit /b 1
)

echo MS-MPI Include: !MSMPI_INC!
echo MS-MPI Lib:     !MSMPI_LIB!
echo.

REM --- Build bucketsort.exe ---
echo [1/2] Building bucketsort.exe ...
g++ -O2 -std=c++11 -I"!MSMPI_INC!" -L"!MSMPI_LIB!" ..\src\bucketsort.cpp -o bucketsort.exe -lmsmpi -lws2_32

if !ERRORLEVEL! NEQ 0 (
    echo Build FAILED for bucketsort.exe
    pause
    exit /b 1
)
echo   OK
echo.

REM --- Build seq_bucketsort.exe ---
echo [2/2] Building seq_bucketsort.exe ...
g++ -O2 -std=c++11 ..\src\seq_bucketsort.cpp -o seq_bucketsort.exe

if !ERRORLEVEL! NEQ 0 (
    echo Build FAILED for seq_bucketsort.exe
    pause
    exit /b 1
)
echo   OK
echo.

echo ==========================================
echo  Build successful! Running tests...
echo ==========================================
echo.

echo --- Sequential reference (N=1,000,000) ---
seq_bucketsort.exe 1000000

echo.
echo --- 1 process ---
mpiexec -n 1 bucketsort.exe 1000000

echo.
echo --- 2 processes ---
mpiexec -n 2 bucketsort.exe 1000000

echo.
echo --- 4 processes ---
mpiexec -n 4 bucketsort.exe 1000000

echo.
echo Done! Record the Total time for Amdahl's Law.
pause
