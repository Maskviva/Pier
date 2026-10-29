/** protect/FishingHookEvent.cpp: the synthetic, cancellable "PlayerPullFishingHookEvent".
 * A fishing rod reels in whatever its hook caught, and that is not only fish: an item on the
 * ground, a mob, another player. It makes the rod a way to take things from, and to drag people
 * out of, a plot it was cast from outside. FishingHook::_pullCloser is the engine's single place
 * where the hook drags a caught actor toward the angler, so the hook asks right before.
 * Returning without calling origin leaves the actor where it is.
 * Payload {eventId, x, y, z, dim, pulled, pulledId, pulledIsPlayer, item, _player}: the actor
 * being pulled, and `item` when it is an item on the ground. A hook with no player owner is not
 * asked about, and `partial` is 1 when the pulled actor could not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/FishingHook.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& fishingDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            FishingPullHook,
            ll::memory::HookPriority::Normal,
            FishingHook,
            &FishingHook::_pullCloser,
            void,
            ::Actor& inEntity,
            float inSpeed)
        {
            auto& def = fishingDef();
            if (!def.live()) return origin(inEntity, inSpeed);

            ::Player* owner = this->getPlayerOwner();
            if (owner == nullptr) return origin(inEntity, inSpeed);

            std::string pulled;
            std::string item;
            bool pulledIsPlayer = false;
            int64_t pulledId = 0;
            bool readFailed = false;
            try
            {
                pulled = inEntity.getTypeName();
                pulledIsPlayer = inEntity.isPlayer();
                pulledId = inEntity.getOrCreateUniqueID().rawID;
                if (inEntity.isType(::ActorType::ItemEntity))
                {
                    item = static_cast<::ItemActor&>(inEntity).item().getTypeName();
                }
            }
            catch (...)
            {
                pulled.clear();
                item.clear();
                readFailed = true;
            }
            auto const& at = inEntity.getPosition();
            std::string const snbt = "{\"eventId\":\"PlayerPullFishingHookEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(inEntity.getDimensionId()))
                + ",\"pulled\":\"" + snbtEscape(pulled) + "\""
                + ",\"pulledId\":" + snbtNum(pulledId) + "L"
                + ",\"pulledIsPlayer\":" + (pulledIsPlayer ? "1" : "0")
                + ",\"item\":\"" + snbtEscape(item) + "\""
                + (readFailed ? ",\"partial\":1" : "")
                + "," + playerRefSnbt(*owner) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(inEntity, inSpeed);
        }

        HookEventDef gDef{"PlayerPullFishingHookEvent", [] { return FishingPullHook::hook() == 0; }};
        HookEventDef& fishingDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
