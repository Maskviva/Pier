/** player/TotemEvent.cpp: the synthetic, cancellable "PlayerConsumeTotemEvent".
 * A totem of undying saves a player from a death that another rule may want to stand: a game
 * where a hit is final, an arena that counts deaths, a duel with no revives. ActorHurtEvent
 * sees the damage and not the totem, so a rule that refuses only the revive has no event of
 * its own to hang on. Player::consumeTotem is the engine's own question of whether a totem
 * saves the player. Returning false answers no without calling origin, so no totem is spent
 * and the death proceeds.
 * Payload {eventId, x, y, z, dim, _player}. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& totemDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerConsumeTotemHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$consumeTotem,
            bool)
        {
            auto& def = totemDef();
            if (!def.live()) return origin();

            auto const& pos = this->getPosition();
            std::string const snbt = "{\"eventId\":\"PlayerConsumeTotemEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + "," + playerRefSnbt(*this) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return false;
            }
            return origin();
        }

        HookEventDef gDef{"PlayerConsumeTotemEvent", [] { return PlayerConsumeTotemHook::hook() == 0; }};
        HookEventDef& totemDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
