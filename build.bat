@echo off
setlocal
:: Tankoban 3 build — mirrors Tankoban 2's known-good toolchain
:: (Qt6 msvc2022_64 + MSVC 2022 BuildTools + Ninja). Configures if needed,
:: builds Release, then windeployqt so both app exes launch standalone.

set "QT_DIR=C:\tools\qt6sdk\6.10.2\msvc2022_64"
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
set "PROJECT_DIR=%~dp0."
set "BUILD_DIR=%~dp0out"
set "LOG=%BUILD_DIR%\_build.log"

call "%VCVARS%" x64 >nul 2>&1
if errorlevel 1 (
    echo BUILD: MSVC env setup failed.
    exit /b 3
)

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo BUILD: configuring %BUILD_DIR%
    cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%"
    if errorlevel 1 (
        echo BUILD: CMake configure failed.
        exit /b 2
    )
)

cmake --build "%BUILD_DIR%" --config Release --target Tankoban3 Tankoban3Quick > "%LOG%" 2>&1
set BUILD_EXIT=%ERRORLEVEL%

if %BUILD_EXIT% NEQ 0 (
    echo BUILD FAILED exit=%BUILD_EXIT%
    echo --- last 30 lines of %LOG% ---
    powershell -NoProfile -Command "Get-Content -Tail 30 -LiteralPath '%LOG%'"
    exit /b %BUILD_EXIT%
)

:: Deploy Qt runtime DLLs next to the exes so they launch standalone.
"%QT_DIR%\bin\windeployqt.exe" "%BUILD_DIR%\Tankoban3.exe" >nul 2>&1
"%QT_DIR%\bin\windeployqt.exe" --qmldir "%PROJECT_DIR%\qml" "%BUILD_DIR%\Tankoban3Quick.exe" >nul 2>&1

echo BUILD OK
exit /b 0
