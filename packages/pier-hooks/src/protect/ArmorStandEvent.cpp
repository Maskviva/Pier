/** protect/ArmorStandEvent.cpp: a player swapping equipment with an armor stand.
 * An armor stand is a hole in protection: it is not a container, so
 * PlayerOpenContainerEvent does not see it, and not a block, so block protection does not
 * cover it, while one right click moves a whole set of equipment onto the player. On a
 * server that displays gear, that is emptying a display case with a single click.
 * Cancelling means the swap does not happen, and `_trySwapItem` returning false is the
 * engine's own cannot-swap path, leaving the items on both sides untouched. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/shared_types/legacy/EquipmentSlot.h"
#include "mc/world/actor/ArmorStand.h"
#include "mc/world/actor/Actor.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& armorStandDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            ArmorStandSwapItemHook,
            ll::memory::HookPriority::Normal,
            ArmorStand,
            &ArmorStand::_trySwapItem,
            bool,
            ::Player& player,
            ::SharedTypes::Legacy::EquipmentSlot slot)
        {
            auto& def = armorStandDef();
            if (!def.live()) return origin(player, slot);

            auto const& pos = this->getPosition();
            std::string snbt = "{\"eventId\":\"ArmorStandSwapItemEvent\""
                ",\"x\":" + snbtDouble(pos.x)
                + ",\"y\":" + snbtDouble(pos.y)
                + ",\"z\":" + snbtDouble(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + ",\"slot\":" + snbtNum(static_cast<int>(slot))
                + ",\"standId\":" + snbtNum(static_cast<int64_t>(this->getOrCreateUniqueID().rawID)) + "L"
                + "," + playerRefSnbt(player) + "}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return origin(player, slot);
        }

        HookEventDef gDef{
            "ArmorStandSwapItemEvent",
            []
            {
                int const r = ArmorStandSwapItemHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/ArmorStandEvent] the ArmorStand::_trySwapItem detour failed to "
                        "install with code={}, so equipment on an armor stand is "
                        "unprotected", r);
                }
                return r == 0;
            }
        };
        HookEventDef& armorStandDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
