@echo off
REM Build and launch PocketPartner. Usage: run.cmd [desktop|companion|test]
REM ponytail: hardcoded VS2022/Qt paths, parameterize when a second machine needs it.
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
set CMAKEDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin
set QTBIN=C:\Qt\6.7.2\msvc2019_64\bin
set ROOT=%~dp0

call "%VCVARS%" >nul || exit /b 1
"%CMAKEDIR%\cmake.exe" --build "%ROOT%build" || exit /b 1

set PATH=%QTBIN%;%PATH%
if "%1"=="test" ( "%CMAKEDIR%\ctest.exe" --test-dir "%ROOT%build" --output-on-failure & exit /b )
if "%1"=="companion" ( start "" "%ROOT%build\apps\companion\PocketCompanion.exe" & exit /b )
start "" "%ROOT%build\apps\desktop\PocketPartner.exe"
