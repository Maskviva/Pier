/** player/EffectEvents.cpp: "ActorGetEffectEvent", cancellable, and "ActorRemoveEffectEvent",
 * observation only.
 * A beacon, a potion, a poisonous bite or a command gives an actor an effect, and nothing told
 * a mod, though a minigame bans speed, an arena refuses regeneration and a shop sells effects.
 * Actor::addEffect is the engine's one door for a new or renewed effect, so the hook asks right
 * before it, and returning without calling origin leaves the actor as it was. Removal is
 * reported from Actor::onEffectRemoved, which the engine runs after the effect is gone, so it
 * cannot be refused: cancelling there would leave the modifiers of an effect that no longer
 * exists. Both dispatch before origin.
 * Payload {eventId, effect, effectId, amplifier, duration, updated, x, y, z, dim, actor, actorId,
 * _player}: `effect` is the engine's name for it, `updated` is 1 when the actor already had that
 * effect, and `_player` appears only for a player. The removal payload has no `updated`.
 * A field that could not be read leaves the payload with `partial` 1. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/effect/MobEffect.h"
#include "mc/world/effect/MobEffectIds.h"
#include "mc/world/effect/MobEffectInstance.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& getEffectDef(); // Forward declarations
        HookEventDef& removeEffectDef();

        /** The engine's name of an effect, or an empty text for an id the table does not hold. */
        std::string effectName(unsigned int id)
        {
            auto const& table = ::MobEffect::mMobEffects();
            if (id >= table.size()) return {};
            auto const& slot = table[static_cast<::MobEffectIds>(id)];
            if (!slot) return {};
            return slot->mComponentName->getString();
        }

        /** `updated` is 1 or 0 for an effect being given and -1 for one being removed. */
        std::string buildSnbt(char const* eventId, ::Actor& actor, ::MobEffectInstance const& effect, int updated)
        {
            std::string name;
            std::string actorType;
            int64_t actorId = 0;
            bool readFailed = false;
            try
            {
                name = effectName(effect.mId);
                actorType = actor.getTypeName();
                actorId = actor.getOrCreateUniqueID().rawID;
            }
            catch (...)
            {
                readFailed = true;
            }
            auto const& at = actor.getPosition();
            return std::string{"{\"eventId\":\""} + eventId + "\""
                + ",\"effect\":\"" + snbtEscape(name) + "\""
                + ",\"effectId\":" + snbtNum(effect.mId)
                + ",\"amplifier\":" + snbtNum(effect.mAmplifier)
                + ",\"duration\":" + snbtNum(effect.mDuration->mValue)
                + (updated < 0 ? std::string{} : ",\"updated\":" + std::string{updated != 0 ? "1" : "0"})
                + ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(actor.getDimensionId()))
                + ",\"actor\":\"" + snbtEscape(actorType) + "\""
                + ",\"actorId\":" + snbtNum(actorId) + "L"
                + (readFailed ? ",\"partial\":1" : "")
                + (actor.isPlayer() ? "," + playerRefSnbt(static_cast<::Player&>(actor)) : std::string{}) + "}";
        }

        LL_TYPE_INSTANCE_HOOK(
            ActorAddEffectHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::addEffect,
            void,
            ::MobEffectInstance const& effect)
        {
            auto& def = getEffectDef();
            if (!def.live()) return origin(effect);

            bool updated = false;
            try
            {
                updated = this->getEffect(effect.mId) != nullptr;
            }
            catch (...)
            {
                updated = false;
            }
            if (dispatchHookEventCancellable(def, buildSnbt("ActorGetEffectEvent", *this, effect, updated ? 1 : 0)))
            {
                return;
            }
            origin(effect);
        }

        LL_TYPE_INSTANCE_HOOK(
            ActorEffectRemovedHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::$onEffectRemoved,
            void,
            ::MobEffectInstance& effect)
        {
            auto& def = removeEffectDef();
            if (def.live())
            {
                dispatchHookEvent(def, buildSnbt("ActorRemoveEffectEvent", *this, effect, -1));
            }
            origin(effect);
        }

        HookEventDef gGetDef{"ActorGetEffectEvent", [] { return ActorAddEffectHook::hook() == 0; }};
        HookEventDef& getEffectDef() { return gGetDef; }

        HookEventDef gRemoveDef{"ActorRemoveEffectEvent", [] { return ActorEffectRemovedHook::hook() == 0; }};
        HookEventDef& removeEffectDef() { return gRemoveDef; }

        HookEventRegistrar gGetReg{gGetDef};
        HookEventRegistrar gRemoveReg{gRemoveDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
