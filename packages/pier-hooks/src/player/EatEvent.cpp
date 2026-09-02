/** player/EatEvent.cpp: a player finished eating or drinking what they held.
 * completeUsingItem is where a use timer running out ends up: food, potions, milk, a
 * spyglass and a raised shield all leave through it. The event is emitted before origin,
 * so the item in the payload is still the one about to be consumed; asking afterwards
 * yields air or an empty bottle.
 * Observation only. Cancelling on this path leaves the player stuck holding the item
 * forever, with the client having played the animation while the server never settled it.
 * Banning a food means stopping the use at the start, in PlayerUseItemEvent in LL. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackBase.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& eatDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerCompleteUsingItemHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::completeUsingItem,
            void)
        {
            auto& def = eatDef();
            if (!def.live()) return origin();

            std::string item;
            int count = 0;
            try
            {
                ::ItemStack const& held = this->getSelectedItem();
                if (!held.isNull())
                {
                    item = held.getTypeName();
                    count = held.mCount;
                }
            }
            catch (...)
            {
                item.clear();
            }

            std::string snbt = "{\"eventId\":\"PlayerUseItemCompleteEvent\""
                ",\"item\":\"" + snbtEscape(item) + "\""
                + ",\"count\":" + snbtNum(count)
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + "," + playerRefSnbt(*this) + "}";
            dispatchHookEvent(def, snbt);

            origin();
        }

        HookEventDef gDef{
            "PlayerUseItemCompleteEvent",
            []
            {
                int const r = PlayerCompleteUsingItemHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/EatEvent] the Player::completeUsingItem detour failed to install "
                        "with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& eatDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
