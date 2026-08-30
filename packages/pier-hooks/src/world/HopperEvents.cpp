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
        HookEventDef& hopperDef(); // 前向 —— 钩体要用

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

            // 卫 —— 必须跑在任何 this-> 虚派发**之前**。
            //
            // Container::setItem 的函数体很平凡，于是 MSVC 的 ICF（相同
            // COMDAT 折叠）极可能把它和箱子/木桶/熔炉/发射器同形状的
            // setItem 折叠到**同一个**代码地址上。钩那个地址意味着这个
            // detour 对那些方块实体也会进来，那时 `this` 是比如
            // ChestBlockActor*，而经它做一次虚调用会按**漏斗的**
            // Container 子对象偏移去读 vptr ⇒ 垃圾 vptr ⇒ DEP 跳转 ⇒ 崩溃。
            //
            // getType() 是 MCFOLD（非虚），读的是**主基类**（偏移 0）上的
            // BlockActor::mType，而且只在 BlockActor 上定义一次（没有容器
            // 覆写它）—— 就算它自己也被折叠也对任何方块实体安全，因为它对
            // 所有方块实体是同一个意思。卫放行之后的东西一定是真正的
            // HopperBlockActor，所以后面那些非虚的 $getItem / getPosition 是
            // 安全的。
            if (this->getType() != ::BlockActorType::Hopper)
            {
                // 判别器：如果崩溃真是 ICF 折叠导致的，这里会对
                // 箱子/熔炉/等触发，而卫把它修好了。如果反过来是 this
                // 调整块（adjustor thunk）不匹配，getType() 读到的是垃圾、
                // 这里会带着一个没意义的值触发 —— 那时计数器将**收不到**任
                // 何事件，正好告诉我们该换钩点而不是加卫。
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
                // $getItem：非虚实现 —— 不走 vtable。
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

        HookEventDef gDef{"HopperTransferEvent", [] { HopperSetItemHook::hook(); }};
        HookEventDef& hopperDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
