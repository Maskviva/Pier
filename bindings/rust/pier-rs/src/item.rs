//! Items: a value object and not a handle.
//!
//! On the ABI an item is a string of SNBT throughout. Reading a property means asking a
//! property with that SNBT, and changing one means exchanging that SNBT for a new one
//! through `item_transform`. This layer copies that shape and hides no pointer in between.
//!
//! # The consequence: an `ItemStack` has no connection to the item in the world
//!
//! `container.item(0)` returns a snapshot. Changing it does not move the one in the
//! container, and writing it back takes an explicit `container.set_item(0, &stack)`. This
//! is the other side of a handle being an identity and not a pointer: with no implicit
//! synchronization in between, there is no I changed it and nothing happened.

use crate::nbt::NbtValue;
use crate::rt::error::{Error, Result};
use crate::rt::ffi::{call_out_str, s};
use crate::sys;

/// An SNBT snapshot of one item.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct ItemStack {
    snbt: String,
}

/// One enchantment.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Enchant {
    /// The enchantment id. Whether the host reports a numeric id or a name depends on the
    /// BDS version, and it is carried through unchanged.
    pub id: String,
    pub level: i32,
}

impl ItemStack {
    /// Uses a string of SNBT directly, without validation, since validating would cross the
    /// ABI once and this constructor sits on a hot path.
    /// A wrong shape reports an error at the first call that really uses it.
    pub fn from_snbt(snbt: impl Into<String>) -> ItemStack {
        ItemStack { snbt: snbt.into() }
    }

    /// Builds one from a type name and a count.
    ///
    /// It assembles the minimal shape `{Name:"...",Count:Nb}` and the engine fills in the rest
    /// inside `ItemStack::fromTag`. A name that does not exist fails at the moment the item is
    /// used and not here.
    pub fn create(type_name: &str, count: u8) -> ItemStack {
        // Through `NbtValue::to_snbt` and not `format!`: the escaping rules for quotes,
        // backslashes and control characters should have one implementation, which lives in
        // `nbt::write` and has already been tested against `CompoundTag::fromSnbt` on the
        // other side.
        //
        // Count is a byte in NBT. A `u8` above 127 cannot be expressed, so it is clamped
        // rather than wrapping negative, since the engine reads a negative stack count as an
        // empty slot.
        let v = NbtValue::obj([
            ("Name", NbtValue::from(type_name)),
            ("Count", NbtValue::Byte(count.min(127) as i8)),
        ]);
        ItemStack { snbt: v.to_snbt() }
    }

    /// Air. An empty slot in a container reads back as this.
    pub fn empty() -> ItemStack {
        ItemStack::create("minecraft:air", 0)
    }

    /// The underlying SNBT.
    pub fn snbt(&self) -> &str {
        &self.snbt
    }

    /// Parses into an NBT tree, for reading a field the ABI gives no named accessor for.
    pub fn to_nbt(&self) -> Result<NbtValue> {
        NbtValue::parse(&self.snbt).map_err(|e| Error(format!("parsing the item SNBT failed: {e}")))
    }

    // Numeric properties

    /// Reads a `PIER_IPROP_*` numeric property.
    ///
    /// A property number the host does not recognize returns `Err` and not 0:
    /// cannot-be-determined and an answer of 0 must stay apart (contract §5.2).
    pub fn num(&self, prop: i32) -> Result<f64> {
        let f = crate::require_slot!(item_get_num, "reading a numeric item property");
        let mut out = 0.0f64;
        let ok = unsafe { f(s(&self.snbt), prop, &mut out) };
        if ok {
            Ok(out)
        } else {
            Err(Error(format!(
                "the host could not read item property {prop}: the property number is unrecognized, or this SNBT is not a valid item"
            )))
        }
    }

    fn num_i32(&self, prop: i32) -> Result<i32> {
        self.num(prop).map(|v| v as i32)
    }

    fn num_bool(&self, prop: i32) -> Result<bool> {
        self.num(prop).map(|v| v != 0.0)
    }

