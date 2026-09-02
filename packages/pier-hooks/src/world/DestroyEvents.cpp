/** hooks/world/DestroyEvents.cpp: the synthetic "PlayerStartDestroyBlockEvent".
 * GameMode::startDestroyBlock is hooked, which is earlier than the built-in
 * PlayerDestroyBlockEvent of LeviLamina, since that one fires when the destruction
 * completes. The event dispatches before origin and callbacks run synchronously, so a
 * subscriber that changes the hotbar slot finishes before the destruction logic reads the
 * held tool, which is exactly the moment automatic tool switching needs. The lifetime
 * rules are in hook_events.h. */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/level/BlockPos.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& destroyDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            StartDestroyBlockHook,
            ll::memory::HookPriority::Normal,
            GameMode,
            &GameMode::$startDestroyBlock,
            bool,
            ::BlockPos const& pos,
            uchar face,
            bool& hasDestroyedBlock)
        {
            auto& def = destroyDef();
            if (!def.live())
            {
                return origin(pos, face, hasDestroyedBlock); // Installed but idle
            }

            // GameMode::mPlayer is a TypedStorage wrapping a Player&, and a reference
            // takes the collapse specialization, so the member is that reference itself
            // and needs no .get().
            Player& p = this->mPlayer;

            std::string snbt = "{\"eventId\":\"PlayerStartDestroyBlockEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"face\":" + snbtNum(static_cast<int>(face))
                + "," + playerRefSnbt(p) + "}";
            dispatchHookEvent(def, snbt); // Before origin, see the file header

            return origin(pos, face, hasDestroyedBlock);
        }

        HookEventDef gDef{"PlayerStartDestroyBlockEvent", [] { return StartDestroyBlockHook::hook() == 0; }};
        HookEventDef& destroyDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
