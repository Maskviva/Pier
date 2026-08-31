/**
 * hooks/player/AttackEvent.cpp —— 合成事件 "PlayerAttackTargetEvent"，可取消。
 *
 * LeviLamina 的 PlayerAttackEvent 可取消，但载荷里的 target 是 LL 反射对
 * Actor& 的序列化，只有裸指针和静态类型名（恒为 "Actor"），分不出打的是玩家
 * 还是生物。权限侧必须分开这两件事：分不开时 pvp 旗标一关连打怪都被拦。本事件
 * 补上 targetIsPlayer 与 target 的动态类型名。
 *
 * 取消时返回值初始化的 ActorHurtResult（全零即无伤害），不调 origin。
 * Player::attack 两个重载都挂（哪个是实现路径、哪个是转发无法在此确认，挂错的
 * 后果是保护静默失效），thread_local 重入闸保证一次攻击只派发一次，否则计数型
 * 订阅方会把一次攻击算成两次。载荷 {eventId, x, y, z, dim, target, targetIsPlayer, cause, _player:{…}}。
 * x/y/z 取目标位置而非攻击者位置，与 InteractEntityEvent、RideEvent 一致；不一
 * 致时「打」和「右键」同一只羊会落到两块地皮上。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorHurtResult.h"
#include "mc/world/actor/player/Player.h"
// ActorDamageCause 没有独立头文件，它是 SharedTypes::Legacy::ActorDamageCause，
// 由 Player.h 传递引入。actors/Players.cpp 的 PIER_PACT_ATTACK 分支同此。

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& attackDef(); // 前向

        /**
         * Player::attack 有两个重载，&Player::attack 本身歧义（C2664），所以先
         * typedef 再 static_cast。typedef 不可省：static_cast<A (T::*)(X, Y)> 里
         * 的逗号会被预处理器当成宏参数分隔符切开。
         *
         * 二参那个是 Mob::attack 的重写，虚函数必须挂 LeviLamina 生成的 $ 别名，
         * 否则 static_assert 拦下。三参那个是否为虚未确认：若它也报同一个
         * static_assert，把 PlayerAttackHook3 的 &Player::attack 改成
         * &Player::$attack；若报「没有匹配的函数」，删掉 PlayerAttackHook3 与 r3。
         */
        using AttackFn2 = ::ActorHurtResult (Player::*)(
            ::Actor&,
            ::SharedTypes::Legacy::ActorDamageCause const&);
        using AttackFn3 = ::ActorHurtResult (Player::*)(
            ::Actor&,
            ::SharedTypes::Legacy::ActorDamageCause const&,
            ::Player::AttackParameters const&);

        /** 见文件头「只派发一次」。按线程计：并发攻击不是重入。 */
        thread_local int tlAttackDepth = 0;

        struct AttackDepthGuard
        {
            AttackDepthGuard() { ++tlAttackDepth; }
            ~AttackDepthGuard() { --tlAttackDepth; }
        };

        /**
         * 拦下时返回值初始化的 ActorHurtResult，全零即无伤害。字段名未确认，
         * 所以不逐字段改。若拦下之后仍然掉血，就是这个假设错了。
         */
        ::ActorHurtResult refusedHurt() { return ::ActorHurtResult{}; }

        std::string buildAttackSnbt(Player& self, ::Actor& actor, int cause)
        {
            // getTypeName / isPlayer 在实体正在销毁时会抛，异常穿过 detour 等于
            // 整服崩，所以就地吞掉。订阅方读到空串会退回粗判定，不会更松。
            std::string targetName;
            bool isPlayerTarget = false;
            try
            {
                targetName = actor.getTypeName();
                isPlayerTarget = actor.isPlayer();
            }
            catch (...)
            {
                targetName.clear();
                isPlayerTarget = false;
            }

            auto const& pos = actor.getPosition();
            return std::string{"{\"eventId\":\"PlayerAttackTargetEvent\""}
                + ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(actor.getDimensionId()))
                + ",\"targetIsPlayer\":" + (isPlayerTarget ? "1" : "0")
                + ",\"target\":\"" + snbtEscape(targetName)
                + "\",\"cause\":" + snbtNum(cause)
                + "," + playerRefSnbt(self) + "}";
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerAttackHook2,
            ll::memory::HookPriority::Normal,
            Player,
            // 虚函数走 $ 别名。static_cast 仍要留：两个重载都虚时 $attack 也有
            // 两个，有 cast 才不歧义；没歧义时它无害。
            static_cast<AttackFn2>(&Player::$attack),
            ::ActorHurtResult,
            ::Actor& actor,
            ::SharedTypes::Legacy::ActorDamageCause const& cause)
        {
            auto& def = attackDef();
            if (!def.live() || tlAttackDepth > 0)
            {
                return origin(actor, cause);
            }
            AttackDepthGuard depth;
            if (dispatchHookEventCancellable(
                    def, buildAttackSnbt(*this, actor, static_cast<int>(cause))))
            {
                return refusedHurt();
            }
            return origin(actor, cause);
        }

        LL_TYPE_INSTANCE_HOOK(
            PlayerAttackHook3,
            ll::memory::HookPriority::Normal,
            Player,
            static_cast<AttackFn3>(&Player::attack),
            ::ActorHurtResult,
            ::Actor& actor,
            ::SharedTypes::Legacy::ActorDamageCause const& cause,
            ::Player::AttackParameters const& params)
        {
            auto& def = attackDef();
            if (!def.live() || tlAttackDepth > 0)
            {
                return origin(actor, cause, params);
            }
            AttackDepthGuard depth;
            if (dispatchHookEventCancellable(
                    def, buildAttackSnbt(*this, actor, static_cast<int>(cause))))
            {
                return refusedHurt();
            }
            return origin(actor, cause, params);
        }

        HookEventDef gDef{
            "PlayerAttackTargetEvent",
            []
            {
                // hook() 返回 ll::memory::hookEx 的状态码，0 == 成功。两个都要
                // 报：一个装失败另一个成功时，保护只在部分攻击路径上生效，那比
                // 整个不生效更难查。
                int r2 = PlayerAttackHook2::hook();
                int r3 = PlayerAttackHook3::hook();
                auto& log = hostLogger();
                log.debug(
                    "[AttackEvent] 安装 detour：attack/2={} (code={})，attack/3={} (code={})",
                    r2 == 0 ? "成功" : "失败", r2,
                    r3 == 0 ? "成功" : "失败", r3
                );
                if (r2 != 0 && r3 != 0)
                {
                    log.error(
                        "[AttackEvent] 两个 detour 都没装上 —— 「打玩家」和「打生物」"
                        "在地皮上分不开，pvp 旗标拦不住人。最常见原因是本宿主链接的 "
                        "BDS/LeviLamina 版本和服务器实际跑的不一致。");
                }
                return r2 == 0 || r3 == 0;
            }
        };
        HookEventDef& attackDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
