@echo off
rem build-clang.bat — the same build under clang-cl.
rem
rem clang-cl takes the MSVC driver flags and produces an MSVC-ABI binary, which is what a
rem BDS mod has to be. Plain clang++ with the GNU driver does not, so it is not an option
rem on Windows however familiar the flags look.
rem
rem Needs a Visual Studio Developer Command Prompt as well: clang-cl finds the Windows
rem SDK and the MSVC standard library through the same INCLUDE and LIB variables cl does.
rem
rem   build-clang.bat
rem
rem Output: pier_probe.dll next to this script.

setlocal

if "%INCLUDE%"=="" (
    echo INCLUDE is not set. clang-cl reads the standard library headers from it, the
    echo same as cl.exe. Run this from a Visual Studio Developer Command Prompt.
    exit /b 1
)

where clang-cl >nul 2>&1
if errorlevel 1 (
    echo clang-cl is not on PATH. Add the bin directory of an LLVM install, for example
    echo   set "PATH=D:\Compiler\clang+llvm-21.1.8-x86_64-pc-windows-msvc\bin;%%PATH%%"
    exit /b 1
)

clang-cl /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I include ^
   /I ..\..\packages\pier-abi\include ^
   src\Probe.cpp src\Census.cpp src\ReadOnly.cpp src\Entry.cpp ^
   /Fe:pier_probe.dll ^
   /Fo:obj\

if errorlevel 1 (
    echo build failed
    exit /b 1
)

echo.
echo built pier_probe.dll with clang-cl
endlocal
