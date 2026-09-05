//! Changing the contents of the world: scanning a region, spawning actors, explosions,
//! particles and pathfinding.
//!
//! What separates these from the level settings in `mod.rs` is that they change things
//! inside the world rather than the switches of the world itself. A large region always
//! goes through [`World::scan_with`].

use core::ffi::c_void;

use crate::block::BlockInfo;
use crate::types::BlockUpdate;
use crate::entity::Entity;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, r_owned, s};
use crate::sys;
use crate::types::Bounds;
use crate::world::{BlockCell, EntityInfo, IndexedScan, PaletteEntry, Scan, World};

impl World {
    // Scanning

    /// Scans a region and collects every block and actor into memory.
    ///
    /// A large region uses [`World::scan_with`] instead, since the memory this function uses
    /// is proportional to the cell count.
    pub fn scan(&self, dim: i32, bounds: Bounds) -> Result<Scan> {
        // The two accumulators have to be two variables: one `Scan` would be mutably borrowed
        // once by each closure, which the borrow checker does not accept.
        let mut blocks: Vec<BlockInfo> = Vec::new();
        let mut entities: Vec<EntityInfo> = Vec::new();
        {
            let mut on_block = |b: BlockInfo| blocks.push(b);
            let mut on_entity = |e: EntityInfo| entities.push(e);
            self.scan_with(dim, bounds, &mut on_block, &mut on_entity)?;
        }
        Ok(Scan { blocks, entities })
    }