    pub fn count(&self) -> Result<u8> {
        self.num(sys::PIER_IPROP_COUNT)
            .map(|v| v.clamp(0.0, 255.0) as u8)
    }
    pub fn max_stack_size(&self) -> Result<u8> {
        self.num(sys::PIER_IPROP_MAX_STACK_SIZE)
            .map(|v| v.clamp(0.0, 255.0) as u8)
    }
    pub fn aux_value(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_AUX_VALUE)
    }
    pub fn id(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_ID)
    }
    pub fn damage(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_DAMAGE)
    }
    pub fn max_damage(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_MAX_DAMAGE)
    }
    pub fn attack_damage(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_ATTACK_DAMAGE)
    }
    pub fn repair_cost(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_REPAIR_COST)
    }
    pub fn enchant_value(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_ENCHANT_VALUE)
    }
    pub fn use_duration(&self) -> Result<i32> {
        self.num_i32(sys::PIER_IPROP_USE_DURATION)
    }
    pub fn is_null(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_NULL)
    }
    pub fn is_block(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_BLOCK)
    }
    pub fn is_enchanted(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_ENCHANTED)
    }
    pub fn is_armor(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_ARMOR)
    }
    pub fn is_damageable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_DAMAGEABLE)
    }
    pub fn is_damaged(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_DAMAGED)
    }
    pub fn is_unbreakable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_UNBREAKABLE)
    }
    pub fn has_durability(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_HAS_DURABILITY)
    }
    pub fn is_potion(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_POTION)
    }
    pub fn is_throwable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_THROWABLE)
    }
    pub fn is_fire_resistant(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_FIRE_RESISTANT)
    }
    pub fn is_stackable(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_STACKABLE)
    }
    pub fn is_music_disc(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_MUSIC_DISC)
    }
    pub fn is_offhand(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_OFFHAND)
    }
    pub fn is_glint(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_GLINT)
    }
    pub fn is_bundle(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_IS_BUNDLE)
    }
    pub fn has_user_data(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_HAS_USER_DATA)
    }
    pub fn has_custom_name(&self) -> Result<bool> {
        self.num_bool(sys::PIER_IPROP_HAS_CUSTOM_NAME)
    }

    // String properties

    /// Reads a `PIER_ISTR_*` string property.
    pub fn text(&self, prop: i32) -> Result<String> {
        let f = crate::require_slot!(item_get_str, "reading a string item property");
        call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), prop, ctx, sink) }).ok_or_else(|| {
            Error(format!(
                "the host could not read string item property {prop}"
            ))
        })
    }

    pub fn type_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_TYPE_NAME)
    }
    pub fn name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_NAME)
    }
    pub fn custom_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_CUSTOM_NAME)
    }
    pub fn hover_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_HOVER_NAME)
    }
    pub fn raw_name_id(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_RAW_NAME_ID)
    }
    pub fn effect_name(&self) -> Result<String> {
        self.text(sys::PIER_ISTR_EFFECT_NAME)
    }

    /// The custom lore. The host gives an SNBT string list, parsed here into a `Vec<String>`.
    pub fn lore(&self) -> Result<Vec<String>> {
        parse_str_list(&self.text(sys::PIER_ISTR_LORE)?, "lore")
    }

    pub fn can_destroy(&self) -> Result<Vec<String>> {
        parse_str_list(&self.text(sys::PIER_ISTR_CAN_DESTROY)?, "can_destroy")
    }

    pub fn can_place_on(&self) -> Result<Vec<String>> {
        parse_str_list(&self.text(sys::PIER_ISTR_CAN_PLACE_ON)?, "can_place_on")
    }

    /// The custom NBT of the item, the `tag` section. It goes through a dedicated slot rather
    /// than `PIER_ISTR_USER_DATA`: the content is the same and the dedicated slot saves one
    /// property-number dispatch on the host side.
    pub fn user_data(&self) -> Result<NbtValue> {
        let f = crate::require_slot!(item_get_user_data, "reading the custom NBT of an item");
        let text =
            call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), ctx, sink) }).ok_or_else(|| {
                Error("the host could not read the custom NBT of this item".to_owned())
            })?;
        NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the custom NBT of the item failed: {e}")))
    }

    /// The color as `{r,g,b}`. Only a dyeable item has one.
    pub fn color(&self) -> Result<(i32, i32, i32)> {
        let text = self.text(sys::PIER_ISTR_COLOR)?;
        let v = NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the color SNBT failed: {e}")))?;
        Ok((v.get_i32("r")?, v.get_i32("g")?, v.get_i32("b")?))
    }

    // Transforms

    /// Runs one `PIER_IOP_*` transform and replaces itself with the result.
    ///
    /// On failure it stays unchanged: with no new SNBT from the host there is nothing to write
    /// back, and writing half a result in is harder to diagnose than doing nothing.
    pub fn transform(&mut self, op: i32, sarg: &str, narg: f64) -> Result<()> {
        let f = crate::require_slot!(item_transform, "transforming an item");
        let out =
            call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), op, s(sarg), narg, ctx, sink) })
                .ok_or_else(|| {
                    Error(format!("item transform {op} failed: the operation number is unrecognized, or an argument is invalid"))
                })?;
        self.snbt = out;
        Ok(())
    }

    /// As above, returning a new item and leaving this one unchanged.
    pub fn transformed(&self, op: i32, sarg: &str, narg: f64) -> Result<ItemStack> {
        let mut copy = self.clone();
        copy.transform(op, sarg, narg)?;
        Ok(copy)
    }

    pub fn set_custom_name(&mut self, name: &str) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_CUSTOM_NAME, name, 0.0)
    }
    pub fn reset_name(&mut self) -> Result<()> {
        self.transform(sys::PIER_IOP_RESET_NAME, "", 0.0)
    }
    pub fn set_damage(&mut self, damage: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_DAMAGE, "", damage as f64)
    }
    pub fn set_count(&mut self, count: u8) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_COUNT, "", count as f64)
    }
    pub fn set_unbreakable(&mut self, on: bool) -> Result<()> {
        self.transform(
            sys::PIER_IOP_SET_UNBREAKABLE,
            "",
            if on { 1.0 } else { 0.0 },
        )
    }
    pub fn hurt_and_break(&mut self, damage: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_HURT_AND_BREAK, "", damage as f64)
    }
    pub fn set_repair_cost(&mut self, cost: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_REPAIR_COST, "", cost as f64)
    }
    pub fn clear_lore(&mut self) -> Result<()> {
        self.transform(sys::PIER_IOP_CLEAR_LORE, "", 0.0)
    }
    pub fn remove_enchants(&mut self) -> Result<()> {
        self.transform(sys::PIER_IOP_REMOVE_ENCHANTS, "", 0.0)
    }

    pub fn set_lore(&mut self, lines: &[&str]) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_LORE, &str_list_snbt(lines), 0.0)
    }
    pub fn set_can_destroy(&mut self, blocks: &[&str]) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_CAN_DESTROY, &str_list_snbt(blocks), 0.0)
    }
    pub fn set_can_place_on(&mut self, blocks: &[&str]) -> Result<()> {
        self.transform(sys::PIER_IOP_SET_CAN_PLACE_ON, &str_list_snbt(blocks), 0.0)
    }

    /// Adds an enchantment. A level of 0 removes it, as far as the engine is concerned.
    pub fn add_enchant(&mut self, id: &str, level: i32) -> Result<()> {
        self.transform(sys::PIER_IOP_ADD_ENCHANT, &format!("{id}:{level}"), 0.0)
    }

    // Enchantments and comparison

    pub fn enchants(&self) -> Result<Vec<Enchant>> {
        let f = crate::require_slot!(item_get_enchants, "reading the enchantments of an item");
        let text =
            call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), ctx, sink) }).ok_or_else(|| {
                Error("the host could not read the enchantments of this item".to_owned())
            })?;
        let v = NbtValue::parse(&text)
            .map_err(|e| Error(format!("parsing the enchantment SNBT failed: {e}")))?;
        let Some(items) = v.as_list() else {
            return Err(Error(format!(
                "the enchantment SNBT is not a list but {}",
                v.type_name()
            )));
        };
        Ok(items
            .iter()
            .filter_map(|e| {
                let id = match e.get("id") {
                    // The id may be a number or a string, and both are accepted.
                    Some(NbtValue::String(s)) => s.clone(),
                    Some(other) => other.as_i64()?.to_string(),
                    None => return None,
                };
                Some(Enchant {
                    id,
                    level: e.opt_i32("level").unwrap_or(1),
                })
            })
            .collect())
    }

    /// Replaces the whole enchantment set and returns a new item.
    pub fn with_enchants(&self, enchants: &[Enchant]) -> Result<ItemStack> {
        let f = crate::require_slot!(item_set_enchants, "writing the enchantments of an item");
        let list = NbtValue::list(enchants.iter().map(|e| {
            NbtValue::obj([
                ("id", NbtValue::from(e.id.as_str())),
                ("level", NbtValue::Int(e.level)),
            ])
        }))
        .to_snbt();
        let out = call_out_str(|ctx, sink| unsafe { f(s(&self.snbt), s(&list), ctx, sink) })
            .ok_or_else(|| Error("the host refused to write the enchantments: an enchantment name is unrecognized, or the item cannot be enchanted".to_owned()))?;
        Ok(ItemStack { snbt: out })
    }

    /// Whether two items are the same kind of thing.
    ///
    /// The criterion comes from the engine, `ItemStack::matches`, and is not string equality:
    /// fields such as count and durability take no part, while comparing SNBT text would
    /// include them.
    /// A missing slot returns `Err` and does not fall back to a text comparison, which would
    /// make the criterion differ between hosts.
    pub fn matches(&self, other: &ItemStack) -> Result<bool> {
        let f = crate::require_slot!(item_matches, "comparing two items");
        Ok(unsafe { f(s(&self.snbt), s(&other.snbt)) })
    }
}

