/** hooks/world/ActorChangeDimensionEvent.cpp: the synthetic, cancellable
 * "ActorChangeDimensionEvent".
 * PlayerChangeDimensionEvent is about a player. A dropped item, a mob, a minecart or a boat
 * going through a portal is not, and it is how a plot loses its livestock to the nether or
 * gets a stranger's mob dropped into it. ActorDimensionTransferManager::canChangeDimension is
 * the engine's question of whether an actor may make the trip, so the hook asks right after the
 * engine has said yes, and false is the engine's own answer of no. Players are not asked about
 * here, because their transfer has an event of its own with a redirect.
 * Payload {eventId, x, y, z, dim, from, to, actor, actorId}: the actor and where it stands now,
 * `from` the dimension it is in and `to` the one it is headed for.
 * A field that could not be read leaves the payload with `partial` 1. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/level/ActorDimensionTransferManager.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& changeDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            ActorCanChangeDimensionHook,
            ll::memory::HookPriority::Normal,
            ActorDimensionTransferManager,
            &ActorDimensionTransferManager::canChangeDimension,
            bool,
            ::Actor const& actor,
            ::DimensionType toId)
        {
            bool const allowed = origin(actor, toId);
            auto& def = changeDef();
            if (!allowed || !def.live() || actor.isPlayer()) return allowed;

            std::string type;
            int64_t id = 0;
            bool readFailed = false;
            try
            {
                type = const_cast<::Actor&>(actor).getTypeName();
                id = const_cast<::Actor&>(actor).getOrCreateUniqueID().rawID;
            }
            catch (...)
            {
                type.clear();
                readFailed = true;
            }
            int const from = static_cast<int>(actor.getDimensionId());
            auto const& at = actor.getPosition();
            std::string const snbt = "{\"eventId\":\"ActorChangeDimensionEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(at.x))
                + ",\"y\":" + snbtNum(static_cast<int>(at.y))
                + ",\"z\":" + snbtNum(static_cast<int>(at.z))
                + ",\"dim\":" + snbtNum(from)
                + ",\"from\":" + snbtNum(from)
                + ",\"to\":" + snbtNum(static_cast<int>(toId))
                + ",\"actor\":\"" + snbtEscape(type) + "\""
                + ",\"actorId\":" + snbtNum(id) + "L"
                + (readFailed ? ",\"partial\":1" : "") + "}";

            return !dispatchHookEventCancellable(def, snbt);
        }

        HookEventDef gDef{
            "ActorChangeDimensionEvent", [] { return ActorCanChangeDimensionHook::hook() == 0; }};
        HookEventDef& changeDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
