//! Containers: the four on a player, plus the one at a coordinate in the world.
//!
//! On the ABI a container is an owner plus which one, a `PierContainerRef`, and not a
//! pointer. A `Container` value can therefore be kept indefinitely: it resolves again on
//! every call and still points at the right thing after a player leaves and rejoins.
//!
//! # Call [`Container::refresh`] after writing
//!
//! `set_item`, `add_item` and `clear` go through `Container::setItem`, which changes only
//! the server copy and sends no packet. The client keeps rendering what it last received
//! until the player clicks a slot and it resynchronizes passively. One `refresh` after a
//! bulk change pushes the whole container across. It must not be called per slot in a
//! loop, since it pushes the whole container and doing so per slot is a packet storm.

use crate::item::ItemStack;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sel::PlayerSel;
use crate::sys;

/// The kind of a container. The values align with `PierContainerRef::which`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ContainerKind {
    Inventory = 0,
    EnderChest = 1,
    Armor = 2,
    OffHand = 3,
    /// A container at a coordinate in the world: a chest, a hopper, a furnace.
    Block = 4,
}

impl ContainerKind {
    pub fn as_i32(self) -> i32 {
        self as i32
    }

    /// A block container has no single owner and therefore cannot be resynchronized; see
    /// [`Container::refresh`].
    pub fn is_player_owned(self) -> bool {
        !matches!(self, ContainerKind::Block)
    }
}

/// A reference to one container.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Container {
    kind: ContainerKind,
    /// A block container does not need it while the field in `PierContainerRef` always needs
    /// a value, so it is then an empty by-name selector, which resolves to nobody and which
    /// the host never resolves.
    owner: PlayerSel,
    dim: i32,
    x: i32,
    y: i32,
    z: i32,
}

impl Container {
    /// One container on a player. Rarely called directly; the `Player::inventory()` family is
    /// the usual route.
    pub fn of_player(owner: PlayerSel, kind: ContainerKind) -> Container {
        Container {
            kind,
            owner,
            dim: 0,
            x: 0,
            y: 0,
            z: 0,
        }
    }

    /// A block container at a coordinate in the world.
    pub fn block(dim: i32, x: i32, y: i32, z: i32) -> Container {
        Container {
            kind: ContainerKind::Block,
            owner: PlayerSel::Name(String::new()),
            dim,
            x,
            y,
            z,
        }
    }

    pub fn kind(&self) -> ContainerKind {
        self.kind
    }

    /// The coordinate of a block container. A player container gives `None`, since it is not
    /// on a cell.
    pub fn position(&self) -> Option<(i32, i32, i32, i32)> {
        match self.kind {
            ContainerKind::Block => Some((self.dim, self.x, self.y, self.z)),
            _ => None,
        }
    }

    /// Converts into the FFI shape. The `PierStr` inside the returned struct borrows `self`,
    /// so every call site has the shape of constructing it and passing it in immediately.
    fn raw(&self) -> sys::PierContainerRef {
        sys::PierContainerRef {
            which: self.kind.as_i32(),
            player: self.owner.raw(),
            dim: self.dim,
            x: self.x,
            y: self.y,
            z: self.z,
        }
    }

