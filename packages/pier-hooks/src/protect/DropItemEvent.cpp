/** hooks/protect/DropItemEvent.cpp: the synthetic, cancellable "PlayerDropItemEvent".
 * The LL event bus has no PlayerDropItemEvent; the only thing of that name is the vanilla
 * mc/world/events/PlayerDropItemEvent.h, which can be neither subscribed nor cancelled. Dropping
 * takes two routes and each gets a hook. Player::drop is the Q key, returns bool, and refusing
 * means returning false without calling origin. ComplexInventoryTransaction::handle is dragging a
 * stack out of an open inventory screen and never touches Player::drop, so hooking only the first
 * blocks the Q key while silently allowing the screen. Cancelling there means returning NoError
 * without calling origin, so the transaction reports as handled while nothing moved. The filter
 * on hook 2, a NormalTransaction with exactly one action from a ContainerInventory, separates
 * throwing a stack on the ground from the other inventory moves flowing through the same virtual.
 * Payload {eventId, x, y, z, dim, item, randomly, viaInventoryUi, _player:{...}}. x, y and z are
 * truncated to integers, because LL reflection serializes a Vec3 as a JSON array. / */
#include "pier/hooks/hook_events.h"

#include <string>

#include "ll/api/memory/Hook.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/world/ContainerID.h"
#include "mc/world/actor/player/Player.h"
// PlayerInventory.h is required. Player.h only forward declares it, and ll::TypedStorage
// collapses unique_ptr<T> to a bare unique_ptr<T>, so player.mInventory really is a
// unique_ptr and .get() needs the pointee complete. Without this include MSVC reports
// C2027 plus a C2039 claiming unique_ptr is missing a member, which reads like the wrong
// number of dereferences. The full collapse rules are in the file header of
// tools/typed-storage.py.
#include "mc/world/actor/player/PlayerInventory.h"
#include "mc/world/inventory/transaction/ComplexInventoryTransaction.h"
#include "mc/world/inventory/transaction/InventoryAction.h"
#include "mc/world/inventory/transaction/InventorySource.h"
#include "mc/world/inventory/transaction/InventorySourceType.h"
#include "mc/world/inventory/transaction/InventoryTransaction.h"
#include "mc/world/inventory/transaction/InventoryTransactionError.h"
#include "mc/world/item/ItemStack.h"

#include "pier/support/log.h"
#include "pier/support/snbt.h"

namespace pier::hooks
{
    namespace
    {
        HookEventDef& dropItemDef(); // Forward declaration

        /** The item type name, or "" when it cannot be read. */
        std::string safeTypeName(::ItemStack const& item)
        {
            try
            {
                return item.isNull() ? std::string{} : item.getTypeName();
            }
            catch (...)
            {
                return {};
            }
        }

        /** Shared payload assembly, so both hooks report the same event shape. */
        std::string buildSnbt(Player& p, std::string const& itemName, bool randomly, bool viaUi)
        {
            auto const& pos = p.getPosition();
            return "{\"eventId\":\"PlayerDropItemEvent\""
                ",\"x\":" + snbtNum(static_cast<int>(pos.x))
                + ",\"y\":" + snbtNum(static_cast<int>(pos.y))
                + ",\"z\":" + snbtNum(static_cast<int>(pos.z))
                + ",\"dim\":" + snbtNum(static_cast<int>(p.getDimensionId()))
                + ",\"randomly\":" + (randomly ? "1" : "0")
                + ",\"viaInventoryUi\":" + (viaUi ? "1" : "0")
                + ",\"item\":\"" + snbtEscape(itemName)
                + "\"," + playerRefSnbt(p) + "}";
        }

        /** Warns once per process when the hooks are installed and no subscriber exists.
         *  Both hooks share one flag, since one subscription arms them together. */
        void warnNoSubscriberOnce(char const* which)
        {
            static bool warned = false;
            if (warned) return;
            warned = true;
            hostLogger().warn(
                "[hooks/DropItemEvent] {} fired with no subscriber at all, def.live() is "
                "false, so the native detour is installed while the other side never "
                "subscribed to 'PlayerDropItemEvent'; check the startup log for a failed "
                "subscription to that event",
                which);
        }

