//! Block states and block entities.
//!
//! A block state is part of the identity of a block, so changing one makes it a different
//! block, while a block entity is extra data attached to the cell, such as the contents of
//! a chest or the text on a sign. The two are not the same thing.

use super::parse_boxes;
use crate::block::Block;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;
use crate::types::Bounds;

impl Block {
    // Block states

    /// Reads the value of one block state.
    pub fn state(&self, name: &str) -> Result<String> {
        let f = crate::require_slot!(block_get_state, "reading a block state");
        call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, s(name), ctx, sink) })
            .ok_or_else(|| Error(format!("{self} has no block state named {name}")))
    }

    /// Every block state.
    pub fn states(&self) -> Result<NbtValue> {
        let text = self.text(sys::PIER_BSTR_STATE)?;
        NbtValue::parse(&text).map_err(|e| Error(format!("parsing the block state SNBT failed: {e}")))
    }

    pub fn set_state(&self, name: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(block_set_state, "writing a block state");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(name), s(value)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the state {name}={value} of {self} could not be written: the block has no such state, or the value is outside its range"
            )))
        }
    }

    /// The collision box.
    pub fn collision_shape(&self) -> Result<Vec<Bounds>> {
        let f = crate::require_slot!(block_get_collision_shape, "reading the collision box of a block");
        let text =
            call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, ctx, sink) })
                .ok_or_else(|| Error(format!("the collision box of {self} could not be read")))?;
        parse_boxes(&text)
    }

    // Block entities

    /// The NBT of the block entity. A cell with no block entity gives `Ok(None)`.
    pub fn block_entity(&self) -> Result<Option<NbtValue>> {
        let f = crate::require_slot!(block_entity_snbt, "reading a block entity");
        let Some(text) =
            call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, ctx, sink) })
        else {
            return Ok(None);
        };
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("parsing the block entity SNBT failed: {e}")))?;
        Ok(Some(v))
    }

    /// Writes the NBT of the block entity back, through `BlockActor::load`.
    /// The cell already has to hold the matching kind of block.
    pub fn set_block_entity(&self, snbt: &str) -> Result<()> {
        let f = crate::require_slot!(edit_set_block_entity, "writing a block entity");
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, s(snbt)) };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "the block entity of {self} could not be written: the cell does not hold the matching block, or the NBT shape is wrong"
            )))
        }
    }
}
