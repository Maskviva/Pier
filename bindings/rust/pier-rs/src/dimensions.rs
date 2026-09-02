//! Custom dimensions: the facade of the optional `pier-dimensions` capability package.
//!
//! When it is not built into the host the whole family of slots is NULL,
//! [`is_available`] returns false and every other call returns an `Err` saying the host
//! does not provide it. That is rule 3 of contract §1 at runtime: the optional package is
//! absent, the layout is unchanged, and the slots are empty.
//!
//! # Registration is idempotent, so register unconditionally at startup
//!
//! [`add_simple`] and [`add_plot`] return the same persisted id for the same name on the
//! next startup, so the right usage is registering directly at startup rather than probing
//! with [`dimension_id`] first, which necessarily misses on the first startup.

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{collect_strs, s};

/// The terrain generator. The values are the engine's `GeneratorType`.
///
/// Note it starts at 1 and not 0: numbering from 0 would make superflat generate a nether.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GeneratorType {
    Overworld = 1,
    Flat = 2,
    Nether = 3,
    TheEnd = 4,
    Void = 5,
}

impl GeneratorType {
    pub fn from_i32(v: i32) -> Option<GeneratorType> {
        Some(match v {
            1 => GeneratorType::Overworld,
            2 => GeneratorType::Flat,
            3 => GeneratorType::Nether,
            4 => GeneratorType::TheEnd,
            5 => GeneratorType::Void,
            _ => return None,
        })
    }

    pub fn as_i32(self) -> i32 {
        self as i32
    }

    /// What the engine itself calls this generator.
    ///
    /// Not the same thing as the enum name: the enum name belongs to this layer while this is
    /// the string the engine recognizes, appearing in generation parameters and in the save.
    /// Use it when assembling something for the engine, not `{:?}`.
    pub fn engine_name(self) -> &'static str {
        match self {
            GeneratorType::Overworld => "Overworld",
            GeneratorType::Flat => "Flat",
            GeneratorType::Nether => "Nether",
            GeneratorType::TheEnd => "TheEnd",
            GeneratorType::Void => "Void",
        }
    }
}

/// Per-dimension rules. The values correspond to `PIER_DIMRULE_*`.
///
/// Why not a game rule: a Bedrock game rule applies to the whole server, so turning
/// `doMobSpawning` off for a creative plot world turns it off for the survival world too.
/// These flags are checked at the real call sites, `Spawner::spawnMob`, `Level::explode`
/// and others, so they really are per dimension.
///
/// A dimension that was never registered is entirely unaffected: the hook falls straight
/// through to the vanilla implementation and a caller need not allow vanilla dimensions
/// explicitly.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DimensionRule {
    SpawnMonster = 0,
    SpawnAnimal = 1,
    SpawnSpawner = 2,
    ExplodeBlocks = 3,
    FireSpread = 4,
    MobGriefing = 5,
    Projectile = 6,
    PistonPush = 7,
    LiquidFlow = 8,
    FarmlandDecay = 9,
    Ride = 10,
    /// Blocks only a piston push crossing a plot boundary, leaving the plot interior alone. It
    /// applies together with [`DimensionRule::PistonPush`] and either one forbidding stops
    /// the push.
    PistonCrossPlot = 11,
    /// Blocks only actor movement crossing a plot boundary. Players and ridden vehicles are
    /// never restricted.
    EntityCrossPlot = 12,
}

impl DimensionRule {
    pub fn as_i32(self) -> i32 {
        self as i32
    }
}

/// The grid layout of a plot world.
///
/// The grid convention, which the SDK and the host must share. With
/// `cell = plot_size + road_width`, a world coordinate `(x, z)` is road when
/// `mod(x,cell) >= plot_size || mod(z,cell) >= plot_size`; otherwise it is border within
/// `border_width` of the plot edge; otherwise it is plot.
#[derive(Debug, Clone, PartialEq)]
pub struct PlotLayout {
    pub plot_size: i32,
    pub road_width: i32,
    pub border_width: i32,
    pub floor_y: i32,
    pub floor_block: String,
    pub fill_block: String,
    pub road_block: String,
    pub border_block: String,
    pub biome: String,
}

impl Default for PlotLayout {
    fn default() -> PlotLayout {
        PlotLayout {
            plot_size: 64,
            road_width: 7,
            border_width: 1,
            floor_y: 64,
            floor_block: "minecraft:grass_block".to_owned(),
            fill_block: "minecraft:dirt".to_owned(),
            road_block: "minecraft:birch_planks".to_owned(),
            border_block: "minecraft:stone_block_slab".to_owned(),
            biome: "minecraft:plains".to_owned(),
        }
    }
}

