@echo off
rem build-clang.bat — the same build under clang-cl.
rem
rem clang-cl and not clang++. A BDS mod has to be an MSVC-ABI binary, and on Windows only
rem the clang-cl driver produces one; the GNU-driver clang++ produces something that
rem compiles, links, and cannot be loaded.
rem
rem Needs a Visual Studio Developer Command Prompt as well: clang-cl finds the Windows SDK
rem and the MSVC standard library through the same INCLUDE and LIB variables cl does.

setlocal

if "%INCLUDE%"=="" (
    echo INCLUDE is not set. clang-cl reads the standard library headers from it, the
    echo same as cl.exe. Run this from a Visual Studio Developer Command Prompt.
    exit /b 1
)

where clang-cl >nul 2>&1
if errorlevel 1 (
    echo clang-cl is not on PATH. Add the bin directory of an LLVM install, for example
    echo   set "PATH=C:\Program Files\LLVM\bin;%%PATH%%"
    exit /b 1
)

if not exist obj mkdir obj

clang-cl /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I ..\..\packages\pier-abi\include ^
   src\Main.cpp ^
   /Fe:hello_pier_cpp.dll ^
   /Fo:obj\

if errorlevel 1 (
    echo build failed
    exit /b 1
)

echo.
echo built hello_pier_cpp.dll with clang-cl
endlocal
