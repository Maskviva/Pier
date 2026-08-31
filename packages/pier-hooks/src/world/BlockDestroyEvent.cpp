/** world/BlockDestroyEvent.cpp —— 「有东西把这一格挖掉了」，不问是谁。
 *
 * 玩家挖方块有 LL 的 PlayerDestroyBlockEvent 和本包的
 * PlayerStartDestroyBlockEvent，其余一切都没有事件：末影人搬走草方块、凋灵撞碎
 * 墙、爬行者炸坑、蠹虫钻石头、村民踩坏耕地、/setblock air destroy、别的插件调
 * destroyBlock。在保护看来它们全是「方块凭空消失」，事后无从追查。
 * Level::destroyBlock 是这些路径的公共汇合点，钩它一个就够；方块被替换
 * （BlockSource::setBlock）这里看不见，那条路归 LL 的 BlockChangedEvent，两者
 * 互补，都不是对方的超集。
 *
 * 载荷不带「谁干的」：这个签名没有 Actor，引擎在这一层已经把来源丢掉，硬编一个
 * _player 只会让消费方以为自己知道来源。要区分来源就订阅更上游的那几个事件。
 */
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
        HookEventDef& blockDestroyDef(); // 前向

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
                // 读不出来不是拒绝的理由：判定用位置和维度，方块名只给日志看。
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
                // 返回 false 即没破坏成功。调用方本来就要处理这个返回值，所以取
                // 消是安全的，引擎不会停在半更新状态。
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
                        "[BlockDestroyEvent] Level::$destroyBlock 的 detour 安装失败（code={}）—— "
                        "非玩家来源的破坏（末影人、凋灵、爆炸、命令）将完全不受保护。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& blockDestroyDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
