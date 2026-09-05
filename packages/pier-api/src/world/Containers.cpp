/** world/Containers.cpp: container access.
 *
 * A container handle is an owner plus which reference, re-resolved on every call.
 * Everything goes through the Container virtual interface, so a chest, a player
 * inventory and an ender chest share one code path. */
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

        bool api_container_get_items(PierContainerRef ref, void* ctx, PierSlotSink sink)
        {
            PIER_API_GUARD_BEGIN
                Container* c = bridge::resolveContainer(ref);
                if (!c || !sink) return false;
                int const size = c->getContainerSize();
                for (int slot = 0; slot < size; ++slot)
                {
                    sink(ctx, slot, ps(bridge::itemToSnbt(c->getItem(slot))));
                }
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
                // Cleared slot by slot through the virtual interface. removeAllItems
                // and clearContent come and go across engine revisions, this does not.
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
                // A block container has no single owner to resend to. Its viewers are
                // kept in sync by the engine's own container transaction path, so
                // there is nothing useful to do here and this says so.
                if (ref.which == 4) return false;

                Player* p = bridge::resolvePlayer(ref.player);
                if (!p) return false;

                // Player::sendInventory rebuilds and sends the InventoryContentPacket
                // for the player container. `false` means the hotbar slot is not
                // reselected. Forcing a slot change on every refresh would pull the
                // item out of the player's hand, which is worse than the stale
                // rendering this fixes.
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
            api.container_get_items = &api_container_get_items;
        }

        spi::SlotPackReg reg{{"containers", &fill}};
    } // namespace
} // namespace pier::api_impl
