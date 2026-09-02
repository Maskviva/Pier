/** world/BlockDestroyEvent.cpp: something removed this cell, without asking who.
 * A player mining a block has PlayerDestroyBlockEvent in LL and
 * PlayerStartDestroyBlockEvent in this package, while everything else has no event at
 * all: an enderman taking a grass block, a wither smashing a wall, a creeper blowing a
 * crater, a silverfish burrowing into stone, a villager trampling farmland,
 * /setblock air destroy, another plugin calling destroyBlock. To protection they are all
 * a block vanishing, untraceable afterwards. Level::destroyBlock is where those paths
 * meet and hooking it alone is enough. A block being replaced through
 * BlockSource::setBlock is invisible here and belongs to BlockChangedEvent in LL; the two
 * are complementary and neither is a superset of the other.
 * The payload carries no who: this signature has no Actor, the engine already dropped the
 * source at this layer, and inventing a _player would only make a consumer believe it
 * knows the source. Telling sources apart means subscribing to the events further
 * upstream. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/block/BlockChangeContext.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& blockDestroyDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            LevelDestroyBlockHook,
            ll::memory::HookPriority::Normal,
            Level,
            &Level::$destroyBlock,
            bool,
            ::BlockSource& region,
            ::BlockPos const& pos,
            bool dropResources,
            ::BlockChangeContext const& changeSourceContext)
        {
            auto& def = blockDestroyDef();
            if (!def.live()) return origin(region, pos, dropResources, changeSourceContext);

            int dim = -1;
            std::string name;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                name = region.getBlock(pos).getTypeName();
            }
            catch (...)
            {
                // Being unreadable is no reason to refuse: the decision uses the position
                // and the dimension, and the block name is only for the log.
            }

            std::string snbt = "{\"eventId\":\"BlockDestroyEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"dropResources\":" + (dropResources ? "1" : "0")
                + ",\"block\":\"" + snbtEscape(name) + "\"}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                // Returning false means the destruction did not succeed. Every caller
                // already handles that return, so cancelling is safe and the engine is not
                // left half updated.
                return false;
            }
            return origin(region, pos, dropResources, changeSourceContext);
        }

        HookEventDef gDef{
            "BlockDestroyEvent",
            []
            {
                int const r = LevelDestroyBlockHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/BlockDestroyEvent] the Level::$destroyBlock detour failed to "
                        "install with code={}, so destruction from non-player sources such as "
                        "endermen, withers, explosions and commands is entirely "
                        "unprotected", r);
                }
                return r == 0;
            }
        };
        HookEventDef& blockDestroyDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
