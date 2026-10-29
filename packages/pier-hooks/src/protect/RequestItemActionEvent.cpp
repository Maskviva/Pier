/** protect/RequestItemActionEvent.cpp: the synthetic, cancellable "PlayerRequestItemActionEvent".
 * A move of a stack between slots, a removal, and the ingredients a craft consumes reach the
 * server as item stack requests that are applied one action at a time. The three handler
 * functions that apply the transfers, the removals and the consumes are hooked. Refusing one
 * returns the engine's own error result, which makes the client put its screen back the way the
 * server has it, so no stack is left on the cursor that the server never moved.
 * Payload {eventId, action, amount, srcContainer, srcSlot, dstContainer, dstSlot, x, y, z, dim,
 * _player}: `action` is the engine's action type number and the containers are its container
 * enum numbers. A removal or a consume has no destination worth reading, and its `dst` fields
 * are only what the client sent. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/containers/FullContainerName.h"
#include "mc/world/inventory/network/ItemStackNetResult.h"
#include "mc/world/inventory/network/ItemStackRequestActionConsume.h"
#include "mc/world/inventory/network/ItemStackRequestActionHandler.h"
#include "mc/world/inventory/network/ItemStackRequestActionTransferBase.h"
#include "mc/world/inventory/network/ItemStackRequestSlotInfo.h"
#include "mc/world/inventory/network/crafting/ItemStackRequestActionCraftHandler.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& requestDef(); // Forward declaration

        /** Asks the subscribers about one action, and says whether it is refused. */
        bool refused(::Player& player, ::ItemStackRequestActionTransferBase const& action)
        {
            auto& def = requestDef();
            if (!def.live()) return false;

            auto const& src = action.mSrc.get();
            auto const& dst = action.mDst.get();
            auto const& at = player.getPosition();
            std::string const snbt = "{\"eventId\":\"PlayerRequestItemActionEvent\""
                ",\"action\":" + snbtNum(static_cast<int>(action.mActionType))
                + ",\"amount\":" + snbtNum(static_cast<int>(action.mAmount))
                + ",\"srcContainer\":" + snbtNum(static_cast<int>(src.mFullContainerName.mName))
                + ",\"srcSlot\":" + snbtNum(static_cast<int>(src.mSlot))
                + ",\"dstContainer\":" + snbtNum(static_cast<int>(dst.mFullContainerName.mName))
                + ",\"dstSlot\":" + snbtNum(static_cast<int>(dst.mSlot))
                + ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(player.getDimensionId()))
                + "," + playerRefSnbt(player) + "}";
            return dispatchHookEventCancellable(def, snbt);
        }

        LL_TYPE_INSTANCE_HOOK(
            RequestTransferHook,
            ll::memory::HookPriority::Normal,
            ItemStackRequestActionHandler,
            &ItemStackRequestActionHandler::_handleTransfer,
            ::ItemStackNetResult,
            ::ItemStackRequestActionTransferBase const& requestAction,
            bool isSrcHintSlot,
            bool isDstHintSlot,
            bool isSwap)
        {
            ::Player& player = this->mPlayer;
            if (refused(player, requestAction)) return ::ItemStackNetResult::Error;
            return origin(requestAction, isSrcHintSlot, isDstHintSlot, isSwap);
        }

        LL_TYPE_INSTANCE_HOOK(
            RequestRemoveHook,
            ll::memory::HookPriority::Normal,
            ItemStackRequestActionHandler,
            &ItemStackRequestActionHandler::_handleRemove,
            ::ItemStackNetResult,
            ::ItemStackRequestActionTransferBase const& requestAction,
            ::ItemStack& removedItem,
            ::ItemStackRequestActionHandler::RemoveType removeType)
        {
            ::Player& player = this->mPlayer;
            if (refused(player, requestAction)) return ::ItemStackNetResult::Error;
            return origin(requestAction, removedItem, removeType);
        }

        LL_TYPE_INSTANCE_HOOK(
            RequestConsumeHook,
            ll::memory::HookPriority::Normal,
            ItemStackRequestActionCraftHandler,
            &ItemStackRequestActionCraftHandler::handleConsume,
            ::ItemStackNetResult,
            ::ItemStackRequestActionConsume const& requestAction)
        {
            ::Player& player = this->mPlayer;
            if (refused(player, requestAction)) return ::ItemStackNetResult::Error;
            return origin(requestAction);
        }

        HookEventDef gDef{
            "PlayerRequestItemActionEvent",
            []
            {
                int const r1 = RequestTransferHook::hook();
                int const r2 = RequestRemoveHook::hook();
                int const r3 = RequestConsumeHook::hook();
                if (r1 != 0 || r2 != 0 || r3 != 0)
                {
                    hostLogger().error(
                        "{}", pier::trf("hooks.request_item_action_event.1", r1, r2, r3));
                }
                return r1 == 0 && r2 == 0 && r3 == 0;
            }
        };
        HookEventDef& requestDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
