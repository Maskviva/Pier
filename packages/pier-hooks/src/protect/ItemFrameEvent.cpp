/** protect/ItemFrameEvent.cpp —— 玩家打了一下物品展示框。
 *
 * 展示框和盔甲架一样是保护的盲区：左键打一下就能把里面的物品打出来，而这条路既
 * 不是破坏方块（框还在）也不是打实体（框是方块），症状是展示品被偷而日志里什么
 * 都没有。
 *
 * 钩 ItemFrameBlock::$attack，也就是左键取物。放物品进去走方块交互，已被 LL 的
 * PlayerInteractBlockEvent 与本包的 PlayerUseItemOnEvent 覆盖。取消即这一下不生
 * 效，返回 false 是引擎自己的「打了但没反应」路径。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/ItemFrameBlock.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& itemFrameDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            ItemFrameAttackHook,
            ll::memory::HookPriority::Normal,
            ItemFrameBlock,
            &ItemFrameBlock::$attack,
            bool,
            ::Player* player,
            ::BlockPos const& pos)
        {
            auto& def = itemFrameDef();
            // 没有玩家就没有归属可判（掉落的方块、活塞），照原样放行。
            if (!def.live() || player == nullptr) return origin(player, pos);

            std::string snbt = "{\"eventId\":\"PlayerAttackItemFrameEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(player->getDimensionId()))
                + "," + playerRefSnbt(*player) + "}";

            if (dispatchHookEventCancellable(def, snbt)) return false;
            return origin(player, pos);
        }

        HookEventDef gDef{
            "PlayerAttackItemFrameEvent",
            []
            {
                int const r = ItemFrameAttackHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[PlayerAttackItemFrameEvent] ItemFrameBlock::$attack 的 detour 安装失败"
                        "（code={}）—— 展示框里的物品不受保护。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& itemFrameDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
