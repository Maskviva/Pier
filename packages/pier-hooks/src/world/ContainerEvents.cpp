/** hooks/world/ContainerEvents.cpp —— "PlayerOpenContainerEvent"：玩家即将
 * 打开箱子 / 熔炉 / 漏斗 / … 时触发，**并且可以取消**。
 *
 * # 为什么这件事要紧
 *
 * 权限模型里有 `open_container` 这个动作，但在此之前没有任何东西喂它：宿
 * 主没有容器钩子，于是一个访客只要不破坏方块，就能走进别人的领地把箱子搬
 * 空。这是整个安全模型上最大的一个洞。
 *
 * # 取消
 *
 * `VanillaServerGameplayEventListener::onEvent` 返回 `EventResult`，而
 * `StopProcessing` 会中止这次打开。这个桥里的合成事件默认只观察
 *（hook_events.h），所以本文件自带一条小小的取消通道：载荷派发给订阅者，
 * 任一订阅者写回含取消旗的应答，这次打开就被拒绝。
 *
 * 这也是它用 `dispatchHookEventCancellable` 而不是普通
 * `dispatchHookEvent` 的原因 —— 后者的写回 sink 按设计是 no-op。
 *
 * 钩点取自 LegacyScriptEngine 的 `onOpenContainer`。
 */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/ecs/WeakEntityRef.h"
#include "mc/server/module/VanillaServerGameplayEventListener.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/events/EventResult.h"
#include "mc/world/events/PlayerOpenContainerEvent.h"
#include "mc/world/level/BlockPos.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& openContainerDef(); // 前向

        LL_TYPE_INSTANCE_HOOK(
            PlayerOpenContainerHook,
            ll::memory::HookPriority::Normal,
            VanillaServerGameplayEventListener,
            &VanillaServerGameplayEventListener::$onEvent,
            ::EventResult,
            ::PlayerOpenContainerEvent const& ev)
        {
            auto& def = openContainerDef();
            if (!def.live())
            {
                return origin(ev);
            }

            // mPlayer 是 WeakEntityRef；我们看的时候它可能已经死了，所以
            // tryUnwrap，不成就安静放行。
            Actor* actor = nullptr;
            auto opt = ev.mPlayer->tryUnwrap<Actor>();
            actor = opt ? &*opt : nullptr;
            if (!actor || !actor->isType(::ActorType::Player))
            {
                return origin(ev);
            }
            auto& p = *static_cast<Player*>(actor);

            auto const& pos = ev.mBlockPos.get();
            std::string snbt = "{\"eventId\":\"PlayerOpenContainerEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(actor->getDimensionId()))
                + ",\"containerType\":" + snbtNum(static_cast<int>(ev.mContainerType))
                + "," + playerRefSnbt(p) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return ::EventResult::StopProcessing;
            }
            return origin(ev);
        }

        HookEventDef gDef{"PlayerOpenContainerEvent", [] { PlayerOpenContainerHook::hook(); }};
        HookEventDef& openContainerDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
