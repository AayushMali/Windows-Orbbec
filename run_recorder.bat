@echo off
SETLOCAL EnableDelayedExpansion

:: --- CONFIGURATION ---
:: Path to your compiled executable
SET RECORDER_EXE=build\Release\ob_batch_recorder.exe
:: Duration of each batch in seconds (60 = 1 minute)
SET DURATION=60
:: Output directory
SET OUTPUT_DIR=recordings

:: Create output dir if it doesn't exist
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo ====================================================
echo   ORBBEC WINDOWS RECORDER (1-Minute Batches)
echo ====================================================

if not exist "%RECORDER_EXE%" (
    echo [ERROR] %RECORDER_EXE% not found. 
    echo Please build the project first using CMake and Visual Studio.
    pause
    exit /b
)

echo Starting recorder...
echo Batch Duration: %DURATION%s
echo Saving to: %OUTPUT_DIR%
echo.
echo [Tip] On your i3 laptop, add --no-gui to the command below for better performance.

"%RECORDER_EXE%" --duration %DURATION% --output "%OUTPUT_DIR%"

pause
