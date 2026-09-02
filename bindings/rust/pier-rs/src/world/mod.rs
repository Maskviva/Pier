//! The world: reads and writes at the level layer, covering time, weather, difficulty,
//! game rules, biomes and chunks.
//!
//! The boundary with [`crate::host`] is whether something speaks about the host or the
//! world. The server stage, scheduling and executing a command belong to the host and hold
//! for another game; time, weather and chunks belong to the world.
//!
//! The switches of the level itself are here, changing things inside the world is in
//! `edit`, and assembling commands is in `commands`.

mod commands;
mod edit;

pub use commands::{is_valid_ticking_area_name, split_box, Box3D, MAX_FILL_VOLUME};

use crate::block::BlockInfo;
use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, collect_raw, collect_strs, s, s_raw};
use crate::types::{Bounds, Difficulty, PositionI32, Weather};

/// One actor that fell inside the region during a scan.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct EntityInfo {
    /// The cell the actor is in, its position floored.
    pub cell: PositionI32,
    pub type_name: String,
    /// The full NBT of `Actor::save`.
    pub snbt: String,
}

/// The result of one scan.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct Scan {
    pub blocks: Vec<BlockInfo>,
    pub entities: Vec<EntityInfo>,
}

impl Scan {
    /// Indexes a block by coordinate.
    ///
    /// It rebuilds a table on every call, so it does not belong in a loop, where it is O(n^2).
    /// Repeated lookups keep the returned value. The ABI does not guarantee the traversal
    /// order of the sink, so a position cannot be computed from an index.
    pub fn block_map(&self) -> std::collections::HashMap<PositionI32, &BlockInfo> {
        self.blocks.iter().map(|b| (b.pos, b)).collect()
    }

    pub fn non_air_count(&self) -> usize {
        self.blocks.iter().filter(|b| !b.is_air()).count()
    }

    /// How many actors fell inside this region.
    pub fn entity_count(&self) -> usize {
        self.entities.len()
    }
}

/// One village.
#[derive(Debug, Clone, PartialEq)]
pub struct VillageInfo {
    pub uuid: String,
    pub center: PositionI32,
    pub bounds: Bounds,
    pub poi_count: i32,
}

/// One hardcoded generation area: a stronghold, a witch hut, an ocean monument or a
/// pillager outpost.
#[derive(Debug, Clone, PartialEq)]
pub struct StructureInfo {
    pub kind: String,
    pub bounds: Bounds,
}

/// The value of one game rule.
#[derive(Debug, Clone, PartialEq)]
pub enum GameRuleValue {
    Bool(bool),
    Int(i64),
    Float(f64),
}

/// The sleep status, from `level_get_sleep_status`.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct SleepStatus {
    pub sleeping: bool,
    pub total_players: i32,
    pub active_sleeping: i32,
}

/// The level facade. Zero sized.
#[derive(Clone, Copy)]
pub struct World(());

impl World {
    pub fn get() -> World {
        World(())
    }

    // The clock and the weather

    pub fn time(&self) -> Result<i64> {
        let f = crate::require_slot!(get_time, "reading the world time");
        let mut out = 0i64;
        if unsafe { f(&mut out) } {
            Ok(out)
        } else {
            Err(Error("the level is not ready, so the world time cannot be read".to_owned()))
        }
    }

    pub fn set_time(&self, t: i64) -> Result<()> {
        let f = crate::require_slot!(set_time, "setting the world time");
        if unsafe { f(t) } {
            Ok(())
        } else {
            Err(Error("the level is not ready, so the world time cannot be set".to_owned()))
        }
    }

    pub fn set_weather(&self, weather: Weather) -> Result<()> {
        let f = crate::require_slot!(set_weather, "setting the weather");
        if unsafe { f(weather.as_i32()) } {
            Ok(())
        } else {
            Err(Error("the level is not ready, so the weather cannot be set".to_owned()))
        }
    }

