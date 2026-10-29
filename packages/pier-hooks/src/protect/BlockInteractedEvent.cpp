/** protect/BlockInteractedEvent.cpp: the synthetic, cancellable "BlockInteractedEvent".
 * A right click on a block that does something, a door, a button, a lever, a chest, a crafting
 * table, a bed, is decided inside Block::use, below the item the player holds. It is what a
 * plot rule that forbids visitors to touch anything needs, and PlayerUseItemOnEvent cannot
 * give it: that one is about using an item, and a bare hand on a lever is not.
 * Block::use runs for every block the game asks, so the hook asks only for blocks the engine
 * marks interactive or crafting. Returning false is the engine's own answer of a block that
 * did nothing.
 * Payload {eventId, x, y, z, dim, face, block, _player}. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <optional>
#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/item/HandSlot.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& blockInteractedDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            BlockUseHook,
            ll::memory::HookPriority::Normal,
            Block,
            &Block::use,
            bool,
            ::Player& player,
            ::BlockPos const& pos,
            uchar face,
            ::HandSlot handSlot,
            ::std::optional<::Vec3> hit)
        {
            auto& def = blockInteractedDef();
            if (!def.live()) return origin(player, pos, face, handSlot, hit);

            bool interactive = false;
            std::string name;
            try
            {
                interactive = this->mBlockType->isInteractiveBlock() || this->mBlockType->isCraftingBlock();
                if (interactive) name = this->getTypeName();
            }
            catch (...)
            {
                // A block that cannot be read is asked about by an empty name, which is
                // the direction a protection fails in.
                interactive = true;
                name.clear();
            }
            if (!interactive) return origin(player, pos, face, handSlot, hit);

            std::string const snbt = "{\"eventId\":\"BlockInteractedEvent\""
                ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(player.getDimensionId()))
                + ",\"face\":" + snbtNum(static_cast<int>(face))
                + ",\"block\":\"" + snbtEscape(name) + "\""
                + "," + playerRefSnbt(player) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                return false;
            }
            return origin(player, pos, face, handSlot, hit);
        }

        HookEventDef gDef{"BlockInteractedEvent", [] { return BlockUseHook::hook() == 0; }};
        HookEventDef& blockInteractedDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
