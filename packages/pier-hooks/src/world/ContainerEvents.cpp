/** hooks/world/ContainerEvents.cpp —— 合成事件 "PlayerOpenContainerEvent"，
 * 可取消。
 *
 * 权限模型里有 open_container 这个动作，但宿主此前没有容器钩子去喂它：访客只要
 * 不破坏方块，就能走进别人的领地把箱子搬空。
 *
 * 钩点是 VanillaServerGameplayEventListener::onEvent，返回 EventResult，
 * StopProcessing 中止这次打开。本包的合成事件默认只观察（见 hook_events.h），
 * 所以这里用 dispatchHookEventCancellable 而不是 dispatchHookEvent：后者的写回
 * sink 按设计是 no-op。任一订阅者写回含取消旗的应答即拒绝这次打开。
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

            // mPlayer 是 WeakEntityRef，取的时候可能已经死了，所以走 tryUnwrap，
            // 不成就放行。
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

        HookEventDef gDef{"PlayerOpenContainerEvent", [] { return PlayerOpenContainerHook::hook() == 0; }};
        HookEventDef& openContainerDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
