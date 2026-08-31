/**
 * hooks/protect/InteractEntityEvent.cpp —— 合成事件 "PlayerInteractEntityEvent"，
 * 可取消。
 *
 * PlayerUseItemOnEvent 只覆盖「物品用在方块上」，实体侧没有对应事件，于是右键
 * 生物能做的事全无保护：剪羊毛、染色、挤奶、拴绳、命名牌、装鞍、开马驼背包、
 * 村民交易、喂食繁殖。钩点是 Player::interact(Actor&, Vec3 const&)，位于 Actor::interactPreventDefault
 * 之上，所以也抓得到完全在生物组件代码里处理掉的交互（剪羊毛就住在那里）。
 * 取消返回 InteractionResult{false, true}：交互不成功但手臂仍挥动，因为连动画
 * 都被吃掉的拒绝在玩家看来是丢包，他会一直点。
 *
 * 载荷 {eventId, x, y, z, dim, target, targetIsPlayer, item, _player:{…}}。
 * x/y/z 取目标实体位置（同 RideEvent），item 是手持物品类型名，让订阅方把剪刀、
 * 拴绳、染料、食物拆成不同动作。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/ItemStack.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& interactEntityDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerInteractEntityHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::interact,
            ::InteractionResult,
            ::Actor& actor,
            ::Vec3 const& location)
        {
            auto& def = interactEntityDef();
            if (!def.live())
            {
                return origin(actor, location);
            }

            // 这两个取名调用在实体正在销毁时会抛，异常穿过 detour 等于整服崩，
            // 所以就地吞掉。
            std::string targetName;
            bool targetIsPlayer = false;
            std::string itemName;
            try
            {
                targetName = actor.getTypeName();
                targetIsPlayer = actor.isPlayer();
                ::ItemStack const& held = this->getSelectedItem();
                if (!held.isNull()) itemName = held.getTypeName();
            }
            catch (...)
            {
                // 部分失败也按「读不出来」处理：订阅方看到空串会退回粗动作。
            }

            auto const& pos = actor.getPosition();
            std::string snbt = "{\"eventId\":\"PlayerInteractEntityEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(actor.getDimensionId()))
                + ",\"targetIsPlayer\":" + (targetIsPlayer ? "1" : "0")
                + ",\"target\":\"" + snbtEscape(targetName)
                + "\",\"item\":\"" + snbtEscape(itemName)
                + "\"," + playerRefSnbt(*this) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                ::InteractionResult refused{};
                refused.mSuccess = false;
                refused.mSwing = true; // 保留挥手动画，见文件头
                return refused;
            }
            return origin(actor, location);
        }

        HookEventDef gDef{"PlayerInteractEntityEvent", [] { return PlayerInteractEntityHook::hook() == 0; }};
        HookEventDef& interactEntityDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
