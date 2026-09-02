/** hooks/protect/TakeEntityEvent.cpp: the synthetic, cancellable "PlayerTakeEntityEvent".
 * It fills the gap in PlayerPickUpItemEvent from LeviLamina, which is published only for
 * ActorCategory::Item while a landed arrow or trident is still a projectile actor and never
 * reaches the decision. The two cover complementary sets with no overlap: dropped items belong to
 * that event, arrows and tridents to this one, and covering both means subscribing to both.
 * The hook points are the playerTouch virtual of each projectile and not Player::take. Arrow and
 * ThrownTrident each implement playerTouch themselves and put the item straight into the
 * inventory without going through Player::take. Covering another projectile that slips past takes
 * one more PIER_PICKUP_HOOK line.
 * Payload {eventId, x, y, z, dim, entity, entityId, isItemActor, item, _player:{...}}.
 * isItemActor is always false with the current hook set and is kept so the payload shape stays
 * stable. x, y and z are truncated to integers, because LL reflection serializes a Vec3 as a JSON
 * array. / */
#include "pier/hooks/hook_events.h"

#include <set>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorCategory.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/projectile/Arrow.h"
#include "mc/world/actor/projectile/ThrownTrident.h"
#include "mc/world/item/ItemStack.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& takeDef(); // Forward declaration; gDef is defined at the end of this file

        /** The type name of the actor being picked up. An empty string when it cannot be
         *  read, never a guess. */
        std::string safeActorType(Actor const& a)
        {
            try
            {
                return std::string{a.getTypeName()};
            }
            catch (...)
            {
                return {};
            }
        }

        /** What a dropped item contains. An empty string for anything else. */
        std::string carriedItemName(Actor const& a, bool isItem)
        {
            if (!isItem) return {};
            try
            {
                auto const& stack = static_cast<ItemActor const&>(a).item();
                return std::string{stack.getTypeName()};
            }
            catch (...)
            {
                return {};
            }
        }

        std::string buildSnbt(Player& p, Actor const& taken, bool isItem)
        {
            auto const pos = p.getPosition();
            return std::string{"{\"eventId\":\"PlayerTakeEntityEvent\",\"x\":"}
                + snbtNum(static_cast<int>(pos.x)) + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"entity\":\"" + snbtEscape(safeActorType(taken))
                + "\",\"entityId\":"
                + snbtNum(static_cast<int64_t>(taken.getOrCreateUniqueID().rawID))
                + "L,\"isItemActor\":" + (isItem ? "true" : "false")
                + ",\"item\":\"" + snbtEscape(carriedItemName(taken, isItem))
                + "\"," + playerRefSnbt(p) + "}";
        }

        /** Proof of arrival, once per actor type. A hook that never installed, one
         *  attached to the wrong function, and one installed correctly whose decision
         *  allows the action all look identical, and this line is the only thing that
         *  tells them apart. */
        void logFirstTouch(Actor const& a)
        {
            static std::set<std::string> seen;
            std::string const key = safeActorType(a);
            if (seen.insert(key).second)
            {
                hostLogger().debug("[hooks/TakeEntityEvent] first touch of '{}'", key);
            }
        }

        /**
         * Intercepts the pickup of one projectile kind.
         *
         * playerTouch returns void and carries no cancel bit, so interception means not
         * calling origin: the touch did not happen, the actor stays where it is, and
         * walking over it again retries.
         */
#define PIER_PICKUP_HOOK(HookName, ActorClass)                                                  \
    LL_TYPE_INSTANCE_HOOK(                                                                      \
        /* A virtual must be hooked through the $ prefixed alias, which LeviLamina uses    \
         * to bypass vtable dispatch; taking &Cls::playerTouch directly is stopped by a     \
         * static_assert. Same as DropItemEvent. */                                         \
        HookName, ll::memory::HookPriority::Normal, ActorClass, &ActorClass::$playerTouch, void, \
        ::Player& player)                                                                       \
    {                                                                                           \
        auto& def = takeDef();                                                                  \
        if (!def.live())                                                                        \
        {                                                                                       \
            origin(player);                                                                     \
            return;                                                                             \
        }                                                                                       \
        logFirstTouch(*this);                                                                   \
        if (dispatchHookEventCancellable(def, buildSnbt(player, *this, false)))                 \
        {                                                                                       \
            /* origin is not called: the touch did not happen, the actor stays, retryable. */ \
            return;                                                                             \
        }                                                                                       \
        origin(player);                                                                         \
    }

        PIER_PICKUP_HOOK(ArrowPickupHook, Arrow)
        PIER_PICKUP_HOOK(TridentPickupHook, ThrownTrident)

#undef PIER_PICKUP_HOOK

        HookEventDef gDef{
            "PlayerTakeEntityEvent",
            []
            {
                // Installed explicitly and the status reported explicitly, where 0 is
                // success. Protection that never installed and protection that installed
                // but never blocks behave identically, so a failed install must be
                // visible.
                int ra = ArrowPickupHook::hook();
                int rt = TridentPickupHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[hooks/TakeEntityEvent] installing detours: Arrow::$playerTouch={} (code={}), "
                    "ThrownTrident::$playerTouch={} (code={})",
                    ra == 0 ? "ok" : "failed", ra,
                    rt == 0 ? "ok" : "failed", rt);
                if (ra != 0 || rt != 0)
                {
                    log.error(
                        "[hooks/TakeEntityEvent] a native detour failed to install with a "
                        "non-zero status, so pickup protection is entirely inactive for "
                        "projectiles such as arrows and tridents, while dropped items are "
                        "still covered by PlayerPickUpItemEvent. The usual cause is a "
                        "mismatch between the BDS or LeviLamina version this host was linked "
                        "against and the one the server runs, so the symbol address of "
                        "$playerTouch resolved wrongly.");
                }
                return ra == 0 && rt == 0;
            }
        };

        HookEventDef& takeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
