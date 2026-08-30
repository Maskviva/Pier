/** pier-client/Client.cpp —— 仅客户端的能力组（PIER_BUILD_CLIENT）。
 *
 * 所有回调都跑在**客户端线程**上（KeyRegistry 在那里派发）。
 *
 * 键绑定生命周期：KeyRegistry 拥有 KeyHandle；我们在 ClientKeyEntry 里持
 * 一个指向它的裸指针。「反注册」把条目标记为死（handler 变 no-op）并释放
 * 条目。
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
            /** 只作身份用，绝不解引用 —— 用来在它的模组卸载时摘掉这个绑定。 */
            HostedMod* owner = nullptr;
        };

        /**
         * 所有还活着的绑定，好让卸载能够到那些模组忘了反注册的。
         *
         * KeyRegistry 的 handler 比模组的 dylib 活得久：它捕获的是裸的
         * down_cb/up_cb 函数指针，而且注册之后没有任何办法摘掉。`alive` 是让
         * 它们变成 no-op 的唯一手段，而在这张表之前，除了显式
         * client_unregister_key 之外没有任何东西会把它置 false。一个注册了热
         * 键然后被卸载的模组，会留下一个仍然武装着、指向未映射内存的
         * handler。
         *
         * 仅客户端线程（KeyRegistry 在那里派发，注册也来自同一线程），无需加
         * 锁。
         */
        std::vector<ClientKeyEntry*>& liveEntries()
        {
            static std::vector<ClientKeyEntry*> entries;
            return entries;
        }

        /** 拆一个：让 handler 变 no-op，释放条目。 */
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
                // 当前 LL 头里 ClientInstance 没有稳定的 getScreenName() 访问
                // 器。报「不支持」。
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
                // KeyRegistry 想要一个 ll::mod::Mod 的 weak_ptr 做归属标注。托
                // 管模组不是 LL 模组（它们是宿主自己的孩子），所以把**宿主**
                // （pier 这个 NativeMod）报给它 —— 对 LL 来说键就是 pier 注册
                // 的；按托管模组清理是我们自己的活（liveEntries + owner）。
                auto modWeak =
                    std::weak_ptr<ll::mod::Mod>(ll::mod::NativeMod::current()->shared_from_this());

                auto& handle = ll::input::KeyRegistry::getInstance().getOrCreateKey(
                    entry->name, keys, allowRemap, modWeak
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
                // 不在表里说明已经拆过了（重复反注册，或者它的模组先卸载了）。
                // 早先这里是无条件 delete —— 重复反注册就是 double free。
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

        /** 拆除（stage 110）：摘掉该模组名下的全部键绑定。 */
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
