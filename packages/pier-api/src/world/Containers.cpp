/** world/Containers.cpp —— 容器访问。
 *
 * 容器句柄是「拥有者 + which」引用，每次调用重新解析（一切经 Container
 * 虚接口走：箱子 / 玩家背包 / 末影箱共用一条代码路径）。 */
#include <string>
#include <utility>

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/Container.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        bool api_container_size(PierContainerRef ref, int32_t* out)
        {
            PIER_API_GUARD_BEGIN
                Container* c = bridge::resolveContainer(ref);
                if (!c || !out) return false;
                *out = c->getContainerSize();
                return true;
            PIER_API_GUARD_END
        }

        bool api_container_get_item(PierContainerRef ref, int32_t slot, void* ctx, PierStrSink sink)
        {
            PIER_API_GUARD_BEGIN
                Container* c = bridge::resolveContainer(ref);
                if (!c || !sink) return false;
                if (slot < 0 || slot >= c->getContainerSize()) return false;
                sink(ctx, ps(bridge::itemToSnbt(c->getItem(slot))));
                return true;
            PIER_API_GUARD_END
        }

        bool api_container_set_item(PierContainerRef ref, int32_t slot, PierStr itemSnbt)
        {
            PIER_API_GUARD_BEGIN
                Container* c = bridge::resolveContainer(ref);
                if (!c) return false;
                if (slot < 0 || slot >= c->getContainerSize()) return false;
                auto opt = bridge::itemFromSnbt(sv(itemSnbt));
                if (!opt) return false;
                ItemStack item = std::move(*opt);
                c->setItem(slot, item);
                return true;
            PIER_API_GUARD_END
        }

        bool api_container_add_item(PierContainerRef ref, PierStr itemSnbt)
        {
            PIER_API_GUARD_BEGIN
                Container* c = bridge::resolveContainer(ref);
                if (!c) return false;
                auto opt = bridge::itemFromSnbt(sv(itemSnbt));
                if (!opt) return false;
                ItemStack item = std::move(*opt);
                if (item.isNull()) return false;
                return c->addItem(item);
            PIER_API_GUARD_END
        }

        bool api_container_remove_item(PierContainerRef ref, int32_t slot, int32_t count)
        {
            PIER_API_GUARD_BEGIN
                Container* c = bridge::resolveContainer(ref);
                if (!c) return false;
                if (slot < 0 || slot >= c->getContainerSize() || count <= 0) return false;
                c->removeItem(slot, count);
                return true;
            PIER_API_GUARD_END
        }

        bool api_container_clear(PierContainerRef ref)
        {
            PIER_API_GUARD_BEGIN
                Container* c = bridge::resolveContainer(ref);
                if (!c) return false;
                // 逐格清、走虚接口 —— removeAllItems / clearContent 在不同引擎
                // 版次上时有时无；这一条不会。
                int size = c->getContainerSize();
                for (int i = 0; i < size; ++i)
                {
                    c->setItem(i, ItemStack::EMPTY_ITEM());
                }
                return true;
            PIER_API_GUARD_END
        }

        bool api_container_refresh(PierContainerRef ref)
        {
            PIER_API_GUARD_BEGIN
                // 方块容器没有一个可以补发的单一拥有者。它的观看者由引擎自己
                // 的容器事务路径保持同步，这里没有任何有用的事可做 —— 直说，
                // 不装。
                if (ref.which == 4) return false;

                Player* p = bridge::resolvePlayer(ref.player);
                if (!p) return false;

                // Player::sendInventory 重建并发送玩家容器的
                // InventoryContentPacket。`false` = 不顺带重选快捷栏槽位；每次
                // 刷新都强制换槽会把玩家手里的东西拽走 —— 比我们来修的渲染陈
                // 旧更糟。
                p->sendInventory(false);
                return true;
            PIER_API_GUARD_END
        }

        void fill(PierApi& api)
        {
            api.container_size = &api_container_size;
            api.container_get_item = &api_container_get_item;
            api.container_set_item = &api_container_set_item;
            api.container_add_item = &api_container_add_item;
            api.container_remove_item = &api_container_remove_item;
            api.container_clear = &api_container_clear;
            api.container_refresh = &api_container_refresh;
        }

        spi::SlotPackReg reg{{"containers", &fill}};
    } // namespace
} // namespace pier::api_impl
