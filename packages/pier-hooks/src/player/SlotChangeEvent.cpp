/** player/SlotChangeEvent.cpp —— 玩家换了手上拿的格子。
 *
 * 只观察，不可取消：`setSelectedSlot` 返回的是**新槽位里那件物品的引用**，
 * 取消就得凭空造一个 ItemStack 引用出来 —— 没有正确答案。要「锁定手持」
 * 的话在换完之后把槽位设回去（`PIER_PACT_SET_SELECTED_SLOT`）。
 *
 * 用途：手持物触发的技能/菜单（拿着某个物品才显示 HUD）、反作弊里的
 * 「一 tick 内连续切槽」检测、以及记录玩家实际用哪件工具挖了什么。
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
        HookEventDef& slotDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerSetSelectedSlotHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::setSelectedSlot,
            ::ItemStack const&,
            int slot)
        {
            auto& def = slotDef();
            if (!def.live()) return origin(slot);

            int from = -1;
            try
            {
                from = this->getSelectedItemSlot();
            }
            catch (...)
            {
                from = -1;
            }

            // 先让引擎换完，再上报 —— 这样载荷里的 `item` 就是玩家**现在**手上
            // 的东西，消费方不用自己再查一次。
            ::ItemStack const& held = origin(slot);

            if (from == slot) return held; // 没真的换（客户端会重发同一个槽位）

            std::string item;
            try
            {
                if (!held.isNull()) item = held.getTypeName();
            }
            catch (...)
            {
                item.clear();
            }

            std::string snbt = "{\"eventId\":\"PlayerChangeSlotEvent\""
                ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(slot)
                + ",\"item\":\"" + snbtEscape(item) + "\""
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + "," + playerRefSnbt(*this) + "}";
            dispatchHookEvent(def, snbt);

            return held;
        }

        HookEventDef gDef{
            "PlayerChangeSlotEvent",
            []
            {
                int const r = PlayerSetSelectedSlotHook::hook();
                if (r != 0)
                {
                    hostLogger().error(
                        "[PlayerChangeSlotEvent] Player::setSelectedSlot 的 detour 安装失败（code={}）。", r);
                }
                return r == 0;
            }
        };
        HookEventDef& slotDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