    /// Sets the level and the remaining duration, in ticks, of rain and of lightning
    /// individually.
    ///
    /// Finer than [`World::set_weather`], which has three settings, this can do light rain for
    /// three minutes.
    pub fn update_weather(
        &self,
        rain_level: f32,
        rain_ticks: i32,
        lightning_level: f32,
        lightning_ticks: i32,
    ) -> Result<()> {
        let f = crate::require_slot!(level_update_weather, "updating the weather parameters");
        let ok = unsafe { f(rain_level, rain_ticks, lightning_level, lightning_ticks) };
        if ok {
            Ok(())
        } else {
            Err(Error("the level is not ready, so the weather parameters cannot be updated".to_owned()))
        }
    }

    // Level settings

    pub fn difficulty(&self) -> Result<Difficulty> {
        let f = crate::require_slot!(get_difficulty, "reading the difficulty");
        let mut out = 0i32;
        if !unsafe { f(&mut out) } {
            return Err(Error("the level is not ready, so the difficulty cannot be read".to_owned()));
        }
        Difficulty::from_i32(out).ok_or_else(|| Error(format!("the host reported an unrecognized difficulty {out}")))
    }

    pub fn set_difficulty(&self, d: Difficulty) -> Result<()> {
        let f = crate::require_slot!(set_difficulty, "setting the difficulty");
        if unsafe { f(d.as_i32()) } {
            Ok(())
        } else {
            Err(Error("the level is not ready, so the difficulty cannot be set".to_owned()))
        }
    }

    pub fn seed(&self) -> Result<i64> {
        let f = crate::require_slot!(get_seed, "reading the world seed");
        let mut out = 0i64;
        if unsafe { f(&mut out) } {
            Ok(out)
        } else {
            Err(Error("the level is not ready, so the world seed cannot be read".to_owned()))
        }
    }

