/** world/ChestPairEvent.cpp —— 两个箱子要合成一个大箱子。
 *
 * 越界配对是一条真实的绕过：把箱子贴着地皮边界放下，与邻居地里的箱子配成大箱
 * 子，打开自己这半边就能看见对面的全部物品。容器保护判的是「你点的那一格」，
 * 而那一格确实是你自己的。
 *
 * 事件带两个箱子的坐标，取消即不配对，两个箱子各自独立（原版行为）。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/actor/ChestBlockActor.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& chestPairDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            ChestTryPairHook,
            ll::memory::HookPriority::Normal,
            ChestBlockActor,
            &ChestBlockActor::_tryToPairWith,
            void,
            ::BlockSource& region,
            ::BlockPos const& position)
        {
            auto& def = chestPairDef();
            if (!def.live()) return origin(region, position);

            int dim = -1;
            int sx = 0, sy = 0, sz = 0;
            try
            {
                dim = static_cast<int>(region.getDimensionId());
                auto const& self = this->getPosition();
                sx = self.x;
                sy = self.y;
                sz = self.z;
            }
            catch (...)
            {
                // 坐标读不出来时不拦：这个事件是加固，不是最后一道安全闸。
            }

            std::string snbt = "{\"eventId\":\"ChestPairEvent\""
                ",\"x\":" + snbtNum(sx)
                + ",\"y\":" + snbtNum(sy)
                + ",\"z\":" + snbtNum(sz)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"otherX\":" + snbtNum(position.x)
                + ",\"otherY\":" + snbtNum(position.y)
                + ",\"otherZ\":" + snbtNum(position.z) + "}";

            if (dispatchHookEventCancellable(def, snbt)) return;
            origin(region, position);
        }

        HookEventDef gDef{
            "ChestPairEvent",
            []
            {
                int const r = ChestTryPairHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[ChestPairEvent] ChestBlockActor::_tryToPairWith 的 detour 安装失败（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& chestPairDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
