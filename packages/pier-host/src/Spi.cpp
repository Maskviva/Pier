#include "pier/host/spi.h"

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

#include "ll/api/io/Logger.h"

#include "pier/host/hosted_mod.h"

namespace pier::spi
{
    // 注册表全部是 Meyers 单例：注册发生在各包文件级静态对象的构造里，
    // 跨 TU 的静态初始化顺序未定义 —— 函数内静态是唯一不赌顺序的写法。
    namespace
    {
        std::vector<SlotPack>& slotPacks()
        {
            static std::vector<SlotPack> v;
            return v;
        }
        std::vector<Bootstrap>& bootstraps()
        {
            static std::vector<Bootstrap> v;
            return v;
        }
        std::vector<UnloadVeto>& vetoes()
        {
            static std::vector<UnloadVeto> v;
            return v;
        }
        std::vector<Teardown>& teardowns()
        {
            static std::vector<Teardown> v;
            return v;
        }
        std::vector<EventProvider>& providers()
        {
            static std::vector<EventProvider> v;
            return v;
        }
    } // namespace

    void addSlotPack(SlotPack p) { slotPacks().push_back(p); }
    void addBootstrap(Bootstrap b) { bootstraps().push_back(b); }
    void addUnloadVeto(UnloadVeto v) { vetoes().push_back(v); }
    void addTeardown(Teardown t) { teardowns().push_back(t); }
    void addEventProvider(EventProvider p) { providers().push_back(p); }

    void buildApi(PierApi& api, ll::io::Logger& log)
    {
        api.struct_size = sizeof(PierApi);
        api.abi_version = PIER_ABI_VERSION;
#ifdef PIER_BUILD_CLIENT
        api.host_flags = PIER_FLAG_CLIENT;
#else
        api.host_flags = 0;
#endif
        api._reserved0 = 0;

        std::string names;
        for (auto const& p : slotPacks())
        {
            p.fill(api);
            if (!names.empty()) names += ", ";
            names += p.name;
        }
        // 这行 debug 是排查「某能力缺席」的第一站：包没列在这里 = 没编进来。
        log.debug("PierApi：{} 字节，槽位包 [{}]", sizeof(PierApi), names);
    }

    void runBootstrap(ll::io::Logger& log)
    {
        auto steps = bootstraps();
        std::stable_sort(
            steps.begin(), steps.end(), [](auto const& a, auto const& b) { return a.stage < b.stage; });
        for (auto const& s : steps)
        {
            log.debug("引导：{}（stage {}）", s.name, s.stage);
            s.run();
        }
    }

    void runTeardown(HostedMod* mod)
    {
        auto steps = teardowns();
        std::stable_sort(
            steps.begin(), steps.end(), [](auto const& a, auto const& b) { return a.stage < b.stage; });
        for (auto const& s : steps) s.run(mod);
    }

    std::optional<VetoAnswer> askUnloadVetoes(HostedMod* mod)
    {
        for (auto const& v : vetoes())
        {
            if (char const* reason = v.why(mod)) return VetoAnswer{v.name, reason};
        }
        return std::nullopt;
    }

    bool forEachEventProvider(bool (*visit)(EventProvider const&, void*), void* ctx)
    {
        for (auto const& p : providers())
        {
            if (visit(p, ctx)) return true;
        }
        return false;
    }

    namespace
    {
        DimensionBridge const* gDimBridge = nullptr;
    } // namespace

    void setDimensionBridge(DimensionBridge const* bridge) { gDimBridge = bridge; }
    DimensionBridge const* dimensionBridge() noexcept { return gDimBridge; }

    bool idMatches(std::string_view wanted, std::string_view canonical) noexcept
    {
        if (wanted == canonical) return true;
        if (wanted.size() <= canonical.size()) return false;
        if (!wanted.ends_with(canonical)) return false;
        // 后缀成立还不够 —— 前一个字符必须是命名空间分隔符，否则
        // "MyPlayerChatEvent" 会匹配上 "PlayerChatEvent"（那就是子串匹配）。
        char const before = wanted[wanted.size() - canonical.size() - 1];
        return before == ':' || before == '.';
    }

    std::uint64_t nextListenerId() noexcept
    {
        // 从 1 起：0 留给「无效句柄」。relaxed 足够 —— 只要求唯一，不排序。
        static std::atomic<std::uint64_t> next{1};
        return next.fetch_add(1, std::memory_order_relaxed);
    }
} // namespace pier::spi
