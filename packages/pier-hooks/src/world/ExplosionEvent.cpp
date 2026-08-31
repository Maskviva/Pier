/** world/ExplosionEvent.cpp —— 爆炸事件，可取消。
 *
 * pier-dimensions 也钩 Level::$explode，但那一层回答的是按维度的粗粒度问题（这
 * 个维度允不允许爆炸破坏方块）。地皮级判断需要坐标和半径，只能由模组做，所以这
 * 里再挂一个更外层（HookPriority::High）的钩子：模组先看到每一次爆炸，不取消的
 * 话维度规则照常生效。
 *
 * 取消语义是这次爆炸不发生，连伤害带方块。只想保住方块、留下伤害的，用维度规则
 * PIER_DIMRULE_EXPLODE_BLOCKS，或在模组侧用 explode(..., breaks_blocks=false)
 * 重放一次。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& explosionDef(); // 前向

        /** Level::$explode 有两个重载（八参版和 Explosion& 版）。宏内部按目标类型
         *  解析标识符本可消歧，但显式转型无论如何都成立，且把钩的是哪一个写在明
         *  处。同 AttackEvent。 */
        using ExplodeFn = bool (Level::*)(
            ::BlockSource&, ::Actor*, ::Vec3 const&, float, bool, bool, float, bool);

        LL_TYPE_INSTANCE_HOOK(
            LevelExplodeHook,
            ll::memory::HookPriority::High, // 比维度规则更外层：模组先判，规则后判
            Level,
            static_cast<ExplodeFn>(&Level::$explode),
            bool,
            ::BlockSource& region,
            ::Actor* source,
            ::Vec3 const& pos,
            float explosionRadius,
            bool fire,
            bool breaksBlocks,
            float maxResistance,
            bool allowUnderwater)
        {
            auto& def = explosionDef();
            if (!def.live())
            {
                return origin(region, source, pos, explosionRadius, fire, breaksBlocks, maxResistance,
                              allowUnderwater);
            }

            int dim = -1;
            std::string sourceType;
            bool sourceIsPlayer = false;
            int64_t sourceId = 0;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                if (source)
                {
                    sourceType = source->getTypeName();
                    sourceIsPlayer = source->isPlayer();
                    sourceId = source->getOrCreateUniqueID().rawID;
                }
            }
            catch (...)
            {
                sourceType.clear();
            }

            std::string snbt = "{\"eventId\":\"ExplosionEvent\""
                ",\"x\":" + snbtDouble(pos.x)
                + ",\"y\":" + snbtDouble(pos.y)
                + ",\"z\":" + snbtDouble(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"radius\":" + snbtDouble(explosionRadius)
                + ",\"maxResistance\":" + snbtDouble(maxResistance)
                + ",\"fire\":" + (fire ? "1" : "0")
                + ",\"breaksBlocks\":" + (breaksBlocks ? "1" : "0")
                + ",\"underwater\":" + (allowUnderwater ? "1" : "0")
                + ",\"sourceIsPlayer\":" + (sourceIsPlayer ? "1" : "0")
                + ",\"sourceId\":" + snbtNum(sourceId) + "L"
                + ",\"source\":\"" + snbtEscape(sourceType) + "\"}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return origin(region, source, pos, explosionRadius, fire, breaksBlocks, maxResistance,
                          allowUnderwater);
        }

        HookEventDef gDef{
            "ExplosionEvent",
            []
            {
                int const r = LevelExplodeHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[ExplosionEvent] Level::$explode 的 detour 安装失败（code={}）—— "
                        "爆炸保护不生效。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& explosionDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
