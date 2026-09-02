#include "pier/api/money_guard.h"

#ifndef PIER_BUILD_CLIENT

#include <atomic>
#include <string_view>

#include "ll/api/memory/Symbol.h"
#include "ll/api/mod/Mod.h"
#include "ll/api/mod/ModManagerRegistry.h"

#include "pier/support/log.h"

namespace pier::api_impl
{
    namespace
    {
        // The name LegacyMoney carries in its LeviLamina manifest.
        constexpr std::string_view kMoneyModName = "LegacyMoney";

        // A stable exported symbol of LegacyMoney. It is declared extern "C" and on
        // x64 MSVC the exported name is undecorated, so it is exactly this string. If
        // LLMoney_Get is present the whole LLMoney_* family is, since they ship
        // together.
        constexpr std::string_view kProbeSymbol = "LLMoney_Get";

        // -1 means not probed yet, 0 means the symbol is missing and 1 means it is
        // present. The DLL owning the symbol cannot be swapped mid-process, so one
        // observation is enough and it is never resolved again.
        std::atomic<int> gSymbolState{-1};

        // Warns once per process, whichever check fails first.
        std::atomic_flag gWarned = ATOMIC_FLAG_INIT;

        bool symbolPresent()
        {
            int cached = gSymbolState.load(std::memory_order_acquire);
            if (cached >= 0)
            {
                return cached == 1;
            }
            // With disableErrorOutput=true, resolve() returns nullptr on a miss and
            // logs nothing. The wording belongs to the host side.
            void* addr = ll::memory::SymbolView{kProbeSymbol}.resolve(true);
            int result = addr != nullptr ? 1 : 0;
            // A benign race. Two threads may both resolve, the answer is the same and
            // the later write wins.
            gSymbolState.store(result, std::memory_order_release);
            return result == 1;
        }

        bool modLoadedAndEnabled()
        {
            auto& registry = ll::mod::ModManagerRegistry::getInstance();
            if (!registry.hasMod(kMoneyModName))
            {
                return false;
            }
            auto mod = registry.getMod(kMoneyModName);
            return mod && mod->isEnabled();
        }

        void warnOnce(std::string_view reason)
        {
            if (gWarned.test_and_set(std::memory_order_relaxed))
            {
                return; // Already warned
            }
            hostLogger().warn(
                "[money] no usable LLMoney backend ({}); every money::* entry point is "
                "inert for this session, reads return 0 and writes fail; check that "
                "LegacyMoney is installed and enabled, under mod name \"{}\"",
                reason,
                kMoneyModName
            );
        }
    } // namespace

    bool moneyBackendReady() noexcept
    {
        // The cheap checks first, the mod table and its state, then the memoized
        // symbol probe. Either failing means not ready.
        if (!modLoadedAndEnabled())
        {
            warnOnce("no enabled LegacyMoney in the mod list");
            return false;
        }
        if (!symbolPresent())
        {
            warnOnce("LegacyMoney is loaded but the LLMoney_Get export could not be resolved");
            return false;
        }
        return true;
    }
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
