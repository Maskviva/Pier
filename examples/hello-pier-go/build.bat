@echo off
rem build.bat: builds hello_pier_go.dll with the Go toolchain and a MinGW-w64 gcc.
rem
rem cgo compiles the binding's C side with gcc, so a MinGW-w64 gcc has to be on PATH. The
rem DLL meets Pier through C functions only, which MinGW and MSVC call the same way on x64,
rem so the MSVC-ABI requirement of a C++ mod does not apply here.

setlocal

where go >nul 2>&1
if errorlevel 1 (
    echo go is not on PATH. Install Go 1.21 or later.
    exit /b 1
)
where gcc >nul 2>&1
if errorlevel 1 (
    echo gcc is not on PATH. Install a MinGW-w64 gcc, for example through MSYS2.
    exit /b 1
)

set CGO_ENABLED=1
go build -buildmode=c-shared -trimpath -ldflags "-s -w" -o hello_pier_go.dll .
if errorlevel 1 exit /b 1

echo Built hello_pier_go.dll. Copy it and manifest.json into plugins\hello-pier-go\