impl PlotLayout {
    /// The width of one plot plus one road, which is the modulus of the grid.
    pub fn cell_size(&self) -> i32 {
        self.plot_size + self.road_width
    }

    pub fn to_snbt(&self) -> String {
        NbtValue::obj([
            ("plotSize", NbtValue::Int(self.plot_size)),
            ("roadWidth", NbtValue::Int(self.road_width)),
            ("borderWidth", NbtValue::Int(self.border_width)),
            ("floorY", NbtValue::Int(self.floor_y)),
            ("floorBlock", NbtValue::from(self.floor_block.as_str())),
            ("fillBlock", NbtValue::from(self.fill_block.as_str())),
            ("roadBlock", NbtValue::from(self.road_block.as_str())),
            ("borderBlock", NbtValue::from(self.border_block.as_str())),
            ("biome", NbtValue::from(self.biome.as_str())),
        ])
        .to_snbt()
    }
}

/// One custom dimension that has been registered.
#[derive(Debug, Clone, PartialEq)]
pub struct ExistingDimension {
    pub name: String,
    pub dim: i32,
    /// The raw generation parameters, interpreted by the caller.
    pub snbt: String,
}

/// Whether this host was built with the custom dimension capability.
pub fn is_available() -> bool {
    if !crate::has_slot!(md_is_available) {
        return false;
    }
    match crate::__rt::api().md_is_available {
        Some(f) => unsafe { f() },
        None => false,
    }
}

/// Registers a simple custom dimension. It returns the dimension id, 3 or above.
pub fn add_simple(name: &str, seed: u32, generator: GeneratorType) -> Result<i32> {
    let f = crate::require_slot!(md_add_simple_dimension, "registering a custom dimension");
    let id = unsafe { f(s(name), seed, generator.as_i32()) };
    if id < 0 {
        Err(Error(format!(
            "the dimension {name} could not be registered: the name is invalid, or the dimension numbers are exhausted"
        )))
    } else {
        Ok(id)
    }
}

/// Registers a plot world. The generator lays the grid down during generation rather than
/// blocks being placed afterwards.
pub fn add_plot(name: &str, seed: u32, layout: &PlotLayout) -> Result<i32> {
    let f = crate::require_slot!(md_add_plot_dimension, "registering a plot dimension");
    let spec = layout.to_snbt();
    let id = unsafe { f(s(name), seed, s(&spec)) };
    if id < 0 {
        Err(Error(format!(
            "the plot dimension {name} could not be registered: the name is invalid, or a layout parameter is out of range"
        )))
    } else {
        Ok(id)
    }
}

/// Looks up a dimension id by name.
///
/// It gives an id only for a name really registered and returns `None` otherwise, rather
/// than the undefined dimension whose value changes at runtime while looking like a valid
/// id.
///
/// It is rarely needed; see the note on registering unconditionally in the module
/// documentation.
pub fn dimension_id(name: &str) -> Option<i32> {
    if !crate::has_slot!(md_get_dimension_id) {
        return None;
    }
    let f = crate::__rt::api().md_get_dimension_id?;
    match unsafe { f(s(name)) } {
        id if id >= 0 => Some(id),
        _ => None,
    }
}

/// Every registered custom dimension.
///
/// A world manager adopting an existing save has to ask this first: the dimensions a
/// previous plugin created are alive in the save and players can teleport into them while
/// the manager's table has no row for them. The consequence is not a few missing rows but
/// those dimensions being governed by no rule, and a newly created world possibly being
/// assigned a dimension id that collides with theirs.
pub fn list() -> Vec<ExistingDimension> {
    if !crate::has_slot!(md_list_dimensions) {
        return Vec::new();
    }
    let Some(f) = crate::__rt::api().md_list_dimensions else {
        return Vec::new();
    };
    // The host sinks once per dimension rather than handing over the whole array at once,
    // unlike `lane_list`. Using `call_out_str` would keep only the last entry.
    let raw = collect_strs(|ctx, sink| unsafe { f(ctx, sink) });
    let mut out = Vec::with_capacity(raw.len());
    for text in raw {
        if text.trim().is_empty() {
            continue;
        }
        match serde_json::from_str::<ExistingDimensionJson>(&text) {
            Ok(i) => out.push(ExistingDimension {
                name: i.name,
                dim: i.dim,
                snbt: i.snbt,
            }),
            // A bad entry is skipped rather than voiding the whole batch, since
            // `dimension_selector` depends on this table.
            Err(e) => crate::Logger::get().warn(&format!(
                "one entry of the dimension listing could not be parsed and was skipped: {e} (raw: {})",
                text.chars().take(200).collect::<String>()
            )),
        }
    }
    out
}

