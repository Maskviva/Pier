/** protect/ItemFrameOperateEvent.cpp: a player right-clicked an item frame.
 * `PlayerAttackItemFrameEvent` covers taking the item out with a left click and nothing
 * covered the right click, which rotates what is already in the frame. On a server where
 * frames carry a shop display or a puzzle, rotating is the whole attack.
 * `ItemFrameBlock::use` is hooked. It handles both putting an item in and rotating one
 * already there; the first is also seen by PlayerInteractBlockEvent and
 * PlayerUseItemOnEvent, so a subscriber that refuses here refuses a strict superset and
 * never the reverse.
 * Returning without calling origin leaves the frame untouched, which is the same result
 * as clicking a frame the engine decided not to act on.
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/ItemFrameBlock.h"
#include "mc/world/level/block/block_events/BlockPlayerInteractEvent.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& operateItemFrameDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            ItemFrameUseHook,
            ll::memory::HookPriority::Normal,
            ItemFrameBlock,
            &ItemFrameBlock::use,
            void,
            ::BlockEvents::BlockPlayerInteractEvent& eventData)
        {
            auto& def = operateItemFrameDef();
            if (!def.live()) return origin(eventData);

            // `TypedStorage` is an identity alias for a reference or a scalar and a
            // wrapper for anything else, so `mPlayer` is already a `Player&` while `mPos`
            // holds a class type and needs `get()`. Reaching for `get()` on both is the
            // obvious mistake; see ll/api/base/Alias.h.
            auto& p = eventData.mPlayer;
            auto const& pos = eventData.mPos.get();
            std::string snbt = "{\"eventId\":\"PlayerOperatedItemFrameEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + "," + playerRefSnbt(p) + "}";

            if (dispatchHookEventCancellable(def, snbt)) return;
            origin(eventData);
        }

        HookEventDef gDef{
            "PlayerOperatedItemFrameEvent",
            [] { return ItemFrameUseHook::hook() == 0; }};
        HookEventDef& operateItemFrameDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // PIER_BUILD_CLIENT
