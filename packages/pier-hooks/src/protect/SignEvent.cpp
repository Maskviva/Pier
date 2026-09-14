/** protect/SignEvent.cpp: a player changed the text on a sign.
 * Placing a sign goes through block placement and editing it goes through nothing:
 * a visitor who may not place a block can still rewrite every sign in a claim, and the
 * only trace is the new text.
 * `SignBlockActor::$_playerCanUpdate` is hooked. It is the engine's own gate, asked
 * before the text is taken from the packet, so returning false is a refusal the engine
 * already knows how to handle rather than a half-applied edit. The side being written is
 * not in the payload: it is not known at this point, and a permission that depends on
 * which face of a sign somebody is standing at would be unusable.
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/SignBlockActor.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& editSignDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerEditSignHook,
            ll::memory::HookPriority::Normal,
            SignBlockActor,
            &SignBlockActor::$_playerCanUpdate,
            bool,
            ::Player const& fromPlayer)
        {
            auto& def = editSignDef();
            if (!def.live()) return origin(fromPlayer);
            // The engine's own answer comes first. A subscriber may only refuse an edit
            // the engine was going to allow; letting one through that the engine refused
            // would put this hook in the business of granting, which no hook here does.
            if (!origin(fromPlayer)) return false;

            // `mPosition` and not a getter: BlockActor keeps the position as a member and
            // exposes no accessor for it.
            auto const& pos = mPosition.get();
            std::string snbt = "{\"eventId\":\"PlayerEditSignEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(fromPlayer.getDimensionId()))
                + "," + playerRefSnbt(fromPlayer) + "}";

            return !dispatchHookEventCancellable(def, snbt);
        }

        HookEventDef gDef{"PlayerEditSignEvent", [] { return PlayerEditSignHook::hook() == 0; }};
        HookEventDef& editSignDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // PIER_BUILD_CLIENT
