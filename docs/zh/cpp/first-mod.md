# 第一个 C++ 模组

从空目录到服务器能加载的 DLL。下面的内容都在 `examples/hello-pier-cpp` 里，想直接抄也可以。

## 需要什么

- 一个能产出 **MSVC ABI x64** 的编译器：Visual Studio Build Tools 的 `cl`，或 LLVM 的
  `clang-cl`。MinGW 和 GNU 驱动的 `clang++` 都不行，而且这个错误要等到服务器拒绝加载才暴露。
- 一台装好并启用了 Pier 的服务器。
- 一份仓库副本，为的是 `packages/pier-abi/include/sdk/abi.h`。那一个头文件就是全部依赖。

## 目录结构

```
my-mod/
  manifest.json
  src/Main.cpp
```

## manifest.json

```json
{
    "name": "my-mod",
    "entry": "my_mod.dll",
    "type": "pier",
    "version": "1.0.0",
    "dependencies": [
        { "name": "pier" }
    ]
}
```

`entry` 必须和你构建出的 DLL 名字**完全一致**。`type` 是 `pier`，这是告诉 LeviLamina
把文件交给 Pier 而不是自己加载。依赖 `pier` 是让装载器先启动宿主；不写的话你的模组可能在宿主存在之前就加载了。

## src/Main.cpp

见[上一页的完整代码](/zh/cpp/)，或者仓库里的 `examples/hello-pier-cpp/src/Main.cpp`。

关键在于 enable 路径在 `register_command` 缺失时做了什么：打一条 warning，然后**仍然返回 true**。
拒绝启用是错的，因为模组少了一项能力仍然能用。不能接受的是什么都不注册、也什么都不说。

## 构建

下面每一种都产出同一个 DLL。用你项目已经在用的那种。

**MSVC**，在 Visual Studio Developer Command Prompt 里：

```
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I <pier>\packages\pier-abi\include ^
   src\Main.cpp /Fe:my_mod.dll
```

**clang-cl**，同一个命令提示符，PATH 上要有 LLVM 的 `bin`：

```
clang-cl /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I <pier>\packages\pier-abi\include ^
   src\Main.cpp /Fe:my_mod.dll
```

**CMake**：

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_mod CXX)
set(CMAKE_CXX_STANDARD 20)
add_library(my_mod SHARED src/Main.cpp)
target_include_directories(my_mod PRIVATE <pier>/packages/pier-abi/include)
set_target_properties(my_mod PROPERTIES OUTPUT_NAME "my_mod" PREFIX "")
```

```
cmake -B build -A x64
cmake --build build --config Release
```

要用 clang 就在 configure 那步加 `-T ClangCL`。

**xmake**：

```lua
target("my-mod")
    set_kind("shared")
    set_languages("c++20")
    add_files("src/*.cpp")
    add_includedirs("<pier>/packages/pier-abi/include")
    set_basename("my_mod")
```

## 安装

```
plugins/my-mod/
  manifest.json
  my_mod.dll
```

启动服务器，日志里应该有你那几行：

```
[my-mod] my-mod loaded
[my-mod] my-mod enabled
```

然后进游戏打 `/hello`。

## 加载不了的时候

**"does not export pier_main"** —— 符号有名字但没导出。在定义前面加 `PIER_MAIN_EXPORT`。
这是最常见的第一个失败，而且在 Linux 上复现不出来——ELF 默认导出它。

**日志里什么都没有** —— 核对 manifest.json 的 `entry` 和 DLL 名字，
并确认文件在 `plugins/<名字>/` 目录里而不是散在 `plugins/` 下。

**能加载，但某个槽位什么都不做** —— 那个槽位可能是缺失而不是坏了。
拿 `tools/pier-probe` 对着同一个宿主跑一遍：它会把每个槽位报成存在、NULL 或超出
`struct_size`，你就知道该看自己的代码还是看宿主的构建。

## 接着看

- [ABI](/zh/guide/abi) —— 完整契约，也就是你在直接调用的东西。
- [C++ 绑定](/zh/cpp/) —— 能力检查、字符串和错误约定，一页讲完。
