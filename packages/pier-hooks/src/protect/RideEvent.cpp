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
        HookEventDef& rideDef();      // 前向：玩家上骑乘物
        HookEventDef& actorRideDef(); // 前向：非玩家实体上骑乘物

        LL_TYPE_INSTANCE_HOOK(
            PlayerRideHook,
            ll::memory::HookPriority::Normal,
            Actor,
            &Actor::$canAddPassenger,
            bool,
            ::Actor& passenger)
        {
            auto& playerDef = rideDef();
            auto& actorDef = actorRideDef();

            bool const passengerIsPlayer = passenger.isPlayer();
            auto& def = passengerIsPlayer ? playerDef : actorDef;
            if (!def.live())
            {
                return origin(passenger);
            }

            std::string vehicleName;
            std::string passengerName;
            try
            {
                vehicleName = this->getTypeName();
                passengerName = passenger.getTypeName();
            }
            catch (...)
            {
                vehicleName.clear();
                passengerName.clear();
            }

            auto const& pos = this->getPosition();
            std::string snbt = passengerIsPlayer
                ? std::string{"{\"eventId\":\"PlayerRideEvent\""}
                : std::string{"{\"eventId\":\"ActorRideEvent\""};
            snbt += ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(this->getDimensionId()))
                + ",\"vehicle\":\"" + snbtEscape(vehicleName) + "\""
                + ",\"vehicleId\":" + snbtNum(static_cast<int64_t>(this->getOrCreateUniqueID().rawID)) + "L";
            if (passengerIsPlayer)
            {
                snbt += "," + playerRefSnbt(*static_cast<Player*>(&passenger));
            }
            else
            {
                // 非玩家乘客：船里的村民、矿车里的猪、被拴住的动物。载荷给类型和
                // id，而不是伪造一个 `_player` —— 消费方按「有没有 _player」就能
                // 分辨两条路径。
                snbt += ",\"passenger\":\"" + snbtEscape(passengerName) + "\""
                    + ",\"passengerId\":"
                    + snbtNum(static_cast<int64_t>(passenger.getOrCreateUniqueID().rawID)) + "L";
            }
            snbt += "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return false; // 载具谢绝这位乘客
            }
            return origin(passenger);
        }

        HookEventDef gDef{"PlayerRideEvent", [] { return PlayerRideHook::hook() == 0; }};
        HookEventDef& rideDef() { return gDef; }

        // 同一个 detour 供两个事件 id 使用：谁先被订阅谁负责装钩子，第二个订阅
        // 时 `hook()` 已经装过（LL 的 hook 是幂等的，重复调用返回 0）。
        HookEventDef gActorDef{"ActorRideEvent", [] { return PlayerRideHook::hook() == 0; }};
        HookEventDef& actorRideDef() { return gActorDef; }

        HookEventRegistrar gReg{gDef};
        HookEventRegistrar gActorReg{gActorDef};
    } // namespace
} // namespace pier::hooks
