//! Inventory, equipment, held item and cooldown.
//!
//! The container accessors only build a [`crate::Container`] value and do not cross the
//! ABI; the real read or write happens when it is used.

use crate::container::{Container, ContainerKind};
use crate::item::ItemStack;
use crate::nbt::NbtValue;
use crate::player::Player;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};

impl Player {
    // Inventory

    pub fn inventory(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::Inventory)
    }
    pub fn ender_chest(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::EnderChest)
    }
    pub fn armor(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::Armor)
    }
    pub fn offhand_container(&self) -> Container {
        Container::of_player(self.sel.clone(), ContainerKind::OffHand)
    }

    /// The item in the off hand. An empty hand gives the air item and is not an error.
    pub fn offhand(&self) -> Result<ItemStack> {
        self.offhand_container().item(0)
    }

    /// Writes the off hand. Remember [`Container::refresh`] afterwards, otherwise the client
    /// keeps showing the old item.
    pub fn set_offhand(&self, item: &ItemStack) -> Result<()> {
        self.offhand_container().set_item(0, item)
    }

    /// The item in hand.
    pub fn carried_item(&self) -> Result<ItemStack> {
        let f = crate::require_slot!(player_get_carried_item, "reading the held item of a player");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), ctx, sink) })
            .ok_or_else(|| Error(format!("what player {} holds could not be read", self.sel)))?;
        Ok(ItemStack::from_snbt(snbt))
    }

    /// One inventory slot.
    pub fn item(&self, slot: i32) -> Result<ItemStack> {
        let f = crate::require_slot!(player_get_item, "reading an inventory slot of a player");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), slot, ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "inventory slot {slot} of player {} could not be read",
                    self.sel
                ))
            })?;
        Ok(ItemStack::from_snbt(snbt))
    }

    pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(player_set_item, "writing an inventory slot of a player");
        let ok = unsafe { f(self.sel.raw(), slot, s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "inventory slot {slot} of player {} could not be written: they are offline, the slot is out of range, or the item SNBT is invalid",
                self.sel
            )))
        }
    }

    /// The full equipment set. For the `slot` numbering see [`crate::types::EquipSlot`].
    pub fn equipment(&self) -> Result<Vec<(i32, ItemStack)>> {
        let f = crate::require_slot!(player_get_equipment, "reading the equipment of a player");
        let text =
            call_out_str(|ctx, sink| unsafe { f(self.sel.raw(), ctx, sink) }).ok_or_else(|| {
                Error(format!(
                    "the equipment of player {} could not be read",
                    self.sel
                ))
            })?;
        let v = NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the equipment SNBT failed: {e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!(
                "the equipment is not a list but {}",
                v.type_name()
            )));
        };
        Ok(items
            .iter()
            .filter_map(|e| {
                let slot = e.opt_i32("slot")?;
                let snbt = e.opt_str("item_snbt")?;
                Some((slot, ItemStack::from_snbt(snbt)))
            })
            .collect())
    }

    /// How many ticks of cooldown one item has left.
    ///
    /// On the ABI a -1 means both not on cooldown and the player being offline. It is handed
    /// over unchanged and stated rather than guessed at on the caller's behalf
    /// (contract §5.2). Telling them apart starts with
    /// [`Player::is_online`].
    pub fn cooldown(&self, item_name: &str) -> Result<i32> {
        let f = crate::require_slot!(player_get_cooldown, "reading an item cooldown");
        Ok(unsafe { f(self.sel.raw(), s(item_name)) })
    }

    pub fn start_cooldown(&self, item_name: &str, ticks: i32) -> Result<()> {
        let f = crate::require_slot!(player_start_cooldown, "starting an item cooldown");
        let ok = unsafe { f(self.sel.raw(), s(item_name), ticks) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the {item_name} cooldown of player {} could not be started: they are offline, or the item name is unrecognized",
                self.sel
            )))
        }
    }
}
