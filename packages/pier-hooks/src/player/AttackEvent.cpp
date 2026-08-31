/**
 * hooks/player/AttackEvent.cpp —— `PlayerAttackTargetEvent`：攻击事件，
 * **带上被打的是谁**。
 *
 * # 为什么不用 LeviLamina 自己的 `PlayerAttackEvent`
 *
 * 它存在，也可以取消，唯一的问题是**载荷里读不出目标是什么**。真机上抓到
 * 的那一份：
 *
 * ```text
 * {_player:{name,pos,uuid,xuid}, cancelled:0b, cause:"EntityAttack", dim:1006,
 *  eventId:"ll::event::PlayerAttackEvent",
 *  self  :{_pointer_:2226787454976L, _type_:"Player"},
 *  target:{_pointer_:2226776030720L, _type_:"Actor"}}
 * ```
 *
 * `target` 是 LL 的反射对一个 `Actor&` 的序列化 —— 一个裸指针加一个**静
 * 态**类型名（永远是 `"Actor"`，因为事件就是这么声明的）。里面没有任何一
 * 格能区分「打的是玩家」和「打的是一头牛」。
 *
 * 而权限侧必须分开这两件事：
 *
 * > 分不开的话，`pvp` 旗标一关，在自己的地皮里打怪也被当成 PvP 拦下 ——
 * > 整个服务器不能战斗。反过来把攻击一律当「打生物」，PvP 就拦不住。
 *
 * 所以这里按 InteractEntityEvent / PushEntityEvent 的同一个形状加一个合成
 * 事件，把 `targetIsPlayer` 和 `target`（**动态**类型名）发出来。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, target, targetIsPlayer, cause, _player:{name,xuid,uuid}}
 * ```
 *
 * `x/y/z` 是**目标的**位置，不是攻击者的 —— 权限问题问的是「你能不能在那
 * 一格动手」。这一条和 InteractEntityEvent / RideEvent 保持一致；不一致的
 * 话「打」和「右键」在同一只羊上会落到两块不同的地皮上。
 *
 * # 取消
 *
 * 返回一个值初始化的 `ActorHurtResult`（全零 = 没造成伤害），不调 origin。
 * 挥空的动画照常播 —— 玩家看到的是「打了但没伤害」，和被别的保护插件拦下
 * 时一样。
 *
 * # 两个重载都挂，但**只派发一次**
 *
 * `Player::attack` 有 `(Actor&, ActorDamageCause const&)` 和多一个
 * `AttackParameters const&` 两个重载。哪个是真正的实现路径、哪个只是转发，
 * 没法在这里确认 —— 挂错一个的后果是**保护静默不生效**，而那正是这个文件
 * 要消掉的东西。所以两个都挂。
 *
 * 双挂本身带来一个真实缺陷：若二参转发给三参，一次攻击会派发**两次**。早
 * 先的说法是「放行时两次得到同一个答案，只是白跑一遍」—— 对判定确实如
 * 此，但对**计数型**订阅者不是：战斗日志、连击统计、攻击冷却各自会把一次
 * 攻击算成两次，而症状（数字总是双倍）看不出根因在宿主这一侧。所以加一道
 * thread_local 重入闸：外层派发时把深度加一，内层看到深度非零就直接
 * origin。闸是按线程的 —— 两个线程并发攻击不是重入。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorHurtResult.h"
#include "mc/world/actor/player/Player.h"
// `ActorDamageCause` **没有独立头文件** —— 它是
// `SharedTypes::Legacy::ActorDamageCause`，由 `Player.h` 传递引入。
// actors/Players.cpp 的 PIER_PACT_ATTACK 分支里就是这么用的，而那一处是编过的。

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& attackDef(); // 前向

        /**
         * `Player::attack` **有两个重载**，所以 `&Player::attack` 本身是歧义
         * 的（C2664：无法从 overloaded-function 推导）。两个签名分别是：
         *
         * ```cpp
         * ActorHurtResult Player::attack(Actor&, ActorDamageCause const&);
         * ActorHurtResult Player::attack(Actor&, ActorDamageCause const&,
         *                                Player::AttackParameters const&);
         * ```
         *
         * 返回值是 `ActorHurtResult`，不是 `bool`。
         *
         * 先 typedef 再 `static_cast` —— **不能把 `static_cast<A (T::*)(X, Y)>`
         * 直接写进宏**，模板参数里的逗号会被预处理器当成宏参数分隔符切开。
         *
         * # `$` 前缀：二参那个是**虚函数**
         *
         * ```text
         * static_assert failed: '...&Player::attack) is a virtual function,
         *                        you need use prefix $ workaround to hook it.'
         * ```
         *
         * LeviLamina 给每个虚函数生成一个 `$` 别名，钩虚函数必须用它 —— 这个
         * 目录里 `&Player::$drop` / `&Player::$setPlayerGameType` /
         * `&GameMode::$startDestroyBlock` 都是这么写的。
         *
         * 二参那个是 `Mob::attack` 的重写，所以是虚的。三参那个未确认：编译
         * 在二参就停了，没报到它。
         *
         * ⚠ **如果三参那条也报同一个 static_assert**，把下面
         * `PlayerAttackHook3` 里的 `&Player::attack` 改成 `&Player::$attack`
         * 就行（一处）。反过来如果它报「没有匹配的函数」，说明三参不是虚函数
         * **而且** `$attack` 只有一个 —— 那就把整个 `PlayerAttackHook3` 连同
         * 下面 `hook()` 里的 `r3` 一起删掉，只留虚函数那条。
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
         * 拦下时返回什么。
         *
         * 值初始化的 `ActorHurtResult` —— 全零，也就是「没造成伤害」。
         * InteractEntityEvent 那边对 `InteractionResult` 用的是同一招（构造
         * 一个再改字段）。这里的字段名未确认，所以不改，只给全零。如果实测发
         * 现拦下之后仍然掉血，就是这个假设错了。
         */
        ::ActorHurtResult refusedHurt() { return ::ActorHurtResult{}; }

        std::string buildAttackSnbt(Player& self, ::Actor& actor, int cause)
        {
            // `getTypeName` / `isPlayer` 会抛（实体正在被销毁时）。抛出去的话
            // 整台服务器在一次攻击上崩掉，所以就地吞掉 —— 订阅方读到空字符串
            // 会退回粗动作，不会更松。
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
            // 虚函数 → `$` 别名。`static_cast` 仍然要留：`$attack` 也可能有两
            // 个（两个重载都虚的话），有 cast 才不歧义，没歧义时它也无害。
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
                // `hook()` 返回 `ll::memory::hookEx` 的状态码：0 = 成功。
                // **两个都要报** —— 一个装失败、另一个成功的话，保护会在某些
                // 攻击路径上生效、另一些上不生效，而那比整个不生效更难查。
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