    /// A streaming scan: the callback runs once per entry the host sinks and nothing is
    /// accumulated.
    ///
    /// Both callbacks run synchronously during this call, the pointers the host passes become
    /// invalid the moment it returns, and a callback therefore receives an already copied
    /// `String` (contract §3).
    pub fn scan_with(
        &self,
        dim: i32,
        bounds: Bounds,
        on_block: &mut dyn FnMut(BlockInfo),
        on_entity: &mut dyn FnMut(EntityInfo),
    ) -> Result<()> {
        let f = crate::require_slot!(scan_region, "scanning a region");
        let mut ctx = ScanCtx {
            on_block,
            on_entity,
            panicked: false,
        };
        let ok = unsafe {
            f(
                dim,
                bounds.min.0,
                bounds.min.1,
                bounds.min.2,
                bounds.max.0,
                bounds.max.1,
                bounds.max.2,
                (&mut ctx as *mut ScanCtx).cast(),
                scan_block_sink,
                scan_entity_sink,
            )
        };
        if ctx.panicked {
            return Err(Error(
                "a scan callback panicked. It was caught here and the result of this scan is incomplete".to_owned(),
            ));
        }
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "scanning dimension {dim} failed: the level is not ready, or the dimension is unavailable"
            )))
        }
    }

    /// Scans the blocks of a region into a palette plus cells.
    ///
    /// The host serializes each distinct block state once and reports every cell as an
    /// index, so a large region costs a few strings instead of two per cell; this is the
    /// form for copying, saving and diffing regions. Entities are not included; use
    /// [`World::scan`] with the entity half for those. The same 2^24-cell limit applies.
    pub fn scan_indexed(&self, dim: i32, bounds: Bounds) -> Result<IndexedScan> {
        let f = crate::require_slot!(scan_region_indexed, "scanning a region by palette");
        let mut out = IndexedScan::default();
        let ok = unsafe {
            f(
                dim,
                bounds.min.0,
                bounds.min.1,
                bounds.min.2,
                bounds.max.0,
                bounds.max.1,
                bounds.max.2,
                (&mut out as *mut IndexedScan).cast(),
                palette_sink,
                cell_sink,
            )
        };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "scanning dimension {dim} by palette failed: the level is not ready, the dimension is unavailable, or the region exceeds 2^24 cells"
            )))
        }
    }

    // Bulk writes

    /// Fills a box with one block. `spec` is a bare name such as `minecraft:stone` or full
    /// SNBT, resolved once for the whole box. `update_flags` is as in
    /// [`crate::Block::set_nbt`]: `BlockUpdate::NONE` is the fastest and leaves the client to
    /// catch up on the next chunk send. Returns the number of cells written.
    ///
    /// One call replaces a loop over `set_block`, which cost an FFI call, a dimension lookup
    /// and a spec parse per cell.
    pub fn fill_region(&self, dim: i32, bounds: Bounds, spec: &str, update: BlockUpdate) -> Result<u64> {
        let f = crate::require_slot!(edit_fill_region, "filling a region");
        let n = unsafe {
            f(
                dim,
                bounds.min.0,
                bounds.min.1,
                bounds.min.2,
                bounds.max.0,
                bounds.max.1,
                bounds.max.2,
                s(spec),
                update.bits(),
            )
        };
        if n < 0 {
            return Err(Error(format!(
                "filling a region in dimension {dim} with `{spec}` failed: the dimension is not ready, the spec does not resolve, or the box exceeds 2^24 cells"
            )));
        }
        Ok(n as u64)
    }

    /// Writes many cells in one call. Each entry of `palette` is a block spec resolved once,
    /// and each cell names one by index. Returns the number of cells written; a cell whose
    /// index is out of range or whose spec did not resolve is skipped, so a result below
    /// `cells.len()` means some were.
    ///
    /// Pasting an [`IndexedScan`] is `set_blocks(dim, &scan.palette_specs(), &scan.cells, BlockUpdate::NONE)`.
    pub fn set_blocks(
        &self,
        dim: i32,
        palette: &[&str],
        cells: &[BlockCell],
        update: BlockUpdate,
    ) -> Result<u64> {
        let f = crate::require_slot!(edit_set_blocks, "writing many blocks");
        if palette.len() > u32::MAX as usize {
            return Err(Error("the palette has more than 2^32 entries".to_owned()));
        }
        let specs: Vec<sys::PierStr> = palette.iter().map(|p| s(p)).collect();
        // BlockCell and PierBlockCell have the same layout; the copy keeps the public
        // type free of the sys crate.
        let raw: Vec<sys::PierBlockCell> = cells
            .iter()
            .map(|c| sys::PierBlockCell {
                x: c.pos.0,
                y: c.pos.1,
                z: c.pos.2,
                index: c.index,
            })
            .collect();
        let n = unsafe {
            f(
                dim,
                specs.as_ptr(),
                specs.len() as u32,
                raw.as_ptr(),
                raw.len(),
                update.bits(),
            )
        };
        if n < 0 {
            return Err(Error(format!(
                "writing {} blocks in dimension {dim} failed: the dimension is not ready",
                cells.len()
            )));
        }
        Ok(n as u64)
    }

    // Actors and effects

    pub fn spawn_mob(&self, dim: i32, type_name: &str, x: f64, y: f64, z: f64) -> Result<Entity> {
        let f = crate::require_slot!(spawn_mob, "spawning a mob");
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(dim, s(type_name), x, y, z, &mut out) };
        if ok {
            Ok(Entity::from_id(out))
        } else {
            Err(Error(format!(
                "{type_name} could not be spawned: the type name is unrecognized, or dimension {dim} is unavailable"
            )))
        }
    }

    /// Spawns an actor from full NBT, the inverse of [`Entity::snapshot`].
    ///
    /// A given `pos` overrides the `Pos` inside the NBT. The engine allocates a new UniqueID,
    /// so the id from the save is not reused.
    pub fn spawn_entity_nbt(
        &self,
        dim: i32,
        snbt: &str,
        pos: Option<(f64, f64, f64)>,
    ) -> Result<Entity> {
        let f = crate::require_slot!(edit_spawn_entity_nbt, "spawning an actor from NBT");
        let (use_pos, (x, y, z)) = match pos {
            Some(p) => (true, p),
            None => (false, (0.0, 0.0, 0.0)),
        };
        let mut out: sys::PierActorId = 0;
        let ok = unsafe { f(dim, s(snbt), use_pos, x, y, z, &mut out) };
        if ok {
            Ok(Entity::from_id(out))
        } else {
            Err(Error(
                "an actor could not be spawned from NBT: the NBT shape is wrong, or the dimension is unavailable".to_owned(),
            ))
        }
    }

    /// Detonates. A `source` of `None` means there is no source actor.
    #[allow(clippy::too_many_arguments)]
    pub fn explode(
        &self,
        dim: i32,
        x: f64,
        y: f64,
        z: f64,
        radius: f32,
        max_resistance: f32,
        source: Option<Entity>,
        fire: bool,
        breaks_blocks: bool,
        allow_underwater: bool,
    ) -> Result<()> {
        let f = crate::require_slot!(explode, "detonating");
        let ok = unsafe {
            f(
                dim,
                x,
                y,
                z,
                radius,
                max_resistance,
                source.map_or(0, |e| e.id()),
                fire,
                breaks_blocks,
                allow_underwater,
            )
        };
        if ok {
            Ok(())
        } else {
            Err(Error(format!(
                "detonating in dimension {dim} failed: the level is not ready, or the dimension is unavailable"
            )))
        }
    }

    /// A particle visible across the whole dimension. Showing it to one person only uses
    /// [`crate::player::Player::spawn_particle`].
    pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()> {
        let f = crate::require_slot!(spawn_particle, "spawning a particle");
        if unsafe { f(dim, s(effect), x, y, z) } {
            Ok(())
        } else {
            Err(Error(format!(
                "spawning a particle in dimension {dim} failed: the level is not ready"
            )))
        }
    }

    /// Computes a path for an actor to a target cell.
    pub fn find_path(&self, who: Entity, x: i32, y: i32, z: i32) -> Result<NbtValue> {
        let f = crate::require_slot!(level_find_path, "pathfinding");
        let text = call_out_str(|ctx, sink| unsafe { f(who.id(), x, y, z, ctx, sink) })
            .ok_or_else(|| {
                Error(format!(
                    "pathfinding for {who} failed: the actor is gone, or it cannot walk"
                ))
            })?;
        NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the pathfinding result SNBT failed: {e}")))
    }
}

