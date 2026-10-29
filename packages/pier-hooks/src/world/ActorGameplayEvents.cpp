/** hooks/world/ActorGameplayEvents.cpp: "ActorDestroyBlockEvent" and "ActorPickupItemEvent",
 * both cancellable.
 * A mob taking or breaking a block, an enderman picking one up for instance, and any actor
 * picking an item up off the ground, are not player actions, so no player event sees them, and
 * they are how a plot loses a block or an item to something nobody placed. The engine asks its
 * own listener before each, through the actor event coordinator, and an answer of Cancel stops
 * it. The coordinator function is hooked once for each kind of event it carries here, and the
 * actor is looked up from the event, so one that is already gone is not asked about.
 * Refusing calls origin first, so the listener still hears of the event, and then answers Cancel.
 * Payload of a block {eventId, x, y, z, dim, actor, actorId, block}, with the cell the actor
 * wants to change. Payload of a pickup {eventId, x, y, z, dim, actor, actorId, item, itemId},
 * with the position of the item, and `_player` when a player is the one picking it up.
 * A field that could not be read leaves the payload with `partial` 1. */
#ifndef PIER_BUILD_CLIENT

#if !defined(__clang__)
#error "This file hooks EventCoordinatorPimpl::_processEvent and builds with clang-cl only: cl rejects its declaration (C2491). Configure with: xmake f --toolchain=clang-cl"
#endif

#include "pier/hooks/hook_events.h"