        /** Hook 1: the Q key, dropping the held item. */
        LL_TYPE_INSTANCE_HOOK(
            PlayerDropItemHook,
            ll::memory::HookPriority::Normal,
            Player,
            &Player::$drop,
            bool,
            ::ItemStack const& item,
            bool randomly)
        {
            auto& def = dropItemDef();
            if (!def.live())
            {
                warnNoSubscriberOnce("PlayerDropItemHook");
                return origin(item, randomly);
            }

            if (dispatchHookEventCancellable(
                    def, buildSnbt(*this, safeTypeName(item), randomly, false)))
            {
                // false means the drop did not happen and the stack is still in the
                // container.
                return false;
            }
            return origin(item, randomly);
        }

        /** Hook 2: dragging a stack out of an open inventory screen. */
        LL_TYPE_INSTANCE_HOOK(
            InventoryUiDropHook,
            ll::memory::HookPriority::Normal,
            ComplexInventoryTransaction,
            &ComplexInventoryTransaction::$handle,
            ::InventoryTransactionError,
            ::Player& player,
            bool isSenderAuthority)
        {
            auto& def = dropItemDef();
            if (!def.live())
            {
                warnNoSubscriberOnce("InventoryUiDropHook");
                return origin(player, isSenderAuthority);
            }
            if (mType != ComplexInventoryTransaction::Type::NormalTransaction)
            {
                return origin(player, isSenderAuthority);
            }

            // One action from the player's own inventory is the shape of throwing a stack
            // on the ground. Crafting, moving between containers and taking items in
            // creative each either come from a different source or carry more than one
            // action.
            ::InventorySource source{
                ::InventorySourceType::ContainerInventory,
                ::ContainerID::Inventory,
                ::InventorySource::InventorySourceFlags::NoFlag
            };

            auto const& actions = mTransaction->getActions(source);
            if (actions.size() != 1)
            {
                return origin(player, isSenderAuthority);
            }

            std::string itemName = safeTypeName(
                player.mInventory.get()->getItem(actions[0].mSlot, ::ContainerID::Inventory));

            if (dispatchHookEventCancellable(def, buildSnbt(player, itemName, false, true)))
            {
                // NoError rather than an error code: the client treats the transaction as
                // handled, neither retries nor desyncs, and the item simply never left the
                // inventory.
                return ::InventoryTransactionError::NoError;
            }
            return origin(player, isSenderAuthority);
        }

        HookEventDef gDef{
            "PlayerDropItemEvent",
            []
            {
                // hook() returns the ll::memory::hookEx status, where 0 is success and
                // anything else means the symbol was not found or the patch was refused.
                // It must be visible, because a failed install looks exactly like working
                // normally and a version mismatch can only be diagnosed here.
                int r1 = PlayerDropItemHook::hook();
                int r2 = InventoryUiDropHook::hook();
                auto& log = hostLogger();
                log.debug(
                    "[hooks/DropItemEvent] installing detours: PlayerDropItemHook={} (code={}), "
                    "InventoryUiDropHook={} (code={})",
                    r1 == 0 ? "ok" : "failed", r1,
                    r2 == 0 ? "ok" : "failed", r2
                );
                if (r1 != 0 || r2 != 0)
                {
                    log.error(
                        "[hooks/DropItemEvent] a native detour failed to install with a "
                        "non-zero status. The usual cause is a mismatch between the BDS or "
                        "LeviLamina version this host was linked against and the one the "
                        "server runs, so the symbol address of Player::$drop or "
                        "ComplexInventoryTransaction::$handle resolved wrongly. Drop "
                        "protection is now entirely inactive: items drop as usual and no "
                        "interception is logged. Rebuild this host against the LeviLamina "
                        "and BDS versions the server actually runs.");
                }
                return r1 == 0 && r2 == 0;
            }
        };
        HookEventDef& dropItemDef() { return gDef; }

        HookEventRegistrar gReg{gDef};
    } // namespace
} // namespace pier::hooks
