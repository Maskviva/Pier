/** hosted_mod.h: one loaded pier mod, and the guard every callback dispatch holds. */
#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

#include "ll/api/event/ListenerBase.h"
#include "ll/api/mod/Manifest.h"
#include "ll/api/mod/Mod.h"
#include "ll/api/utils/SystemUtils.h"

#include "sdk/abi.h"

#include "pier/host/watchdog.h"

namespace pier
{
    /** The value of `"type"` in a manifest. User-visible strings fall under the same
     *  ban on language names (contract §7). This reads "pier" and names no language.
     *  The manager loads any cdylib that speaks the Pier ABI and does not know which
     *  language produced it. */
    inline constexpr std::string_view ModHostName = "pier";

    /**
     * A `"type": "pier"` mod. An ordinary cdylib, loaded through the PierApi
     * function table contract instead of the native C++ mod ABI.
     */
    class HostedMod : public ll::mod::Mod, public std::enable_shared_from_this<HostedMod>
    {
    public:
        explicit HostedMod(ll::mod::Manifest manifest) : Mod(std::move(manifest)) {}

        ll::sys_utils::DynamicLibrary lib;
        PierModVTable vtable{};

        /**
         * Keeps DynamicListener objects alive. Cleared on unload.
         *
         * Indexed by a process-wide monotonic id and not by the listener address.
         * Unsubscribing frees the listener and the next subscription may land on the
         * same memory, so a stale address handle that a mod forgot to drop can match
         * a different subscription and silently unsubscribe someone else's listener.
         * An id is never reused, so a stale handle only fails to match.
         */
        struct ListenerSlot
        {
            std::uint64_t id;
            std::shared_ptr<ll::event::ListenerBase> listener;
        };

        std::vector<ListenerSlot> listeners;

        /** True once disabled. Every registered command callback becomes a no-op,
         *  because a command cannot truly be unregistered. */
        bool commandsMuted = false;

        /**
         * Number of stack frames currently executing a callback of this mod, summed
         * across threads.
         *
         * ModHost::unload refuses to unload while it is non-zero. Otherwise
         * FreeLibrary would run beneath the mod's own frame, which happens when a
         * callback issues execute_command("pier unload <self>"), or would unmap the
         * code section while another thread dispatches a bus or packet callback of
         * that mod. Dispatch sites maintain it through CallbackScope. A site that
         * omits the scope loses this protection and introduces no new failure.
         */
        std::atomic<int> inCallback{0};

        /**
         * Set by ModHost::unload before it reads inCallback, and cleared again when the
         * unload is refused. A dispatch site tests it before entering the dylib, through
         * acceptsCallbacks() where isEnabled() applies as well, and directly in Services
         * and PacketHooks where it must not. Without it a callback starting on another
         * thread enters between the inCallback read and FreeLibrary.
         */
        std::atomic<bool> unloading{false};

        /** Whether a callback may be dispatched into this mod right now. */
        [[nodiscard]] bool acceptsCallbacks() const noexcept
        {
            return isEnabled() && !unloading.load(std::memory_order_seq_cst);
        }
    };

    /** RAII counter around a mod callback dispatch, which also tells the watchdog which
     *  mod holds this thread and through which entry. Does nothing when mod is null.
     *
     *  The counter and `unloading` form a store-then-load pair on each side, and x64
     *  reorders a store with a later load unless both are seq_cst. With weaker orders an
     *  unload and a dispatch on another thread can each miss the other. A dispatch that
     *  can run off the server thread therefore checks revoked() after construction. */
    class CallbackScope
    {
    public:
        explicit CallbackScope(HostedMod* mod, char const* site = "callback") noexcept
            : mMod(mod), mWatch(mod, site)
        {
            if (mMod) mMod->inCallback.fetch_add(1, std::memory_order_seq_cst);
        }
        ~CallbackScope()
        {
            if (mMod) mMod->inCallback.fetch_sub(1, std::memory_order_seq_cst);
        }
        CallbackScope(CallbackScope const&) = delete;
        CallbackScope& operator=(CallbackScope const&) = delete;

        /** True when an unload began before this scope was counted; the dispatch must not
         *  enter the dylib then, since FreeLibrary may already be running. */
        [[nodiscard]] bool revoked() const noexcept
        {
            return mMod && mMod->unloading.load(std::memory_order_seq_cst);
        }

    private:
        HostedMod* mMod;
        watchdog::Scope mWatch;
    };

    /** PierModHandle to HostedMod*. The handle is the pointer itself. Its lifetime
     *  is guaranteed by the ModHost table, which runs every Teardown step before it
     *  erases the entry (see mod_host.cpp). */
    [[nodiscard]] inline HostedMod* asMod(PierModHandle h) noexcept
    {
        return static_cast<HostedMod*>(h);
    }
} // namespace pier
