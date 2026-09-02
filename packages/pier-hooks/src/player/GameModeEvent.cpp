/** hooks/player/GameModeEvent.cpp: the synthetic, cancellable
 * "PlayerChangeGameModeEvent".
 * Applying a mode only on join and on a dimension change misses the /gamemode, command
 * block and scoreboard trigger paths that follow. The hook point is the virtual
 * Player::$setPlayerGameType, through which every mode change passes; the inner
 * non-virtual _setPlayerGameType is an implementation detail and is not hooked.
 * A subscriber setting the mode back from its callback does not self-trigger, since the
 * target mode is then inside the allowed set and the decision is idempotent. A re-entry
 * gate is added anyway, so a subscriber that judges the target mode disallowed cannot
 * recurse into the engine until it crashes, with a stack of identical frames and nothing
 * in the log. Re-entry passes straight through. Cancelling means not calling origin, the
 * server sends no change packet and the client realigns within a tick.
 * Payload {eventId, x, y, z, dim, from, to, _player:{...}}. from and to carry the
 * ::GameType integers verbatim, -1 Undefined, 0 Survival, 1 Creative, 2 Adventure,
 * 5 Default, 6 Spectator, and are not translated into a numbering of this layer, because
 * such a table would diverge from the engine. */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/GameType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& gameModeDef(); // Forward declaration

        /** The no-self-trigger rule from the file header. Re-entering during a dispatch
         *  always passes through. */
        bool gDispatching = false;

        std::string buildSnbt(Player& p, int from, int to)
        {
            auto const& pos = p.getPosition();
            return "{\"eventId\":\"PlayerChangeGameModeEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(to)
                + "," + playerRefSnbt(p) + "}";
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerChangeGameModeHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$setPlayerGameType,
            void,
            ::GameType gameType)
        {
            auto& def = gameModeDef();
            if (!def.live() || gDispatching)
            {
                return origin(gameType);
            }

            int const from = static_cast<int>(this->getPlayerGameType());
            int const to = static_cast<int>(gameType);

            // Nothing changed, so nothing is asked. The engine sets the current mode
            // again on every respawn and every dimension change.
            if (from == to)
            {
                return origin(gameType);
            }

            std::string snbt = buildSnbt(*this, from, to);

            bool cancelled = false;
            {
                gDispatching = true;
                struct Reset
                {
                    ~Reset() { gDispatching = false; }
                } reset;
                cancelled = dispatchHookEventCancellable(def, snbt);
            }
            if (cancelled)
            {
                // The return is void, so cancelling means not calling origin: the mode
                // does not move and the server sends no change packet.
                return;
            }
            return origin(gameType);
        }

        HookEventDef gDef{
            "PlayerChangeGameModeEvent",
            []
            {
                int const r = PlayerChangeGameModeHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[hooks/GameModeEvent] installing detour: PlayerChangeGameModeHook={} (code={})",
                    r == 0 ? "ok" : "failed", r);
                if (r != 0)
                {
                    log.error(
                        "[hooks/GameModeEvent] the native detour failed to install with a "
                        "non-zero status. The usual cause is a mismatch between the BDS or "
                        "LeviLamina version this host was linked against and the one the "
                        "server runs, so the symbol address of Player::$setPlayerGameType "
                        "resolved wrongly. Game mode enforcement now applies only at the "
                        "moment of entering the world, a player can bypass it with "
                        "/gamemode afterwards, and nothing is logged. Rebuild this host "
                        "against the version the server actually runs.");
                }
                return r == 0;
            }
        };
        HookEventDef& gameModeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
