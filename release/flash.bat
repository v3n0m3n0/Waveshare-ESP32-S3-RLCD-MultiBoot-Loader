@echo off
setlocal
chcp 65001 > nul

set PORT=%1
if "%PORT%"=="" (
    set /p PORT="Enter COM port (e.g. COM11): "
)

if "%PORT%"=="" (
    echo [ERROR] No COM port provided.
    exit /b 1
)

set SCRIPT_DIR=%~dp0
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%flash.ps1" -Port %PORT%
pause
