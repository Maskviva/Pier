# C++ 绑定

C++ 直接对着 ABI 头文件写。中间没有 SDK 层，因为这一侧没什么可做的：`sdk/abi.h` 本身就是 C、
本身就是契约，而且所有其他绑定都是从它生成的。

这是一笔交换。Rust 给你 `Result`、RAII，每个槽位都有检查过的封装；C++ 给你没有任何遮挡的契约，
SDK 本来会替你做的四件事归你自己做：

| | Rust | C++ |
|---|---|---|
| 入口点 | 宏生成 | 自己写 `pier_main` |
| 能力检查 | 每个封装里都有 | 每个调用点自己比对 `struct_size` |
| 字符串 | `&str` 转换 | 自己构造 `PierStr` |
| 错误 | `Result<T>` | `bool` 返回值加出参 |

模组大部分工作在引擎侧、ABI 只占一小块，或者它得待在一个已经是 C++ 的代码库里——选 C++。
ABI 就是模组的主要内容——选 [Rust](/zh/rust/)。

## 构建一个需要什么

一个头文件，不链库，不需要构建系统：

```
cl /std:c++20 /EHsc /utf-8 /LD /I <pier>/packages/pier-abi/include ^
   src\Main.cpp /Fe:my_mod.dll
```

仓库里的 `examples/hello-pier-cpp` 就是这个模组，旁边有四个构建文件——MSVC、clang-cl、
CMake、xmake——产出同一个 DLL。用你项目已经在用的那个，源码不因此改变。

编译器必须产出 **MSVC ABI 的 x64** 二进制。Windows 上就是 `cl` 或 `clang-cl`。
GNU 驱动的 `clang++` 和 MinGW 的 `g++` 编得过、链得上，然后加载不了——这是个很慢才发现的错误。

## 最小的模组

```cpp
#include <cstring>
#include "sdk/abi.h"

namespace {
    PierApi const* gApi = nullptr;
    PierModHandle gSelf = nullptr;

    PierStr str(char const* s) { return PierStr{s, std::strlen(s)}; }

    bool onEnable(void*) {
        gApi->log(gSelf, 3, str("enabled"));
        return true;
    }
    bool onDisable(void*) { return true; }
    bool onUnload(void*)  { return true; }
}

PIER_MAIN_EXPORT bool pier_main(PierApi const* api, PierModHandle self,
                                PierModVTable* out) {
    if (api == nullptr || out == nullptr) return false;
    gApi = api;
    gSelf = self;

    out->struct_size = sizeof(PierModVTable);
    out->abi_version = PIER_ABI_VERSION;
    out->mod_flags = 0;
    out->_reserved0 = 0;

    out->instance = nullptr;
    out->on_enable = &onEnable;
    out->on_disable = &onDisable;
    out->on_unload = &onUnload;
    return true;
}
```

里面有三处不是可选的。

**`PIER_MAIN_EXPORT`，不是裸的 `extern "C"`。** 给符号起名不等于把它导出。
Windows 的 DLL 不主动要求就什么都不导出，光声明能干净编过，加载时才被拒，报
"does not export pier_main"。ELF 构建默认导出它，所以在 Linux 上测不出来。

**那四个头部标量。** 宿主读 `struct_size` 才知道能碰你这张表的多少，这正是你对它的表做的事情的镜像。
留零就等于告诉宿主这张表长度为零。

**`log` 调用前不做能力检查**，而且只有 `log` 这样。它是固定头部里四个槽位之一，任何宿主都有。
头部之后的一切都要做下面这个检查。

## 调用前先检查能力

`PierApi` 只增不减。比你编译时用的头文件旧的宿主，那张表更短、提前结束，
所以读一个超出末尾的槽位读的是宿主从没写过的内存。

```cpp
bool has(std::size_t end) {
    return gApi != nullptr && gApi->struct_size >= end;
}

#define HAS_SLOT(m) (has(offsetof(PierApi, m) + sizeof(void*)) && gApi->m != nullptr)

if (HAS_SLOT(register_command)) {
    gApi->register_command(gSelf, str("hello"), str("Says hello."), 0, &onHello, nullptr);
}
```

槽位不可用有两种原因，都属正常：

- **超出 `struct_size`** —— 宿主比这个能力还早。
- **为 NULL** —— 宿主够新，但构建时没编进填这个槽位的包。
  `pier-dimensions` 和 `pier-lane` 是可裁的，客户端构建填的是另一组。
  NULL 不是要上报的 bug，是一个要学会没有它也能过的能力。

`tools/pier-probe` 是一个把这两种原因逐槽位报出来的模组。

## 字符串

`PierStr` 是 `{指针, 长度}`，**不保证以 NUL 结尾**。传进你回调的字符串归调用方所有，
只在那一次调用内有效：想留就自己拷贝。

```cpp
std::string_view view(PierStr s) {
    return s.ptr == nullptr ? std::string_view{} : std::string_view{s.ptr, s.len};
}
PierStr wrap(std::string const& s) { return PierStr{s.data(), s.size()}; }
```

模组通过 sink 回调在当前调用帧内把字符串交出去。所有权在两个方向上都不跨越边界；
这个 ABI 里没有任何一个调用会返回一个「对方必须负责释放」的指针。

## 错误

会失败的槽位返回 `bool`，值放进出参。false 表示宿主没有答案，
而且它**不区分**「没找到」和「这个引擎版本不暴露该属性」。两者都是结果、不是错误，宿主也都不打日志。

false 和「结果是 0」是两个不同的答案。把它们合并正是契约 §5.2 要防的那个错误。

## 接着看

- [第一个模组](/zh/cpp/first-mod) —— 从空目录到服务器能加载的 DLL。
- [ABI](/zh/guide/abi) —— 契约本身，这一侧你是直接读它的。
- [Rust 绑定](/zh/rust/) —— 同样的能力，上面表格那四栏由它替你填好。