#[derive(serde::Deserialize)]
struct ExistingDimensionJson {
    name: String,
    dim: i32,
    #[serde(default)]
    snbt: String,
}

/// Sets one per-dimension rule.
pub fn set_rule(dimension: i32, rule: DimensionRule, allow: bool) -> Result<()> {
    let f = crate::require_slot!(md_set_dimension_rule, "setting a dimension rule");
    unsafe { f(dimension, rule.as_i32(), allow) };
    Ok(())
}

/// Reads one rule. A dimension with no explicit registration for that rule gives
/// `Ok(None)`, meaning it follows vanilla behavior, which is different from being
/// registered with the value false.
pub fn rule(dimension: i32, rule: DimensionRule) -> Result<Option<bool>> {
    let f = crate::require_slot!(md_get_dimension_rule, "reading a dimension rule");
    let mut out = false;
    if unsafe { f(dimension, rule.as_i32(), &mut out) } {
        Ok(Some(out))
    } else {
        Ok(None)
    }
}

/// Clears every rule of a dimension, for when the world was deleted.
pub fn clear_rules(dimension: i32) -> Result<()> {
    let f = crate::require_slot!(md_clear_dimension_rules, "clearing the dimension rules");
    unsafe { f(dimension) };
    Ok(())
}

/// The merge marks of one plot.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PlotMerge {
    pub x: i32,
    pub z: i32,
    /// A bit set where 1 is north, 2 east, 4 south and 8 west.
    pub mask: u32,
}

impl PlotMerge {
    pub const NORTH: u32 = 1;
    pub const EAST: u32 = 2;
    pub const SOUTH: u32 = 4;
    pub const WEST: u32 = 8;

    pub fn is_empty(&self) -> bool {
        self.mask == 0
    }

    /// Assembles a mask from the four directions in the order north, east, south, west.
    ///
    /// The order is the bit order, with `NORTH` as bit 0. Writing `1 | 4` by hand makes a
    /// reader look the table up in reverse, and getting it backwards shows up as plots merging
    /// in the wrong direction.
    pub fn from_dirs(x: i32, z: i32, dirs: [bool; 4]) -> PlotMerge {
        let mut mask = 0u32;
        for (i, on) in dirs.iter().enumerate() {
            if *on {
                mask |= 1u32 << i;
            }
        }
        PlotMerge { x, z, mask }
    }
}

/// Registers or updates the plot grid of a dimension. A `plot_size <= 0` clears it.
///
/// Changed geometry clears the merge table, since an old merge mark points at a different
/// plot under the new grid.
pub fn set_plot_grid(dimension: i32, plot_size: i32, road_width: i32) -> Result<()> {
    let f = crate::require_slot!(md_set_plot_grid, "registering a plot grid");
    unsafe { f(dimension, plot_size, road_width) };
    Ok(())
}

pub fn clear_plot_grid(dimension: i32) -> Result<()> {
    let f = crate::require_slot!(md_clear_plot_grid, "clearing a plot grid");
    unsafe { f(dimension) };
    Ok(())
}

/// Replaces the merge marks of a dimension as a whole.
///
/// As a whole and not incrementally: an increment requires both sides to agree at all
/// times on the same current state, while unlinking clears the neighbor before storing
/// itself, and a failure in between makes the two views diverge with no way back. A whole
/// push pulls both sides back into agreement every time.
///
/// Call [`set_plot_grid`] first: a push to a dimension with no registered grid is dropped
/// with a warning.
pub fn set_plot_merges(dimension: i32, merges: &[PlotMerge]) -> Result<()> {
    let f = crate::require_slot!(md_set_plot_merges, "pushing the plot merge table");
    // The ABI takes count triples of (x, z, mask), meaning count*3 i32 values.
    let mut flat: Vec<i32> = Vec::with_capacity(merges.len() * 3);
    for m in merges {
        flat.push(m.x);
        flat.push(m.z);
        flat.push(m.mask as i32);
    }
    unsafe { f(dimension, flat.as_ptr(), merges.len() as i32) };
    Ok(())
}
