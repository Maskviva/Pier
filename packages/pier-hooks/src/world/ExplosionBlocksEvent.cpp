/** hooks/world/ExplosionBlocksEvent.cpp: the synthetic, cancellable "ExplosionBlocksEvent".
 * ExplosionEvent is asked before the engine has worked out what a blast reaches, so it can
 * refuse the whole explosion and nothing finer. A land plot wants the other thing: let it
 * bang, and keep the blocks of the plot next door. The engine sends an ExplosionStartedEvent
 * through BlockEventCoordinator::sendEvent after it has the set of blocks and before it reads
 * the set back, which is the one moment the set can still be edited.
 * The Explosion::explode detour only notes which explosion is running, because the started
 * event carries the blocks, the dimension and the source but not the center or the radius.
 * Payload {eventId, x, y, z, dim, radius, sourceIsPlayer, sourceId, source, count, truncated,
 * blocks:[[x,y,z],...]}, with `partial` 1 when the source could not be read. Above 16384 cells
 * `blocks` is empty and `truncated` is 1, and a subscriber that cannot decide should cancel.
 * Reply {keep:[[x,y,z],...]} to take those cells out of the blast, or cancelled to stop it. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <cstddef>
#include <string>
#include <type_traits>
#include <unordered_set>

// cl rejects EventCoordinatorPimpl::_processEvent, an inline member template marked MCAPI
// (dllimport), with C2491; clang-cl, which the host builds with, accepts it. Emptying
// MCAPI for this one header keeps the file building under cl as well, and #pragma once
// stops the headers below from reading it again. A file that takes the address of
// _processEvent cannot do this: the address would be this local copy, not the engine's.
#include "mc/_HeaderOutputPredefine.h"
#include "mc/world/events/EventResult.h"
#pragma push_macro("MCAPI")
#undef MCAPI
#define MCAPI
#include "mc/world/events/EventCoordinatorPimpl.h"
#pragma pop_macro("MCAPI")

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/gameplayhandlers/CoordinatorResult.h"
#include "mc/legacy/ActorUniqueID.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/events/BlockEventCoordinator.h"
#include "mc/world/events/EventRef.h"
#include "mc/world/events/ExplosionStartedEvent.h"
#include "mc/world/events/MutableBlockGameplayEvent.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/Explosion.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& blocksDef(); // Forward declaration

        constexpr std::size_t kMaxListed = 16384;

        /** The explosion whose explode() is running on this thread, or null. */
        thread_local ::Explosion* tlExplosion = nullptr;

        struct ExplosionScope
        {
            ::Explosion* saved;

            explicit ExplosionScope(::Explosion* now) : saved(tlExplosion) { tlExplosion = now; }

            ~ExplosionScope() { tlExplosion = saved; }
        };

        std::string buildSnbt(::Explosion& explosion, ::ExplosionStartedEvent& started)
        {
            int dim = -1;
            std::string sourceType;
            bool sourceIsPlayer = false;
            int64_t sourceId = 0;
            bool readFailed = false;
            try
            {
                ::BlockSource& region = explosion.mRegion;
                dim = static_cast<int>(region.getDimensionId());
                ::ActorUniqueID const& id = explosion.mSourceID.get();
                if (!(id == ::ActorUniqueID::INVALID_ID()))
                {
                    sourceId = id.rawID;
                    if (::Actor* source = region.getLevel().fetchEntity(id, true))
                    {
                        sourceType = source->getTypeName();
                        sourceIsPlayer = source->isPlayer();
                    }
                }
            }
            catch (...)
            {
                sourceType.clear();
                readFailed = true;
            }
            auto const& cells = started.mBlocks.get();
            bool const truncated = cells.size() > kMaxListed;
            std::string list;
            if (!truncated)
            {
                list.reserve(cells.size() * 16);
                for (auto const& cell : cells)
                {
                    if (!list.empty()) list += ",";
                    list += "[" + snbtNum(cell.x) + "," + snbtNum(cell.y) + "," + snbtNum(cell.z) + "]";
                }
            }
            ::Vec3 const& pos = explosion.mPos.get();
            return "{\"eventId\":\"ExplosionBlocksEvent\""
                ",\"x\":" + snbtDouble(pos.x)
                + ",\"y\":" + snbtDouble(pos.y)
                + ",\"z\":" + snbtDouble(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"radius\":" + snbtDouble(explosion.mRadius)
                + ",\"sourceIsPlayer\":" + (sourceIsPlayer ? "1" : "0")
                + ",\"sourceId\":" + snbtNum(sourceId) + "L"
                + ",\"source\":\"" + snbtEscape(sourceType) + "\""
                + ",\"count\":" + snbtNum(cells.size())
                + ",\"truncated\":" + (truncated ? "1" : "0")
                + ",\"blocks\":[" + list + "]"
                + (readFailed ? ",\"partial\":1" : "") + "}";
        }

        /** Asks the subscribers, takes the cells they keep out of the set the engine will
         *  read back, and says whether the whole explosion is cancelled. */
        bool decide(::Explosion& explosion, ::ExplosionStartedEvent& started)
        {
            auto const keep = dispatchHookEventKeeping(blocksDef(), buildSnbt(explosion, started));
            if (keep.cancelled) return true;
            auto& cells = started.mBlocks.get();
            for (auto const& cell : keep.cells)
            {
                cells.erase(::BlockPos{cell[0], cell[1], cell[2]});
            }
            return false;
        }

        LL_TYPE_INSTANCE_HOOK(
            ExplosionScopeHook,
            ll::memory::HookPriority::Normal,
            Explosion,
            &Explosion::explode,
            bool,
            ::IRandom& random)
        {
            if (!blocksDef().live()) return origin(random);
            ExplosionScope scope{this};
            return origin(random);
        }

        // sendEvent has two overloads. The parameter below names the one that carries the
        // events a listener may change, which is the one the explosion goes through.
        LL_TYPE_INSTANCE_HOOK(
            ExplosionStartedHook,
            ll::memory::HookPriority::Normal,
            BlockEventCoordinator,
            &BlockEventCoordinator::sendEvent,
            ::CoordinatorResult,
            ::EventRef<::MutableBlockGameplayEvent<::CoordinatorResult>> event)
        {
            if (!blocksDef().live() || tlExplosion == nullptr) return origin(event);

            bool cancel = false;
            event.get().visit(
                [&](auto& wrapped)
                {
                    using Event = std::remove_cvref_t<decltype(wrapped.value())>;
                    if constexpr (std::is_same_v<Event, ::ExplosionStartedEvent>)
                    {
                        cancel = decide(*tlExplosion, wrapped.value());
                    }
                });
            if (cancel)
            {
                return ::CoordinatorResult::Cancel;
            }
            return origin(event);
        }

        HookEventDef gDef{
            "ExplosionBlocksEvent",
            []
            {
                int const r1 = ExplosionScopeHook::hook();
                int const r2 = ExplosionStartedHook::hook();
                if (r1 != 0 || r2 != 0)
                {
                    hostLogger().error("{}", pier::trf("hooks.explosion_blocks_event.1", r1, r2));
                }
                return r1 == 0 && r2 == 0;
            }
        };
        HookEventDef& blocksDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
