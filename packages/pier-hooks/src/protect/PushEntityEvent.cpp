/**
 * hooks/protect/PushEntityEvent.cpp —— 合成事件 "PlayerPushEntityEvent"，可取消。
 *
 * 推挤不需要点击，是唯一一种能在完全锁死的领地上存活、且不留日志的破坏手段：
 * 访客可以把牲畜赶出围栏、把船顶进虚空、把展示框挪走。
 *
 * 钩点是自由函数 PushableByEntityUtility::skipPush（故用 LL_STATIC_HOOK）。它是
 * 引擎自己的「这次推挤该不该跳过」之问，返回 true 是每个调用方本来就处理好的结
 * 果；在推挤内部拒绝会留下两个实体重叠、碰撞悬而未决。碰撞解算从两边都会跑，玩家可能作为 owner 也可能作为 other 到达，两边都要认。
 * 权限在被推实体的位置上判定，因为站在边界外的玩家可以推里面的动物。两边都是
 * 玩家时不管，那是正常移动。节流见 decision_throttle.h。
 *
 * 载荷 {eventId, x, y, z, dim, target, _player:{name,xuid,uuid}}。
 */
#include "pier/hooks/decision_throttle.h"
#include "pier/hooks/hook_events.h"

#include <string>
#include <unordered_map>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/util/PushableByEntityUtility.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& pushDef(); // 前向

        std::unordered_map<std::string, ThrottledDecision>& pushCache()
        {
            static std::unordered_map<std::string, ThrottledDecision> c;
            return c;
        }

        LL_STATIC_HOOK(
            PlayerPushEntityHook,
            ll::memory::HookPriority::Normal,
            &::PushableByEntityUtility::skipPush,
            bool,
            ::Actor& owner,
            ::Actor& other)
        {
            auto& def = pushDef();
            if (!def.live()) return origin(owner, other);

            // 两边都可能是玩家，见文件头。
            ::Actor* pusher = nullptr;
            ::Actor* target = nullptr;
            if (owner.isPlayer() && !other.isPlayer())
            {
                pusher = &owner;
                target = &other;
            }
            else if (other.isPlayer() && !owner.isPlayer())
            {
                pusher = &other;
                target = &owner;
            }
            else
            {
                // 两边都不是玩家，或者两边都是。生物推生物是世界行为；玩家推
                // 玩家是正常移动。
                return origin(owner, other);
            }

            auto& p = *static_cast<Player*>(pusher);

            std::string key = p.getXuid();
            if (key.empty()) key = p.getRealName(); // 离线模式服务器

            auto const& tpos = target->getPosition();
            int const x = static_cast<int>(tpos.x);
            int const y = static_cast<int>(tpos.y);
            int const z = static_cast<int>(tpos.z);
            int const dim = static_cast<int>(target->getDimensionId());

            long long const now = throttleNowMs();
            bool cached = false;
            if (throttleLookup(pushCache(), key, x, y, z, dim, now, cached))
            {
                return cached ? true : origin(owner, other);
            }

            // getTypeName 在实体正在销毁时会抛。这条路每 tick 都跑，异常穿过
            // detour 等于整服崩，所以就地吞掉。
            std::string targetName;
            try
            {
                targetName = target->getTypeName();
            }
            catch (...)
            {
                targetName.clear();
            }

            std::string snbt = "{\"eventId\":\"PlayerPushEntityEvent\""
                ",\"x\":" + snbtNum(x)
                + ",\"y\":" + snbtNum(y)
                + ",\"z\":" + snbtNum(z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"target\":\"" + snbtEscape(targetName)
                + "\"," + playerRefSnbt(p) + "}";

            bool const cancelled = dispatchHookEventCancellable(def, snbt);
            throttleStore(pushCache(), key, x, y, z, dim, now, cancelled);

            // true == 跳过这次推挤，正是取消的意思。
            return cancelled ? true : origin(owner, other);
        }

        HookEventDef gDef{"PlayerPushEntityEvent", [] { return PlayerPushEntityHook::hook() == 0; }};
        HookEventDef& pushDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
