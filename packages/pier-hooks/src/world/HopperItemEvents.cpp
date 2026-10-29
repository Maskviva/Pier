/** hooks/world/HopperItemEvents.cpp: "HopperSearchItemEvent" and "HopperPushOutEvent".
 * HopperTransferEvent sees a slot after it changed, carries no dimension and cannot refuse,
 * so a hopper pulling from the chest above it or pushing into the chest it faces could only
 * be watched. These two ask before the item moves.
 * The hopper helper class pulls from the container above, pulls from items lying above, and
 * pushes out. Each of the three notes which direction is under way and where the hopper is,
 * and the move of one item, which knows no direction, asks the matching event. Returning false
 * from that move is the engine's own answer of an item that did not move, and the hopper
 * tries again later.
 * Payload {eventId, x, y, z, dim, minecart, fromEntity, item, count, slot, face}: x, y and z
 * are the block coordinates of the hopper, minecart is 1 for a hopper minecart, fromEntity is 1
 * for a dropped item, and face is the argument the engine passes to the move, which for a
 * push-out names the side it pushes toward. `partial` is 1 when a field could not be read. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <cmath>
#include <functional>
#include <optional>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/Container.h"
#include "mc/world/actor/Hopper.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/i18n.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& searchDef();  // Forward declarations
        HookEventDef& pushOutDef();

        enum class Move
        {
            None,
            PullIn,
            PushOut
        };

        /** Which of the two outer functions is running and where the hopper stands. The move
         *  of one item reads them. They are set for the length of the outer call and put back
         *  after it, so a move reached some other way finds none and raises nothing. */
        thread_local Move tlMove = Move::None;
        thread_local int tlX = 0;
        thread_local int tlY = 0;
        thread_local int tlZ = 0;

        int cell(float v) { return static_cast<int>(std::floor(v)); }

        struct MoveScope
        {
            Move saved;

            MoveScope(Move move, ::Vec3 const& at) : saved(tlMove)
            {
                tlMove = move;
                tlX = cell(at.x);
                tlY = cell(at.y);
                tlZ = cell(at.z);
            }

            ~MoveScope() { tlMove = saved; }
        };

        std::string buildSnbt(
            char const* eventId, bool minecart, bool fromEntity, ::BlockSource& region,
            ::ItemStack const& item, int slot, int face)
        {
            std::string name;
            int count = 0;
            bool readFailed = false;
            try
            {
                if (!item.isNull())
                {
                    name = item.getTypeName();
                    count = item.mCount;
                }
            }
            catch (...)
            {
                name.clear();
                readFailed = true;
            }
            return std::string{"{\"eventId\":\""} + eventId + "\""
                + ",\"x\":" + snbtNum(tlX)
                + ",\"y\":" + snbtNum(tlY)
                + ",\"z\":" + snbtNum(tlZ)
                + ",\"dim\":" + snbtNum(static_cast<int>(region.getDimensionId()))
                + ",\"minecart\":" + (minecart ? "1" : "0")
                + ",\"fromEntity\":" + (fromEntity ? "1" : "0")
                + ",\"item\":\"" + snbtEscape(name) + "\""
                + ",\"count\":" + snbtNum(count)
                + ",\"slot\":" + snbtNum(slot)
                + ",\"face\":" + snbtNum(face)
                + (readFailed ? ",\"partial\":1" : "") + "}";
        }

        LL_TYPE_INSTANCE_HOOK(
            HopperPullInHook,
            ll::memory::HookPriority::Normal,
            Hopper,
            &Hopper::_tryPullInItemsFromAboveContainer,
            bool,
            ::BlockSource& region,
            ::Container& toContainer,
            ::Vec3 const& pos)
        {
            MoveScope scope{Move::PullIn, pos};
            return origin(region, toContainer, pos);
        }

        LL_TYPE_INSTANCE_HOOK(
            HopperPullInWorldHook,
            ll::memory::HookPriority::Normal,
            Hopper,
            &Hopper::_tryPullInItemsFromWorld,
            bool,
            ::BlockSource& region,
            ::Container& toContainer,
            ::Vec3 const& pos)
        {
            MoveScope scope{Move::PullIn, pos};
            return origin(region, toContainer, pos);
        }

        LL_TYPE_INSTANCE_HOOK(
            HopperPushOutHook,
            ll::memory::HookPriority::Normal,
            Hopper,
            &Hopper::_pushOutItems,
            bool,
            ::BlockSource& region,
            ::Container& fromContainer,
            ::Vec3 const& position,
            int attachedFace)
        {
            MoveScope scope{Move::PushOut, position};
            return origin(region, fromContainer, position, attachedFace);
        }

        LL_TYPE_INSTANCE_HOOK(
            HopperMoveInHook,
            ll::memory::HookPriority::Normal,
            Hopper,
            &Hopper::_tryMoveInItem,
            bool,
            ::BlockSource& region,
            ::Container& container,
            ::std::optional<::std::reference_wrapper<::ItemActor>> const& itemActor,
            ::ItemStack& item,
            int slot,
            int face,
            int itemCount)
        {
            if (tlMove == Move::None)
            {
                return origin(region, container, itemActor, item, slot, face, itemCount);
            }
            bool const pulling = tlMove == Move::PullIn;
            auto& def = pulling ? searchDef() : pushOutDef();
            if (!def.live())
            {
                return origin(region, container, itemActor, item, slot, face, itemCount);
            }
            std::string const snbt = buildSnbt(
                pulling ? "HopperSearchItemEvent" : "HopperPushOutEvent",
                this->mIsEntity, itemActor.has_value(), region, item, slot, face);
            if (dispatchHookEventCancellable(def, snbt))
            {
                return false;
            }
            return origin(region, container, itemActor, item, slot, face, itemCount);
        }

        /**
         * Installs the four detours both events share, once. A second `hook()` call on a
         * detour is a second registration, so the two events call this one function and the
         * second to be subscribed reuses the first result.
         */
        bool installHopperItemHooks()
        {
            static bool const ok = []
            {
                int const r1 = HopperPullInHook::hook();
                int const r2 = HopperPullInWorldHook::hook();
                int const r3 = HopperPushOutHook::hook();
                int const r4 = HopperMoveInHook::hook();
                if (r1 != 0 || r2 != 0 || r3 != 0 || r4 != 0)
                {
                    hostLogger().error(
                        "{}", pier::trf("hooks.hopper_item_events.1", r1, r2, r3, r4));
                }
                return r1 == 0 && r2 == 0 && r3 == 0 && r4 == 0;
            }();
            return ok;
        }

        HookEventDef gSearchDef{"HopperSearchItemEvent", [] { return installHopperItemHooks(); }};
        HookEventDef& searchDef() { return gSearchDef; }

        HookEventDef gPushOutDef{"HopperPushOutEvent", [] { return installHopperItemHooks(); }};
        HookEventDef& pushOutDef() { return gPushOutDef; }

        HookEventRegistrar gSearchReg{gSearchDef};
        HookEventRegistrar gPushOutReg{gPushOutDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
