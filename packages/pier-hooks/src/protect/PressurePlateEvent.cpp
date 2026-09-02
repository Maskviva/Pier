/** hooks/protect/PressurePlateEvent.cpp: the synthetic, cancellable
 * "PlayerStepOnPressurePlateEvent".
 * A pressure plate has no player action to hook: walking is the action and the plate is a
 * side effect, so from the engine's view the player did nothing. What runs the trigger
 * logic is entityInside, where BasePressurePlateBlock covers every plate type and
 * TripWireBlock goes through _checkPressed. It returns void, so cancelling means not
 * calling origin: the plate does not depress, the tripwire does not fire and no redstone
 * signal is emitted. shouldTriggerEntityInside is kept as a cheap early exit, at the cost
 * of one extra cache lookup, so the behavior does not depend on which one a given build
 * happens to call.
 * Both virtuals run once per tick for every actor inside the block, so the throttle cache
 * is a requirement and not an optimization; decision_throttle.h explains the key choice.
 * Payload {eventId, x, y, z, dim, kind, _player:{...}}, where kind is "pressure_plate" or
 * "tripwire". */
#include "pier/hooks/decision_throttle.h"
#include "pier/hooks/hook_events.h"
#include "pier/support/log.h"

#include <string>
#include <unordered_map>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/BasePressurePlateBlock.h"
#include "mc/world/level/block/TripWireBlock.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& plateDef();      // Forward: a player steps on it
        HookEventDef& actorPlateDef(); // Forward: a non-player actor steps on it

        std::unordered_map<std::string, ThrottledDecision>& plateCache()
        {
            static std::unordered_map<std::string, ThrottledDecision> c;
            return c;
        }

        /** Non-player actors use a separate table. Both key kinds in one table would
         *  crowd each other out of the 512-entry cap, and actors outnumber players by
         *  orders of magnitude. */
        std::unordered_map<std::string, ThrottledDecision>& actorPlateCache()
        {
            static std::unordered_map<std::string, ThrottledDecision> c;
            return c;
        }

        /**
         * Returns true when this trigger must be refused. Each (player, block position)
         * dispatches at most once within kDecisionTtlMs.
         */
        bool refuseTrigger(
            ::Actor& entity, ::BlockSource& region, ::BlockPos const& pos, char const* kind)
        {
            bool const isPlayer = entity.isPlayer();
            auto& def = isPlayer ? plateDef() : actorPlateDef();
            if (!def.live()) return false;

            Player* p = isPlayer ? static_cast<Player*>(&entity) : nullptr;

            // The throttle key uses the xuid for a player, since pointers are recycled
            // (see decision_throttle.h), and the type name plus the actor id for anything
            // else, since an id is likewise never reused and is safer than a raw
            // pointer.
            std::string key;
            if (p)
            {
                key = p->getXuid();
                if (key.empty()) key = p->getRealName(); // An offline-mode server
            }
            else
            {
                try
                {
                    key = std::to_string(entity.getOrCreateUniqueID().rawID);
                }
                catch (...)
                {
                    // An actor whose id cannot even be read is not worth dispatching for.
                    return false;
                }
            }

            int const dim = static_cast<int>(region.getDimensionId());
            long long const now = throttleNowMs();

            bool cached = false;
            auto& cache = isPlayer ? plateCache() : actorPlateCache();
            if (throttleLookup(cache, key, pos.x, pos.y, pos.z, dim, now, cached))
            {
                return cached;
            }

            std::string snbt = isPlayer
                ? std::string{"{\"eventId\":\"PlayerStepOnPressurePlateEvent\""}
                : std::string{"{\"eventId\":\"ActorStepOnPressurePlateEvent\""};
            snbt += ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"kind\":\"" + kind + "\"";
            if (p)
            {
                snbt += "," + playerRefSnbt(*p);
            }
            else
            {
                std::string type;
                try
                {
                    type = entity.getTypeName();
                }
                catch (...)
                {
                    type.clear();
                }
                snbt += ",\"actor\":\"" + snbtEscape(type) + "\",\"actorId\":" + key + "L";
            }
            snbt += "}";

            bool const cancelled = dispatchHookEventCancellable(def, snbt);
            throttleStore(cache, key, pos.x, pos.y, pos.z, dim, now, cancelled);
            return cancelled;
        }

        // The path that actually runs the trigger logic.

        LL_TYPE_INSTANCE_HOOK(
            PressurePlateInsideHook,
            ll::memory::HookPriority::Normal,
            BasePressurePlateBlock,
            &BasePressurePlateBlock::$entityInside,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "pressure_plate")) return;
            origin(region, pos, entity);
        }

        LL_TYPE_INSTANCE_HOOK(
            TripWireInsideHook,
            ll::memory::HookPriority::Normal,
            TripWireBlock,
            &TripWireBlock::$entityInside,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "tripwire")) return;
            origin(region, pos, entity);
        }

        // A cheap early exit, effective on builds where the engine really consults it.

        LL_TYPE_INSTANCE_HOOK(
            PressurePlateShouldTriggerHook,
            ll::memory::HookPriority::Normal,
            BasePressurePlateBlock,
            &BasePressurePlateBlock::$shouldTriggerEntityInside,
            bool,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "pressure_plate")) return false;
            return origin(region, pos, entity);
        }

        LL_TYPE_INSTANCE_HOOK(
            TripWireShouldTriggerHook,
            ll::memory::HookPriority::Normal,
            TripWireBlock,
            &TripWireBlock::$shouldTriggerEntityInside,
            bool,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor& entity)
        {
            if (refuseTrigger(entity, region, pos, "tripwire")) return false;
            return origin(region, pos, entity);
        }

        HookEventDef gDef{
            "PlayerStepOnPressurePlateEvent",
            []
            {
                int const r1 = PressurePlateInsideHook::hook();
                int const r2 = TripWireInsideHook::hook();
                int const r3 = PressurePlateShouldTriggerHook::hook();
                int const r4 = TripWireShouldTriggerHook::hook();
                if (r1 != 0 || r2 != 0 || r3 != 0 || r4 != 0)
                {
                    hostLogger().error(
                        "[hooks/PressurePlateEvent] a detour failed to install (codes: {} {} {} {}), so the subscription is refused",
                        r1, r2, r3, r4);
                }
                return r1 == 0 && r2 == 0 && r3 == 0 && r4 == 0;
            }
        };
        HookEventDef& plateDef() { return gDef; }

        // One set of detours serves two event ids, as in RideEvent.
        HookEventDef gActorDef{
            "ActorStepOnPressurePlateEvent",
            []
            {
                int const r1 = PressurePlateInsideHook::hook();
                int const r2 = TripWireInsideHook::hook();
                int const r3 = PressurePlateShouldTriggerHook::hook();
                int const r4 = TripWireShouldTriggerHook::hook();
                return r1 == 0 && r2 == 0 && r3 == 0 && r4 == 0;
            }
        };
        HookEventDef& actorPlateDef() { return gActorDef; }

        HookEventRegistrar gReg{gDef};
        HookEventRegistrar gActorReg{gActorDef};
    } // namespace
} // namespace pier::hooks
