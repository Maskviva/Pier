/** player/InventoryScreenEvents.cpp: "PlayerOpenInventoryEvent" and
 * "PlayerOpenContainerScreenEvent", both cancellable.
 * PlayerOpenContainerEvent is asked when a player opens a container block. These two are what
 * it never sees: the player's own inventory screen, opened by its key, and the screen of any
 * other owner, a minecart chest or a horse for one. ServerPlayer::openInventory and
 * ItemStackNetManagerServer::onContainerScreenOpen are the engine's single places for them.
 * Returning without calling origin means the screen does not open.
 * Payload of the first {eventId, x, y, z, dim, _player}. The second adds `owner`, one of
 * "block", "actor" or "none", with `ownerX` `ownerY` `ownerZ` for a block and `ownerId` for an
 * actor. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>
#include <variant>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/inventory/network/ContainerScreenContext.h"
#include "mc/world/inventory/network/ItemStackNetManagerServer.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& inventoryDef(); // Forward declarations
        HookEventDef& screenDef();

        std::string playerPart(::Player& player)
        {
            auto const& at = player.getPosition();
            return ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(player.getDimensionId()));
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerOpenInventoryHook,
            ll::memory::HookPriority::Normal,
            ServerPlayer,
            &ServerPlayer::$openInventory,
            void)
        {
            auto& def = inventoryDef();
            if (!def.live()) return origin();

            std::string const snbt = "{\"eventId\":\"PlayerOpenInventoryEvent\"" + playerPart(*this)
                + "," + playerRefSnbt(*this) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin();
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerOpenContainerScreenHook,
            ll::memory::HookPriority::Normal,
            ItemStackNetManagerServer,
            &ItemStackNetManagerServer::$onContainerScreenOpen,
            void,
            ::ContainerScreenContext const& screenContext)
        {
            auto& def = screenDef();
            if (!def.live()) return origin(screenContext);

            ::Player& player = this->mPlayer;
            std::string owner = "none";
            std::string ownerPart;
            auto const& variant = screenContext.mOwner.get();
            if (auto const* pos = std::get_if<::BlockPos>(&variant))
            {
                owner = "block";
                ownerPart = ",\"ownerX\":" + snbtNum(pos->x) + ",\"ownerY\":" + snbtNum(pos->y)
                    + ",\"ownerZ\":" + snbtNum(pos->z);
            }
            else if (auto const* id = std::get_if<::ActorUniqueID>(&variant))
            {
                owner = "actor";
                ownerPart = ",\"ownerId\":" + snbtNum(static_cast<int64_t>(id->rawID)) + "L";
            }
            std::string const snbt = "{\"eventId\":\"PlayerOpenContainerScreenEvent\"" + playerPart(player)
                + ",\"owner\":\"" + owner + "\"" + ownerPart + "," + playerRefSnbt(player) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(screenContext);
        }

        HookEventDef gInventoryDef{
            "PlayerOpenInventoryEvent", [] { return PlayerOpenInventoryHook::hook() == 0; }};
        HookEventDef& inventoryDef() { return gInventoryDef; }

        HookEventDef gScreenDef{
            "PlayerOpenContainerScreenEvent", [] { return PlayerOpenContainerScreenHook::hook() == 0; }};
        HookEventDef& screenDef() { return gScreenDef; }

        HookEventRegistrar gInventoryReg{gInventoryDef};
        HookEventRegistrar gScreenReg{gScreenDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
