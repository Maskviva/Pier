/** pier-client/Client.cpp: the client-only capability group (PIER_BUILD_CLIENT).
 *
 * Every callback runs on the client thread, where KeyRegistry dispatches.
 *
 * Key binding lifetime: KeyRegistry owns the KeyHandle and this package holds a raw
 * pointer to it inside ClientKeyEntry. Unregistering marks the entry dead, which
 * turns the handler into a no-op, and frees the entry.
 */
#ifdef PIER_BUILD_CLIENT

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "ll/api/input/KeyHandle.h"
#include "ll/api/input/KeyRegistry.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/service/TargetedBedrock.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/game/IClientInstance.h"
#include "mc/deps/input/enums/FocusImpact.h"
#include "mc/client/player/LocalPlayer.h"
#include "mc/world/actor/player/Player.h"

#include "sdk/abi.h"

#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        struct ClientKeyEntry
        {
            std::string name;
            ll::input::KeyHandle* handle;
            PierKeyCb down_cb;
            PierKeyCb up_cb;
            void* user;
            std::shared_ptr<bool> alive;
            /** Identity only, never dereferenced. Used to drop this binding when its
             *  mod is unloaded. */
            HostedMod* owner = nullptr;
        };

        /**
         * Every live binding, so that an unload can reach the ones a mod forgot to
         * unregister.
         *
         * A KeyRegistry handler outlives the mod's dylib. It captures the raw down_cb
         * and up_cb function pointers and nothing can detach it after registration.
         * `alive` is the only way to turn such a handler into a no-op, and this table
         * is what sets it to false outside an explicit client_unregister_key. A mod
         * that registers a hotkey and is then unloaded would otherwise leave behind an
         * armed handler pointing into unmapped memory.
         *
         * Client thread only, since KeyRegistry dispatches there and registration
         * comes from the same thread, so no lock is needed.
         */
        std::vector<ClientKeyEntry*>& liveEntries()
        {
            static std::vector<ClientKeyEntry*> entries;
            return entries;
        }

        /** Tears one down. Turns the handler into a no-op and frees the entry. */
        void destroyEntry(ClientKeyEntry* entry)
        {
            *entry->alive = false;
            delete entry;
        }

        PierFocusImpact toAbiFocus(::FocusImpact fi)
        {
            return static_cast<PierFocusImpact>(static_cast<int>(fi));
        }

        bool api_client_get_local_player(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!sink) return false;
                auto ci = ll::service::getClientInstance();
                if (!ci) return false;
                auto* player = ci->getLocalPlayer();
                if (!player) return false;
                sink(ctx, ps(player->getRealName()));
                return true;
            PIER_API_GUARD_END
        }

        bool api_client_is_in_level()
        {
            PIER_API_GUARD_BEGIN
                return ll::service::getMultiPlayerLevel() != nullptr;
            PIER_API_GUARD_END
        }

        bool api_client_get_screen_name(void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                // The current LL headers expose no stable getScreenName() accessor on
                // ClientInstance, so this reports unsupported.
                (void)ctx;
                (void)sink;
                return false;
            PIER_API_GUARD_END
        }

        PierKeyHandle api_client_register_key(
            PierModHandle modHandle,
            PierStr name,
            int32_t const* keyCodes,
            int32_t keyCount,
            bool allowRemap,
            PierKeyCb downCb,
            PierKeyCb upCb,
            void* user)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || name.len == 0 || !keyCodes || keyCount <= 0) return nullptr;

                auto entry = std::make_unique<ClientKeyEntry>();
                entry->name = toString(name);
                entry->down_cb = downCb;
                entry->up_cb = upCb;
                entry->user = user;
                entry->alive = std::make_shared<bool>(true);

                std::vector<int> keys(keyCodes, keyCodes + keyCount);
                // The fourth argument, the owning mod, is left at its default, which
                // KeyRegistry declares as mod::NativeMod::current(). That is the host
                // NativeMod named pier, which is what attribution should say: a hosted
                // mod is not an LL mod, it is a child of the host. To LL the key is
                // registered by pier, while cleanup per hosted mod is this package's
                // job through liveEntries and owner.
                auto& handle = ll::input::KeyRegistry::getInstance().getOrCreateKey(
                    entry->name, keys, allowRemap
                );
                entry->handle = &handle;

                auto alive = entry->alive;
                auto downCbCapture = downCb;
                auto upCbCapture = upCb;
                auto userCapture = user;

                handle.registerButtonDownHandler(
                    [alive, downCbCapture, userCapture](::FocusImpact fi, ::IClientInstance&)
                    {
                        if (!*alive || !downCbCapture) return;
                        downCbCapture(userCapture, /*Down=*/1, toAbiFocus(fi));
                    }
                );
                handle.registerButtonUpHandler(
                    [alive, upCbCapture, userCapture](::FocusImpact fi, ::IClientInstance&)
                    {
                        if (!*alive || !upCbCapture) return;
                        upCbCapture(userCapture, /*Up=*/0, toAbiFocus(fi));
                    }
                );

                entry->owner = mod;
                auto* raw = entry.release();
                liveEntries().push_back(raw);
                return reinterpret_cast<PierKeyHandle>(raw);
            PIER_API_GUARD_END
        }

        bool api_client_unregister_key(PierKeyHandle handle)
        {
            PIER_API_GUARD_BEGIN
                if (!handle) return false;
                auto* entry = reinterpret_cast<ClientKeyEntry*>(handle);
                auto& live = liveEntries();
                auto it = std::find(live.begin(), live.end(), entry);
                // Absence from the table means it was already torn down, either by a
                // repeated unregister or because its mod was unloaded first. Deleting
                // unconditionally here would make a repeated unregister a double free.
                if (it == live.end()) return false;
                live.erase(it);
                destroyEntry(entry);
                return true;
            PIER_API_GUARD_END
        }

        bool api_client_get_key_codes(PierKeyHandle handle, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                if (!handle || !sink) return false;
                auto* entry = reinterpret_cast<ClientKeyEntry*>(handle);
                if (!entry->handle) return false;
                auto const& codes = entry->handle->getKeyCodes();
                std::string out = "[";
                for (size_t i = 0; i < codes.size(); ++i)
                {
                    if (i) out += ',';
                    out += snbtNum(codes[i]);
                }
                out += "]";
                sink(ctx, ps(out));
                return true;
            PIER_API_GUARD_END
        }

        /** Teardown at stage 110. Drops every key binding held under this mod. */
        void teardown(HostedMod* mod)
        {
            if (!mod) return;
            auto& live = liveEntries();
            for (auto it = live.begin(); it != live.end();)
            {
                if ((*it)->owner != mod)
                {
                    ++it;
                    continue;
                }
                destroyEntry(*it);
                it = live.erase(it);
            }
        }

        void fill(PierApi& api)
        {
            api.client_get_local_player = &api_client_get_local_player;
            api.client_is_in_level = &api_client_is_in_level;
            api.client_get_screen_name = &api_client_get_screen_name;
            api.client_register_key = &api_client_register_key;
            api.client_unregister_key = &api_client_unregister_key;
            api.client_get_key_codes = &api_client_get_key_codes;
        }

        spi::SlotPackReg regSlots{{"client", &fill}};
        spi::TeardownReg regDown{{110, "client", &teardown}};
    } // namespace
} // namespace pier::api_impl

#endif // PIER_BUILD_CLIENT
