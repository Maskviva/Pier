//! The engine's own registries: every block type, every item and every entity the running game
//! has, with the facts a rule needs to pick from them.
//!
//! A mod that hands out "a random block" or "a random rare item" should not keep a list of
//! names of its own: the game has more than any such list, and the list goes stale with every
//! version. It reads these instead and decides by rules (solid, not technical, of this rarity).
//! A host older than the `registry_list` slot answers with an error, never with an empty list
//! that would look like a game without content.

use crate::rt::error::{Error, Result};
use crate::rt::ffi::collect_strs;
use crate::sys;

/// Reads a whole number written either as `3` or as `3.0`: a host that formats every number
/// the same way is still read right.
fn whole<'de, D, T>(d: D) -> std::result::Result<T, D::Error>
where
    D: serde::Deserializer<'de>,
    T: TryFrom<i64>,
{
    let v = <f64 as serde::Deserialize>::deserialize(d)?;
    if !v.is_finite() || v.fract() != 0.0 {
        return Err(serde::de::Error::custom(format!(
            "{v} is not a whole number"
        )));
    }
    T::try_from(v as i64).map_err(|_| serde::de::Error::custom(format!("{v} is out of range")))
}

/// The engine's creative categories, as it numbers them.
pub mod category {
    pub const ALL: i32 = 0;
    pub const CONSTRUCTION: i32 = 1;
    pub const NATURE: i32 = 2;
    pub const EQUIPMENT: i32 = 3;
    pub const ITEMS: i32 = 4;
    /// Reachable by commands only: barriers, command blocks, structure blocks and the like.
    pub const COMMAND_ONLY: i32 = 5;
    pub const UNDEFINED: i32 = 6;
}

/// One block type, read from its default state.
#[derive(Debug, Clone, PartialEq, serde::Deserialize)]
pub struct BlockType {
    pub name: String,
    /// A [`category`] value.
    #[serde(default, deserialize_with = "whole")]
    pub category: i32,
    #[serde(default)]
    pub solid: bool,
    #[serde(default)]
    pub vanilla: bool,
    #[serde(default)]
    pub container: bool,
    /// Gives a redstone signal of its own, as a lever or a button does.
    #[serde(default)]
    pub signal: bool,
    #[serde(default)]
    pub fence: bool,
    #[serde(default)]
    pub rail: bool,
    #[serde(default)]
    pub slab: bool,
    #[serde(default)]
    pub wall: bool,
    #[serde(default)]
    pub crop: bool,
    /// Holds a block entity: a chest, a sign, a spawner, a bed.
    #[serde(default)]
    pub entity: bool,
    /// Bare-hand destroy speed; negative for a block nothing breaks.
    #[serde(default)]
    pub destroy: f64,
    #[serde(default)]
    pub resistance: f64,
    /// The light it emits, 0 to 15.
    #[serde(default, deserialize_with = "whole")]
    pub light: i32,
    /// The localization key.
    #[serde(default)]
    pub description: String,
}

/// One item.
#[derive(Debug, Clone, PartialEq, serde::Deserialize)]
pub struct ItemType {
    pub name: String,
    /// 0 common, 1 uncommon, 2 rare, 3 epic.
    #[serde(default, deserialize_with = "whole")]
    pub rarity: i32,
    #[serde(default, deserialize_with = "whole")]
    pub stack: i32,
    /// A [`category`] value.
    #[serde(default, deserialize_with = "whole")]
    pub category: i32,
    /// Commands do not offer it.
    #[serde(default)]
    pub hidden: bool,
}

/// One entity the level knows.
#[derive(Debug, Clone, PartialEq, serde::Deserialize)]
pub struct EntityType {
    pub name: String,
    #[serde(default)]
    pub spawn_egg: bool,
    #[serde(default)]
    pub summonable: bool,
    /// An experiment has to be on for it to exist.
    #[serde(default)]
    pub experimental: bool,
    /// The engine's actor type number; see [`EntityType::is_monster`] and the others.
    #[serde(default, rename = "type", deserialize_with = "whole")]
    pub type_id: i64,
}

impl EntityType {
    /// The engine sets this bit on every mob.
    pub const MOB: i64 = 0x100;
    pub const MONSTER: i64 = 0x800;
    pub const ANIMAL: i64 = 0x1000;
    pub const WATER_ANIMAL: i64 = 0x2000;

    pub fn is_mob(&self) -> bool {
        self.type_id & Self::MOB != 0
    }

