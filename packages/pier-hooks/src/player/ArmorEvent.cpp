/** player/ArmorEvent.cpp: the synthetic, cancellable "PlayerSetArmorEvent".
 * Actor::setArmor is where the engine stores a piece of armor in a slot, so it is the one place
 * a rule such as "no helmets in this arena" or "the kit cannot be changed" can stand without
 * knowing which of the ways of equipping reached it. Only players are asked about. Returning
 * without calling origin leaves the slot as it was.
 * Payload {eventId, slot, item, count, x, y, z, dim, _player}: `slot` is the engine's armor slot
 * number, and `item` is what is being put there, an empty text when the slot is being emptied.
 * `partial` is 1 when the item could not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/shared_types/legacy/actor/ArmorSlot.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& armorDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            ActorSetArmorHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::$setArmor,
            void,
            ::SharedTypes::Legacy::ArmorSlot slot,
            ::ItemStack const& item)
        {
            auto& def = armorDef();
            if (!def.live() || !this->isPlayer()) return origin(slot, item);

            std::string name;
            int count = 0;
            bool readFailed = false;
            try
            {
                if (!item.isNull())
                {
                    name = item.getTypeName();
                    count = item.mCount;
                }
            }
            catch (...)
            {
                name.clear();
                readFailed = true;
            }
            ::Player& player = static_cast<::Player&>(static_cast<::Actor&>(*this));
            auto const& at = this->getPosition();
            std::string const snbt = "{\"eventId\":\"PlayerSetArmorEvent\""
                ",\"slot\":" + snbtNum(static_cast<int>(slot))
                + ",\"item\":\"" + snbtEscape(name) + "\""
                + ",\"count\":" + snbtNum(count)
                + ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + (readFailed ? ",\"partial\":1" : "")
                + "," + playerRefSnbt(player) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(slot, item);
        }

        HookEventDef gDef{"PlayerSetArmorEvent", [] { return ActorSetArmorHook::hook() == 0; }};
        HookEventDef& armorDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
