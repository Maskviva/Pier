//! Custom dimensions: the facade of the optional `pier-dimensions` capability package.
//!
//! When it is not built into the host the whole family of slots is NULL,
//! [`is_available`] returns false and every other call returns an `Err` saying the host
//! does not provide it. That is rule 3 of contract §1 at runtime: the optional package is
//! absent, the layout is unchanged, and the slots are empty.
//!
//! Registration is idempotent: [`add_dimension`] and [`add_dimension_pack`] return the
//! same persisted id for the same name on the next startup, so a mod registers at startup
//! rather than probing with [`dimension_id`] first, which misses on the first startup.
//! A pack terrain is a directory with a config and a binary built by `tools/pier-pack`;
//! [`pack_inspect`] tells what it asks for and [`add_dimension_pack`] mounts it, and the
//! host stores the file hash and every bound value with the dimension.

use crate::rt::error::{Error, Result};
use crate::rt::ffi::{collect_strs, s};
use crate::sys;

/// The vanilla generator of a `terrain:{kind:"native"}` spec.
///
/// The values are the engine's `GeneratorType`, which starts at 1 and not 0: numbering from
/// 0 would make superflat generate a nether. The spec itself names the generator with the
/// lower-case string of [`GeneratorType::spec_name`]; the numbers survive because
/// `md_list_dimensions` and old saves both carry them.
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

    /// What `terrain.generator` and `sky.client` are spelled as in a spec.
    pub fn spec_name(self) -> &'static str {
        match self {
            GeneratorType::Overworld => "overworld",
            GeneratorType::Flat => "flat",
            GeneratorType::Nether => "nether",
            GeneratorType::TheEnd => "end",
            GeneratorType::Void => "void",
        }
    }

    /// What the engine itself calls this generator.
    ///
    /// Not the same string as [`GeneratorType::spec_name`]: this one appears in generation
    /// parameters and in old saves. Use it when assembling something for the engine, not
    /// `{:?}`.
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
    /// Blocks only a piston push crossing a cell boundary, leaving the cell interior alone. It
    /// applies together with [`DimensionRule::PistonPush`] and either one forbidding stops
    /// the push.
    PistonCrossCell = 11,
    /// Blocks only actor movement crossing a cell boundary. Players and ridden vehicles are
    /// never restricted.
    EntityCrossCell = 12,
}

// The retired names keep the variant spelling they had, so an existing caller compiles
// unchanged; that spelling is not the one a const wants, so the lint is silenced for the
// whole block rather than per constant.
#[allow(non_upper_case_globals)]
impl DimensionRule {
    /// The spelling a plot world used, retired since 26.20.3. Same value as
    /// [`DimensionRule::PistonCrossCell`]; the ABI keeps both names permanently, so an
    /// existing caller keeps compiling and resolves to the same rule.
    #[deprecated(since = "26.20.3", note = "renamed to PistonCrossCell")]
    pub const PistonCrossPlot: Self = Self::PistonCrossCell;
    /// Retired since 26.20.3; see [`DimensionRule::PistonCrossPlot`].
    #[deprecated(since = "26.20.3", note = "renamed to EntityCrossCell")]
    pub const EntityCrossPlot: Self = Self::EntityCrossCell;

