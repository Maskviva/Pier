/** hooks/protect/PushEntityEvent.cpp: the synthetic, cancellable
 * "PlayerPushEntityEvent".
 * Pushing needs no click and is the one form of griefing that survives a fully locked
 * claim while leaving no log: a visitor can herd livestock out of a fence, shove a boat
 * into the void, or move an item frame away.
 * The hook point is the free function PushableByEntityUtility::skipPush, hence
 * LL_STATIC_HOOK. It is the engine's own question of whether this push should be skipped,
 * and returning true is an outcome every caller already handles, while refusing inside
 * the push itself would leave two actors overlapping with the collision unresolved.
 * Collision resolution runs from both sides and the player may arrive as owner or as
 * other, so both are recognized. Permission is decided at the position of the actor being
 * pushed, because a player standing outside the boundary can push an animal inside. With
 * players on both sides nothing happens, since that is ordinary movement. Throttling is
 * described in decision_throttle.h.
 * Payload {eventId, x, y, z, dim, target, _player:{name,xuid,uuid}}. */
#include "pier/hooks/decision_throttle.h"
#include "pier/hooks/hook_events.h"

#include <string>
#include <unordered_map>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/util/PushableByEntityUtility.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& pushDef(); // Forward declaration

        std::unordered_map<std::string, ThrottledDecision>& pushCache()
        {
            static std::unordered_map<std::string, ThrottledDecision> c;
            return c;
        }

        LL_STATIC_HOOK(
            PlayerPushEntityHook,
            ll::memory::HookPriority::Normal,
            &::PushableByEntityUtility::skipPush,
            bool,
            ::Actor& owner,
            ::Actor& other)
        {
            auto& def = pushDef();
            if (!def.live()) return origin(owner, other);

            // Either side may be the player, see the file header.
            ::Actor* pusher = nullptr;
            ::Actor* target = nullptr;
            if (owner.isPlayer() && !other.isPlayer())
            {
                pusher = &owner;
                target = &other;
            }
            else if (other.isPlayer() && !owner.isPlayer())
            {
                pusher = &other;
                target = &owner;
            }
            else
            {
                // Neither side is a player, or both are. A mob pushing a mob is world
                // behavior and a player pushing a player is ordinary movement.
                return origin(owner, other);
            }

            auto& p = *static_cast<Player*>(pusher);

            std::string key = p.getXuid();
            if (key.empty()) key = p.getRealName(); // An offline-mode server

            auto const& tpos = target->getPosition();
            int const x = static_cast<int>(tpos.x);
            int const y = static_cast<int>(tpos.y);
            int const z = static_cast<int>(tpos.z);
            int const dim = static_cast<int>(target->getDimensionId());

            long long const now = throttleNowMs();
            bool cached = false;
            if (throttleLookup(pushCache(), key, x, y, z, dim, now, cached))
            {
                return cached ? true : origin(owner, other);
            }

            // getTypeName throws while an actor is being destroyed. This path runs every
            // tick and an exception crossing a detour takes the whole server down, so it
            // is caught here.
            std::string targetName;
            try
            {
                targetName = target->getTypeName();
            }
            catch (...)
            {
                targetName.clear();
            }

            std::string snbt = "{\"eventId\":\"PlayerPushEntityEvent\""
                ",\"x\":" + snbtNum(x)
                + ",\"y\":" + snbtNum(y)
                + ",\"z\":" + snbtNum(z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"target\":\"" + snbtEscape(targetName)
                + "\"," + playerRefSnbt(p) + "}";

            bool const cancelled = dispatchHookEventCancellable(def, snbt);
            throttleStore(pushCache(), key, x, y, z, dim, now, cancelled);

            // true means skip this push, which is exactly what cancelling means.
            return cancelled ? true : origin(owner, other);
        }

        HookEventDef gDef{"PlayerPushEntityEvent", [] { return PlayerPushEntityHook::hook() == 0; }};
        HookEventDef& pushDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
