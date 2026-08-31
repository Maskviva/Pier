/**
 * hooks/protect/PushEntityEvent.cpp —— "PlayerPushEntityEvent"：玩家即将靠
 * 走路把另一个实体推开，**并且可以取消**。
 *
 * # 为什么这是保护而不是锦上添花
 *
 * 这个桥里其他每一项实体保护覆盖的都是一次**点击**。推挤不需要点击：一个
 * 不能破坏、不能放置、不能交互、不能攻击的访客，照样能走进领地把主人的牲
 * 畜赶出围栏、把船顶进虚空、或者把盔甲架和展示框里的东西一点点挪走。它是
 * 唯一一种能在完全锁死的领地上存活的破坏手段，而且什么日志都不留。
 *
 * # 钩点
 *
 * `PushableByEntityUtility::skipPush(Actor& owner, Actor& other)` —— 命名空
 * 间里的自由函数，所以用 `LL_STATIC_HOOK` 而不是实例宏。它是引擎自己的
 * 「这次推挤该不该跳过」之问，也就意味着返回 `true` 是每个调用方本来就正
 * 确处理的结果。这比听上去值钱：在推挤内部拒绝会让两个实体重叠着、碰撞悬
 * 而未决。
 *
 * `skipPush` 没有重载，不像 `push`（它同时有 `Actor&,Vec3` 和
 * `Actor&,Actor&,bool` 两种形式，写进宏参数还得加消歧 cast）。少一件会出错
 * 的事。
 *
 * # 哪一个是玩家
 *
 * 别假设。碰撞解算从两边都会跑 —— 牛的 tick 推玩家，玩家的 tick 推牛 ——
 * 所以玩家可能作为 `owner` 到达，也可能作为 `other` 到达，取决于正在 tick
 * 哪个实体。只检查一边得到的是一个大约一半时间生效的保护，那比完全不生效
 * 更糟，因为它测起来像是「基本能用」。
 *
 * 权限在**被推实体的**位置上判定：问题是这个人能不能扰动**那里**的东西，
 * 而一个站在地皮边界外的玩家可以推里面的动物。
 *
 * # 玩家推玩家不管
 *
 * 两边都是玩家时这个钩子什么都不做。玩家间碰撞是正常移动，不对称地拦它
 * （A 能推 B、B 不能推 A）会产生看起来像网络延迟而不像保护的橡皮筋效应。
 *
 * # 节流
 *
 * 这对每一对重叠实体每 tick 都跑。为什么这个缓存是必需而不是优化，见
 * decision_throttle.h。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, target, _player:{name,xuid,uuid}}
 * ```
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

            // 两边都可能是玩家 —— 见文件头。
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

            // getTypeName 会抛（实体正在被销毁时）。这条路每 tick 都跑，抛出
            // 去就是整台服务器在一次碰撞上崩掉，所以就地吞掉。
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

            // true == 「跳过这次推挤」，正是取消的意思。
            return cancelled ? true : origin(owner, other);
        }

        HookEventDef gDef{"PlayerPushEntityEvent", [] { return PlayerPushEntityHook::hook() == 0; }};
        HookEventDef& pushDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