    pub fn as_i32(self) -> i32 {
        self as i32
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

/// Replaces the merge marks of a dimension as a whole.
///
/// As a whole and not incrementally: an increment requires both sides to agree at all
/// times on the same current state, while unlinking clears the neighbor before storing
/// itself, and a failure in between makes the two views diverge with no way back. A whole
/// push pulls both sides back into agreement every time.
///
/// The grid comes from the template pack's CONF section at [`add_dimension_pack`]: a push
/// to a dimension without one is dropped with a warning. The geometry the mod side must
/// match: with `period = cell + gap`, a column `(x, z)` is inside a cell when
/// `mod(x, period) < cell && mod(z, period) < cell`.
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

/// Adds a custom dimension with a native terrain; see `md_add_dimension` in `abi.h`.
///
/// The spec is opaque to this SDK: the shape is owned by the host and the RSW world
/// manager (`rsw_world_spec::Generator::to_spec_snbt`) writes it. This function only
/// carries the string across. A pack terrain is refused here; see [`add_dimension_pack`].
pub fn add_dimension(name: &str, spec_snbt: &str) -> Result<i32> {
    let f = crate::require_slot!(md_add_dimension, "registering a dimension from a spec");
    let r = unsafe { f(s(name), s(spec_snbt)) };
    if r < 0 {
        return Err(Error(format!(
            "the host refused the dimension '{name}'; the reason is in the host log"
        )));
    }
    Ok(r)
}

/// Why the host refused a pack; the values are `PIER_PACK_*`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PackStatus {
    BadPath,
    ConfigUnreadable,
    ConfigInvalid,
    BinaryUnreadable,
    Corrupt,
    KindMismatch,
    HashMismatch,
    Unsupported,
    Params,
    Constraint,
    Height,
    StoredMismatch,
    Spec,
    Host,
    /// A code this SDK does not know; the host is newer than the mirror.
    Other(i32),
}

impl PackStatus {
    pub fn from_code(code: i32) -> PackStatus {
        match code {
            sys::PIER_PACK_BAD_PATH => PackStatus::BadPath,
            sys::PIER_PACK_CONFIG_UNREADABLE => PackStatus::ConfigUnreadable,
            sys::PIER_PACK_CONFIG_INVALID => PackStatus::ConfigInvalid,
            sys::PIER_PACK_BINARY_UNREADABLE => PackStatus::BinaryUnreadable,
            sys::PIER_PACK_CORRUPT => PackStatus::Corrupt,
            sys::PIER_PACK_KIND_MISMATCH => PackStatus::KindMismatch,
            sys::PIER_PACK_HASH_MISMATCH => PackStatus::HashMismatch,
            sys::PIER_PACK_UNSUPPORTED => PackStatus::Unsupported,
            sys::PIER_PACK_PARAMS => PackStatus::Params,
            sys::PIER_PACK_CONSTRAINT => PackStatus::Constraint,
            sys::PIER_PACK_HEIGHT => PackStatus::Height,
            sys::PIER_PACK_STORED_MISMATCH => PackStatus::StoredMismatch,
            sys::PIER_PACK_SPEC => PackStatus::Spec,
            sys::PIER_PACK_HOST => PackStatus::Host,
            other => PackStatus::Other(other),
        }
    }
}

/// A refusal of [`add_dimension_pack`] or [`pack_inspect`], with the host's reasons when
/// the call produced any.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PackError {
    pub status: PackStatus,
    pub problems: Vec<String>,
}

impl std::fmt::Display for PackError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self.status)?;
        for p in &self.problems {
            write!(f, ": {p}")?;
        }
        Ok(())
    }
}

/// Adds a custom dimension whose terrain is a pack; see `md_add_dimension_pack` in
/// `abi.h`.
///
/// `config_path` names the pack config relative to the server root with forward slashes;
/// `spec_snbt` is a spec whose terrain has `kind:"template"` or `"volume"` plus `params`
/// and `roles`. The host verifies the pack, binds the values and stores everything with
/// the dimension. A negative return is a `PIER_PACK_*` code and becomes a [`PackError`]
/// with no problem lines; the reasons are in the host log, and [`pack_inspect`] on the
/// same path returns them as JSON.
pub fn add_dimension_pack(
    name: &str,
    config_path: &str,
    spec_snbt: &str,
) -> std::result::Result<i32, PackError> {
    let f = match crate::rt::runtime::api().md_add_dimension_pack {
        Some(f) => f,
        None => {
            return Err(PackError {
                status: PackStatus::Host,
                problems: vec!["this host does not provide md_add_dimension_pack".to_owned()],
            })
        }
    };
    let r = unsafe { f(s(name), s(config_path), s(spec_snbt)) };
    if r < 0 {
        return Err(PackError {
            status: PackStatus::from_code(r),
            problems: Vec::new(),
        });
    }
    Ok(r)
}

/// What a pack asks for, as the JSON document `md_pack_inspect` describes, without
/// registering anything. A refusal carries the host's problem lines.
pub fn pack_inspect(config_path: &str) -> std::result::Result<String, PackError> {
    let f = match crate::rt::runtime::api().md_pack_inspect {
        Some(f) => f,
        None => {
            return Err(PackError {
                status: PackStatus::Host,
                problems: vec!["this host does not provide md_pack_inspect".to_owned()],
            })
        }
    };
    let mut r = 0i32;
    let out = collect_strs(|ctx, sink| r = unsafe { f(s(config_path), ctx, sink) });
    let json = out.into_iter().next().unwrap_or_default();
    if r == 0 {
        return Ok(json);
    }
    // The refusal document carries the same reasons the host logged; they are handed
    // back rather than parsed, since the SDK carries no JSON reader.
    Err(PackError {
        status: PackStatus::from_code(r),
        problems: if json.is_empty() {
            Vec::new()
        } else {
            vec![json]
        },
    })
}

/// Retires a custom dimension; see `md_retire_dimension` in `abi.h`.
///
/// The host drops the name from `dimension_config.json`, from its own tables and from the
/// dimension factory, so it is not registered again on the next boot. Nothing in the
/// running engine is undone: the dimension built for this session stays and a player
/// inside it is not moved.
///
/// The chunks stay in the save and the id is not handed out again. Registering the same
/// name afterwards is a new dimension with a new id, so the old terrain is orphaned
/// rather than inherited.
///
/// `Ok(false)` when the host had no dimension of that name, which is also the answer to a
/// second call.
pub fn retire_dimension(name: &str) -> Result<bool> {
    let f = crate::require_slot!(md_retire_dimension, "retiring a dimension");
    Ok(unsafe { f(s(name)) })
}
