# CMakeLists.txt — one of four build files that produce the same DLL.
#
# There is nothing Pier-specific in here. A mod is a shared library that includes one
# header and exports one symbol, so this is what any CMake project looks like.
#
#   cmake -B build -A x64
#   cmake --build build --config Release
#
# Output: build/Release/hello_pier_cpp.dll
#
# On Windows this uses MSVC by default. For clang, add -T ClangCL to the configure step:
# that is the clang-cl toolset, which produces an MSVC-ABI binary. A GNU-driver clang++
# does not, and a mod built that way cannot load.

cmake_minimum_required(VERSION 3.20)
project(hello_pier_cpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(hello_pier_cpp SHARED src/Main.cpp)

# The one dependency. A relative path rather than a package, so the example builds
# against the abi.h sitting in this repository and a change to the contract shows up on
# the next build rather than the next release.
target_include_directories(hello_pier_cpp PRIVATE ../../packages/pier-abi/include)

# The DLL name has to match the "entry" field of manifest.json.
set_target_properties(hello_pier_cpp PROPERTIES
    OUTPUT_NAME "hello_pier_cpp"
    PREFIX ""
)

if(MSVC)
    target_compile_options(hello_pier_cpp PRIVATE /EHsc /utf-8)
endif()
