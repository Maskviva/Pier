/** world/SpawnItemActorEvent.cpp —— 一个掉落物实体要被生成。
 *
 * 两个用途：
 *   - 反刷物 / 反卡服：掉落物是最容易堆出几千个实体的东西，取消掉一部分
 *     （比如某个区域里的、或者某种物品）比事后清理便宜得多；
 *   - 地皮内的掉落归属：知道「哪一格掉了什么」才能做「只有主人能捡」。
 *
 * 取消 = 不生成这个掉落物（返回 nullptr 是引擎自己的失败路径）。**注意物品
 * 会因此消失**，不是掉在地上不动 —— 想保留就别取消，改在拾取事件上判。
 */
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
        HookEventDef& spawnItemDef(); // 前向

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
                        "[SpawnItemActorEvent] Spawner::$spawnItem 的 detour 安装失败（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& spawnItemDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
