@echo off
:: Tankoban 3 - build then launch (double-click friendly).
:: Closes any running instance, builds Release via build.bat, then launches
:: out\Tankoban3.exe. On build failure it pauses so you can read the error.
setlocal
set "HERE=%~dp0"

:: Always launch the real app (Home -> Detail -> Play Picker), never the dev-only
:: standalone player demo. (main.cpp opens the demo if TANKOBAN3_PLAYER_DEMO merely
:: EXISTS, even empty; clearing it here guarantees the normal shell.)
set "TANKOBAN3_PLAYER_DEMO="
set "TANKOBAN3_PLAYER_DEMO_SUB="

echo Closing any running Tankoban 3...
taskkill /F /IM Tankoban3.exe >nul 2>&1

echo Building (this can take a couple of minutes the first time)...
call "%HERE%build.bat"
if errorlevel 1 (
    echo.
    echo BUILD FAILED - not launching.  See out\_build.log for details.
    echo.
    pause
    exit /b 1
)

echo Launching Tankoban 3...
start "" "%HERE%out\Tankoban3.exe"
exit /b 0
