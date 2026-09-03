@echo off
REM Build and launch PocketPartner. Usage: run.cmd [desktop|companion|test]
REM ponytail: hardcoded VS2022/Qt paths, parameterize when a second machine needs it.
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
set CMAKEDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
set QTBIN=C:\Qt\6.7.2\msvc2019_64\bin
set ROOT=%~dp0

REM A running copy holds its own .exe open and the link fails with LNK1104. Closing it
REM here is the difference between "it rebuilt" and a window that vanishes.
tasklist /fi "imagename eq PocketPartner.exe" | find /i "PocketPartner.exe" >nul && (
    echo Cerrando la instancia abierta de PocketPartner...
    taskkill /im PocketPartner.exe >nul 2>&1
    REM ping, not timeout: timeout aborts when stdin is redirected.
    ping -n 3 127.0.0.1 >nul
)

call "%VCVARS%" >nul || goto :failed
"%CMAKEDIR%\cmake.exe" --build "%ROOT%build" || goto :failed

set PATH=%QTBIN%;%PATH%
if "%1"=="test" ( "%CMAKEDIR%\ctest.exe" --test-dir "%ROOT%build" --output-on-failure & exit /b )
if "%1"=="companion" ( start "" "%ROOT%build\apps\companion\PocketCompanion.exe" & exit /b )
start "" "%ROOT%build\apps\desktop\PocketPartner.exe"
exit /b

:failed
echo.
echo La compilacion fallo: revisa los errores de arriba.
pause
exit /b 1
