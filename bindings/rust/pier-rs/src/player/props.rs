//! Player property accessors, generated from the `PIER_PPROP_*` and `PIER_PSTR_*` constant
//! tables.
//!
//! Every one of these methods is one slot plus one constant, which makes them a table
//! rather than a wall. Anything needing more explanation, whether a write, a combination or
//! failure semantics, stays hand-written in `player/mod.rs`.

use crate::player::Player;
use crate::rt::accessors::accessors;

accessors! { Player;
    /// The account name.
    str  real_name                  = PIER_PSTR_REAL_NAME;
    str  uuid                       = PIER_PSTR_UUID;
    str  xuid                       = PIER_PSTR_XUID;
    /// `address:port`. IPv6 has the same shape, so it must not be split on the last colon.
    str  ip_and_port                = PIER_PSTR_IP_AND_PORT;
    str  locale_code                = PIER_PSTR_LOCALE_CODE;
    /// The name shown above the head, which can be changed. An identity decision uses
    /// [`Player::xuid`].
    str  name_tag                   = PIER_PSTR_NAME_TAG;
    str  platform_online_id         = PIER_PSTR_PLATFORM_ONLINE_ID;
    i32  dimension                  = PIER_PPROP_DIMENSION;
    /// The experience level.
    i32  level                      = PIER_PPROP_LEVEL;
    /// The progress of the experience bar, from 0 to 1. It is not the accumulated experience.
    f64  experience                 = PIER_PPROP_EXPERIENCE;
    f64  hunger                     = PIER_PPROP_HUNGER;
    /// The saturation; hunger only starts dropping once it is spent.
    f64  saturation                 = PIER_PPROP_SATURATION;
    /// The exhaustion; filling one unit costs a point of saturation.
    f64  exhaustion                 = PIER_PPROP_EXHAUSTION;
    /// How much experience remains to the next level.
    i32  xp_needed_for_next_level   = PIER_PPROP_XP_NEEDED_NEXT_LEVEL;
    f64  luck                       = PIER_PPROP_LUCK;
    i32  selected_slot              = PIER_PPROP_SELECTED_SLOT;
    /// The `score` pseudo-objective of the scoreboard, not any custom objective.
    i32  score                      = PIER_PPROP_SCORE;
    /// The view distance the client requested, in chunks.
    i32  chunk_radius               = PIER_PPROP_CHUNK_RADIUS;
    i32  enchantment_seed           = PIER_PPROP_ENCHANTMENT_SEED;
    /// The value of `BuildPlatform`, not an operating system name.
    i32  platform                   = PIER_PPROP_PLATFORM;
    /// The facing: 0 is south, 1 west, 2 north and 3 east.
    i32  direction                  = PIER_PPROP_DIRECTION;
    /// The round-trip latency in milliseconds. For the detail see [`Player::network_status`].
    i32  ping                       = PIER_PPROP_NETWORK_RTT;
    i32  client_sub_id              = PIER_PPROP_CLIENT_SUB_ID;
    f64  fall_distance              = PIER_PPROP_FALL_DISTANCE;
    /// Listed as an operator in `permissions.json`. Not the same thing as
    /// [`Player::permission_level`], which can change at runtime.
    bool is_operator                = PIER_PPROP_IS_OPERATOR;
    bool can_use_operator_blocks    = PIER_PPROP_CAN_USE_OPERATOR_BLOCKS;
    bool is_flying                  = PIER_PPROP_IS_FLYING;
    bool can_jump                   = PIER_PPROP_CAN_JUMP;
    bool is_emoting                 = PIER_PPROP_IS_EMOTING;
    bool is_in_raid                 = PIER_PPROP_IS_IN_RAID;
    /// Currently inside the invulnerability frames after taking damage.
    bool is_hurt                    = PIER_PPROP_IS_HURT;
    bool is_scoping                 = PIER_PPROP_IS_SCOPING;
    bool can_sleep                  = PIER_PPROP_CAN_SLEEP;
    bool has_respawn_position       = PIER_PPROP_HAS_RESPAWN_POSITION;
    bool is_using_item              = PIER_PPROP_IS_USING_ITEM;
    bool is_blocking                = PIER_PPROP_IS_BLOCKING;
    bool is_gliding                 = PIER_PPROP_IS_GLIDING;
    bool is_swimming                = PIER_PPROP_IS_SWIMMING;
    bool is_dead                    = PIER_PPROP_IS_DEAD;
    /// Has died at least once in this save.
    bool has_died_before            = PIER_PPROP_HAS_DIED_BEFORE;
}
