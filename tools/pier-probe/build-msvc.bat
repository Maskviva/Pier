@echo off
rem build-msvc.bat — the whole build, without a build system.
rem
rem This mod includes one engine header and links no library, so a build system has
rem nothing to decide. Anything that needs a project file, a package manager or a
rem toolchain plugin to compile four files against one header is describing itself, not
rem the contract.
rem
rem Run from a Visual Studio Developer Command Prompt, or call vcvars64.bat first:
rem   "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
rem
rem Then:
rem   build-msvc.bat
rem
rem Output: pier_probe.dll next to this script.

setlocal

if "%INCLUDE%"=="" (
    echo INCLUDE is not set, so cl.exe cannot find the standard library.
    echo Run this from a Visual Studio Developer Command Prompt, or call vcvars64.bat first.
    exit /b 1
)

cl /nologo /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I include ^
   /I ..\..\packages\pier-abi\include ^
   src\Probe.cpp src\Census.cpp src\ReadOnly.cpp src\Entry.cpp ^
   /Fe:pier_probe.dll ^
   /Fo:obj\ ^
   /link /DLL

if errorlevel 1 (
    echo build failed
    exit /b 1
)

echo.
echo built pier_probe.dll
echo copy it next to manifest.json into plugins\pier-probe\ on the server
endlocal