#include <cmath>
#include <string>
#include <type_traits>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/ecs/gamerefs_entity/GameRefsEntity.h"
#include "mc/gameplayhandlers/ActorGameplayHandler.h"
#include "mc/gameplayhandlers/CoordinatorResult.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorType.h"
#include "mc/world/actor/item/ItemActor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/events/ActorBeforeAcquireItemEvent.h"
#include "mc/world/events/ActorEventListener.h"
#include "mc/world/events/ActorGameplayEvent.h"
#include "mc/world/events/ActorGriefingBlockEvent.h"
#include "mc/world/events/EventCoordinatorPimpl.h"
#include "mc/world/events/MutableActorGameplayEvent.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& destroyDef(); // Forward declarations
        HookEventDef& pickupDef();

        int cell(float v) { return static_cast<int>(std::floor(v)); }

        /** The actor and its position, as the tail every payload here shares. */
        std::string actorPart(::Actor& actor)
        {
            std::string type;
            int64_t id = 0;
            try
            {
                type = actor.getTypeName();
                id = actor.getOrCreateUniqueID().rawID;
            }
            catch (...)
            {
                type.clear();
            }
            return ",\"actor\":\"" + snbtEscape(type) + "\",\"actorId\":" + snbtNum(id) + "L";
        }

        /** Asks about one mob changing one block. True means refuse it. */
        bool askAboutBlock(::ActorGriefingBlockEvent const& griefing)
        {
            auto& def = destroyDef();
            auto opt = griefing.mActorContext->tryUnwrap<::Actor>();
            ::Actor* actor = opt ? &*opt : nullptr;
            if (actor == nullptr) return false;

            std::string block;
            bool readFailed = false;
            try
            {
                auto const& blockPtr = griefing.mBlock.get();
                block = blockPtr->getTypeName();
            }
            catch (...)
            {
                block.clear();
                readFailed = true;
            }
            ::Vec3 const& at = griefing.mPos.get();
            std::string const snbt = "{\"eventId\":\"ActorDestroyBlockEvent\""
                ",\"x\":" + snbtNum(cell(at.x))
                + ",\"y\":" + snbtNum(cell(at.y))
                + ",\"z\":" + snbtNum(cell(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(actor->getDimensionId()))
                + actorPart(*actor)
                + ",\"block\":\"" + snbtEscape(block) + "\""
                + (readFailed ? ",\"partial\":1" : "") + "}";
            return dispatchHookEventCancellable(def, snbt);
        }

        /** Asks about one actor picking one item entity up. True means refuse it. */
        bool askAboutPickup(::ActorBeforeAcquireItemEvent const& acquire)
        {
            auto& def = pickupDef();
            ::Actor& actor = acquire.mActor;
            ::Actor& item = acquire.mItem;
            if (!item.isType(::ActorType::ItemEntity)) return false;

            std::string itemName;
            int64_t itemId = 0;
            bool readFailed = false;
            try
            {
                itemName = static_cast<::ItemActor&>(item).item().getTypeName();
                itemId = item.getOrCreateUniqueID().rawID;
            }
            catch (...)
            {
                itemName.clear();
                readFailed = true;
            }
            auto const& at = item.getPosition();
            std::string const snbt = "{\"eventId\":\"ActorPickupItemEvent\""
                ",\"x\":" + snbtNum(cell(at.x))
                + ",\"y\":" + snbtNum(cell(at.y))
                + ",\"z\":" + snbtNum(cell(at.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(actor.getDimensionId()))
                + actorPart(actor)
                + ",\"item\":\"" + snbtEscape(itemName) + "\""
                + ",\"itemId\":" + snbtNum(itemId) + "L"
                + (readFailed ? ",\"partial\":1" : "")
                + (actor.isPlayer() ? "," + playerRefSnbt(static_cast<::Player&>(actor)) : std::string{}) + "}";
            return dispatchHookEventCancellable(def, snbt);
        }

        // _processEvent is a template that the engine instantiates once per kind of event
        // list. The parameter types below pick the instantiation that carries the events a
        // listener may answer with Cancel, which holds the griefing event.
        LL_TYPE_INSTANCE_HOOK(
            ActorGriefingHook,
            ll::memory::HookPriority::Normal,
            EventCoordinatorPimpl<ActorEventListener>,
            &EventCoordinatorPimpl<ActorEventListener>::_processEvent,
            ::CoordinatorResult,
            ::ActorGameplayHandler* handler,
            ::ActorGameplayEvent<::CoordinatorResult> const& event)
        {
            if (!destroyDef().live()) return origin(handler, event);

            bool refuse = false;
            event.visit(
                [&](auto&& arg)
                {
                    using Wrapped = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<Wrapped, ::Details::ValueOrRef<::ActorGriefingBlockEvent const>>)
                    {
                        refuse = askAboutBlock(arg.value());
                    }
                });
            if (refuse)
            {
                origin(handler, event);
                return ::CoordinatorResult::Cancel;
            }
            return origin(handler, event);
        }

        // The same function for the list whose events can be changed, which holds the event
        // that precedes an actor acquiring an item.
        LL_TYPE_INSTANCE_HOOK(
            ActorAcquireHook,
            ll::memory::HookPriority::Normal,
            EventCoordinatorPimpl<ActorEventListener>,
            &EventCoordinatorPimpl<ActorEventListener>::_processEvent,
            ::CoordinatorResult,
            ::ActorGameplayHandler* handler,
            ::MutableActorGameplayEvent<::CoordinatorResult>& event)
        {
            if (!pickupDef().live()) return origin(handler, event);

            bool refuse = false;
            event.visit(
                [&](auto&& arg)
                {
                    using Wrapped = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<Wrapped, ::Details::ValueOrRef<::ActorBeforeAcquireItemEvent const>>)
                    {
                        refuse = askAboutPickup(arg.value());
                    }
                });
            if (refuse)
            {
                origin(handler, event);
                return ::CoordinatorResult::Cancel;
            }
            return origin(handler, event);
        }

        HookEventDef gDestroyDef{"ActorDestroyBlockEvent", [] { return ActorGriefingHook::hook() == 0; }};
        HookEventDef& destroyDef() { return gDestroyDef; }

        HookEventDef gPickupDef{"ActorPickupItemEvent", [] { return ActorAcquireHook::hook() == 0; }};
        HookEventDef& pickupDef() { return gPickupDef; }

        HookEventRegistrar gDestroyReg{gDestroyDef};
        HookEventRegistrar gPickupReg{gPickupDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
