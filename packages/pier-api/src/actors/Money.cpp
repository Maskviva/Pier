/** actors/Money.cpp: the economy entry points, backed by the optional LLMoney plugin
 *  from LegacyMoney.
 *
 * LegacyMoney.dll is delay loaded, so this TU compiles normally and the host starts
 * normally when LegacyMoney is not installed. Every entry point is gated by
 * moneyBackendReady(), which checks the mod table and the symbols (see
 * money_guard.h). When the backend is absent or disabled the entry returns a safe
 * default rather than calling an unresolved LLMoney_* stub, which would raise a
 * delay-load structured exception and take BDS down. */
#ifndef PIER_BUILD_CLIENT

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "LLMoney.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/server/ServerStartedEvent.h"
#include "ll/api/mod/NativeMod.h"

#include "sdk/abi.h"

#include "pier/api/money_guard.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/mod_host.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/module.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        /** Forward declaration. Trampoline installation is lazy, see
         *  ensureTrampolines in the money event listener section below, and every
         *  economy entry point retries it after confirming the backend is ready. Those
         *  entry points appear earlier in this file. */
        void ensureTrampolines();

        /**
         * Balance. Returns -1 on failure and not 0.
         *
         * This follows the LegacyMoney source. `LLMoney_Get` itself returns -1 on an
         * empty xuid or a database error, and a normal balance is never negative, since
         * `LLMoney_Trans` refuses negative values and rolls back a negative result. A
         * value below zero therefore already means "cannot be determined" for this slot,
         * so returning -1 when the backend is absent is the consistent answer, while 0
         * would be indistinguishable from a balance that is genuinely 0 (contract §5.2).
         *
         * `LLMoney_Get` creates an account for an unseen xuid, inserting a row with the
         * configured def_money, so this call is not a side-effect-free read.
         */
        long long api_get_money(PierStr xuid)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return -1;
                ensureTrampolines();
                return LLMoney_Get(toString(xuid));
            PIER_API_GUARD_END_VAL(-1)
        }

        bool api_set_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
                return LLMoney_Set(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_add_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
                return LLMoney_Add(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_reduce_money(PierStr xuid, long long money)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
                return LLMoney_Reduce(toString(xuid), money);
            PIER_API_GUARD_END
        }

        bool api_trans_money(PierStr from, PierStr to, long long val, PierStr note)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return false;
                ensureTrampolines();
                return LLMoney_Trans(toString(from), toString(to), val, toString(note));
            PIER_API_GUARD_END
        }

        void api_money_get_hist(PierStr xuid, int timediff, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                // No backend means no records. The sink is simply not called, so the
                // other side sees an empty history, which is equivalent to finding
                // nothing.
                if (!moneyBackendReady()) return;
                sink(ctx, ps(LLMoney_GetHist(toString(xuid), timediff)));
            PIER_API_GUARD_END_VOID
        }

        void api_money_clear_hist(int difftime)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return;
                LLMoney_ClearHist(difftime);
            PIER_API_GUARD_END_VOID
        }

        /*  Money event listeners.
         *
         * LLMoney_ListenBeforeEvent only appends and LegacyMoney offers no way to
         * unregister, so the host installs one permanent trampoline and fans out
         * itself. The fan-out does not break early, so the decision does not depend on
         * registration order. Any before handler returning false vetoes; every after
         * handler is notified.
         *
         * These two slots predate the mod-scoped convention and take a bare function
         * pointer with no mod handle, so ownership is recovered through
         * addressOwnedBy(). Installation must be retryable, because LegacyMoney is not
         * enabled yet during the LL load phase and a veto registered from on_load would
         * stay inactive forever. Every entry point and registration retries, plus once
         * after ServerStartedEvent. */
        struct MoneyListener
        {
            PierMoneyCb cb = nullptr;
            void const* moduleBase = nullptr; // Module base at registration, null if unresolved
        };

        std::mutex gMoneyMutex;
        std::vector<MoneyListener> g_before;
        std::vector<MoneyListener> g_after;
        bool g_beforeHooked = false;
        bool g_afterHooked = false;

        /** Resolves the base address of the module owning a callback. A Pier mod's
         *  callback lies inside its own DLL. */
        void const* moduleBaseOf(PierMoneyCb cb)
        {
            auto* host = ModHost::instance();
            if (!host) return nullptr;
            for (auto const& hosted : host->hostedMods())
            {
                void const* base = hosted->lib.handle();
                if (base && addressOwnedBy(base, reinterpret_cast<void const*>(cb))) return base;
            }
            return nullptr;
        }

        std::vector<PierMoneyCb> snapshotListeners(std::vector<MoneyListener> const& v)
        {
            std::vector<PierMoneyCb> out;
            std::lock_guard lock(gMoneyMutex);
            out.reserve(v.size());
            for (auto const& l : v) out.push_back(l.cb);
            return out;
        }

        /** Installs both trampolines once the backend is ready. Idempotent, and called
         *  from every economy entry point. */
        void ensureTrampolines()
        {
            // Fast path. Once both trampolines are installed this runs on every
            // economy call, so it must not walk the mod registry again, which
            // moneyBackendReady does through ModManagerRegistry.
            if (g_beforeHooked && g_afterHooked) return;
            if (!moneyBackendReady()) return;
            if (!g_beforeHooked)
            {
                g_beforeHooked = true;
                LLMoney_ListenBeforeEvent(
                    [](::LLMoneyEvent t, std::string f, std::string to, long long v)
                    {
                        bool allow = true;
                        for (auto cb : snapshotListeners(g_before))
                        {
                            if (cb && !cb(static_cast<PierMoneyEvent>(t), ps(f), ps(to), v)) allow = false;
                        }
                        return allow;
                    });
            }
            if (!g_afterHooked)
            {
                g_afterHooked = true;
                LLMoney_ListenAfterEvent(
                    [](::LLMoneyEvent t, std::string f, std::string to, long long v)
                    {
                        for (auto cb : snapshotListeners(g_after))
                        {
                            if (cb) (void)cb(static_cast<PierMoneyEvent>(t), ps(f), ps(to), v);
                        }
                        return true;
                    });
            }
        }

        void addListener(std::vector<MoneyListener>& list, PierMoneyCb cb)
        {
            if (!cb) return;
            void const* base = moduleBaseOf(cb);
            if (!base)
            {
                hostLogger().warn(
                    "[money] money_listen_* callback {:p} belongs to no loaded pier mod, "
                    "so it cannot be cleaned up on unload and lives until the process "
                    "exits", reinterpret_cast<void const*>(cb));
            }
            std::lock_guard lock(gMoneyMutex);
            for (auto const& l : list)
            {
                if (l.cb == cb) return; // Idempotent, the same callback is not registered twice
            }
            list.push_back(MoneyListener{cb, base});
        }

        void api_money_listen_before_event(PierMoneyCb callback)
        {
            PIER_API_GUARD_BEGIN
                addListener(g_before, callback);
                ensureTrampolines();
            PIER_API_GUARD_END_VOID
        }

        void api_money_listen_after_event(PierMoneyCb callback)
        {
            PIER_API_GUARD_BEGIN
                addListener(g_after, callback);
                ensureTrampolines();
            PIER_API_GUARD_END_VOID
        }

        /** One more installation attempt after server startup completes, by which
         *  point LegacyMoney is enabled. */
        std::shared_ptr<ll::event::ListenerBase> gStartedListener;

        void bootstrap()
        {
            auto listener = ll::event::Listener<ll::event::ServerStartedEvent>::create(
                [](ll::event::ServerStartedEvent&) { ensureTrampolines(); },
                ll::event::EventPriority::Normal,
                ll::mod::NativeMod::current()
            );
            if (ll::event::EventBus::getInstance().addListener<ll::event::ServerStartedEvent>(listener))
            {
                gStartedListener = listener;
            }
        }

        void api_money_ranking(unsigned short num, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!moneyBackendReady()) return;
                for (auto const& [x, m] : LLMoney_Ranking(num))
                {
                    sink(ctx, ps(x + ":" + snbtNum(m)));
                }
            PIER_API_GUARD_END_VOID
        }

        /** Teardown at stage 100. Clears the callbacks belonging to this mod by the
         *  module base recorded at registration. The trampolines stay, because
         *  LegacyMoney has no way to unregister. */
        void teardown(HostedMod* mod)
        {
            if (!mod) return;
            void const* base = mod->lib.handle();
            std::lock_guard lock(gMoneyMutex);
            auto drop = [&](std::vector<MoneyListener>& v)
            {
                std::erase_if(v, [&](MoneyListener const& l)
                {
                    return l.moduleBase == base
                        || addressOwnedBy(base, reinterpret_cast<void const*>(l.cb));
                });
            };
            drop(g_before);
            drop(g_after);
        }

        void fill(PierApi& api)
        {
            api.get_money = &api_get_money;
            api.set_money = &api_set_money;
            api.add_money = &api_add_money;
            api.reduce_money = &api_reduce_money;
            api.trans_money = &api_trans_money;
            api.money_get_hist = &api_money_get_hist;
            api.money_clear_hist = &api_money_clear_hist;
            api.money_listen_before_event = &api_money_listen_before_event;
            api.money_listen_after_event = &api_money_listen_after_event;
            api.money_ranking = &api_money_ranking;
        }

        spi::SlotPackReg regSlots{{"money", &fill}};
        spi::TeardownReg regDown{{100, "money", &teardown}};
        spi::BootstrapReg regBoot{{100, "money-trampoline", &bootstrap}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
