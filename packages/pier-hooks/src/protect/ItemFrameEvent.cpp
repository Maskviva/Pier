/** protect/ItemFrameEvent.cpp: a player hit an item frame.
 * An item frame, like an armor stand, is a blind spot for protection: one left click
 * knocks the item out, and that path is neither breaking a block, since the frame
 * remains, nor hitting an actor, since the frame is a block. The symptom is a stolen
 * display with nothing in the log.
 * ItemFrameBlock::$attack is hooked, which is taking the item with a left click. Putting
 * an item in goes through block interaction and is already covered by
 * PlayerInteractBlockEvent in LL and PlayerUseItemOnEvent in this package. Cancelling
 * makes the hit have no effect, and returning false is the engine's own hit-without-a-
 * response path. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/ItemFrameBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& itemFrameDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            ItemFrameAttackHook,
            ll::memory::HookPriority::Normal,
            ItemFrameBlock,
            &ItemFrameBlock::$attack,
            bool,
            ::Player* player,
            ::BlockPos const& pos)
        {
            auto& def = itemFrameDef();
            // Without a player there is no ownership to judge, as with a falling block or
            // a piston, so it passes through unchanged.
            if (!def.live() || player == nullptr) return origin(player, pos);

            std::string snbt = "{\"eventId\":\"PlayerAttackItemFrameEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(player->getDimensionId()))
                + "," + playerRefSnbt(*player) + "}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return origin(player, pos);
        }

        HookEventDef gDef{
            "PlayerAttackItemFrameEvent",
            []
            {
                int const r = ItemFrameAttackHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/ItemFrameEvent] the ItemFrameBlock::$attack detour failed to "
                        "install with code={}, so items in an item frame are unprotected", r);
                }
                return r == 0;
            }
        };
        HookEventDef& itemFrameDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
