/** world/PistonPushEvent.cpp —— 活塞要推/拉一组方块了。
 *
 * 活塞是跨地皮破坏的经典手段：机器建在自己地里，臂伸进邻居家把方块推走。
 * pier-dimensions 的 PISTON_CROSS_PLOT 规则用**网格**判同区，这个事件把
 * 决定权交给模组 —— 地皮的实际归属（谁是主人、谁被授权）只有模组知道。
 *
 * 载荷带活塞自身坐标、朝向和它这次挂到的方块列表（最多 12 个，原版上限）。
 * 取消 = 这次推拉不发生（`_checkAttachedBlocks` 返回 false 是引擎自己的
 * 「推不动」路径，安全）。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/actor/PistonBlockActor.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& pistonDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PistonCheckAttachedHook,
            ll::memory::HookPriority::High, // 比维度规则更外层
            PistonBlockActor,
            &PistonBlockActor::_checkAttachedBlocks,
            bool,
            ::BlockSource& region)
        {
            auto& def = pistonDef();
            if (!def.live()) return origin(region);

            // 先让引擎算出这次到底挂了哪些方块 —— 不调 origin 的话
            // mAttachedBlocks 还是上一次的。引擎算完再问模组要不要放行。
            if (!origin(region)) return false;

            int dim = -1;
            std::string blocks = "[";
            int px = 0, py = 0, pz = 0;
            int fx = 0, fy = 0, fz = 0;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                auto const& self = this->mPosition.get();
                px = self.x;
                py = self.y;
                pz = self.z;
                auto const& facing = this->getFacingDir(region);
                fx = facing.x;
                fy = facing.y;
                fz = facing.z;
                bool first = true;
                for (auto const& b : this->mAttachedBlocks.get())
                {
                    if (!first) blocks += ",";
                    first = false;
                    blocks += "[" + snbtNum(b.x) + "," + snbtNum(b.y) + "," + snbtNum(b.z) + "]";
                }
            }
            catch (...)
            {
                // 读不出附着表就把这次当成「不知道推了什么」——安全判定应当拒绝，
                // 但这里只是**上报**：拒绝与否由模组按坐标决定，它至少拿到了活塞位置。
            }
            blocks += "]";

            std::string snbt = "{\"eventId\":\"PistonPushEvent\""
                ",\"x\":" + snbtNum(px)
                + ",\"y\":" + snbtNum(py)
                + ",\"z\":" + snbtNum(pz)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"facing\":[" + snbtNum(fx) + "," + snbtNum(fy) + "," + snbtNum(fz) + "]"
                + ",\"attached\":" + blocks + "}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return true;
        }

        HookEventDef gDef{
            "PistonPushEvent",
            []
            {
                int const r = PistonCheckAttachedHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[PistonPushEvent] PistonBlockActor::_checkAttachedBlocks 的 detour 安装失败"
                        "（code={}）—— 跨地皮活塞推拉不受保护。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& pistonDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