    /// Reads one game rule. An unrecognized rule name is an `Err` and not some default value.
    pub fn game_rule(&self, name: &str) -> Result<GameRuleValue> {
        let f = crate::require_slot!(game_rule_get, "reading a game rule");
        let text = call_out_str(|ctx, sink| unsafe { f(s(name), ctx, sink) })
            .ok_or_else(|| Error(format!("there is no game rule named {name}")))?;
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("parsing the game rule SNBT failed: {e}")))?;
        let kind = v.get_str("type")?.to_owned();
        match kind.as_str() {
            "bool" => Ok(GameRuleValue::Bool(v.get_bool("value")?)),
            "int" => Ok(GameRuleValue::Int(v.get_i64("value")?)),
            "float" => Ok(GameRuleValue::Float(v.get_f64("value")?)),
            other => Err(Error(format!("game rule {name} has the unrecognized type {other:?}"))),
        }
    }

    pub fn set_game_rule(&self, name: &str, value: &str) -> Result<()> {
        let f = crate::require_slot!(game_rule_set, "setting a game rule");
        if unsafe { f(s(name), s(value)) } {
            Ok(())
        } else {
            Err(Error(format!(
                "the game rule {name}={value} could not be set, since the rule name or the value is invalid"
            )))
        }
    }

    pub fn default_spawn(&self) -> Result<PositionI32> {
        let f = crate::require_slot!(level_get_default_spawn, "reading the default spawn point");
        let (mut x, mut y, mut z) = (0i32, 0i32, 0i32);
        if unsafe { f(&mut x, &mut y, &mut z) } {
            Ok((x, y, z))
        } else {
            Err(Error("the level is not ready, so the default spawn point cannot be read".to_owned()))
        }
    }

    pub fn set_default_spawn(&self, x: i32, y: i32, z: i32) -> Result<()> {
        let f = crate::require_slot!(level_set_default_spawn, "setting the default spawn point");
        if unsafe { f(x, y, z) } {
            Ok(())
        } else {
            Err(Error("the level is not ready, so the default spawn point cannot be set".to_owned()))
        }
    }

    /// Saves to disk immediately.
    pub fn save(&self) -> Result<()> {
        let f = crate::require_slot!(level_save, "saving the level");
        if unsafe { f() } {
            Ok(())
        } else {
            Err(Error("the level is not ready, so it cannot be saved".to_owned()))
        }
    }

    pub fn sleep_status(&self) -> Result<SleepStatus> {
        let f = crate::require_slot!(level_get_sleep_status, "reading the sleep status");
        let text = call_out_str(|ctx, sink| unsafe { f(ctx, sink) })
            .ok_or_else(|| Error("the level is not ready, so the sleep status cannot be read".to_owned()))?;
        let v =
            NbtValue::parse(&text).map_err(|e| Error(format!("parsing the sleep status SNBT failed: {e}")))?;
        Ok(SleepStatus {
            sleeping: v.opt_bool("sleeping").unwrap_or(false),
            total_players: v.opt_i32("total_players").unwrap_or(0),
            active_sleeping: v.opt_i32("active_sleeping").unwrap_or(0),
        })
    }

    // Biomes

    pub fn biome(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<String> {
        let f = crate::require_slot!(level_get_biome, "reading a biome");
        call_out_str(|ctx, sink| unsafe { f(dim, x, y, z, ctx, sink) })
            .ok_or_else(|| Error(format!("the biome at ({x},{y},{z}) in dimension {dim} could not be read")))
    }

    /// Sets the biome of a region, column by column.
    ///
    /// It takes no y. `setBiome3d` works per y while Bedrock stores a biome per column, and
    /// taking a y would suggest layers can be set separately. It returns how many columns were
    /// set, and 0 means none were, because the chunks are not loaded or the biome name is
    /// unrecognized, rather than being set with nothing changing.
    pub fn set_biome(
        &self,
        dim: i32,
        from: (i32, i32),
        to: (i32, i32),
        biome: &str,
    ) -> Result<i32> {
        let f = crate::require_slot!(level_set_biome, "setting a biome");
        let (min_x, max_x) = (from.0.min(to.0), from.0.max(to.0));
        let (min_z, max_z) = (from.1.min(to.1), from.1.max(to.1));
        Ok(unsafe { f(dim, min_x, min_z, max_x, max_z, s(biome)) })
    }

    // Read-only queries

    /// The villages in one dimension.
    pub fn villages(&self, dim: i32) -> Vec<VillageInfo> {
        // Neither the length gate nor the non-null gate may be skipped (contract §2.2): with a
        // table too short to reach this field, reading it is out of bounds, and what comes
        // back often looks like a valid function pointer.
        if !crate::has_slot!(villages) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().villages else {
            return Vec::new();
        };
        parse_each(
            collect_strs(|ctx, sink| unsafe { f(dim, ctx, sink) }),
            "village",
            |v| {
                Some(VillageInfo {
                    uuid: v.opt_str("uuid").unwrap_or_default().to_owned(),
                    center: v.get_block_pos("center").ok()?,
                    bounds: parse_bounds(v)?,
                    poi_count: v.opt_i32("poi_count").unwrap_or(0),
                })
            },
        )
    }

    /// The hardcoded generation areas in the loaded chunks within a radius.
    ///
    /// Only loaded chunks are examined, since a read-only query should not force chunks to
    /// load. An empty result therefore means either that there are none nearby or that the
    /// nearby chunks are not loaded.
    pub fn structures_near(
        &self,
        dim: i32,
        x: i32,
        y: i32,
        z: i32,
        radius: i32,
    ) -> Vec<StructureInfo> {
        if !crate::has_slot!(structures_near) {
            return Vec::new();
        }
        let Some(f) = crate::__rt::api().structures_near else {
            return Vec::new();
        };
        parse_each(
            collect_strs(|ctx, sink| unsafe { f(dim, x, y, z, radius, ctx, sink) }),
            "structure",
            |v| {
                Some(StructureInfo {
                    kind: v.opt_str("type").unwrap_or_default().to_owned(),
                    bounds: parse_bounds(v)?,
                })
            },
        )
    }

    // Chunks and save keys

    /// Whether every chunk covered by `[min..max]` is in memory.
    ///
    /// This has to be asked before deleting a save key: a loaded chunk has a copy in memory
    /// and writes the key just deleted straight back on unload, while the deletion itself
    /// succeeded and reported a positive number.
    pub fn chunks_loaded(
        &self,
        dim: i32,
        min_x: i32,
        min_z: i32,
        max_x: i32,
        max_z: i32,
    ) -> Result<bool> {
        let f = crate::require_slot!(level_chunks_loaded, "querying the chunk load state");
        match unsafe { f(dim, min_x, min_z, max_x, max_z) } {
            1 => Ok(true),
            0 => Ok(false),
            _ => Err(Error(format!("dimension {dim} is unavailable, so the chunk load state cannot be determined"))),
        }
    }

    /// Deletes every save key of a chunk, so the engine regenerates it from the generator on
    /// the next load.
    ///
    /// The chunk must be unloaded first; see [`World::chunks_loaded`]. Getting a chunk
    /// unloaded is the caller's business, since who is nearby and when unloading is possible
    /// needs domain knowledge this layer should not have.
    ///
    /// It returns how many keys were deleted. A 0 is a normal result and means that chunk was
    /// never generated.
    pub fn delete_chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<i32> {
        let f = crate::require_slot!(level_delete_chunk_keys, "deleting the save keys of a chunk");
        let n = unsafe { f(dim, chunk_x, chunk_z) };
        if n < 0 {
            Err(Error("the save layer is unavailable, so chunk keys cannot be deleted".to_owned()))
        } else {
            Ok(n)
        }
    }

    /// Lists every save key of a chunk.
    ///
    /// A key is binary and contains zero bytes, so it is a `Vec<u8>` and not a `String`: a
    /// UTF-8 conversion would corrupt it into a key that cannot be deleted.
    pub fn chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<Vec<Vec<u8>>> {
        let f = crate::require_slot!(level_chunk_keys, "listing the save keys of a chunk");
        let mut n = 0i32;
        let keys = collect_raw(|ctx, sink| {
            n = unsafe { f(dim, chunk_x, chunk_z, ctx, sink) };
        });
        if n < 0 {
            Err(Error("the save layer is unavailable, so chunk keys cannot be listed".to_owned()))
        } else {
            Ok(keys)
        }
    }

    /// Deletes one save key byte for byte. The content is not interpreted and what is passed
    /// is what is deleted.
    pub fn delete_key(&self, key: &[u8]) -> Result<()> {
        let f = crate::require_slot!(level_delete_key, "deleting a save key");
        if unsafe { f(s_raw(key)) } {
            Ok(())
        } else {
            Err(Error("the save layer is unavailable, or this key does not exist".to_owned()))
        }
    }
}

// Parsing helpers

/// Parses a batch of SNBT one entry at a time, skipping a bad one with a warning.
///
/// Voiding the whole batch is wrong: one broken village entry should not make which
/// villages exist unanswerable. A skip is logged, otherwise it becomes the silent fallback
/// contract §5.1 forbids.
fn parse_each<T>(
    raw: Vec<String>,
    what: &str,
    mut build: impl FnMut(&NbtValue) -> Option<T>,
) -> Vec<T> {
    let mut out = Vec::with_capacity(raw.len());
    for text in raw {
        match NbtValue::parse(&text) {
            Ok(v) => match build(&v) {
                Some(item) => out.push(item),
                None => {
                    crate::Logger::get().warn(&format!("a {what} entry was missing a required field and was skipped: {text}"))
                }
            },
            Err(e) => crate::Logger::get().warn(&format!("parsing the SNBT of a {what} entry failed and it was skipped: {e}")),
        }
    }
    out
}

/// Reads `{bounds:{min:[...],max:[...]}}`.
fn parse_bounds(v: &NbtValue) -> Option<Bounds> {
    let b = v.get("bounds")?;
    Some(Bounds {
        min: b.get_block_pos("min").ok()?,
        max: b.get_block_pos("max").ok()?,
    })
}
