# Your first C++ mod

From an empty directory to a DLL the server loads. Everything here is in
`examples/hello-pier-cpp` if you would rather copy it.

## What you need

- A compiler that produces **MSVC-ABI x64**: `cl` from Visual Studio Build Tools, or
  `clang-cl` from LLVM. MinGW and GNU-driver `clang++` do not, and the failure only shows
  up when the server refuses to load the DLL.
- A server with Pier installed and enabled.
- A copy of the repository, for `packages/pier-abi/include/sdk/abi.h`. That one header is
  the entire dependency.

## The layout

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

`entry` has to match the DLL your build produces, exactly. `type` is `pier`, which is what
tells LeviLamina to hand the file to Pier rather than load it itself. The dependency on
`pier` is what makes the loader start Pier first; without it your mod may load before the
host exists.

## src/Main.cpp

```cpp
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

#include "sdk/abi.h"

namespace {
    PierApi const* gApi = nullptr;
    PierModHandle gSelf = nullptr;

    // PIER_FLAG_CLIENT if this DLL is meant for the client host.
    constexpr std::uint32_t kModFlags = 0;

    PierStr str(char const* s) { return PierStr{s, std::strlen(s)}; }

    void log(int level, char const* msg) {
        if (gApi != nullptr && gApi->log != nullptr) gApi->log(gSelf, level, str(msg));
    }

    bool has(std::size_t end) { return gApi != nullptr && gApi->struct_size >= end; }
#define HAS_SLOT(m) (has(offsetof(PierApi, m) + sizeof(void*)) && gApi->m != nullptr)

    void onHello(void*, PierStr args, PierStr originName, void* ctx,
                 PierStrSink outSuccess, PierStrSink outError) {
        if (outSuccess == nullptr) return;
        (void)args;
        try {
            std::string reply = "hello, ";
            reply.append(originName.ptr, originName.len);
            outSuccess(ctx, PierStr{reply.data(), reply.size()});
        } catch (...) {
            // Nothing may unwind back into the host. Log it and report it to the player.
            log(1, "/hello failed with an exception");
            if (outError != nullptr) outError(ctx, str("/hello failed"));
        }
    }

    bool onEnable(void*) {
        log(3, "my-mod enabled");
        if (!HAS_SLOT(register_command)) {
            log(2, "this host has no register_command, so /hello is not available");
            return true;
        }
        if (!gApi->register_command(gSelf, str("hello"), str("Says hello."),
                                    0, &onHello, nullptr)) {
            log(1, "registering /hello failed; another mod may already own that name");
        }
        return true;
    }

    bool onDisable(void*) { log(3, "my-mod disabled"); return true; }
    bool onUnload(void*)  { log(3, "my-mod unloaded");  return true; }
}

PIER_MAIN_EXPORT bool pier_main(PierApi const* api, PierModHandle self,
                                PierModVTable* out) {
    if (api == nullptr || out == nullptr) return false;
    if (api->struct_size < offsetof(PierApi, log) + sizeof(void*)) return false;
    gApi = api;
    gSelf = self;
    if (api->abi_version < PIER_ABI_VERSION) {
        log(1, "this host is older than the ABI this mod was built against");
        return false;
    }

    out->struct_size = sizeof(PierModVTable);
    out->abi_version = PIER_ABI_VERSION;
    out->mod_flags = kModFlags;
    out->_reserved0 = 0;

    out->instance = nullptr;
    out->on_enable = &onEnable;
    out->on_disable = &onDisable;
    out->on_unload = &onUnload;

    log(3, "my-mod loaded");
    return true;
}
```

Look at what the enable path does when `register_command` is missing: it logs at warning
level and still returns true. The mod then enables without its command, the rest of it
works, and the warning in the log says which command is missing and why.

Note also the `try` in `onHello`. Appending to a `std::string` can throw, and the host calls
`onHello` through a C function pointer, where an exception has no defined way out.
[Exceptions](index.md#exceptions) describes what happens then, and gives the shape to copy into
every callback.

## Build it

Any of these produces the same DLL. Pick the one your project already uses.

**MSVC**, from a Visual Studio Developer Command Prompt:

```
cl /nologo /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I <pier>\packages\pier-abi\include ^
   src\Main.cpp /Fe:my_mod.dll
```

**clang-cl**, from the same prompt with LLVM's `bin` on `PATH`:

```
clang-cl /std:c++20 /EHsc /utf-8 /O2 /LD ^
   /I <pier>\packages\pier-abi\include ^
   src\Main.cpp /Fe:my_mod.dll
```

**CMake**:

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

Add `-T ClangCL` to the configure step for clang.

**xmake**:

```lua
target("my-mod")
    set_kind("shared")
    set_languages("c++20")
    add_files("src/*.cpp")
    add_includedirs("<pier>/packages/pier-abi/include")
    set_basename("my_mod")
```

## Install it

```
plugins/my-mod/
  manifest.json
  my_mod.dll
```

Start the server. The log should carry these two lines, and `my-mod disabled` and `my-mod unloaded` at
shutdown:

```
[my-mod] my-mod loaded
[my-mod] my-mod enabled
```

Then in game, `/hello`.

## When it does not load

**"does not export pier_main"** — the symbol is named but not exported. Use
`PIER_MAIN_EXPORT` in front of the definition. This is the most common first failure.

**Nothing in the log at all** — check `entry` in manifest.json against the DLL name, and
check that the file is in `plugins/<name>/` rather than loose in `plugins/`.

**Loads, then a slot does nothing** — the slot may be absent rather than broken. Run
`tools/pier-probe` against the same host: it reports every slot as present, NULL, or past
`struct_size`, which tells you whether to look at your code or at the host build.

## Next

- [The ABI](../guide/abi.md) — the whole contract, which is what you are calling directly.
- [The C++ binding](index.md) — capability checks, strings and error conventions in one page.