impl std::fmt::Display for ItemStack {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.snbt)
    }
}

impl From<&str> for ItemStack {
    fn from(v: &str) -> ItemStack {
        ItemStack::from_snbt(v)
    }
}

impl From<String> for ItemStack {
    fn from(v: String) -> ItemStack {
        ItemStack::from_snbt(v)
    }
}

/// Assembles an SNBT string list.
pub(crate) fn str_list_snbt(items: &[&str]) -> String {
    NbtValue::list(items.iter().map(|s| NbtValue::from(*s))).to_snbt()
}

/// Parses an SNBT string list.
///
/// An empty string counts as an empty list: for an item with no lore the host sinks an
/// empty string while `NbtValue::parse("")` fails, and reporting absence as a parse error
/// is a false alarm.
pub(crate) fn parse_str_list(text: &str, what: &str) -> Result<Vec<String>> {
    if text.trim().is_empty() {
        return Ok(Vec::new());
    }
    let v = NbtValue::parse(text)
        .map_err(|e| Error(format!("parsing the SNBT of {what} failed: {e}")))?;
    let Some(items) = v.as_list() else {
        return Err(Error(format!("{what} is not a list but {}", v.type_name())));
    };
    Ok(items
        .iter()
        .map(|e| match e.as_str() {
            Some(s) => s.to_owned(),
            None => e.to_snbt(),
        })
        .collect())
}
