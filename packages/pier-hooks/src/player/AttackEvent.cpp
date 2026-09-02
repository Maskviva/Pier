/** hooks/player/AttackEvent.cpp: the synthetic, cancellable "PlayerAttackTargetEvent".
 * PlayerAttackEvent in LeviLamina is cancellable, but the target in its payload is the LL
 * reflection serialization of an Actor&, carrying only a raw pointer and the static type name,
 * which is always "Actor", so it cannot tell a player from a mob. A permission layer has to
 * separate those two: without the distinction, turning the pvp flag off blocks attacking mobs as
 * well. This event adds targetIsPlayer and the dynamic type name of the target.
 * Cancelling returns a value-initialized ActorHurtResult, all zeros meaning no damage, without
 * calling origin. Both Player::attack overloads are hooked, since which one is the implementation
 * and which the forwarder cannot be confirmed here and hooking the wrong one makes the protection
 * fail silently. A thread_local re-entry gate ensures one attack dispatches once, otherwise a
 * counting subscriber would count one attack twice. Payload {eventId, x, y, z, dim, target,
 * targetIsPlayer, cause, _player:{...}}. x, y and z are the target position and not the attacker
 * position, matching InteractEntityEvent and RideEvent; disagreeing would put hitting and right-
 * clicking the same sheep on two different plots. / */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorHurtResult.h"
#include "mc/world/actor/player/Player.h"
// ActorDamageCause has no header of its own. It is SharedTypes::Legacy::ActorDamageCause
// and arrives transitively through Player.h, as it does in the PIER_PACT_ATTACK branch of
// actors/Players.cpp.

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& attackDef(); // Forward declaration

        /**
         * Player::attack has two overloads and &Player::attack is ambiguous on its own
         * (C2664), so a typedef comes first and then a static_cast. The typedef cannot be
         * dropped: the comma inside static_cast<A (T::*)(X, Y)> would be taken by the
         * preprocessor as a macro argument separator.
         *
         * The two-argument one overrides Mob::attack, and a virtual must be hooked
         * through the $ alias LeviLamina generates, otherwise a static_assert stops it.
         * Whether the three-argument one is virtual is unconfirmed: if it reports the
         * same static_assert, change &Player::attack in PlayerAttackHook3 to
         * &Player::$attack, and if it reports no matching function, delete
         * PlayerAttackHook3 and r3.
         */
        using AttackFn2 = ::ActorHurtResult (Player::*)(
            ::Actor&,
            ::SharedTypes::Legacy::ActorDamageCause const&);
        using AttackFn3 = ::ActorHurtResult (Player::*)(
            ::Actor&,
            ::SharedTypes::Legacy::ActorDamageCause const&,
            ::Player::AttackParameters const&);

        /** The dispatch-once rule from the file header. Counted per thread, since
         *  concurrent attacks are not re-entry. */
        thread_local int tlAttackDepth = 0;

        struct AttackDepthGuard
        {
            AttackDepthGuard() { ++tlAttackDepth; }
            ~AttackDepthGuard() { --tlAttackDepth; }
        };

        /**
         * A value-initialized ActorHurtResult on a block, all zeros meaning no damage. The
         * field names are unconfirmed, so no field is set individually. Health still
         * dropping after a block means this assumption is wrong.
         */
        ::ActorHurtResult refusedHurt() { return ::ActorHurtResult{}; }

        std::string buildAttackSnbt(Player& self, ::Actor& actor, int cause)
        {
            // getTypeName and isPlayer throw while an actor is being destroyed, and an
            // exception crossing a detour takes the whole server down, so they are caught
            // here. A subscriber reading an empty string falls back to a coarser decision,
            // which is never more permissive.
            std::string targetName;
            bool isPlayerTarget = false;
            try
            {
                targetName = actor.getTypeName();
                isPlayerTarget = actor.isPlayer();
            }
            catch (...)
            {
                targetName.clear();
                isPlayerTarget = false;
            }

            auto const& pos = actor.getPosition();
            return std::string{"{\"eventId\":\"PlayerAttackTargetEvent\""}
                + ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(actor.getDimensionId()))
                + ",\"targetIsPlayer\":" + (isPlayerTarget ? "1" : "0")
                + ",\"target\":\"" + snbtEscape(targetName)
                + "\",\"cause\":" + snbtNum(cause)
                + "," + playerRefSnbt(self) + "}";
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerAttackHook2,
            ll::memory::HookPriority::Normal,
            Player,
            // A virtual goes through the $ alias. The static_cast stays: if both
            // overloads are virtual there are two $attack as well and the cast resolves
            // the ambiguity, and it is harmless when there is none.
            static_cast<AttackFn2>(&Player::$attack),
            ::ActorHurtResult,
            ::Actor& actor,
            ::SharedTypes::Legacy::ActorDamageCause const& cause)
        {
            auto& def = attackDef();
            if (!def.live() || tlAttackDepth > 0)
            {
                return origin(actor, cause);
            }
            AttackDepthGuard depth;
            if (dispatchHookEventCancellable(
                    def, buildAttackSnbt(*this, actor, static_cast<int>(cause))))
            {
                return refusedHurt();
            }
            return origin(actor, cause);
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerAttackHook3,
            ll::memory::HookPriority::Normal,
            Player,
            static_cast<AttackFn3>(&Player::attack),
            ::ActorHurtResult,
            ::Actor& actor,
            ::SharedTypes::Legacy::ActorDamageCause const& cause,
            ::Player::AttackParameters const& params)
        {
            auto& def = attackDef();
            if (!def.live() || tlAttackDepth > 0)
            {
                return origin(actor, cause, params);
            }
            AttackDepthGuard depth;
            if (dispatchHookEventCancellable(
                    def, buildAttackSnbt(*this, actor, static_cast<int>(cause))))
            {
                return refusedHurt();
            }
            return origin(actor, cause, params);
        }

        HookEventDef gDef{
            "PlayerAttackTargetEvent",
            []
            {
                // hook() returns the ll::memory::hookEx status code, where 0 is success.
                // Both are reported: with one failing and the other succeeding, the
                // protection covers only some attack paths, which is harder to diagnose
                // than none of them working.
                int r2 = PlayerAttackHook2::hook();
                int r3 = PlayerAttackHook3::hook();
                auto& log = hostLogger();
                log.debug(
                    "[hooks/AttackEvent] installing detours: attack/2={} (code={}), attack/3={} (code={})",
                    r2 == 0 ? "ok" : "failed", r2,
                    r3 == 0 ? "ok" : "failed", r3
                );
                if (r2 != 0 && r3 != 0)
                {
                    log.error(
                        "[hooks/AttackEvent] neither detour installed, so attacking a player "
                        "and attacking a mob cannot be told apart on a plot and the pvp flag "
                        "blocks nobody. The usual cause is a mismatch between the BDS or "
                        "LeviLamina version this host was linked against and the one the "
                        "server runs.");
                }
                return r2 == 0 || r3 == 0;
            }
        };
        HookEventDef& attackDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
