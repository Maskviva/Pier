/** hooks/world/HopperEvents.cpp —— "HopperTransferEvent"：漏斗每一次槽位写
 * 入（HopperBlockActor::setItem）时触发，也就是物品进出漏斗的每一刻。载荷
 * 带前后两份堆叠，订阅者自己算差量（增加 = 物品流入）。没有维度字段：
 * setItem 作用域里没有 BlockSource；观察者按自己注册的位置键控。生命周期
 * 规矩见 hook_events.h。 */
#include "pier/hooks/hook_events.h"

#include <atomic>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/world/Container.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/HopperBlockActor.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& hopperDef(); // 前向，钩体要用

        LL_TYPE_INSTANCE_HOOK(
            HopperSetItemHook,
            ll::memory::HookPriority::Normal,
            HopperBlockActor,
            &HopperBlockActor::$setItem,
            void,
            int slot,
            ::ItemStack const& item)
        {
            auto& def = hopperDef();
            if (!def.live())
            {
                origin(slot, item); // 装着但空闲
                return;
            }

            // 这道卫必须跑在任何 this-> 虚派发之前。Container::setItem 函数体平
            // 凡，MSVC 的 ICF 极可能把它和箱子、熔炉同形状的 setItem 折叠到同一
            // 地址，于是本 detour 对那些方块实体也会进来；那时 this 是
            // ChestBlockActor*，经它虚调用按漏斗的 Container 子对象偏移读 vptr，
            // 拿到垃圾 vptr 后 DEP 跳转崩溃。getType() 非虚、读偏移 0 的
            // BlockActor::mType、对所有方块实体同义，被折叠也安全。
            if (this->getType() != ::BlockActorType::Hopper)
            {
                // 判别器。崩溃若真由 ICF 折叠导致，这里会对箱子、熔炉触发，
                // 说明卫修好了它；若是 this 调整块不匹配，getType() 读到垃圾，
                // 这里带着无意义的值触发且计数器收不到事件，说明该换钩点。
                static std::atomic<bool> logged{false};
                if (!logged.exchange(true))
                {
                    hostLogger().debug(
                        "HopperTransferEvent 的卫拒掉了一个非漏斗方块实体（getType={}）。"
                        "ICF 折叠下这是预期现象；如果计数一直是空的，说明该换钩点了。",
                        static_cast<int>(this->getType()));
                }
                origin(slot, item);
                return;
            }
            if (slot < 0 || slot >= 5) // 漏斗恰好 5 格；仍然加固
            {
                origin(slot, item);
                return;
            }

            // 先取写入前的状态，让写入发生，再上报。
            int oldCount = 0;
            std::string oldName;
            {
                // $getItem 是非虚实现，不走 vtable。
                ItemStack const& prev = this->$getItem(slot);
                oldCount = prev.mCount;
                if (oldCount > 0) oldName = prev.getTypeName();
            }
            origin(slot, item);

            int newCount = item.mCount;
            std::string newName = newCount > 0 ? item.getTypeName() : std::string{};
            BlockPos const& pos = this->getPosition();

            std::string snbt = "{\"eventId\":\"HopperTransferEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"slot\":" + snbtNum(slot)
                + ",\"item\":\"" + snbtEscape(newName)
                + "\",\"count\":" + snbtNum(newCount)
                + ",\"old_item\":\"" + snbtEscape(oldName)
                + "\",\"old_count\":" + snbtNum(oldCount) + "}";
            dispatchHookEvent(def, snbt);
        }

        HookEventDef gDef{"HopperTransferEvent", [] { return HopperSetItemHook::hook() == 0; }};
        HookEventDef& hopperDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
