#include "pier/host/spi.h"

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

#include "ll/api/io/Logger.h"

#include "pier/host/hosted_mod.h"

namespace pier::spi
{
    // Every registry is a Meyers singleton. Registration happens in the constructors
    // of file-level static objects, and static initialization order across translation
    // units is undefined, so a function-local static is the only form that does not
    // gamble on that order.
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
        // First stop when diagnosing a missing capability. A package that is not
        // listed here was not compiled in.
        log.debug("[spi] api table {} bytes, slot packs [{}]", sizeof(PierApi), names);
    }

    void runBootstrap(ll::io::Logger& log)
    {
        auto steps = bootstraps();
        std::stable_sort(
            steps.begin(), steps.end(), [](auto const& a, auto const& b) { return a.stage < b.stage; });
        for (auto const& s : steps)
        {
            log.debug("[spi] bootstrap step '{}' at stage {}", s.name, s.stage);
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
        // A matching suffix is not enough. The preceding character must be a
        // namespace separator, otherwise "MyPlayerChatEvent" would match
        // "PlayerChatEvent", which is a substring match.
        char const before = wanted[wanted.size() - canonical.size() - 1];
        return before == ':' || before == '.';
    }

    std::uint64_t nextListenerId() noexcept
    {
        // Starts at 1, because 0 is reserved for the invalid handle. relaxed is
        // enough here, since only uniqueness is required and not ordering.
        static std::atomic<std::uint64_t> next{1};
        return next.fetch_add(1, std::memory_order_relaxed);
    }
} // namespace pier::spi
