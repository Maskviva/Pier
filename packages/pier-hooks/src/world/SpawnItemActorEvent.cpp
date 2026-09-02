/** world/SpawnItemActorEvent.cpp: a dropped item actor is about to spawn.
 * Two uses:
 *   - Anti-duplication and anti-lag: dropped items are the easiest way to pile up
 *     thousands of actors, and cancelling some of them, in one area or of one item type,
 *     is far cheaper than cleaning up afterwards.
 *   - Drop ownership inside a plot: knowing what dropped in which cell is what makes
 *     only the owner may pick it up possible.
 * Cancelling means the drop is not spawned, and returning nullptr is the engine's own
 * failure path. The item disappears as a result and does not lie on the ground, so
 * keeping it means not cancelling here and deciding in the pickup event instead. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackBase.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Spawner.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& spawnItemDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            SpawnerSpawnItemHook,
            ll::memory::HookPriority::Normal,
            Spawner,
            &Spawner::$spawnItem,
            ::ItemActor*,
            ::BlockSource& region,
            ::ItemStack const& inst,
            ::Actor* spawner,
            ::Vec3 const& pos,
            int throwTime)
        {
            auto& def = spawnItemDef();
            if (!def.live()) return origin(region, inst, spawner, pos, throwTime);

            int dim = -1;
            std::string item;
            int count = 0;
            std::string src;
            bool srcIsPlayer = false;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                if (!inst.isNull())
                {
                    item = inst.getTypeName();
                    count = inst.mCount;
                }
                if (spawner)
                {
                    src = spawner->getTypeName();
                    srcIsPlayer = spawner->isPlayer();
                }
            }
            catch (...)
            {
                item.clear();
            }

            std::string snbt = "{\"eventId\":\"SpawnItemActorEvent\""
                ",\"x\":" + snbtDouble(pos.x)
                + ",\"y\":" + snbtDouble(pos.y)
                + ",\"z\":" + snbtDouble(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"item\":\"" + snbtEscape(item) + "\""
                + ",\"count\":" + snbtNum(count)
                + ",\"throwTime\":" + snbtNum(throwTime)
                + ",\"sourceIsPlayer\":" + (srcIsPlayer ? "1" : "0")
                + ",\"source\":\"" + snbtEscape(src) + "\"}";

            if (dispatchHookEventCancellable(def, snbt)) return nullptr;
            return origin(region, inst, spawner, pos, throwTime);
        }

        HookEventDef gDef{
            "SpawnItemActorEvent",
            []
            {
                int const r = SpawnerSpawnItemHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[hooks/SpawnItemActorEvent] the Spawner::$spawnItem detour failed to install with code={}", r);
                }
                return r == 0;
            }
        };
        HookEventDef& spawnItemDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
