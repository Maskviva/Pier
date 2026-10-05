/** MoneyGuard.cpp: decides whether the LLMoney backend can be called without taking BDS down. */
#include "pier/api/money_guard.h"

#ifndef PIER_BUILD_CLIENT

#ifndef _WIN32
#error "Pier targets Windows; LeviLamina has no other build"
#endif

#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "ll/api/mod/Mod.h"
#include "ll/api/mod/ModManagerRegistry.h"

#include "pier/api/pe_exports.h"
#include "pier/support/log.h"
#include "pier/support/i18n.h"

namespace pier::api_impl
{
    namespace
    {
        // The name LegacyMoney carries in its LeviLamina manifest.
        constexpr std::string_view kMoneyModName = "LegacyMoney";

        // The module the delay-load helper binds to: /DELAYLOAD:LegacyMoney.dll in xmake.lua,
        // and the DLL name recorded in the import library. It is a file name, not a mod name.
        constexpr wchar_t kMoneyDll[] = L"LegacyMoney.dll";

        // Imported by name, so each has to be in the export table under exactly this spelling.
        // They are extern "C" in the SDK, and such an export is undecorated on x64. One that is
        // missing raises a delay-load exception at its first call.
        constexpr std::array<std::string_view, 9> kPlainExports{
            "LLMoney_Get",
            "LLMoney_Set",
            "LLMoney_Add",
            "LLMoney_Reduce",
            "LLMoney_Trans",
            "LLMoney_GetHist",
            "LLMoney_ClearHist",
            "LLMoney_ListenBeforeEvent",
            "LLMoney_ListenAfterEvent",
        };

        // Declared with C++ linkage in the SDK, so its export is a decorated name that contains
        // this one.
        constexpr std::string_view kDecoratedExport = "LLMoney_Ranking";

        // -1 not checked yet, 0 something is missing, 1 everything is there. The DLL owning the
        // exports cannot be swapped mid-process, so one answer is enough.
        std::atomic<int> gExportsState{-1};

        // What was missing, for the warning. Written before gExportsState leaves -1.
        std::mutex gReasonMutex;
        std::string gReason;

        // Warns once per process, whichever check fails first.
        std::atomic_flag gWarned = ATOMIC_FLAG_INIT;

        // Empty when every export Pier imports is present, otherwise a sentence saying what is
        // not. The plain names go through the loader, which is what the delay-load helper
        // uses, so the answer is the one the first call would get. The export table is read as
        // well, for the decorated name and to list what the DLL does export.
        std::string missingExports()
        {
            HMODULE const module = ::GetModuleHandleW(kMoneyDll);
            if (module == nullptr)
            {
                return "LegacyMoney.dll is not loaded in this process under that file name";
            }

            std::vector<std::string_view> missing;
            for (auto const name : kPlainExports)
            {
                std::string const terminated{name};
                if (::GetProcAddress(module, terminated.c_str()) == nullptr)
                {
                    missing.push_back(name);
                }
            }
            auto const exported = pe::exportNames(reinterpret_cast<std::byte const*>(module));
            if (!pe::anyContains(exported, kDecoratedExport))
            {
                missing.push_back(kDecoratedExport);
            }
            if (missing.empty())
            {
                return {};
            }

            std::string reason = "LegacyMoney.dll does not export ";
            for (std::size_t i = 0; i < missing.size(); ++i)
            {
                if (i != 0) reason += ", ";
                reason += missing[i];
            }
            reason += "; the LLMoney names it does export: ";
            auto const seen = pe::namesContaining(exported, "LLMoney", 12);
            if (seen.empty())
            {
                reason += "none, among " + std::to_string(exported.size()) + " exports";
            }
            for (std::size_t i = 0; i < seen.size(); ++i)
            {
                if (i != 0) reason += ", ";
                reason += seen[i];
            }
            return reason;
        }

        bool exportsPresent()
        {
            int const cached = gExportsState.load(std::memory_order_acquire);
            if (cached >= 0)
            {
                return cached == 1;
            }
            std::string reason;
            try
            {
                reason = missingExports();
            }
            catch (std::exception const& e)
            {
                reason = std::string{"the export check itself failed: "} + e.what();
            }
            bool const present = reason.empty();
            {
                std::lock_guard lock(gReasonMutex);
                gReason = std::move(reason);
            }
            gExportsState.store(present ? 1 : 0, std::memory_order_release);
            return present;
        }

        std::string failureReason()
        {
            std::lock_guard lock(gReasonMutex);
            return gReason;
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
                "[money] {}", pier::trf("api.money_guard.1", reason,
                kMoneyModName));
        }
    } // namespace

    bool moneyBackendReady() noexcept
    {
        // The cheap checks first, the mod table and its state, then the memoized export
        // check. Either failing means not ready.
        if (!modLoadedAndEnabled())
        {
            warnOnce("no enabled LegacyMoney in the mod list");
            return false;
        }
        if (!exportsPresent())
        {
            warnOnce(failureReason());
            return false;
        }
        return true;
    }
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
