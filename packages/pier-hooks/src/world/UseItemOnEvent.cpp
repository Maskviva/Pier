/** hooks/world/UseItemOnEvent.cpp: the synthetic, cancellable "PlayerUseItemOnEvent".
 * Spawn eggs, buckets, flint and steel, ender pearls and item-driven block placement all
 * reach the world through GameMode::useItemOn, so protection watching only interact and
 * place events lets every one of them through with nothing logged. The dimension rule
 * layer deliberately does not block them either: Spawner::spawnMob allows a spawn whose
 * source is neither natural nor a spawner, because placing a mob on purpose in a creative
 * world is ordinary play. That judgement holds as a world rule and does not hold as a
 * permission check, which needs to know whose claim this is, so this only reports and the
 * mod decides.
 * Cancelling returns an InteractionResult with both bits cleared: no block placed, no mob
 * spawned, no bucket emptied, and the item is not consumed. Payload
 * {eventId, x, y, z, dim, face, item, isFirstEvent, _player:{...}}. x, y and z are
 * truncated to integers, because LL reflection serializes a BlockPos as a JSON array.
 * isFirstEvent passes through verbatim: holding right-click repeats this call many times
 * per second and a subscriber uses it to tell the first from the repeats. */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/gamemode/GameMode.h"
#include "mc/world/gamemode/InteractionResult.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/Block.h"

#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& useItemOnDef(); // Forward declaration

        LL_TYPE_INSTANCE_HOOK(
            PlayerUseItemOnHook,
            ll::memory::HookPriority::Normal,
            GameMode,
            &GameMode::$useItemOn,
            ::InteractionResult,
            ::ItemStack& item,
            ::BlockPos const& at,
            uchar face,
            ::Vec3 const& hit,
            ::Block const* targetBlock,
            bool isFirstEvent)
        {
            auto& def = useItemOnDef();
            if (!def.live())
            {
                return origin(item, at, face, hit, targetBlock, isFirstEvent);
            }

            // mPlayer is a TypedStorage<8, 8, Player&> and not a wrapper: TypedStorageType
            // has collapse specializations for references and scalars, and only a class
            // type by value stays wrapped. Writing .get() here is a compile error. The full
            // rule table is in the file header of tools/typed-storage.py, which is the
            // single source, and the script reads the engine headers to check each call
            // site.
            Player& p = this->mPlayer;

            std::string itemName = item.getTypeName();

            std::string snbt = "{\"eventId\":\"PlayerUseItemOnEvent\""
                ",\"x\":" + snbtNum(at.x)
                + ",\"y\":" + snbtNum(at.y)
                + ",\"z\":" + snbtNum(at.z)
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"face\":" + snbtNum(static_cast<int>(face))
                + ",\"isFirstEvent\":" + (isFirstEvent ? "1" : "0")
                + ",\"item\":\"" + snbtEscape(itemName)
                + "\"," + playerRefSnbt(p) + "}";

            if (dispatchHookEventCancellable(def, snbt))
            {
                // Both bits cleared: the use did not happen and the client plays no swing
                // animation.
                ::InteractionResult refused{};
                refused.mSuccess = false;
                refused.mSwing = false;
                return refused;
            }
            return origin(item, at, face, hit, targetBlock, isFirstEvent);
        }

        HookEventDef gDef{"PlayerUseItemOnEvent", [] { return PlayerUseItemOnHook::hook() == 0; }};
        HookEventDef& useItemOnDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
