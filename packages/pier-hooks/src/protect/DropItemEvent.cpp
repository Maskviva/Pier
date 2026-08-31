/**
 * hooks/protect/DropItemEvent.cpp —— 合成事件 "PlayerDropItemEvent"，可取消。
 *
 * LL 事件总线上没有 PlayerDropItemEvent；同名的只有原版的
 * mc/world/events/PlayerDropItemEvent.h，订阅不了也取消不了。丢弃有两条路，
 * 各挂一个钩子。Player::drop 是 Q 键，返回 bool，不调 origin 返回
 * false 即拒绝。ComplexInventoryTransaction::handle 是从打开的背包界面拖出去，
 * 完全不碰 Player::drop，只挂前者会拦住 Q 键却静默放行背包界面；取消是不调
 * origin 直接返回 NoError，事务报告为已处理而什么都没动。钩子 2 的过滤条件
 * （NormalTransaction、恰好一个来自 ContainerInventory 的动作）把「扔一摞到地
 * 上」和其他流经同一虚函数的背包挪动区分开。
 *
 * 载荷 {eventId, x, y, z, dim, item, randomly, viaInventoryUi, _player:{…}}。
 * x/y/z 取整，因为 LL 的反射把 Vec3 序列化成 JSON 数组。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/ContainerID.h"
#include "mc/world/actor/player/Player.h"
// PlayerInventory.h 必需：Player.h 只前向声明了它，而 ll::TypedStorage 把
// unique_ptr<T> 坍缩成裸 unique_ptr<T>，所以 player.mInventory 是真的
// unique_ptr，.get() 要求被指对象完整。缺这个 include 时 MSVC 报 C2027 加一条
// 把 unique_ptr 说成缺成员的 C2039，读起来像解引用次数写错了。完整坍缩规则见
// tools/typed-storage.py 的文件头。
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

        /** 共享的载荷拼装：两个钩子报告同一个事件形状。 */
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

        /** 钩子装上了却一个订阅者也没有，每进程提醒一次。两个钩子共用一个旗
         *  标，它们由同一次订阅武装。 */
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
                // false == 这次丢弃没有发生，那摞东西还在容器里。
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

            // 一个来自玩家自己背包的单独动作，就是「扔一摞到地上」的形状。合
            // 成、容器间挪动、创造模式取物要么来源不同，要么动作不止一个。
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
                // 返回 NoError 而非错误码：客户端认为事务已处理，不重试也不失
                // 同步，物品只是从未离开背包。
                return ::InventoryTransactionError::NoError;
            }
            return origin(player, isSenderAuthority);
        }

        HookEventDef gDef{
            "PlayerDropItemEvent",
            []
            {
                // hook() 返回 ll::memory::hookEx 的状态：0 == 成功，非 0 == 符号
                // 找不到或补丁被拒。必须显出来：装失败和正常工作长得一模一样，
                // 而版本不匹配只能从这里诊断。
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
                return r1 == 0 && r2 == 0;
            }
        };
        HookEventDef& dropItemDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