    /// The slot count.
    ///
    /// Failing to resolve, because the player left or that cell holds no container, returns
    /// `Err` and not 0: a 0 would make a caller's `for i in 0..size` run zero times in
    /// silence (contract §5.2).
    pub fn size(&self) -> Result<i32> {
        let f = crate::require_slot!(container_size, "reading the size of a container");
        let mut out = 0i32;
        let ok = unsafe { f(self.raw(), &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!("the container {self} could not be resolved")))
        }
    }

    /// What is in one slot. An empty slot gives the SNBT of the air item and is not an error.
    pub fn item(&self, slot: i32) -> Result<ItemStack> {
        let f = crate::require_slot!(container_get_item, "reading a container slot");
        let snbt = call_out_str(|ctx, sink| unsafe { f(self.raw(), slot, ctx, sink) }).ok_or_else(
            || {
                Error(format!(
                    "slot {slot} of container {self} could not be read: the slot is out of range, or the container could not be resolved"
                ))
            },
        )?;
        Ok(ItemStack::from_snbt(snbt))
    }

    /// Every slot.
    ///
    /// One slot failing to read fails the whole thing rather than being skipped: a listing
    /// missing a few items gives the wrong answer to what is in this chest.
    pub fn items(&self) -> Result<Vec<ItemStack>> {
        // One crossing for the whole container where the host offers it; a host without
        // the slot is read one slot at a time.
        if crate::has_slot!(container_get_items) {
            if let Some(f) = crate::rt::runtime::rt().api.container_get_items {
                let mut out: Vec<ItemStack> = Vec::new();
                let ok = unsafe { f(self.raw(), (&mut out as *mut Vec<ItemStack>).cast(), slot_sink) };
                if ok {
                    return Ok(out);
                }
                return Err(Error(format!(
                    "{self} could not be read: the owner is offline, or the block is not a container"
                )));
            }
        }
        let n = self.size()?;
        let mut out = Vec::with_capacity(n.max(0) as usize);
        for slot in 0..n {
            out.push(self.item(slot)?);
        }
        Ok(out)
    }

    /// Writes one slot. Remember [`Container::refresh`] afterwards.
    pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(container_set_item, "writing a container slot");
        let ok = unsafe { f(self.raw(), slot, s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "slot {slot} of container {self} could not be written: the slot is out of range, the item SNBT is invalid, or the container could not be resolved"
            )))
        }
    }

    /// Puts one in and lets the engine pick the slot.
    ///
    /// A full container is an `Err` and not a silent discard, whose symptom is a player's
    /// items vanishing.
    pub fn add_item(&self, item: &ItemStack) -> Result<()> {
        let f = crate::require_slot!(container_add_item, "adding an item to a container");
        let ok = unsafe { f(self.raw(), s(item.snbt())) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "it could not be put into container {self}: the container is full, the item SNBT is invalid, or the container could not be resolved"
            )))
        }
    }

    /// Takes `count` items out of one slot.
    pub fn remove_item(&self, slot: i32, count: i32) -> Result<()> {
        let f = crate::require_slot!(container_remove_item, "removing an item from a container");
        let ok = unsafe { f(self.raw(), slot, count) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "{count} items could not be removed from slot {slot} of container {self}"
            )))
        }
    }

    pub fn clear(&self) -> Result<()> {
        let f = crate::require_slot!(container_clear, "clearing a container");
        let ok = unsafe { f(self.raw()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!("the container {self} could not be cleared")))
        }
    }

    /// Resends the whole container to its owner.
    ///
    /// A block container returns `Err`: a chest has no single owner to send to and its
    /// viewers are refreshed by the engine's own container transaction path. That is a host
    /// rule and not a choice of this layer.
    pub fn refresh(&self) -> Result<()> {
        let f = crate::require_slot!(container_refresh, "resynchronizing a container");
        if !self.kind.is_player_owned() {
            return Err(Error(
                "a block container cannot be resynchronized: it has no single owner, and its viewers are refreshed by the container transaction path of the engine".to_owned(),
            ));
        }
        let ok = unsafe { f(self.raw()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the container {self} could not be resynchronized; the player has most likely left"
            )))
        }
    }
}

impl std::fmt::Display for Container {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self.kind {
            ContainerKind::Block => {
                write!(f, "block[{},{},{},{}]", self.dim, self.x, self.y, self.z)
            }
            other => write!(f, "{:?}({})", other, self.owner),
        }
    }
}

/// # Safety
/// `ctx` must be a valid `*mut Vec<ItemStack>`. Slots arrive in order, so pushing keeps the
/// index equal to the slot number.
unsafe extern "C" fn slot_sink(ctx: *mut core::ffi::c_void, _slot: i32, item_snbt: sys::PierStr) {
    (*ctx.cast::<Vec<ItemStack>>()).push(ItemStack::from_snbt(crate::rt::ffi::r_owned(item_snbt)));
}
