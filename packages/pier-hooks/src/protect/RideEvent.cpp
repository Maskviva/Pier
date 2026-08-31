/**
 * hooks/protect/RideEvent.cpp —— 合成事件 "PlayerRideEvent"，可取消。
 *
 * 原版 ActorStartRidingEvent 是骑上之后才触发的通知，拒绝不了。钩点取
 * Actor::canAddPassenger 而非 Actor::startRiding：前者是载具的否决权，回答
 * 「不」是每个调用方本来就处理好的结果；在 startRiding 里拒绝等于对一个已经决
 * 定要骑上去的函数撒谎。
 *
 * 随之而来的反转：this 是载具，参数是骑乘者。搞反会检查船的权限而不是玩家的，
 * 且只在地皮边界处失效。x/y/z 取载具位置，因为问题是这个人能不能用那个载具。
 *
 * 载荷 {eventId, x, y, z, dim, vehicle, _player:{…}}，vehicle 为实体类型名。
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
                // 非玩家乘客（船里的村民、矿车里的猪）：载荷给类型和 id，不伪造
                // _player，消费方按有没有 _player 分辨两条路径。
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

        // 同一个 detour 供两个事件 id 使用：谁先被订阅谁装钩子，LL 的 hook 幂
        // 等，第二次调用返回 0。
        HookEventDef gActorDef{"ActorRideEvent", [] { return PlayerRideHook::hook() == 0; }};
        HookEventDef& actorRideDef() { return gActorDef; }

        HookEventRegistrar gReg{gDef};
        HookEventRegistrar gActorReg{gActorDef};
    } // namespace
} // namespace pier::hooks
