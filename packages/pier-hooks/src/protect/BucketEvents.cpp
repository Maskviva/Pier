/** protect/BucketEvents.cpp: "PlayerUseBucketPlaceEvent" and "PlayerUseBucketTakeEvent",
 * both cancellable.
 * Emptying a bucket puts water or lava into a cell and filling one takes a source out of it.
 * Neither is a block place or a block break, so PlayerPlacingBlockEvent and
 * PlayerDestroyBlockEvent do not see them, and a bucket is the cheapest way to flood a
 * neighbor's plot or drain their pond. BucketItem::_emptyBucket is where the liquid goes in,
 * and BucketItem::_useOn is where a bucket is used on a cell: an empty bucket aimed at a liquid
 * or at powder snow takes it. Both are asked right before. Cancelling a place returns false,
 * the engine's answer of a bucket that did not empty, and cancelling a take returns a refused
 * interaction.
 * Payload {eventId, x, y, z, dim, face, item, block, byPlayer, _player}: the cell, the bucket,
 * and the liquid in it for a place or the block aimed at for a take. `_player` is absent when
 * a dispenser empties the bucket.
 * A field that could not be read leaves the payload with `partial` 1. */
#ifndef PIER_BUILD_CLIENT

#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/BucketItem.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/dimension/DimensionType.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& placeDef(); // Forward declarations
        HookEventDef& takeDef();

        /** The type name of a bucket or of a block, or an empty text when it cannot be read. */
        template <class F>
        std::string nameOf(F&& read, bool& readFailed)
        {
            try
            {
                return read();
            }
            catch (...)
            {
                readFailed = true;
                return {};
            }
        }

        std::string buildSnbt(
            char const* eventId, ::BlockPos const& pos, int dim, int face, std::string const& item,
            std::string const& block, ::Actor* who, bool readFailed)
        {
            bool const byPlayer = who != nullptr && who->isPlayer();
            return std::string{"{\"eventId\":\""} + eventId + "\""
                + ",\"x\":" + snbtNum(pos.x)
                + ",\"y\":" + snbtNum(pos.y)
                + ",\"z\":" + snbtNum(pos.z)
                + ",\"dim\":" + snbtNum(dim)
                + ",\"face\":" + snbtNum(face)
                + ",\"item\":\"" + snbtEscape(item) + "\""
                + ",\"block\":\"" + snbtEscape(block) + "\""
                + ",\"byPlayer\":" + (byPlayer ? "1" : "0")
                + (byPlayer ? "," + playerRefSnbt(*static_cast<::Player*>(who)) : std::string{})
                + (readFailed ? ",\"partial\":1" : "") + "}";
        }

        LL_TYPE_INSTANCE_HOOK(
            BucketEmptyHook,
            ll::memory::HookPriority::Normal,
            BucketItem,
            &BucketItem::_emptyBucket,
            bool,
            ::BlockSource& region,
            ::Block const& contents,
            ::BlockPos const& pos,
            ::Actor* placer,
            ::ItemStack const& instance,
            uchar face)
        {
            auto& def = placeDef();
            if (!def.live()) return origin(region, contents, pos, placer, instance, face);

            bool readFailed = false;
            std::string const item = nameOf([&] { return instance.getTypeName(); }, readFailed);
            std::string const block = nameOf([&] { return contents.getTypeName(); }, readFailed);
            if (dispatchHookEventCancellable(
                    def,
                    buildSnbt(
                        "PlayerUseBucketPlaceEvent", pos, static_cast<int>(region.getDimensionId()),
                        static_cast<int>(face), item, block, placer, readFailed)))
            {
                return false;
            }
            return origin(region, contents, pos, placer, instance, face);
        }

        LL_TYPE_INSTANCE_HOOK(
            BucketUseOnHook,
            ll::memory::HookPriority::Normal,
            BucketItem,
            &BucketItem::$_useOn,
            ::InteractionResult,
            ::ItemStack& instance,
            ::Actor& entity,
            ::BlockPos pos,
            uchar face,
            ::HandSlot handSlot,
            ::Vec3 const& clickPos)
        {
            auto& def = takeDef();
            if (!def.live()) return origin(instance, entity, pos, face, handSlot, clickPos);

            bool readFailed = false;
            std::string const item = nameOf([&] { return instance.getTypeName(); }, readFailed);
            // Only an empty bucket aimed at a liquid, or at powder snow, takes anything. Any
            // other use of a bucket is the place hook's business or nobody's.
            if (readFailed || item != "minecraft:bucket")
            {
                return origin(instance, entity, pos, face, handSlot, clickPos);
            }
            std::string const block = nameOf(
                [&] { return entity.getDimensionBlockSource().getBlock(pos).getTypeName(); },
                readFailed);
            bool const takes = block == "minecraft:water" || block == "minecraft:flowing_water"
                || block == "minecraft:lava" || block == "minecraft:flowing_lava"
                || block == "minecraft:powder_snow";
            if (!takes)
            {
                return origin(instance, entity, pos, face, handSlot, clickPos);
            }
            if (dispatchHookEventCancellable(
                    def,
                    buildSnbt(
                        "PlayerUseBucketTakeEvent", pos, static_cast<int>(entity.getDimensionId()),
                        static_cast<int>(face), item, block, &entity, readFailed)))
            {
                ::InteractionResult refused{};
                refused.mSuccess = false;
                refused.mSwing = false;
                return refused;
            }
            return origin(instance, entity, pos, face, handSlot, clickPos);
        }

        HookEventDef gPlaceDef{"PlayerUseBucketPlaceEvent", [] { return BucketEmptyHook::hook() == 0; }};
        HookEventDef& placeDef() { return gPlaceDef; }

        HookEventDef gTakeDef{"PlayerUseBucketTakeEvent", [] { return BucketUseOnHook::hook() == 0; }};
        HookEventDef& takeDef() { return gTakeDef; }

        HookEventRegistrar gPlaceReg{gPlaceDef};
        HookEventRegistrar gTakeReg{gTakeDef};
    } // namespace
} // namespace pier::hooks

#endif // !PIER_BUILD_CLIENT
