//! The command assembly layer: `/fill` and `/tickingarea`.
//!
//! These are not ABI slots: they assemble a command and hand it to
//! `Host::execute_command`. They live here rather than being assembled by a caller because
//! the places one gets wrong, the dimension selector, the volume cap and the name character
//! set, all have definite failure modes whose symptoms are far from their causes.

use crate::rt::error::{Error, Result};
use crate::types::PositionI32;
use crate::world::World;

/// A cuboid in whole cells, as `(min, max)`.
pub type Box3D = (PositionI32, PositionI32);

/// The volume cap of one `/fill` command.
///
/// The engine's own cap is 32768 cells and exceeding it fails the whole command: not a few
/// cells short, but not one cell filled.
pub const MAX_FILL_VOLUME: i64 = 32_768;

/// Cuts a cuboid along y into slices, each within [`MAX_FILL_VOLUME`].
///
/// Along y rather than along the longest edge: the cost of a `/fill` lies mostly in how
/// many chunks it spans, and the cells of one y column are necessarily in the same chunk.
pub fn split_box(from: PositionI32, to: PositionI32) -> Vec<Box3D> {
    let (x0, x1) = (from.0.min(to.0), from.0.max(to.0));
    let (y0, y1) = (from.1.min(to.1), from.1.max(to.1));
    let (z0, z1) = (from.2.min(to.2), from.2.max(to.2));
    let area = (x1 - x0 + 1) as i64 * (z1 - z0 + 1) as i64;
    if area <= 0 {
        return Vec::new();
    }
    // When one layer alone already exceeds the cap, per is clamped to 1, so each command
    // covers one layer, may still exceed it, and the engine refuses it itself. This does
    // not pretend to be able to cut it up on its behalf.
    let per = (MAX_FILL_VOLUME / area).max(1);
    let mut out = Vec::new();
    let mut y = y0 as i64;
    while y <= y1 as i64 {
        let top = (y + per - 1).min(y1 as i64);
        out.push(((x0, y as i32, z0), (x1, top as i32, z1)));
        y = top + 1;
    }
    out
}

/// Whether the name of a ticking area is valid.
///
/// The engine accepts `A-Z a-z 0-9 _` only. A name containing a space or a non-ASCII
/// character makes `/tickingarea add` parse the name as the next argument, and the error
/// it reports has nothing to do with the name.
pub fn is_valid_ticking_area_name(name: &str) -> bool {
    !name.is_empty() && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_')
}

impl World {
    /// Fills a region with `/fill`, cut automatically into slices within the volume cap.
    ///
    /// It returns how many commands ran. A failure partway stops there and returns `Err`
    /// rather than continuing, since continuing gives a half-filled region and the return
    /// value does not say how far it got.
    pub fn fill_blocks(
        &self,
        dim: i32,
        from: PositionI32,
        to: PositionI32,
        block: &str,
    ) -> Result<usize> {
        let sel = dim_sel(dim)?;
        let host = crate::Host::get();
        let mut n = 0;
        for (a, b) in split_box(from, to) {
            let out = host.execute_command(&format!(
                "execute in {sel} run fill {} {} {} {} {} {} {block}",
                a.0, a.1, a.2, b.0, b.1, b.2
            ))?;
            n += 1;
            // An Ok from execute_command means the command ran and not that it succeeded.
            if out.contains("Syntax error") || out.contains("Unknown command") {
                return Err(Error(format!(
                    "fill number {n} was refused by the engine; is the block name {block} unrecognized? {out}"
                )));
            }
        }
        Ok(n)
    }

    /// Creates a ticking area.
    ///
    /// A ticking area belongs to the save, survives a restart and belongs to no mod, so it is
    /// not removed automatically when a mod unloads and needs
    /// [`World::remove_ticking_area`].
    pub fn add_ticking_area(
        &self,
        dim: i32,
        from: (i32, i32),
        to: (i32, i32),
        name: &str,
    ) -> Result<()> {
        if !is_valid_ticking_area_name(name) {
            return Err(Error(format!(
                "the ticking area name `{name}` is invalid: only A-Z, a-z, 0-9 and underscore are allowed"
            )));
        }
        let sel = dim_sel(dim)?;
        let out = crate::Host::get().execute_command(&format!(
            "execute in {sel} run tickingarea add {} 0 {} {} 0 {} {name}",
            from.0, from.1, to.0, to.1
        ))?;
        if out.contains("error") || out.contains("Unknown") {
            return Err(Error(format!(
                "the ticking area `{name}` could not be created: {out}"
            )));
        }
        Ok(())
    }

    pub fn remove_ticking_area(&self, dim: i32, name: &str) -> Result<()> {
        let sel = dim_sel(dim)?;
        let out = crate::Host::get()
            .execute_command(&format!("execute in {sel} run tickingarea remove {name}"))?;
        if out.contains("error") || out.contains("Unknown") {
            return Err(Error(format!(
                "the ticking area `{name}` could not be removed: {out}"
            )));
        }
        Ok(())
    }

    /// Lists the ticking area names of one dimension.
    ///
    /// The engine output is prose meant for a human, split here on commas and whitespace. The
    /// format follows the version, so failing to split returns an empty table rather than an
    /// error, and the raw output is in the log.
    pub fn list_ticking_areas(&self, dim: i32) -> Result<Vec<String>> {
        let sel = dim_sel(dim)?;
        let out = crate::Host::get()
            .execute_command(&format!("execute in {sel} run tickingarea list"))?;
        let names: Vec<String> = out
            .split([',', '\n', ' ', '\t'])
            .map(|s| s.trim())
            .filter(|s| is_valid_ticking_area_name(s))
            .map(|s| s.to_owned())
            .collect();
        if names.is_empty() && !out.trim().is_empty() {
            crate::Logger::get().warn(&format!(
                "not one name could be split out of the tickingarea list output; the engine output format may have changed: {out}"
            ));
        }
        Ok(names)
    }
}

fn dim_sel(dim: i32) -> Result<String> {
    crate::sel::dimension_selector(dim).ok_or_else(|| {
        Error(format!(
            "dimension {dim} has no command selector: the id is negative, or it was never registered (see dimensions::list)"
        ))
    })
}
