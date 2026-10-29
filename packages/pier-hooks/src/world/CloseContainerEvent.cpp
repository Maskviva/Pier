/** hooks/world/CloseContainerEvent.cpp: the synthetic "PlayerCloseContainerEvent", observation
 * only.
 * PlayerOpenContainerEvent says a player opened a chest, and nothing says the player stopped
 * looking at it, which a shop that settles when the customer walks away, or a mod that restores
 * a locked container, needs. A container screen ends in one of two ways. The player's container
 * manager is deleted, which is the ordinary close and the disconnect, or a piston moves the
 * block a player is looking at, which closes it for every viewer. Both are hooked and each
 * dispatches before origin, so a callback still finds the screen open. It cannot be cancelled:
 * the engine has already decided to close it.
 * Payload {eventId, x, y, z, dim, block, _player}: the container block and the player. Only a
 * screen owned by a container block is reported. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>
#include <variant>

#include "ll/api/memory/Hook.h"

#include "mc/server/ServerPlayer.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/containers/managers/models/ContainerManagerModel.h"
#include "mc/world/inventory/network/ContainerScreenContext.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/level/block/actor/PistonBlockActor.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& closeDef(); // Forward declaration

        void report(::Player const& player, ::BlockPos const& pos, ::BlockSource& region)
        {
            std::string name;
            try
            {
                name = region.getBlock(pos).getTypeName();
            }
            catch (...)
            {
                name.clear();
            }
            std::string const snbt = "{\"eventId\":\"PlayerCloseContainerEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(player.getDimensionId()))
                + ",\"block\":\"" + snbtEscape(name) + "\""
                + "," + playerRefSnbt(player) + "}";
            dispatchHookEvent(closeDef(), snbt);
        }

        LL_TYPE_INSTANCE_HOOK(
            CloseManagerHook,
            ll::memory::HookPriority::Normal,
            ServerPlayer,
            &ServerPlayer::doDeleteContainerManager,
            void,
            bool forceDisconnect)
        {
            if (closeDef().live() && mContainerManager)
            {
                if (auto* pos = std::get_if<::BlockPos>(&*mContainerManager->mScreenContext->mOwner); pos)
                {
                    ::BlockSource& region = this->getDimensionBlockSource();
                    if (region.getBlock(*pos).mBlockType->isContainerBlock())
                    {
                        report(*this, *pos, region);
                    }
                }
            }
            origin(forceDisconnect);
        }

        LL_TYPE_INSTANCE_HOOK(
            CloseByPistonHook,
            ll::memory::HookPriority::Normal,
            PistonBlockActor,
            &PistonBlockActor::_spawnMovingBlock,
            void,
            ::BlockSource& region,
            ::BlockPos const& blockPos)
        {
            if (closeDef().live() && region.getBlock(blockPos).mBlockType->isContainerBlock())
            {
                region.mDimension.forEachPlayer(
                    [&](::Player const& player) -> bool
                    {
                        if (player.mContainerManager)
                        {
                            auto* pos = std::get_if<::BlockPos>(&*player.mContainerManager->mScreenContext->mOwner);
                            if (pos && *pos == blockPos)
                            {
                                report(player, *pos, region);
                            }
                        }
                        return true;
                    });
            }
            origin(region, blockPos);
        }

        HookEventDef gDef{
            "PlayerCloseContainerEvent",
            []
            {
                int const r1 = CloseManagerHook::hook();
                int const r2 = CloseByPistonHook::hook();
                if (r1 != 0 || r2 != 0)
                {
                    hostLogger().error("{}", pier::trf("hooks.close_container_event.1", r1, r2));
                }
                return r1 == 0 && r2 == 0;
            }
        };
        HookEventDef& closeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
