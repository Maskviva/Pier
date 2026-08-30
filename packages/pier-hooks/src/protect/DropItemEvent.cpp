/**
 * hooks/protect/DropItemEvent.cpp —— "PlayerDropItemEvent"：玩家即将把物品
 * 从背包里扔出去，**并且可以取消**。
 *
 * # 为什么需要这个钩子
 *
 * 事件名表里的 `PLAYER_DROP_ITEM` 曾自称「对着钉住的 LeviLamina 头核对
 * 过」。并没有：`ll/api/event/player/` 下面没有任何
 * `PlayerDropItemEvent`。同名的东西只有**原版的**
 * `mc/world/events/PlayerDropItemEvent.h`，一个 `PlayerGameplayEvent<void>`
 * 变体 —— 不在 LL 事件总线上，订阅不了，也取消不了。
 *
 * 于是 `subscribe_event("PlayerDropItemEvent")` 会一路落到注册表解析、什么
 * 都找不到、打一句「未知或有歧义的事件 id」、返回空。
 *
 * # 两个钩子，因为丢弃有两条路
 *
 * 这是最容易做错的地方，而 LegacyScriptEngine 做对了
 *（`DropItemHook1` / `DropItemHook2`）：
 *
 *   1. `Player::drop` —— Q 键 / 「丢下手持物品」。返回 bool；不调 origin
 *      直接返回 false 就拒绝了这次丢弃。
 *
 *   2. `ComplexInventoryTransaction::handle` —— 从**打开的背包界面**里把一
 *      摞东西拖出去。这条路根本不碰 `Player::drop`，所以只建立在钩子 1 上
 *      的保护会拦住 Q 键、却静默放行背包界面。取消的做法是不调 origin 直接
 *      返回 `NoError`：事务被报告为已处理，而什么都没动。
 *
 * 钩子 2 上的那个过滤条件（NormalTransaction、恰好一个来自
 * `ContainerInventory` 的动作）取自 LSE，它正是把「玩家把一摞东西扔到地
 * 上」和所有其他流经同一个虚函数的背包挪动区分开的东西。
 *
 * # 载荷
 *
 * ```text
 * {eventId, x, y, z, dim, item, randomly, viaInventoryUi, _player:{name,xuid,uuid}}
 * ```
 *
 * `x/y/z` 是玩家位置的平铺整数 —— 本目录每个合成事件都用这个形状，因为 LL
 * 的反射把 `Vec3` 序列化成 JSON **数组**，而期待 `{x,y,z}` 的消费方从里面
 * 读不到东西。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/ContainerID.h"
#include "mc/world/actor/player/Player.h"
// PlayerInventory.h 是必需的，不是可选的（TypedStorage 的完整坍缩规则见
// `tools/typed-storage.py` 的文件头）：`Player.h` 只**前向声明**了
// PlayerInventory，而 `ll::TypedStorage` 有一个把 `unique_ptr<T>` 坍缩成裸
// `unique_ptr<T>` 的特化 —— 所以 `player.mInventory` 是一个真的 unique_ptr，
// 而 `.get()` 需要被指对象是完整类型。没有这个 include，MSVC 会在不完整类型
// 上报 C2027，外加一个把 `unique_ptr` 说成缺成员的连锁 C2039 —— 读起来像是
// 解引用次数写错了，其实不是。
#include "mc/world/actor/player/PlayerInventory.h"
#include "mc/world/inventory/transaction/ComplexInventoryTransaction.h"
#include "mc/world/inventory/transaction/InventoryAction.h"
#include "mc/world/inventory/transaction/InventorySource.h"
#include "mc/world/inventory/transaction/InventorySourceType.h"
#include "mc/world/inventory/transaction/InventoryTransaction.h"
#include "mc/world/inventory/transaction/InventoryTransactionError.h"
#include "mc/world/item/ItemStack.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& dropItemDef(); // 前向

        /** 物品类型名；读不出来时给 ""。 */
        std::string safeTypeName(::ItemStack const& item)
        {
            try
            {
                return item.isNull() ? std::string{} : item.getTypeName();
            }
            catch (...)
            {
                return {};
            }
        }

        /** 共享的载荷拼装 —— 两个钩子报告同一个事件形状。 */
        std::string buildSnbt(Player& p, std::string const& itemName, bool randomly, bool viaUi)
        {
            auto const& pos = p.getPosition();
            return "{\"eventId\":\"PlayerDropItemEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"randomly\":" + (randomly ? "1" : "0")
                + ",\"viaInventoryUi\":" + (viaUi ? "1" : "0")
                + ",\"item\":\"" + snbtEscape(itemName)
                + "\"," + playerRefSnbt(p) + "}";
        }

        /** 钩子装上了、却一个订阅者也没有 —— 每进程提醒一次。
         *  两个钩子共用一个旗标：它们由同一次订阅武装，分开各喊一次只是把同
         *  一件事说两遍。 */
        void warnNoSubscriberOnce(char const* which)
        {
            static bool warned = false;
            if (warned) return;
            warned = true;
            hostLogger().warn(
                "[DropItemEvent] {} 已触发，但没有任何订阅者（def.live()==false）。"
                "说明原生 detour 已装上，但另一侧没成功订阅 'PlayerDropItemEvent'。"
                "检查启动日志里是否有「订阅事件 PlayerDropItemEvent 失败」。",
                which);
        }

        /** 钩子 1：Q 键 / 丢下手持物品。 */
        LL_TYPE_INSTANCE_HOOK(
            PlayerDropItemHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$drop,
            bool,
            ::ItemStack const& item,
            bool randomly)
        {
            auto& def = dropItemDef();
            if (!def.live())
            {
                warnNoSubscriberOnce("PlayerDropItemHook");
                return origin(item, randomly);
            }

            if (dispatchHookEventCancellable(
                    def, buildSnbt(*this, safeTypeName(item), randomly, false)))
            {
                // false == 「这次丢弃没有发生」。那摞东西还在容器里，没有什么
                // 需要还原的。
                return false;
            }
            return origin(item, randomly);
        }

        /** 钩子 2：从打开的背包界面里把一摞东西拖出去。 */
        LL_TYPE_INSTANCE_HOOK(
            InventoryUiDropHook,
            ll::memory::HookPriority::Normal,
            ComplexInventoryTransaction,
            &ComplexInventoryTransaction::$handle,
            ::InventoryTransactionError,
            ::Player& player,
            bool isSenderAuthority)
        {
            auto& def = dropItemDef();
            if (!def.live())
            {
                warnNoSubscriberOnce("InventoryUiDropHook");
                return origin(player, isSenderAuthority);
            }
            if (mType != ComplexInventoryTransaction::Type::NormalTransaction)
            {
                return origin(player, isSenderAuthority);
            }

            // 过滤条件取自 LSE：一个来自玩家自己背包的、单独的动作，就是「把
            // 一摞东西扔到地上」的形状。其他每一种事务（合成、容器间挪动、创
            // 造模式取物）要么来源不同，要么动作不止一个。
            ::InventorySource source{
                ::InventorySourceType::ContainerInventory,
                ::ContainerID::Inventory,
                ::InventorySource::InventorySourceFlags::NoFlag
            };

            auto const& actions = mTransaction->getActions(source);
            if (actions.size() != 1)
            {
                return origin(player, isSenderAuthority);
            }

            std::string itemName = safeTypeName(
                player.mInventory.get()->getItem(actions[0].mSlot, ::ContainerID::Inventory));

            if (dispatchHookEventCancellable(def, buildSnbt(player, itemName, false, true)))
            {
                // 返回 NoError 而不是某个错误码：告诉客户端这次事务已被处理，
                // 于是它不会重试、也不会失同步。物品只是从未离开背包。
                return ::InventoryTransactionError::NoError;
            }
            return origin(player, isSenderAuthority);
        }

        HookEventDef gDef{
            "PlayerDropItemEvent",
            []
            {
                // `hook()` 返回 ll::memory::hookEx 的状态：0 == 成功，非 0 ==
                // 失败（符号找不到 / 补丁被拒）。早先的代码把它吞了 —— 装失败
                // 和「正常工作」长得一模一样，而丢弃保护静默地什么都没做。把
                // 它显出来，好让版本不匹配能从日志里诊断出来。
                int r1 = PlayerDropItemHook::hook();
                int r2 = InventoryUiDropHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[DropItemEvent] 安装 detour：PlayerDropItemHook={} (code={})，"
                    "InventoryUiDropHook={} (code={})",
                    r1 == 0 ? "成功" : "失败", r1,
                    r2 == 0 ? "成功" : "失败", r2
                );
                if (r1 != 0 || r2 != 0)
                {
                    log.error(
                        "[DropItemEvent] 原生 detour 安装失败（非 0 状态码）。最常见原因是"
                        "本宿主链接的 BDS/LeviLamina 版本与服务器实际运行的版本不一致，"
                        "导致 Player::$drop 或 ComplexInventoryTransaction::$handle 的符号地址"
                        "解析错误。结果：丢弃物品保护完全不生效（物品照常掉落，且不触发任何"
                        "拦截日志）。请用服务器实际运行的 LeviLamina/BDS 版本重新编译本宿主。");
                }
            }
        };
        HookEventDef& dropItemDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
