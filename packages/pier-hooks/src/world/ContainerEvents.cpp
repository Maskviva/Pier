/** hooks/world/ContainerEvents.cpp: the synthetic, cancellable
 * "PlayerOpenContainerEvent".
 * The permission model has an open_container action, and the host had no container hook
 * to feed it: a visitor who breaks no block can walk into someone's claim and empty their
 * chests.
 * The hook point is VanillaServerGameplayEventListener::onEvent, which returns an
 * EventResult where StopProcessing aborts the open. A synthetic event in this package
 * observes only by default (see hook_events.h), so this uses
 * dispatchHookEventCancellable rather than dispatchHookEvent, whose write-back sink is a
 * no-op by design. Any subscriber answering with the cancel flag refuses the open. */
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
        HookEventDef& openContainerDef(); // Forward declaration

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

            // mPlayer is a WeakEntityRef and may already be dead when read, so it goes
            // through tryUnwrap and the action passes on failure.
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
