/** player/EatEvent.cpp —— 玩家吃/喝完了手上那件东西。
 *
 * `completeUsingItem` 是「使用计时走完」的终点：食物、药水、牛奶、望远镜、
 * 盾牌举完都从这里出去。事件在 origin **之前**发出，所以载荷里的 `item`
 * 还是那件即将被消耗的物品 —— 之后再问就已经变成空气或者空瓶了。
 *
 * 只观察：这条路上取消会把玩家卡在「一直在举着」的状态（客户端已经播完
 * 动画、服务端却没结算），比让它吃下去糟得多。要禁食物就在
 * PlayerUseItemEvent（LL 提供）那一层拦住开始使用。
 */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/ItemStackBase.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& eatDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerCompleteUsingItemHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::completeUsingItem,
            void)
        {
            auto& def = eatDef();
            if (!def.live()) return origin();

            std::string item;
            int count = 0;
            try
            {
                ::ItemStack const& held = this->getSelectedItem();
                if (!held.isNull())
                {
                    item = held.getTypeName();
                    count = held.mCount;
                }
            }
            catch (...)
            {
                item.clear();
            }

            std::string snbt = "{\"eventId\":\"PlayerUseItemCompleteEvent\""
                ",\"item\":\"" + snbtEscape(item) + "\""
                + ",\"count\":" + snbtNum(count)
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + "," + playerRefSnbt(*this) + "}";
            dispatchHookEvent(def, snbt);

            origin();
        }

        HookEventDef gDef{
            "PlayerUseItemCompleteEvent",
            []
            {
                int const r = PlayerCompleteUsingItemHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[PlayerUseItemCompleteEvent] Player::completeUsingItem 的 detour 安装失败"
                        "（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& eatDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
