/**
 * hooks/protect/ProjectileEvent.cpp —— "PlayerSpawnProjectileEvent"：玩家即
 * 将发射一个投射物，**并且可以取消**。
 *
 * # 为什么 `PlayerUseItemEvent` 从来就不够
 *
 * 模组侧最初把投掷这件事接在 `PlayerUseItemEvent` 上，然后猜过
 * `PlayerThrowProjectileEvent` / `PlayerShootEvent`（两个在 LL 总线上都不存
 * 在）。`PlayerUseItemEvent` 来自 `GameMode::useItem`，它覆盖点一下就扔的
 * 那类物品，但覆盖不了弓、弩、三叉戟走的蓄力—释放路径。
 *
 * # 为什么这个文件的第一次重写也没work
 *
 * 第一版钩的是 `BedrockSpawner::spawnProjectile`，照抄 LegacyScriptEngine 的
 * `onSpawnProjectile`。它装得上，而在这个构建上对玩家投掷**一次都不触发**
 * —— 被拒绝的玩家扔雪球照样飞。
 *
 * 原因是一次 LSE 还没跟上的版本漂移：原版投射物现在是**物品组件**。一个雪
 * 球是带着 `ThrowableItemComponent` 加 `ProjectileItemComponent` 的
 * `ComponentItem`，而投掷走的是
 *
 *   `ThrowableItemComponent::_doThrow`
 *     → `ProjectileItemComponent::shootProjectile(region, aimPos, aimDir, power, player)`
 *       → `Item::createProjectileActor`（每个物品各自覆写 —— `SnowballItem`、
 *          `EggItem`、`SplashPotionItem`、`ArrowItem`、…）
 *
 * `Spawner::spawnProjectile` 已经不在这条路上了。钩它不算**错**，只是它上游
 * 没有任何玩家会做的事。
 *
 * # 现在钩在哪
 *
 * 按各自能覆盖多少问题排序：
 *
 *   1. `ProjectileItemComponent::shootProjectile` —— 每一个组件驱动的投射
 *      物：雪球、鸡蛋、末影珍珠、喷溅药水与滞留药水、附魔之瓶、风弹、火焰
 *      弹，以及弓或弩释放出的箭。直接带着 `Player*`。返回 `Actor*`，所以取
 *      消就是 `nullptr`。
 *
 *   2. `ShooterItemComponent::_shootProjectiles` —— 弓/弩的释放本身。对投射
 *      物而言与 (1) 冗余，但它在逐箭循环**之前**触发，所以在这里取消能用一
 *      次判定拒掉一发多重射的弩，而不是三次。
 *
 *   3. `TridentItem::releaseUsing` —— 三叉戟仍然是个定制物品，(1) 和 (2) 都
 *      够不到它。
 *
 *   4. `CrossbowItem::_shootFirework` —— 弩发射的烟花火箭，同上。
 *
 *   5. `BedrockSpawner::spawnProjectile` —— 作为兜底留着，给任何仍在走老路
 *      的东西（附加包实体、某些和发射器相邻的代码）。
 *
 * # 重入
 *
 * 钩子 1、2、5 会嵌套：一次弓释放可能为同一支箭穿过全部三层。`gDispatching`
 * 把它收敛成每次发射一个判定 —— 没有它，一个被拒绝的玩家每射一次会得到三
 * 次「没有权限」的判定，更显眼的是三行日志。
 *
 * # 不退还弹药
 *
 * 这些钩子跑到的时候，箭已经离开背包了。取消停下的是**投射物**，不是消耗；
 * 客户端会在一 tick 内自己对齐回来。这是刻意的取舍 —— 拒绝这一发才是安全属
 * 性。如果丢掉的那支箭真的要紧，修的位置在上游的
 * `GameMode::releaseUsingItem`，不在这里。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, projectile, _player:{name,xuid,uuid}}
 * ```
 *
 * `projectile` 是尽力而为的名字，对还不知道实体类型的钩子可能为空（2 和 4
 * 在它被解析出来之前就触发了）。订阅方必须把它当参考信息，而不是判定所依据
 * 的东西。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/common/Globals.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/VanillaActorRendererId.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/CrossbowItem.h"
#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/item/TridentItem.h"
#include "mc/world/item/components/ProjectileItemComponent.h"
#include "mc/world/item/components/ShooterItemComponent.h"
#include "mc/world/level/BedrockSpawner.h"
#include "mc/world/level/BlockSource.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& projectileDef(); // 前向

        /**
         * 判定进行中的标记，让嵌套的钩子不再重复问。
         *
         * thread_local 而不是普通全局：这些钩点实际都跑在服务器线程上，但按
         * 线程计的代价是零，而万一哪天有一条路不在主线程上，全局标记会让两次
         * 并发发射互相吞掉对方的判定 —— 那种漏放是安静的。每个钩子在退出时
         * 由 DispatchGuard 复原。
         */
        thread_local bool gDispatching = false;

        struct DispatchGuard
        {
            DispatchGuard() { gDispatching = true; }
            ~DispatchGuard() { gDispatching = false; }
        };

        /** 共享的载荷拼装 —— 每个钩子报告同一个事件形状。 */
        std::string buildSnbt(Player& p, std::string const& projectile, ::Vec3 const& at, int dim)
        {
            return "{\"eventId\":\"PlayerSpawnProjectileEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(dim)
                + ",\"projectile\":\"" + snbtEscape(projectile)
                + "\"," + playerRefSnbt(p) + "}";
        }

        /** 名字类调用会抛（物品/实体正在被销毁时）。抛出去等于一次射击把服务
         *  器带走，所以就地吞掉 —— 订阅方拿到空名字会退回粗判定，不会更松。 */
        template <class Fn>
        std::string safeName(Fn&& fn)
        {
            try
            {
                return std::string{fn()};
            }
            catch (...)
            {
                return {};
            }
        }

        /** 问一次。必须拒绝这次发射时返回 true。 */
        bool refuseLaunch(Player& p, std::string const& projectile, ::Vec3 const& at, int dim)
        {
            DispatchGuard guard;
            return dispatchHookEventCancellable(projectileDef(), buildSnbt(p, projectile, at, dim));
        }

        // ── 1. 每个组件驱动的投射物 ────────────────────────────────────────

        LL_TYPE_INSTANCE_HOOK(
            ShootProjectileHook,
            ll::memory::HookPriority::Normal,
            ProjectileItemComponent,
            &ProjectileItemComponent::shootProjectile,
            ::Actor*,
            ::BlockSource& region,
            ::Vec3 const& aimPos,
            ::Vec3 const& aimDir,
            float power,
            ::Player* player)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || player == nullptr)
            {
                return origin(region, aimPos, aimDir, power, player);
            }

            if (refuseLaunch(*player, {}, aimPos, static_cast<int>(region.getDimensionId())))
            {
                return nullptr;
            }
            return origin(region, aimPos, aimDir, power, player);
        }

        // ── 2. 弓 / 弩释放 ─────────────────────────────────────────────────

        LL_TYPE_INSTANCE_HOOK(
            ShooterReleaseHook,
            ll::memory::HookPriority::Normal,
            ShooterItemComponent,
            &ShooterItemComponent::_shootProjectiles,
            void,
            ::ItemStack& shooterStack,
            ::Player* player,
            int durationLeft)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || player == nullptr)
            {
                return origin(shooterStack, player, durationLeft);
            }

            std::string shooterName;
            if (!shooterStack.isNull())
            {
                shooterName = safeName([&] { return shooterStack.getTypeName(); });
            }

            if (refuseLaunch(
                    *player,
                    shooterName,
                    player->getPosition(),
                    static_cast<int>(player->getDimensionId())))
            {
                return;
            }
            origin(shooterStack, player, durationLeft);
        }

        // ── 3. 投出的三叉戟 ────────────────────────────────────────────────

        LL_TYPE_INSTANCE_HOOK(
            TridentReleaseHook,
            ll::memory::HookPriority::Normal,
            TridentItem,
            &TridentItem::$releaseUsing,
            void,
            ::ItemStack& item,
            ::Player* player,
            int durationLeft)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || player == nullptr)
            {
                return origin(item, player, durationLeft);
            }

            std::string projName =
                safeName([] { return ::VanillaActorRendererId::trident().getString(); });

            if (refuseLaunch(
                    *player,
                    projName,
                    player->getPosition(),
                    static_cast<int>(player->getDimensionId())))
            {
                return;
            }
            origin(item, player, durationLeft);
        }

        // ── 4. 装了烟花火箭的弩 ────────────────────────────────────────────

        LL_TYPE_INSTANCE_HOOK(
            CrossbowFireworkHook,
            ll::memory::HookPriority::Normal,
            CrossbowItem,
            &CrossbowItem::_shootFirework,
            void,
            ::ItemInstance const& projectileInstance,
            ::Player& player)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching)
            {
                return origin(projectileInstance, player);
            }

            std::string projName = safeName([&] { return projectileInstance.getTypeName(); });

            if (refuseLaunch(
                    player,
                    projName,
                    player.getPosition(),
                    static_cast<int>(player.getDimensionId())))
            {
                return;
            }
            origin(projectileInstance, player);
        }

        // ── 5. 老的 spawner 路径，作为兜底 ─────────────────────────────────

        LL_TYPE_INSTANCE_HOOK(
            SpawnProjectileHook,
            ll::memory::HookPriority::Normal,
            BedrockSpawner,
            &BedrockSpawner::$spawnProjectile,
            ::Actor*,
            ::BlockSource& region,
            ::ActorDefinitionIdentifier const& id,
            ::Actor* spawner,
            ::Vec3 const& position,
            ::Vec3 const& direction)
        {
            auto& def = projectileDef();
            if (!def.live() || gDispatching || spawner == nullptr || !spawner->isPlayer())
            {
                return origin(region, id, spawner, position, direction);
            }

            // 三叉戟由 TridentReleaseHook 负责；在这里再报一次会让一次投掷触
            // 发两遍事件。
            static auto& tridentName = EntityCanonicalName(::ActorType::Trident);
            if (*id.mCanonicalName == tridentName)
            {
                return origin(region, id, spawner, position, direction);
            }

            auto& p = *static_cast<Player*>(spawner);

            std::string projName = safeName([&] { return id.mCanonicalName->getString(); });

            if (refuseLaunch(p, projName, position, static_cast<int>(region.getDimensionId())))
            {
                return nullptr;
            }
            return origin(region, id, spawner, position, direction);
        }

        HookEventDef gDef{
            "PlayerSpawnProjectileEvent",
            []
            {
                // 五个钩点逐个报状态：0 == 成功。**必须逐个报**，因为它们的覆
                // 盖面不同 —— 只有 1 没装上的话，雪球和弓箭全部漏过而三叉戟仍
                // 被拦住，表现是「保护对某些投射物时灵时不灵」；不逐个打出来，
                // 这种局部失效没法从日志区分。
                int r1 = ShootProjectileHook::hook();
                int r2 = ShooterReleaseHook::hook();
                int r3 = TridentReleaseHook::hook();
                int r4 = CrossbowFireworkHook::hook();
                int r5 = SpawnProjectileHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[ProjectileEvent] 安装 detour：shootProjectile={}，_shootProjectiles={}，"
                    "trident.releaseUsing={}，_shootFirework={}，spawnProjectile={}"
                    "（codes: {} {} {} {} {}）",
                    r1 == 0 ? "成功" : "失败", r2 == 0 ? "成功" : "失败",
                    r3 == 0 ? "成功" : "失败", r4 == 0 ? "成功" : "失败",
                    r5 == 0 ? "成功" : "失败",
                    r1, r2, r3, r4, r5);
                if (r1 != 0)
                {
                    log.error(
                        "[ProjectileEvent] 主钩点 ProjectileItemComponent::shootProjectile "
                        "安装失败（code={}）—— 雪球、鸡蛋、末影珍珠、药水、弓箭等**组件驱动**"
                        "的投射物将完全不受保护（其余钩点只覆盖三叉戟、弩烟花和老路径）。"
                        "最常见原因是本宿主链接的 BDS/LeviLamina 版本与服务器实际运行的版本"
                        "不一致。", r1);
                }
                if (r2 != 0 || r3 != 0 || r4 != 0 || r5 != 0)
                {
                    log.warn(
                        "[ProjectileEvent] 有辅助钩点未装上（codes: {} {} {} {}）—— "
                        "对应路径（弓弩整发判定 / 三叉戟 / 弩烟花 / 老 spawner 路径）"
                        "的拦截会缺失。",
                        r2, r3, r4, r5);
                }
                // 主钩点必须在；辅助钩点缺失只降级（已 warn）。
                return r1 == 0;
            }
        };
        HookEventDef& projectileDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
