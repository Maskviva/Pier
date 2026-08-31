/**
 * hooks/protect/InteractEntityEvent.cpp —— "PlayerInteractEntityEvent"：玩家
 * 即将右键一个**实体**，**并且可以取消**。
 *
 * # 为什么需要这个钩子
 *
 * `PlayerUseItemOnEvent`（world/UseItemOnEvent.cpp）覆盖的是「把手里的物品
 * 用在**方块**上」。对实体没有对应的东西，而模组侧猜的那几个
 * （`PlayerInteractEntityEvent`、`PlayerInteractActorEvent`）在 LL 总线上都
 * 解析不到。凡是右键生物能做到的事全都没有保护：
 *
 *   剪羊毛 · 给羊染色 · 挤奶 · 拴绳 · 命名牌 · 装鞍 ·
 *   打开马/羊驼/驴的背包 · 村民交易 · 喂食与繁殖 · 给羊驼装箱子
 *
 * 「剪羊毛没被拦住」只是看得见的症状；这个洞比剪刀宽得多，而且对上面每一
 * 件事都是同一个洞。
 *
 * # 钩点
 *
 * `Player::interact(Actor&, Vec3 const&)` —— 与 LegacyScriptEngine 的
 * `onPlayerInteractEntity` 同一个点。它位于 `Actor::interactPreventDefault`
 * 之上，所以也能抓到完全在生物自己的组件代码里处理掉的交互 —— 剪羊毛正是
 * 住在那里的。
 *
 * # 取消
 *
 * `InteractionResult` 是两个位标志。这里返回 `{false, true}`：交互没有成
 * 功，但手臂仍然挥动。保留挥手是刻意的 —— 一次连动画都被吃掉的拒绝，在玩
 * 家看来就是丢包，于是他会再点、再点。让手臂动起来，让「没有权限」的提示
 * 去解释为什么什么都没发生。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, target, targetIsPlayer, item, _player:{name,xuid,uuid}}
 * ```
 *
 * `x/y/z` 是**目标实体的**位置（权限问题适用在那里 —— 同 RideEvent 的理
 * 由），`target` 是它的类型名（比如 `"minecraft:sheep"`），`item` 是手持物
 * 品的类型名，好让订阅方把剪刀 / 拴绳 / 染料 / 食物拆成不同的动作，而不是
 * 全压在一个「与实体交互」权限下面。
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

            // 这两个取名的调用会抛（实体正在被销毁时）。抛出去等于一次右键把
            // 服务器带走，所以就地吞掉。
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
                refused.mSwing = true; // 见上面「取消」
                return refused;
            }
            return origin(actor, location);
        }

        HookEventDef gDef{"PlayerInteractEntityEvent", [] { return PlayerInteractEntityHook::hook() == 0; }};
        HookEventDef& interactEntityDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
