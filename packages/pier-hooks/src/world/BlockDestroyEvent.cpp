/** world/BlockDestroyEvent.cpp —— 「有东西把这一格挖掉了」，不问是谁。
 *
 * # 为什么需要它
 *
 * 玩家挖方块有 LL 的 PlayerDestroyBlockEvent 和本包的
 * PlayerStartDestroyBlockEvent。**其余一切**都没有事件：末影人搬走草方块、
 * 凋灵撞碎墙、爬行者炸出的坑、蠹虫钻进石头、村民踩坏耕地、命令方块
 * `/setblock air destroy`、其它插件调 destroyBlock —— 在地皮保护看来它们
 * 全是「方块凭空消失」，而且事后无从追查。
 *
 * `Level::destroyBlock` 是这些路径的公共汇合点（`/setblock ... destroy`、
 * `Mob::_destroyBlock`、爆炸的方块清除都终结在这里），所以钩它一个就够。
 * 反过来，这里**看不见**方块被替换（`BlockSource::setBlock`）—— 那条路要靠
 * LL 的 BlockChangedEvent。两者互补，都不是对方的超集。
 *
 * # 载荷里为什么没有「谁干的」
 *
 * 这个签名不带 Actor：引擎在这一层已经把来源丢掉了。硬编一个 `_player`
 * 字段只会让消费方以为自己知道来源。要区分来源就订阅更上游的那几个
 * （PlayerDestroyBlockEvent / ActorHurtEvent / 本包的投射物事件），
 * 这里只回答「哪一格、哪个维度、掉不掉东西」。
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
                // 读不出来不是拒绝的理由：位置和维度才是判定用的，方块名只是
                // 给日志看的。
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
                // 返回 false = 「没破坏成功」。调用方（含原版路径）本来就要处理
                // 这个返回值，所以取消是安全的：引擎不会停在半更新状态。
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