    pub fn is_monster(&self) -> bool {
        self.type_id & Self::MONSTER != 0
    }

    /// A land animal: the animal bit without the water-animal one.
    pub fn is_animal(&self) -> bool {
        self.type_id & Self::ANIMAL != 0 && self.type_id & Self::WATER_ANIMAL == 0
    }

    pub fn is_water_animal(&self) -> bool {
        self.type_id & Self::WATER_ANIMAL != 0
    }
}

fn list<T: serde::de::DeserializeOwned>(kind: i32, what: &str) -> Result<Vec<T>> {
    if !crate::has_slot!(registry_list) {
        return Err(Error(format!(
            "this host cannot list the {what} registry: it predates the registry_list slot"
        )));
    }
    let Some(f) = crate::__rt::api().registry_list else {
        return Err(Error(format!("this host cannot list the {what} registry")));
    };
    let mut ok = false;
    let raw = collect_strs(|ctx, sink| {
        ok = unsafe { f(kind, ctx, sink) };
    });
    if !ok {
        return Err(Error(format!(
            "the {what} registry could not be listed: it does not exist yet, or the host refused"
        )));
    }
    let mut out = Vec::with_capacity(raw.len());
    let mut bad = 0usize;
    for text in raw {
        match serde_json::from_str::<T>(&text) {
            Ok(v) => out.push(v),
            Err(_) => bad += 1,
        }
    }
    if bad > 0 {
        crate::Logger::get().warn(&format!(
            "{bad} entries of the {what} registry could not be read and were skipped"
        ));
    }
    Ok(out)
}

/// Every block type.
pub fn blocks() -> Result<Vec<BlockType>> {
    list(sys::PIER_REGISTRY_BLOCKS, "block")
}

/// Every item, each once under its own full name.
pub fn items() -> Result<Vec<ItemType>> {
    list(sys::PIER_REGISTRY_ITEMS, "item")
}

/// Every entity the level knows.
pub fn entities() -> Result<Vec<EntityType>> {
    list(sys::PIER_REGISTRY_ENTITIES, "entity")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_listing_formats_read_back_with_whole_numbers_written_either_way() {
        let b: BlockType = serde_json::from_str(
            r#"{"name":"minecraft:stone","category":1,"solid":true,"vanilla":true,"container":false,"signal":false,"fence":false,"rail":false,"slab":false,"wall":false,"crop":false,"entity":false,"destroy":1.500000,"resistance":6.000000,"light":0,"description":"tile.stone.name"}"#,
        )
        .unwrap();
        assert_eq!(b.name, "minecraft:stone");
        assert!(b.solid && b.vanilla && !b.entity);
        assert_eq!(b.category, category::CONSTRUCTION);
        assert_eq!(b.destroy, 1.5);
        assert_eq!(b.light, 0);
        for text in [
            r#"{"name":"minecraft:elytra","rarity":3,"stack":1,"category":3,"hidden":false}"#,
            r#"{"name":"minecraft:elytra","rarity":3.000000,"stack":1.0,"category":3.0,"hidden":false}"#,
        ] {
            let i: ItemType = serde_json::from_str(text).unwrap();
            assert_eq!((i.rarity, i.stack, i.category, i.hidden), (3, 1, 3, false));
        }
        assert!(serde_json::from_str::<ItemType>(r#"{"name":"x","rarity":2.5}"#).is_err());
        let e: EntityType = serde_json::from_str(
            r#"{"name":"minecraft:zombie","spawn_egg":true,"summonable":true,"experimental":false,"type":199456}"#,
        )
        .unwrap();
        assert!(e.is_mob() && e.is_monster() && !e.is_animal());
        // A field the host leaves out reads as its default.
        let bare: EntityType = serde_json::from_str(r#"{"name":"minecraft:pig"}"#).unwrap();
        assert_eq!(bare.type_id, 0);
        assert!(!bare.spawn_egg);
    }

    #[test]
    fn the_type_bits_tell_monsters_land_animals_and_water_animals_apart() {
        let of = |t: i64| EntityType {
            name: String::new(),
            spawn_egg: true,
            summonable: true,
            experimental: false,
            type_id: t,
        };
        // Monster, Animal, WaterAnimal and TamableAnimal as the engine numbers them.
        assert!(of(2816).is_monster() && !of(2816).is_animal());
        assert!(of(4864).is_animal() && !of(4864).is_monster());
        assert!(of(8960).is_water_animal() && !of(8960).is_animal());
        assert!(of(21248).is_animal());
        assert!(!of(0).is_mob());
    }
}
