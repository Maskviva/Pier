#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ll/api/Expected.h"
#include "ll/api/mod/Manifest.h"
#include "ll/api/mod/ModManager.h"

#include "pier/host/hosted_mod.h"

namespace pier
{
    /**
     * manifest `"type": "pier"` 的 ModManager。
     *
     * 由加载器在 ll_mod_load 里注册。ModRegistrar 在派发装载时才解析
     * 管理器（见 ModManagerRegistry::loadMod），并按 `dependencies` 做
     * 拓扑排序 —— 所以任何声明了
     *   "dependencies": [{ "name": "pier" }]
     * 的模组，派发到它时本管理器一定已经存在。
     */
    class ModHost : public ll::mod::ModManager
    {
    public:
        ModHost();
        ~ModHost() override;

        ll::Expected<> load(ll::mod::Manifest manifest) override;
        ll::Expected<> unload(std::string_view name) override;

        /**
         * 正在运行的管理器；ll_mod_load 之前 / 关停之后为 nullptr。
         * 构造里设、析构里清 —— shared_ptr 归注册表所有，这只是观察者。
         */
        static ModHost* instance();

        /* ── 运行期模组控制（/pier 的后端）────────────────────────────
         * ModManagerRegistry 的 loadMod/unloadMod/enableMod/disableMod
         * 全是 PRIVATE（只对 ModRegistrar 和 Mod 友元），启动之后没有
         * 公开的 LeviLamina 路径能再装一个模组。能用的是我们继承来的
         * protected ModManager 方法 —— 这两个包装就是干这个的。
         *
         * 值得记住的后果：这样拉起来的模组活在本管理器自己的表里，不在
         * LeviLamina 的依赖图里。依赖检查因此由 ModControl 直接翻
         * manifest 完成 —— 副作用是连「原生 C++ mod 依赖 pier mod」
         * 也能查到。 */

        /** 拉起一份解析好的 manifest：load() 然后 enable()。 */
        ll::Expected<> controlLoad(ll::mod::Manifest manifest);

        /** 放倒一个：disable()（触发 on_disable）然后 unload()。 */
        ll::Expected<> controlUnload(std::string_view name);

        /** 已装载模组 dylib 的基址；没装返回 nullptr。
         *  用来识别 Windows FreeLibrary 引用计数陷阱：reload 后拿到
         *  **同一个**基址 = 映像根本没卸掉，磁盘上的新 dll 没被读进来。 */
        [[nodiscard]] void const* moduleBase(std::string_view name) const;

        /** 本管理器当前装着的全部模组名。 */
        [[nodiscard]] std::vector<std::string> loadedNames() const;

        /** 当前装着的全部模组（强引用快照）。给需要「按回调地址反查归属」的
         *  能力包用（Money/Scheduler 的无主旧槽）：ll::mod::Mod 不是多态类型，
         *  对 ModManagerRegistry::mods() 的 Mod& 做 dynamic_cast 不合法，而本
         *  管理器表里的每一个都一定是 HostedMod。 */
        [[nodiscard]] std::vector<std::shared_ptr<HostedMod>> hostedMods() const;
    };
} // namespace pier
