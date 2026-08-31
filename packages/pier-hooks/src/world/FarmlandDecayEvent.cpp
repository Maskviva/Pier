/** world/FarmlandDecayEvent.cpp —— 有东西踩坏了耕地。
 *
 * 原版规则是「从高处落到耕地上把它踩成泥土」。在地皮服上这是最常见的
 * 「我的地被人毁了但没有任何日志」：踩的人不需要任何权限，而结果是农田没了。
 *
 * 事件带耕地坐标和踩踏者（可能没有 —— 掉落的方块也会触发），可取消。
 */
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
        HookEventDef& farmDecayDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            FarmTransformOnFallHook,
            ll::memory::HookPriority::High, // 比维度规则更外层
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
                        "[FarmlandDecayEvent] FarmBlock::$transformOnFall 的 detour 安装失败（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& farmDecayDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
