/** hooks/world/RespawnAnchorEvents.cpp: "RespawnAnchorExplodeEvent" and
 * "PlayerUseRespawnAnchorEvent", both cancellable.
 * Right-clicking a charged respawn anchor does one of two things the engine decides inside
 * RespawnAnchorBlock::use: outside the nether it explodes, and in the nether it becomes the
 * player's spawn point. The first is a bomb on someone else's ground and the second a spawn
 * point on it, and neither is an event before it happens. One hook covers both: it reads the
 * charge and the held item the way the engine does and raises the event for what is about to
 * happen. Charging with glowstone raises nothing.
 * Returning without calling origin leaves the anchor and the player as they were.
 * Payload {eventId, x, y, z, dim, charge, _player}. When the charge cannot be read nothing is
 * raised and the engine proceeds. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/RespawnAnchorBlock.h"
#include "mc/world/level/block/VanillaStates.h"
#include "mc/world/level/block/block_events/BlockPlayerInteractEvent.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& explodeDef(); // Forward declarations
        HookEventDef& useDef();

        constexpr int kNetherDimension = 1;

        LL_TYPE_INSTANCE_HOOK(
            RespawnAnchorUseHook,
            ll::memory::HookPriority::Normal,
            RespawnAnchorBlock,
            &RespawnAnchorBlock::use,
            void,
            ::BlockEvents::BlockPlayerInteractEvent& eventData)
        {
            if (!explodeDef().live() && !useDef().live())
            {
                return origin(eventData);
            }
            ::Player& player = eventData.mPlayer;
            auto const& pos = eventData.mPos.get();
            int charge = 0;
            bool charging = false;
            bool readFailed = false;
            try
            {
                ::Block const& block = player.getDimensionBlockSource().getBlock(pos);
                charge = block.getState<int>(::VanillaStates::RespawnAnchorCharge().mID).value_or(0);
                int const full = static_cast<int>(::VanillaStates::RespawnAnchorCharge().mVariationCount) - 1;
                charging = player.getSelectedItem().getTypeName() == "minecraft:glowstone" && charge < full;
            }
            catch (...)
            {
                readFailed = true;
            }
            if (readFailed || charge <= 0 || charging)
            {
                return origin(eventData);
            }
            int const dim = static_cast<int>(player.getDimensionId());
            bool const explodes = dim != kNetherDimension;
            auto& def = explodes ? explodeDef() : useDef();
            if (!def.live())
            {
                return origin(eventData);
            }
            std::string const snbt = std::string{"{\"eventId\":\""}
                + (explodes ? "RespawnAnchorExplodeEvent" : "PlayerUseRespawnAnchorEvent") + "\""
                + ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"charge\":" + snbtNum(charge)
                + "," + playerRefSnbt(player) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return;
            }
            origin(eventData);
        }

        /**
         * Installs the one detour both events share, once. A second `hook()` call is a second
         * registration, so both events call this and the later subscription reuses the result.
         */
        bool installRespawnAnchorHook()
        {
            static bool const ok = []
            {
                int const r = RespawnAnchorUseHook::hook();
                if (r != 0)
                {
                    hostLogger().error("{}", pier::trf("hooks.respawn_anchor_events.1", r));
                }
                return r == 0;
            }();
            return ok;
        }

        HookEventDef gExplodeDef{"RespawnAnchorExplodeEvent", [] { return installRespawnAnchorHook(); }};
        HookEventDef& explodeDef() { return gExplodeDef; }

        HookEventDef gUseDef{"PlayerUseRespawnAnchorEvent", [] { return installRespawnAnchorHook(); }};
        HookEventDef& useDef() { return gUseDef; }

        HookEventRegistrar gExplodeReg{gExplodeDef};
        HookEventRegistrar gUseReg{gUseDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
