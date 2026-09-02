/** world/FarmlandDecayEvent.cpp: something trampled farmland.
 * The vanilla rule turns farmland into dirt when something falls onto it from a height.
 * On a plot server this is the most common form of my ground was ruined with nothing in
 * the log: whoever trampled it needed no permission and the field is gone.
 * The event carries the farmland coordinates and the trampler, which may be absent since
 * a falling block also triggers it, and is cancellable. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/FarmBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& farmDecayDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            FarmTransformOnFallHook,
            ll::memory::HookPriority::High, // Outside the dimension rules
            FarmBlock,
            &FarmBlock::$transformOnFall,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            ::Actor* actor,
            float fallDistance)
        {
            auto& def = farmDecayDef();
            if (!def.live()) return origin(region, pos, actor, fallDistance);

            int dim = -1;
            std::string who;
            bool byPlayer = false;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                if (actor)
                {
                    who = actor->getTypeName();
                    byPlayer = actor->isPlayer();
                }
            }
            catch (...)
            {
                who.clear();
            }

            std::string snbt = "{\"eventId\":\"FarmlandDecayEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"fallDistance\":" + snbtDouble(fallDistance)
                + ",\"byPlayer\":" + (byPlayer ? "1" : "0")
                + ",\"actor\":\"" + snbtEscape(who) + "\"";
            if (byPlayer && actor)
            {
                snbt += "," + playerRefSnbt(*static_cast<Player*>(actor));
            }
            snbt += "}";

            if (dispatchHookEventCancellable(def, snbt)) return;
            origin(region, pos, actor, fallDistance);
        }

        HookEventDef gDef{
            "FarmlandDecayEvent",
            []
            {
                int const r = FarmTransformOnFallHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/FarmlandDecayEvent] the FarmBlock::$transformOnFall detour failed to install with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& farmDecayDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
