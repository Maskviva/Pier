@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

F:

cd F:\project\pier

set VCPKG_ROOT=
set CONAN_HOME=
set HTTPS_PROXY=http://127.0.0.1:7890
set HTTP_PROXY=http://127.0.0.1:7890

@REM echo xmake.exe repo -u
@REM xmake.exe repo -u

echo xmake.exe f -c -m release -y -v
xmake.exe f -c -m release -y -v

echo xmake.exe -r
xmake.exe -r

echo.
echo Press any key to exit...
pause >nul
exit /b 0