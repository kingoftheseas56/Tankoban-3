@echo off
setlocal
:: Stage libmpv (headers + MSVC import lib + runtime dll) into third_party\libmpv\.
:: Source = Harbor's matched set (import lib + headers) + a proven libmpv-2.dll.
:: Idempotent: re-running is safe; the (large) dll is only copied if absent.
set "DST=%~dp0third_party\libmpv"
set "HARBOR=%TEMP%\harbor-ref\src-tauri\libmpv"
set "DLL_SRC=%USERPROFILE%\Desktop\Tankoban Electron\resources\mpv\windows\libmpv-2.dll"

if not exist "%DST%\include\mpv" mkdir "%DST%\include\mpv"
if not exist "%DST%\lib" mkdir "%DST%\lib"
if not exist "%DST%\bin" mkdir "%DST%\bin"

copy /Y "%HARBOR%\include\mpv\*.h" "%DST%\include\mpv\" >nul
copy /Y "%HARBOR%\mpv.lib" "%DST%\lib\mpv.lib" >nul
if not exist "%DST%\bin\libmpv-2.dll" copy /Y "%DLL_SRC%" "%DST%\bin\libmpv-2.dll" >nul

if not exist "%DST%\lib\mpv.lib" ( echo SETUP_LIBMPV: mpv.lib missing & exit /b 2 )
if not exist "%DST%\bin\libmpv-2.dll" ( echo SETUP_LIBMPV: libmpv-2.dll missing & exit /b 3 )
if not exist "%DST%\include\mpv\render_gl.h" ( echo SETUP_LIBMPV: headers missing & exit /b 4 )
echo SETUP_LIBMPV OK
exit /b 0
