@echo off
rem build-msvc.bat — the whole build, without a build system.
rem
rem A mod includes one header and links no library, so there is nothing for a build
rem system to decide. This is the shortest true description of what building a Pier mod
rem takes.
rem
rem Run from a Visual Studio Developer Command Prompt, or call vcvars64.bat first:
rem   "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

setlocal

if "%INCLUDE%"=="" (
    echo INCLUDE is not set, so cl.exe cannot find the standard library.
    echo Run this from a Visual Studio Developer Command Prompt, or call vcvars64.bat first.
    exit /b 1
)

if not exist obj mkdir obj

cl /nologo /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I ..\..\packages\pier-abi\include ^
   src\Main.cpp ^
   /Fe:hello_pier_cpp.dll ^
   /Fo:obj\

if errorlevel 1 (
    echo build failed
    exit /b 1
)

echo.
echo built hello_pier_cpp.dll
echo copy it next to manifest.json into plugins\hello-pier-cpp\ on the server
endlocal
