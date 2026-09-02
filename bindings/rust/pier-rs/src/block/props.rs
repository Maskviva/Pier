//! Block property accessors, generated from the `PIER_BPROP_*` and `PIER_BSTR_*` constant
//! tables.
//!
//! The one-slot-plus-one-constant ones live here. Anything with failure semantics, needing
//! several calls combined, or going in the write direction stays hand-written in
//! `block/mod.rs`.

use crate::block::Block;
use crate::rt::accessors::accessors;

accessors! { Block;
    str  name                     = PIER_BSTR_TYPE_NAME;
    str  snbt                     = PIER_BSTR_SNBT;
    /// The localization key, not the text that is displayed.
    str  description_id           = PIER_BSTR_DESCRIPTION_ID;
    /// The localized display name.
    str  display_name             = PIER_BSTR_DISPLAY_NAME;
    /// The engine's own debug string. Its format follows the version, so no decision rests on
    /// it.
    str  debug_string             = PIER_BSTR_DEBUG_STRING;
    bool is_air                   = PIER_BPROP_IS_AIR;
    bool is_solid                 = PIER_BPROP_IS_SOLID;
    bool is_container             = PIER_BPROP_IS_CONTAINER;
    bool is_door                  = PIER_BPROP_IS_DOOR;
    bool is_fence                 = PIER_BPROP_IS_FENCE;
    bool is_rail                  = PIER_BPROP_IS_RAIL;
    bool is_slab                  = PIER_BPROP_IS_SLAB;
    bool is_stair                 = PIER_BPROP_IS_STAIR;
    bool is_wall                  = PIER_BPROP_IS_WALL;
    bool is_crop                  = PIER_BPROP_IS_CROP;
    bool is_unbreakable           = PIER_BPROP_IS_UNBREAKABLE;
    bool is_crafting_block        = PIER_BPROP_IS_CRAFTING_BLOCK;
    bool is_interactive_block     = PIER_BPROP_IS_INTERACTIVE_BLOCK;
    /// It can produce a redstone signal itself, as a lever or a button does.
    bool is_signal_source         = PIER_BPROP_IS_SIGNAL_SOURCE;
    /// It drops nothing unless mined with the right tool.
    bool requires_tool            = PIER_BPROP_REQUIRES_TOOL;
    bool has_block_entity         = PIER_BPROP_HAS_BLOCK_ENTITY;
    /// The legacy data value. A newer block uses [`Block::states`].
    i32  data                     = PIER_BPROP_DATA;
    /// The `variant` data value, whose meaning differs per actor kind.
    i32  variant                  = PIER_BPROP_VARIANT;
    /// The numeric id of the matching item.
    i32  block_item_id            = PIER_BPROP_BLOCK_ITEM_ID;
    /// The actual brightness of this cell, skylight included.
    i32  light                    = PIER_BPROP_LIGHT;
    /// How much light this block emits itself.
    i32  light_emission           = PIER_BPROP_LIGHT_EMISSION;
    /// The mining hardness; larger is slower.
    f64  destroy_speed            = PIER_BPROP_DESTROY_SPEED;
    /// The blast resistance.
    f64  explosion_resistance     = PIER_BPROP_EXPLOSION_RESISTANCE;
    /// The friction coefficient; ice is one of the low ones.
    f64  friction                 = PIER_BPROP_FRICTION;
    /// The bounciness; a slime block is non-zero.
    f64  bounciness               = PIER_BPROP_BOUNCINESS;
    /// The probability weight of catching fire.
    i32  burn_odds                = PIER_BPROP_BURN_ODDS;
    /// The probability weight of spreading fire to a neighbor.
    i32  flame_odds               = PIER_BPROP_FLAME_ODDS;
    /// The redstone signal strength this cell outputs, from 0 to 15.
    i32  redstone_signal          = PIER_BPROP_REDSTONE_SIGNAL;
    /// The strength a comparator reads from this cell; a container computes it from how full
    /// it is.
    i32  comparator_signal        = PIER_BPROP_COMPARATOR_SIGNAL;
}
