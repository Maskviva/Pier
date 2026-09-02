//! Actor property accessors, generated from the `PIER_APROP_*` and `PIER_ASTR_*` constant
//! tables.
//!
//! The one-slot-plus-one-constant ones live here. Anything with failure semantics, needing
//! several calls combined, or going in the write direction stays hand-written in
//! `entity/mod.rs`.

use crate::entity::Entity;
use crate::rt::accessors::accessors;

accessors! { Entity;
    str  type_name                = PIER_ASTR_TYPE_NAME;
    /// The name shown above the head.
    str  name_tag                 = PIER_ASTR_NAME_TAG;
    /// The line below the name.
    str  score_tag                = PIER_ASTR_SCORE_TAG;
    /// The name after profanity filtering.
    str  filtered_name            = PIER_ASTR_FILTERED_NAME;
    i32  dimension                = PIER_APROP_DIMENSION;
    f64  health                   = PIER_APROP_HEALTH;
    f64  max_health               = PIER_APROP_MAX_HEALTH;
    /// The current movement speed, in blocks per tick.
    f64  speed                    = PIER_APROP_SPEED;
    /// How many blocks the current fall has covered, used to compute fall damage on landing.
    f64  fall_distance            = PIER_APROP_FALL_DISTANCE;
    /// The size scale, where 1.0 is the original size.
    f64  scale                    = PIER_APROP_SCALE;
    /// The `variant` data value, whose meaning differs per actor kind.
    i32  variant                  = PIER_APROP_VARIANT;
    /// The `mark_variant` data value, a different numbering from [`Entity::variant`].
    i32  mark_variant             = PIER_APROP_MARK_VARIANT;
    /// How many ticks of the death animation have played.
    i32  death_time               = PIER_APROP_DEATH_TIME;
    bool is_alive                 = PIER_APROP_IS_ALIVE;
    bool is_on_ground             = PIER_APROP_IS_ON_GROUND;
    bool is_in_water              = PIER_APROP_IS_IN_WATER;
    bool is_in_lava               = PIER_APROP_IS_IN_LAVA;
    bool is_on_fire               = PIER_APROP_IS_ON_FIRE;
    bool is_invisible             = PIER_APROP_IS_INVISIBLE;
    bool is_sneaking              = PIER_APROP_IS_SNEAKING;
    bool is_baby                  = PIER_APROP_IS_BABY;
    bool is_riding                = PIER_APROP_IS_RIDING;
    bool is_tame                  = PIER_APROP_IS_TAME;
    /// It is not despawned for being too far from a player.
    bool is_persistent            = PIER_APROP_IS_PERSISTENT;
    bool is_leashed               = PIER_APROP_IS_LEASHED;
    bool is_invulnerable          = PIER_APROP_IS_INVULNERABLE;
    bool is_frozen                = PIER_APROP_IS_FROZEN;
    /// In the breeding state.
    bool is_in_love               = PIER_APROP_IS_IN_LOVE;
    bool is_in_rain               = PIER_APROP_IS_IN_RAIN;
    bool is_in_snow               = PIER_APROP_IS_IN_SNOW;
    bool is_in_thunderstorm       = PIER_APROP_IS_IN_THUNDERSTORM;
    /// Holding a totem of undying in a hand or the off hand.
    bool has_totem                = PIER_APROP_HAS_TOTEM;
    bool has_passenger            = PIER_APROP_HAS_PASSENGER;
}
