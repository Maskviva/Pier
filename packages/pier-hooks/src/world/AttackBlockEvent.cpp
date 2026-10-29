/** hooks/world/AttackBlockEvent.cpp: the synthetic, cancellable "PlayerAttackBlockEvent".
 * PlayerStartDestroyBlockEvent reports the start of mining from inside GameMode and can only
 * observe. This one hooks the handler of the start-destroy request, which runs earlier, so
 * refusing it stops the mining before GameMode hears of it. Returning without calling origin
 * is what the handler does for a request it ignores.
 * Payload {eventId, x, y, z, dim, face, _player}: the block the player started hitting, and
 * the face of it as the engine numbers it. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/network/ServerPlayerBlockUseHandler.h"
#include "mc/server/ServerPlayer.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& attackBlockDef(); // Forward declaration

        // The handler is a function in a namespace and not a member, so it is a static hook.
        LL_STATIC_HOOK(
            PlayerAttackBlockHook,
            ll::memory::HookPriority::Normal,
            &::ServerPlayerBlockUseHandler::onStartDestroyBlock,
            void,
            ::ServerPlayer& player,
            ::BlockPos const& pos,
            int face)
        {
            auto& def = attackBlockDef();
            if (!def.live()) return origin(player, pos, face);

            std::string const snbt = "{\"eventId\":\"PlayerAttackBlockEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(player.getDimensionId()))
                + ",\"face\":" + snbtNum(face)
                + "," + playerRefSnbt(player) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(player, pos, face);
        }

        HookEventDef gDef{"PlayerAttackBlockEvent", [] { return PlayerAttackBlockHook::hook() == 0; }};
        HookEventDef& attackBlockDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
