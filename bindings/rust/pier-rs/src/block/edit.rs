//! Writing blocks, including the liquid layer.
//!
//! Two write paths: `set` goes through `set_block` while `set_nbt` and `set_states` go
//! through `edit_*` and let the caller decide the update flags. The liquid layer is a second
//! block in the same cell and not a state.

use crate::block::Block;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::types::BlockUpdate;

impl Block {
    // Writing

    /// Places a block. `spec` is a name such as `"minecraft:stone"`, or full SNBT.
    ///
    /// An unrecognized name fails and no placeholder block is put down, whose symptom would
    /// be a patch of purple-and-black in the world with no visible origin.
    pub fn set(&self, spec: &str) -> Result<()> {
        let f = crate::require_slot!(set_block, "placing a block");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(spec)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "{spec} could not be placed at {self}: the name is unrecognized, or the level is not ready"
            )))
        }
    }

    /// Places a block from full NBT, deciding the update flags yourself.
    pub fn set_nbt(&self, snbt: &str, update: BlockUpdate) -> Result<()> {
        let f = crate::require_slot!(edit_set_block_nbt, "placing a block from NBT");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(snbt), update.bits()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "a block could not be placed at {self} from NBT: the NBT shape is wrong, or the name is unrecognized"
            )))
        }
    }

    /// Places a block by name plus a subset of its states.
    ///
    /// A `states` of `None` means every state at its default. The host takes the version
    /// number from the default states and a caller must not fill it in: a wrong version number
    /// lands the block under a different set of state meanings.
    pub fn set_states(self, name: &str, states: Option<&str>, update: BlockUpdate) -> Result<()> {
        let f = crate::require_slot!(edit_set_block_states, "placing a block by states");
        let ok = unsafe {
            f(
                self.dim,
                self.x,
                self.y,
                self.z,
                s(name),
                s(states.unwrap_or("")),
                update.bits(),
            )
        };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "{name} could not be placed at {self} by states: the name is unrecognized, or a state is outside its value range"
            )))
        }
    }

    // The liquid layer

    /// Reads the liquid layer. An empty one reads back as `"minecraft:air"` and is not an
    /// error.
    pub fn extra(&self) -> Result<String> {
        let f = crate::require_slot!(get_extra_block, "reading the liquid layer");
        call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, ctx, sink) })
            .ok_or_else(|| Error(format!("the liquid layer of {self} could not be read")))
    }

    /// Writes the liquid layer. Writing `"minecraft:air"` clears it.
    pub fn set_extra(&self, spec: &str, update: BlockUpdate) -> Result<()> {
        let f = crate::require_slot!(set_extra_block, "writing the liquid layer");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(spec), update.bits()) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the liquid layer of {self} could not be written: the name {spec} is unrecognized"
            )))
        }
    }

    /// The container at this cell, a chest or a hopper. It does not check whether one is
    /// really there, since checking would cross the ABI once, and the returned
    /// [`crate::container::Container`] reports it naturally the first time it is used.
    pub fn container(&self) -> crate::container::Container {
        crate::container::Container::block(self.dim, self.x, self.y, self.z)
    }
}