// The two scan sinks

struct ScanCtx<'a> {
    on_block: &'a mut dyn FnMut(BlockInfo),
    on_entity: &'a mut dyn FnMut(EntityInfo),
    /// A callback panicked. A panic crossing `extern "C"` is undefined behavior, so it is
    /// caught in the sink, recorded as one bit, and reported as an `Err` once this call
    /// returns.
    panicked: bool,
}

/// # Safety
/// `ctx` must be a valid `*mut IndexedScan`.
unsafe extern "C" fn palette_sink(ctx: *mut c_void, index: u32, name: sys::PierStr, snbt: sys::PierStr) {
    let out = &mut *ctx.cast::<IndexedScan>();
    // Indices arrive in order of first appearance, so pushing keeps them aligned; a gap
    // would mean a host bug, and it is filled rather than misaligning everything after it.
    while (out.palette.len() as u32) < index {
        out.palette.push(PaletteEntry {
            name: String::new(),
            snbt: String::new(),
        });
    }
    out.palette.push(PaletteEntry {
        name: r_owned(name),
        snbt: r_owned(snbt),
    });
}

/// # Safety
/// `ctx` must be a valid `*mut IndexedScan`.
unsafe extern "C" fn cell_sink(ctx: *mut c_void, x: i32, y: i32, z: i32, index: u32) {
    let out = &mut *ctx.cast::<IndexedScan>();
    out.cells.push(BlockCell {
        pos: (x, y, z),
        index,
    });
}

/// # Safety
/// `ctx` must be a valid `*mut ScanCtx`.
unsafe extern "C" fn scan_block_sink(
    ctx: *mut c_void,
    x: i32,
    y: i32,
    z: i32,
    name: sys::PierStr,
    snbt: sys::PierStr,
) {
    let c = &mut *ctx.cast::<ScanCtx>();
    if c.panicked {
        return;
    }
    let info = BlockInfo {
        pos: (x, y, z),
        name: r_owned(name),
        snbt: r_owned(snbt),
    };
    if std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| (c.on_block)(info))).is_err() {
        c.panicked = true;
    }
}

/// # Safety
/// `ctx` must be a valid `*mut ScanCtx`.
unsafe extern "C" fn scan_entity_sink(
    ctx: *mut c_void,
    x: i32,
    y: i32,
    z: i32,
    type_name: sys::PierStr,
    snbt: sys::PierStr,
) {
    let c = &mut *ctx.cast::<ScanCtx>();
    if c.panicked {
        return;
    }
    let info = EntityInfo {
        cell: (x, y, z),
        type_name: r_owned(type_name),
        snbt: r_owned(snbt),
    };
    if std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| (c.on_entity)(info))).is_err() {
        c.panicked = true;
    }
}
