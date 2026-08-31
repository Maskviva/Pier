/**
 * hooks/protect/ProjectileEvent.cpp —— 合成事件 "PlayerSpawnProjectileEvent"，可取消。
 *
 * 原版投射物是物品组件，投掷走 ThrowableItemComponent::_doThrow →
 * ProjectileItemComponent::shootProjectile → Item::createProjectileActor。
 * BedrockSpawner::spawnProjectile 已不在玩家路径上，仅作兜底；PlayerUseItemEvent
 * 覆盖不了弓弩三叉戟的蓄力—释放路径。五个钩点按覆盖面排列且会嵌套，gDispatching
 * 把一次发射收敛成一个判定。取消停下的是投射物，不退还弹药：钩子跑到时箭已离开
 * 背包，客户端一 tick 内自行对齐。
 *
 * 载荷 {eventId, x, y, z, dim, projectile, _player:{name,xuid,uuid}}。钩点 2、4
 * 触发时实体类型尚未解析，projectile 可能为空，只作参考。
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
         * 判定进行中的标记，嵌套钩点据此不再重复问。
         *
         * thread_local 而非全局：全局标记会让两次并发发射互相吞掉对方的判定，
         * 那种漏放是安静的。DispatchGuard 在每个钩子退出时复原。
         */
        thread_local bool gDispatching = false;

        struct DispatchGuard
        {
            DispatchGuard() { gDispatching = true; }
            ~DispatchGuard() { gDispatching = false; }
        };

        /** 共享的载荷拼装：五个钩点报告同一个事件形状。 */
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

        /** 名字类调用在物品或实体正在销毁时会抛，异常穿过 detour 等于整服崩，
         *  所以就地吞掉。订阅方拿到空名字会退回粗判定，不会更松。 */
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

        // 1. 组件驱动的投射物：雪球、鸡蛋、末影珍珠、药水、附魔之瓶、风弹、
        //    火焰弹，以及弓弩射出的箭。直接带 Player*，返回 nullptr 即取消。

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

        // 2. 弓弩释放本身。与 1 冗余，但它在逐箭循环之前触发，所以多重射的弩
        //    在这里只花一次判定。

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

        // 3. 投出的三叉戟。它是定制物品，钩点 1、2 都够不到。

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

        // 4. 装了烟花火箭的弩，同 3。

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

        // 5. 旧 spawner 路径，兜底附加包实体与发射器邻近代码。

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
                // 逐个报状态（0 == 成功）：钩点覆盖面不同，只有 1 装失败时雪球
                // 和弓箭全部漏过而三叉戟仍被拦，症状是「保护时灵时不灵」。合并
                // 成一条日志就分不出这种局部失效。
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
