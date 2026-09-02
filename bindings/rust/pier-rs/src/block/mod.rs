//! Blocks: a cell addressed by a dimension plus a coordinate.
//!
//! # There are two write paths, and the native one is the default
//!
//! [`Block::set`] goes through `set_block`, meaning `BlockSource::setBlock`, and takes a name or
//! full SNBT. [`Block::set_states`] and [`Block::set_nbt`] go through `edit_*` and add a
//! [`BlockUpdate`] letting the caller decide whether to notify neighbors and synchronize the
//! client. Turning both off during a bulk fill is an order of magnitude faster, at the cost of
//! resynchronizing afterwards.
//!
//! # A waterlogged block needs the liquid layer
//!
//! Waterlogging in Bedrock is not a block state but a second block in the same cell: the main layer
//! is the stair and the liquid layer is the water. [`Block::name`] sees only the main layer, so
//! copying and pasting a waterlogged stair loses the water entirely: the main layer is exact and
//! the water is gone. Moving the water with it means reading and writing [`Block::extra`].

mod edit;
mod props;
mod state;

use core::ffi::c_void;

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r_owned, s};
use crate::sys;
use crate::types::{Bounds, PositionI32};

/// What one block cell reads back as.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BlockInfo {
    pub pos: PositionI32,
    /// The type name, such as `"minecraft:redstone_wire"`.
    pub name: String,
    /// The full serialization, `{name, states, version}`.
    pub snbt: String,
}

impl BlockInfo {
    pub fn is_air(&self) -> bool {
        self.name == "minecraft:air"
    }
}

/// One cell in the world.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Block {
    dim: i32,
    x: i32,
    y: i32,
    z: i32,
}

impl Block {
    pub fn at(dim: i32, x: i32, y: i32, z: i32) -> Block {
        Block { dim, x, y, z }
    }

    pub fn at_pos(dim: i32, pos: PositionI32) -> Block {
        Block {
            dim,
            x: pos.0,
            y: pos.1,
            z: pos.2,
        }
    }

    pub fn dimension(&self) -> i32 {
        self.dim
    }

    pub fn position(&self) -> PositionI32 {
        (self.x, self.y, self.z)
    }

    // Reading

    /// The type name and the full SNBT, both from one call.
    pub fn read(&self) -> Result<BlockInfo> {
        let f = crate::require_slot!(get_block, "reading a block");
        let mut out: Option<BlockInfo> = None;
        let ok = unsafe {
            f(
                self.dim,
                self.x,
                self.y,
                self.z,
                (&mut out as *mut Option<BlockInfo>).cast(),
                set_block_info,
            )
        };
        if !ok {
            return Err(Error(format!(
                "{self} could not be read: the level is not ready, or the dimension is unavailable"
            )));
        }
        out.ok_or_else(|| {
            Error(format!(
                "the host reported reading {self} as a success and wrote nothing back"
            ))
        })
    }

    /// Parses the full serialization into an NBT tree. Writing it back unchanged uses
    /// [`Block::set_nbt`], a path that does not go through the parser of this layer.
    pub fn to_nbt(&self) -> Result<NbtValue> {
        let text = self.snbt()?;
        NbtValue::parse(&text).map_err(|e| Error(format!("parsing the block SNBT failed: {e}")))
    }

    /// Reads a `PIER_BPROP_*` numeric property.
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(block_get_num, "reading a numeric block property");
        let mut out = 0.0f64;
        let ok = unsafe { f(self.dim, self.x, self.y, self.z, prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "property {prop} of {self} could not be read: the level is not ready, or the host does not recognize the property number"
            )))
        }
    }

    /// Reads a `PIER_BSTR_*` string property.
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(block_get_str, "reading a string block property");
        call_out_str(|ctx, sink| unsafe { f(self.dim, self.x, self.y, self.z, prop, ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "string property {prop} of {self} could not be read"
                ))
            })
    }

    /// The block tags.
    pub fn tags(&self) -> Result<Vec<String>> {
        crate::item::parse_str_list(&self.text(sys::PIER_BSTR_TAGS)?, "the block tags")
    }

    /// Runs a `PIER_BACT_*` action.
    pub fn act(&self, action: i32, sarg: &str) -> Result<String> {
        let f = crate::require_slot!(block_action, "running a block action");
        call_out_str(|ctx, sink| unsafe {
            f(self.dim, self.x, self.y, self.z, action, s(sarg), ctx, sink)
        })
        .ok_or_else(|| Error(format!("action {action} on {self} failed")))
    }

    pub fn has_tag(&self, tag: &str) -> Result<bool> {
        let out = self.act(sys::PIER_BACT_HAS_TAG, tag)?;
        Ok(out.trim() == "1")
    }

    /// Treats this cell as an item, through `Block::asItemInstance`.
    pub fn as_item(&self) -> Result<crate::item::ItemStack> {
        Ok(crate::item::ItemStack::from_snbt(
            self.act(sys::PIER_BACT_AS_ITEM, "")?,
        ))
    }

    /// Drops one item at this cell.
    pub fn pop_resource(&self, item: &crate::item::ItemStack) -> Result<()> {
        self.act(sys::PIER_BACT_POP_RESOURCE, item.snbt())
            .map(|_| ())
    }
}

impl std::fmt::Display for Block {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "block[{},{},{},{}]", self.dim, self.x, self.y, self.z)
    }
}

/// # Safety
/// `ctx` must be a valid `*mut Option<BlockInfo>`.
unsafe extern "C" fn set_block_info(
    ctx: *mut c_void,
    x: i32,
    y: i32,
    z: i32,
    name: sys::PierStr,
    snbt: sys::PierStr,
) {
    *ctx.cast::<Option<BlockInfo>>() = Some(BlockInfo {
        pos: (x, y, z),
        name: r_owned(name),
        snbt: r_owned(snbt),
    });
}

/// Parses a box list shaped `[{min:[x,y,z],max:[x,y,z]}, ...]`.
///
/// The coordinates are floats in the SNBT, since a collision box is an offset within a
/// cell, while [`Bounds`] is in whole cells, so they are floored to the cell they are in.
/// A caller needing sub-cell precision reads `PIER_BSTR_COLLISION_SHAPE` and parses it
/// itself.
fn parse_boxes(text: &str) -> Result<Vec<Bounds>> {
    if text.trim().is_empty() {
        return Ok(Vec::new());
    }
    let v = NbtValue::parse(text)
        .map_err(|e| Error(format!("parsing the collision box SNBT failed: {e}")))?;
    let Some(items) = v.as_list() else {
        return Err(Error(format!(
            "the collision box is not a list but {}",
            v.type_name()
        )));
    };
    let floor = |t: (f64, f64, f64)| (t.0.floor() as i32, t.1.floor() as i32, t.2.floor() as i32);
    Ok(items
        .iter()
        .filter_map(|b| {
            Some(Bounds {
                min: floor(b.get_vec3("min").ok()?),
                max: floor(b.get_vec3("max").ok()?),
            })
        })
        .collect())
}
