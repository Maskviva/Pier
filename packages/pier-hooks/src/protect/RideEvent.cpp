/**
 * hooks/protect/RideEvent.cpp —— "PlayerRideEvent"：玩家即将骑上马 / 船 /
 * 矿车 / 猪 / 炽足兽，**并且可以取消**。
 *
 * # 为什么需要这个钩子
 *
 * 模组侧曾经猜过 `PlayerStartRidingEvent` / `PlayerRidingEvent` /
 * `PlayerMountEvent`。这三个在 LL 总线上都不存在。原版的
 * `ActorStartRidingEvent` 确实有（`mc/world/events/`），但它是一个
 * `ActorGameplayEvent` 通知：在骑上**之后**才触发，拒绝不了。
 *
 * # 钩点：canAddPassenger，不是 startRiding
 *
 * `Actor::startRiding` 看着是顺手的选择，但它是错的。LegacyScriptEngine 钩
 * 的是 `Actor::canAddPassenger`，那是更好的点，因为它是**载具的**否决权：
 * 引擎在问载具愿不愿意接收这位乘客，而回答「不」是一个正常、被每个调用方
 * 都处理好的结果。改在 `startRiding` 里拒绝，等于对一个已经决定「这次要骑
 * 上去」的函数撒谎。
 *
 * 注意随之而来的反转：`this` 是**载具**，参数是**骑乘者**。搞反了会得到一
 * 个检查船的权限而不是玩家权限的保护 —— 它在测试里「能用」（两者通常在同
 * 一块地皮上），恰恰在地皮边界处失效。
 *
 * # 位置：载具的
 *
 * 一个站在地皮边界外的玩家可以右键地皮里的船。权限问题是「这个人能不能用
 * 那个载具」，答案在载具所在的位置上给 —— 所以 `x/y/z` 是载具的位置。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, vehicle, _player:{name,xuid,uuid}}
 * ```
 *
 * `vehicle` 是实体类型名，比如 `"minecraft:boat"`。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& rideDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerRideHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::$canAddPassenger,
            bool,
            ::Actor& passenger)
        {
            auto& def = rideDef();
            // 两条快路径，便宜的在前：没人订阅，或者骑乘者是生物。生物骑乘
            //（僵尸骑鸡、村民上船）不是权限问题，而某些农场每 tick 都在产生
            // 它们。
            if (!def.live() || !passenger.isPlayer())
            {
                return origin(passenger);
            }

            auto& p = *static_cast<Player*>(&passenger);

            // getTypeName 会抛（实体正在被销毁时）。抛出去等于一次骑乘把服务
            // 器带走，所以就地吞掉 —— 订阅方读到空名字会退回粗动作，不会更
            // 松。
            std::string vehicleName;
            try
            {
                vehicleName = this->getTypeName();
            }
            catch (...)
            {
                vehicleName.clear();
            }

            auto const& pos = this->getPosition();
            std::string snbt = "{\"eventId\":\"PlayerRideEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + ",\"vehicle\":\"" + snbtEscape(vehicleName)
                + "\"," + playerRefSnbt(p) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return false; // 载具谢绝这位乘客
            }
            return origin(passenger);
        }

        HookEventDef gDef{"PlayerRideEvent", [] { PlayerRideHook::hook(); }};
        HookEventDef& rideDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
