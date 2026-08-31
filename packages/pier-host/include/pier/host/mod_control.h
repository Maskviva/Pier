#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ll/api/Expected.h"
#include "ll/api/mod/Manifest.h"

namespace pier::mod_control
{
    /**
     * 磁盘上一个 `mods/<dir>/manifest.json`（`"type": "pier"`）。
     *
     * 直接读文件而不是复用 ll::mod::Manifest，因为 `reloadSafe` 在那个结构
     * 里没有位置。LeviLamina 的反射反序列化器遍历的是结构体的成员、逐个
     * 去 JSON 里找 —— 它从不遍历 JSON 自己的键，所以 manifest 里多一个顶层
     * 字段对 LeviLamina 完全不可见，加了不会影响正常的启动装载。
     */
    struct Candidate
    {
        std::string name;
        std::string entry;
        std::string version;
        std::vector<std::string> dependencies;
        /** manifest.json 顶层的 `"reload_safe": true`。 */
        bool reloadSafe = false;
        /** manifest 存在但没法用时非空。
         *  叫 `problem` 不叫 `error`，免得在调用点和 ll::Expected 的错误
         *  通道读混。 */
        std::string problem;
    };

    /** 重新扫描磁盘上的 mods/。`/pier list` 调的就是它，也是服务器启动后新
     *  增的目录变得可见的唯一途径。
     *
     *  刻意无缓存：每次查询都打磁盘。缓存在这里买不到任何东西（这些命令是
     *  人手敲的，不在循环里跑），却可能对刚重编好的 dll 给出过期的入口 ——
     *  而那恰恰是这条命令存在要支持的场景。 */
    std::vector<Candidate> rescan();

    /** 按名字从磁盘读一个（绕过任何缓存，永不过期）。 */
    ll::Expected<Candidate> readCandidate(std::string_view name);

    /** 从 manifest.json 建出 ll::mod::Manifest。 */
    ll::Expected<ll::mod::Manifest> readManifest(std::string_view name);

    /**
     * 所有（传递地）把 `name` 声明进 `dependencies` 的已装载模组，按可以
     * 安全卸载的顺序（依赖方在被依赖方之前）。原生 C++ 模组也在内 ——
     * 它们依赖一个 pier 模组和别的 pier 模组一样容易。
     */
    std::vector<std::string> dependentsOf(std::string_view name);

    /** `name` 已装载且归本宿主管（pier 模组）。 */
    bool isHostedMod(std::string_view name);

    /** `name` 被任何管理器装载着。 */
    bool isLoadedAnywhere(std::string_view name);

    /** 注册 `/pier`。仅服务端构建；enable() 里调一次。 */
    void registerCommand();
} // namespace pier::mod_control
