/** world/LiquidFlowEvent.cpp —— 水或岩浆往某一格流。
 *
 * 地皮场景里这是「邻居把水放到我地里」的唯一拦截点：放水本身发生在他自己的地皮
 * 上，PlayerPlaceBlock 拦不住，流过来的那一步才越界。事件带目标格和来源格，消费
 * 方按目标格判权限。
 *
 * 热路径：液体每 tick 都在流，判据要尽量便宜，必要时自己节流。这里不像压力板那
 * 样内置节流缓存，因为「哪一格」比「哪个玩家」变化快得多，命中率低到不值得。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/LiquidBlock.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& liquidFlowDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            LiquidTrySpreadToHook,
            ll::memory::HookPriority::High, // 比维度规则更外层
            LiquidBlock,
            &LiquidBlock::_trySpreadTo,
            void,
            ::BlockSource& region,
            ::BlockPos const& pos,
            int neighbor,
            ::BlockPos const& flowFromPos,
            uchar flowFromDirection)
        {
            auto& def = liquidFlowDef();
            if (!def.live())
            {
                return origin(region, pos, neighbor, flowFromPos, flowFromDirection);
            }

            int dim = -1;
            std::string liquid;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                liquid = this->getTypeName();
            }
            catch (...)
            {
                liquid.clear();
            }

            std::string snbt = "{\"eventId\":\"LiquidFlowEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"fromX\":" + snbtNum(flowFromPos.x)
                + ",\"fromY\":" + snbtNum(flowFromPos.y)
                + ",\"fromZ\":" + snbtNum(flowFromPos.z)
                + ",\"direction\":" + snbtNum(static_cast<int>(flowFromDirection))
                + ",\"liquid\":\"" + snbtEscape(liquid) + "\"}";

            if (dispatchHookEventCancellable(def, snbt)) return;
            origin(region, pos, neighbor, flowFromPos, flowFromDirection);
        }

        HookEventDef gDef{
            "LiquidFlowEvent",
            []
            {
                int const r = LiquidTrySpreadToHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[LiquidFlowEvent] LiquidBlock::_trySpreadTo 的 detour 安装失败（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& liquidFlowDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
