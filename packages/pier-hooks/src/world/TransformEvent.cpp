/** hooks/world/TransformEvent.cpp: the synthetic "ActorTransformEvent", observation only.
 * A villager that turns into a zombie villager, a pig struck by lightning, a piglin in the
 * overworld: the old actor is gone and a new one stands in its place with a new id, so a mod that
 * tracked the first, an owner, a shop villager, a name, has lost it with no event to say so.
 * The engine makes the new actor in ActorFactory::createTransformedActor, which knows the old one
 * and not yet the world, and adds it in Level::addEntity, which knows the world and not the old
 * one. The first notes the old actor and the identity of the new, and the second reports when the
 * actor it adds is that one. It cannot be cancelled: the actor already exists.
 * Payload {eventId, x, y, z, dim, from, fromId, to, toId}: the old actor's type and id, and the new
 * actor's type, id and position. `partial` is 1 when the new actor could not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>
#include <utility>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/ecs/EntityId.h"
#include "mc/deps/ecs/gamerefs_entity/EntityContext.h"
#include "mc/deps/ecs/gamerefs_entity/GameRefsEntity.h"
#include "mc/deps/game_refs/OwnerPtr.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/ActorFactory.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& transformDef(); // Forward declaration

        /** The transformation the engine has begun and not yet added to the world. */
        struct Pending
        {
            bool valid = false;
            ::EntityId id{};
            int64_t fromId = 0;
            std::string fromType;
        };

        thread_local Pending tlPending;

        LL_TYPE_INSTANCE_HOOK(
            TransformCreateHook,
            ll::memory::HookPriority::Normal,
            ActorFactory,
            &ActorFactory::createTransformedActor,
            ::OwnerPtr<::EntityContext>,
            ::ActorDefinitionIdentifier const& identifier,
            ::Actor* from)
        {
            auto context = origin(identifier, from);
            if (transformDef().live() && from != nullptr)
            {
                try
                {
                    tlPending.id = context->mEntity;
                    tlPending.fromId = from->getOrCreateUniqueID().rawID;
                    tlPending.fromType = from->getTypeName();
                    tlPending.valid = true;
                }
                catch (...)
                {
                    tlPending.valid = false;
                }
            }
            return context;
        }

        LL_TYPE_INSTANCE_HOOK(
            TransformAddHook,
            ll::memory::HookPriority::Normal,
            Level,
            &Level::$addEntity,
            ::Actor*,
            ::BlockSource& region,
            ::OwnerPtr<::EntityContext> entity)
        {
            if (!tlPending.valid) return origin(region, std::move(entity));

            bool const matches = entity->mEntity == tlPending.id;
            ::Actor* added = origin(region, std::move(entity));
            if (matches && added != nullptr)
            {
                tlPending.valid = false;
                auto& def = transformDef();
                if (def.live())
                {
                    std::string toType;
                    int64_t toId = 0;
                    bool readFailed = false;
                    try
                    {
                        toType = added->getTypeName();
                        toId = added->getOrCreateUniqueID().rawID;
                    }
                    catch (...)
                    {
                        toType.clear();
                        readFailed = true;
                    }
                    auto const& at = added->getPosition();
                    std::string const snbt = "{\"eventId\":\"ActorTransformEvent\""
                        ",\"x\":" + snbtNum(static_cast<int>(at.x))
                        + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                        + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                        + ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId()))
                        + ",\"from\":\"" + snbtEscape(tlPending.fromType) + "\""
                        + ",\"fromId\":" + snbtNum(tlPending.fromId) + "L"
                        + ",\"to\":\"" + snbtEscape(toType) + "\""
                        + ",\"toId\":" + snbtNum(toId) + "L"
                        + (readFailed ? ",\"partial\":1" : "") + "}";
                    dispatchHookEvent(def, snbt);
                }
            }
            return added;
        }

        HookEventDef gDef{
            "ActorTransformEvent",
            []
            {
                int const r1 = TransformCreateHook::hook();
                int const r2 = TransformAddHook::hook();
                if (r1 != 0 || r2 != 0)
                {
                    hostLogger().error("{}", pier::trf("hooks.transform_event.1", r1, r2));
                }
                return r1 == 0 && r2 == 0;
            }
        };
        HookEventDef& transformDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
