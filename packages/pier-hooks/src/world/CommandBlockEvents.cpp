/** hooks/world/CommandBlockEvents.cpp: the synthetic, cancellable "CommandBlockExecuteEvent".
 * A command block runs its command as nobody. No player is the origin, so
 * ExecutingCommandEvent, which reports players only, never sees it, and a plot that forbids
 * a command to its residents stays open to whoever placed a command block.
 * BaseCommandBlock::_performCommand is the engine's one place where a command block, or a
 * command block minecart, runs its command, so the hook asks right before. Returning false
 * is the engine's own answer of a command that did not run.
 * Payload {eventId, command, x, y, z, dim, minecart}: the command text as stored, the block
 * coordinates of the block or of the minecart, and minecart 1 for a minecart. A field that
 * could not be read leaves the payload with `partial` 1. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <cmath>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOriginType.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/actor/BaseCommandBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& commandBlockDef(); // Forward declaration

        int cell(float v) { return static_cast<int>(std::floor(v)); }

        LL_TYPE_INSTANCE_HOOK(
            CommandBlockExecuteHook,
            ll::memory::HookPriority::Normal,
            BaseCommandBlock,
            &BaseCommandBlock::_performCommand,
            bool,
            ::BlockSource& region,
            ::CommandOrigin const& commandOrigin,
            bool& markForSaving)
        {
            auto& def = commandBlockDef();
            if (!def.live()) return origin(region, commandOrigin, markForSaving);

            int x = 0;
            int y = 0;
            int z = 0;
            bool minecart = false;
            bool readFailed = false;
            try
            {
                if (commandOrigin.getOriginType() == ::CommandOriginType::MinecartCommandBlock)
                {
                    minecart = true;
                    if (::Actor* cart = commandOrigin.getEntity())
                    {
                        auto const& at = cart->getPosition();
                        x = cell(at.x);
                        y = cell(at.y);
                        z = cell(at.z);
                    }
                    else
                    {
                        readFailed = true;
                    }
                }
                else
                {
                    ::BlockPos const at = commandOrigin.getBlockPosition();
                    x = at.x;
                    y = at.y;
                    z = at.z;
                }
            }
            catch (...)
            {
                readFailed = true;
            }
            std::string const snbt = "{\"eventId\":\"CommandBlockExecuteEvent\""
                ",\"command\":" + snbtStr(this->mCommand.get())
                + ",\"x\":" + snbtNum(x)
                + ",\"y\":" + snbtNum(y)
                + ",\"z\":" + snbtNum(z)
                + ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId()))
                + ",\"minecart\":" + (minecart ? "1" : "0")
                + (readFailed ? ",\"partial\":1" : "") + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return false;
            }
            return origin(region, commandOrigin, markForSaving);
        }

        HookEventDef gDef{
            "CommandBlockExecuteEvent", [] { return CommandBlockExecuteHook::hook() == 0; }};
        HookEventDef& commandBlockDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
