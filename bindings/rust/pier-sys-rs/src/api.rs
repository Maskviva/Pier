//! `PierApi`: the function table the host hands to a mod, cell for cell against `sdk/abi.h`.
//! Written by hand rather than generated: half the value of `abi.h` lives in the per-slot comments,
//! and a generator carries signatures but not reasons. The cost is one failure mode, and it is the
//! worst one. A slot is appended at the end, the mirror does not follow, every slot after it is off
//! by one, a call to `bus_publish` lands on a different function pointer, there is no diagnostic at
//! all, and both sides still compile. The `sys-mirrors-abi` check exists for exactly that.
//! There is no conditional compilation anywhere. Contract §2.1: the layout is identical on every
//! build target and an absent capability is a NULL slot. The `client_*` and `md_*` slots always
//! occupy their places and are `None` when the matching capability package was not compiled into
//! the host. A `#[cfg]` in the mirror is refused by the check.
//! Each slot is an `Option<fn>`: an empty slot is the formal way to say the capability was not
//! compiled into the host and is not an exception. It has the same layout as a raw function pointer
//! through the null pointer optimization, so a non-null test costs no byte.
//!
//! `struct_size` is checked before a call as well. Neither gate may be skipped; see `require_slot!`
//! in `pier-rs` and contract §2.2.

#![allow(clippy::type_complexity)]

use core::ffi::c_void;

use crate::types::*;

/// The function table the host hands to a mod at load time. The pointer stays valid for
/// the whole lifetime of the mod.
#[repr(C)]
pub struct PierApi {
    pub struct_size: u32,
    pub abi_version: u32,
    pub host_flags: u32,
    pub _reserved0: u32,
    pub log: Option<unsafe extern "C" fn(PierModHandle, i32, PierStr)>,
    pub gaming_status: Option<unsafe extern "C" fn() -> i32>,
    pub schedule: Option<unsafe extern "C" fn(PierTaskCb, *mut c_void)>,
    pub schedule_after: Option<unsafe extern "C" fn(PierTaskCb, *mut c_void, u64)>,
    pub subscribe_event: Option<
        unsafe extern "C" fn(
            PierModHandle,
            PierStr,
            i32,
            PierEventCb,
            *mut c_void,
        ) -> PierListenerHandle,
    >,
    pub unsubscribe_event: Option<unsafe extern "C" fn(PierModHandle, PierListenerHandle) -> bool>,
    pub list_events: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub execute_command:
        Option<unsafe extern "C" fn(PierStr, *mut c_void, PierCmdOutputSink) -> bool>,
    pub register_command: Option<
        unsafe extern "C" fn(
            PierModHandle,
            PierStr,
            PierStr,
            i32,
            PierCommandCb,
            *mut c_void,
        ) -> bool,
    >,
    pub get_current_tick: Option<unsafe extern "C" fn() -> u64>,
    pub get_tick_delta_time: Option<unsafe extern "C" fn() -> f64>,
    pub get_player_count: Option<unsafe extern "C" fn() -> i32>,
    pub get_sim_paused: Option<unsafe extern "C" fn() -> bool>,
    pub spawn_particle: Option<unsafe extern "C" fn(i32, PierStr, f64, f64, f64) -> bool>,
    pub get_player_position: Option<unsafe extern "C" fn(PierStr) -> PierPlayerPos>,

    // Appended.
    pub scan_region: Option<
        unsafe extern "C" fn(
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            *mut c_void,
            PierBlockSink,
            PierEntitySink,
        ) -> bool,
    >,
    pub get_block:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierBlockSink) -> bool>,
    pub set_block: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr) -> bool>,
    pub get_time: Option<unsafe extern "C" fn(*mut i64) -> bool>,

    // The append area: append only, never reorder.

    // ── §A world read/write & clock
    pub set_time: Option<unsafe extern "C" fn(i64) -> bool>,
    pub set_weather: Option<unsafe extern "C" fn(i32) -> bool>,
    pub list_players: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub player_resolve: Option<unsafe extern "C" fn(PierPlayerSel, *mut PierActorId) -> bool>,
    pub player_send_message: Option<unsafe extern "C" fn(PierPlayerSel, PierStr) -> bool>,

    // ── §B player management
    pub player_disconnect: Option<unsafe extern "C" fn(PierPlayerSel, PierStr) -> bool>,
    pub broadcast_message: Option<unsafe extern "C" fn(PierStr)>,
    pub player_set_gamemode: Option<unsafe extern "C" fn(PierPlayerSel, i32) -> bool>,
    pub player_teleport: Option<unsafe extern "C" fn(PierPlayerSel, i32, f64, f64, f64) -> bool>,
    pub player_get_num: Option<unsafe extern "C" fn(PierPlayerSel, i32, *mut f64) -> bool>,
    pub player_get_str:
        Option<unsafe extern "C" fn(PierPlayerSel, i32, *mut c_void, PierStrSink) -> bool>,
    pub player_set_num: Option<unsafe extern "C" fn(PierPlayerSel, i32, f64) -> bool>,
    pub player_action: Option<
        unsafe extern "C" fn(
            PierPlayerSel,
            i32,
            PierStr,
            f64,
            f64,
            f64,
            *mut c_void,
            PierStrSink,
        ) -> bool,
    >,
    pub list_actors: Option<unsafe extern "C" fn(i32, *mut c_void, PierActorSink)>,
    pub actor_snapshot: Option<unsafe extern "C" fn(PierActorId, *mut c_void, PierStrSink) -> bool>,
    pub actor_get_num: Option<unsafe extern "C" fn(PierActorId, i32, *mut f64) -> bool>,

    // ── §C actors (players resolve here too, via player_resolve)
    pub actor_get_str:
        Option<unsafe extern "C" fn(PierActorId, i32, *mut c_void, PierStrSink) -> bool>,
    pub actor_action: Option<
        unsafe extern "C" fn(
            PierActorId,
            i32,
            PierStr,
            f64,
            f64,
            f64,
            *mut c_void,
            PierStrSink,
        ) -> bool,
    >,
    pub spawn_mob:
        Option<unsafe extern "C" fn(i32, PierStr, f64, f64, f64, *mut PierActorId) -> bool>,
    pub explode: Option<
        unsafe extern "C" fn(i32, f64, f64, f64, f32, f32, PierActorId, bool, bool, bool) -> bool,
    >,
    pub block_get_num: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, *mut f64) -> bool>,
    pub block_get_str:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub block_action: Option<
        unsafe extern "C" fn(i32, i32, i32, i32, i32, PierStr, *mut c_void, PierStrSink) -> bool,
    >,

    // ── §D blocks & block entities
    pub block_entity_snbt:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub item_get_num: Option<unsafe extern "C" fn(PierStr, i32, *mut f64) -> bool>,
    pub item_get_str: Option<unsafe extern "C" fn(PierStr, i32, *mut c_void, PierStrSink) -> bool>,
    pub item_transform:
        Option<unsafe extern "C" fn(PierStr, i32, PierStr, f64, *mut c_void, PierStrSink) -> bool>,

    // ── §E items (SNBT value objects) & containers
    pub container_size: Option<unsafe extern "C" fn(PierContainerRef, *mut i32) -> bool>,
    pub container_get_item:
        Option<unsafe extern "C" fn(PierContainerRef, i32, *mut c_void, PierStrSink) -> bool>,
    pub container_set_item: Option<unsafe extern "C" fn(PierContainerRef, i32, PierStr) -> bool>,
    pub container_add_item: Option<unsafe extern "C" fn(PierContainerRef, PierStr) -> bool>,
    pub container_remove_item: Option<unsafe extern "C" fn(PierContainerRef, i32, i32) -> bool>,
    pub container_clear: Option<unsafe extern "C" fn(PierContainerRef) -> bool>,
    pub scoreboard_op:
        Option<unsafe extern "C" fn(i32, PierStr, PierStr, i64, *mut c_void, PierStrSink) -> bool>,
    pub form_send: Option<
        unsafe extern "C" fn(
            PierModHandle,
            PierPlayerSel,
            i32,
            PierStr,
            PierFormResultCb,
            *mut c_void,
        ) -> bool,
    >,
    pub register_command_ex: Option<
        unsafe extern "C" fn(
            PierModHandle,
            PierStr,
            PierStr,
            i32,
            PierStr,
            PierCommandCb,
            *mut c_void,
        ) -> bool,
    >,

    // ── §F scoreboard
    pub register_command_enum: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,

    // ── §G forms (async result callback)
    pub register_command_soft_enum: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,

    // ── §H parameterized commands & enums
    pub update_command_soft_enum: Option<unsafe extern "C" fn(PierStr, i32, PierStr) -> bool>,
    pub nbt_snbt_to_binary:
        Option<unsafe extern "C" fn(PierStr, i32, *mut c_void, PierBytesSink) -> bool>,
    pub nbt_binary_to_snbt:
        Option<unsafe extern "C" fn(*const u8, usize, i32, *mut c_void, PierStrSink) -> bool>,
    pub kvdb_open: Option<unsafe extern "C" fn(PierModHandle, PierStr, bool) -> PierKvDbHandle>,

    // ── §I NBT binary, KvDb (thread-safe), system & server info
    pub kvdb_close: Option<unsafe extern "C" fn(PierKvDbHandle)>,
    pub kvdb_get:
        Option<unsafe extern "C" fn(PierKvDbHandle, PierStr, *mut c_void, PierStrSink) -> bool>,
    pub kvdb_set: Option<unsafe extern "C" fn(PierKvDbHandle, PierStr, PierStr) -> bool>,
    pub kvdb_del: Option<unsafe extern "C" fn(PierKvDbHandle, PierStr) -> bool>,
    pub kvdb_has: Option<unsafe extern "C" fn(PierKvDbHandle, PierStr) -> bool>,
    pub kvdb_is_empty: Option<unsafe extern "C" fn(PierKvDbHandle) -> bool>,
    pub kvdb_iter: Option<unsafe extern "C" fn(PierKvDbHandle, *mut c_void, PierKvSink)>,
    pub sys_info_str: Option<unsafe extern "C" fn(i32, *mut c_void, PierStrSink) -> bool>,
    pub sys_get_env: Option<unsafe extern "C" fn(PierStr, *mut c_void, PierStrSink) -> bool>,
    pub sys_set_env: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,
    pub sys_is_wine: Option<unsafe extern "C" fn() -> bool>,
    pub get_difficulty: Option<unsafe extern "C" fn(*mut i32) -> bool>,
    pub set_difficulty: Option<unsafe extern "C" fn(i32) -> bool>,
    pub get_seed: Option<unsafe extern "C" fn(*mut i64) -> bool>,
    pub game_rule_get: Option<unsafe extern "C" fn(PierStr, *mut c_void, PierStrSink) -> bool>,
    pub game_rule_set: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,
    pub server_info_str: Option<unsafe extern "C" fn(i32, *mut c_void, PierStrSink) -> bool>,
    pub spawn_particle_for:
        Option<unsafe extern "C" fn(PierPlayerSel, i32, PierStr, f64, f64, f64) -> bool>,
    pub send_packet: Option<unsafe extern "C" fn(PierPlayerSel, i32, *const u8, usize) -> bool>,
    pub tick_freeze: Option<unsafe extern "C" fn(bool) -> bool>,
    pub tick_step: Option<unsafe extern "C" fn(u32) -> bool>,
    pub tick_warp: Option<unsafe extern "C" fn(f64) -> bool>,
    pub profile_begin: Option<unsafe extern "C" fn(u32) -> bool>,
    pub profile_take: Option<unsafe extern "C" fn(*mut c_void, PierStrSink) -> bool>,
    pub sim_spawn: Option<unsafe extern "C" fn(PierStr, i32, f64, f64, f64) -> bool>,
    pub sim_do: Option<unsafe extern "C" fn(PierPlayerSel, PierStr, PierStr) -> bool>,
    pub sim_is: Option<unsafe extern "C" fn(PierPlayerSel) -> bool>,
    pub sim_list: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub villages: Option<unsafe extern "C" fn(i32, *mut c_void, PierStrSink)>,
    pub structures_near:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, *mut c_void, PierStrSink)>,
    pub player_send_message_typed:
        Option<unsafe extern "C" fn(PierPlayerSel, PierStr, i32) -> bool>,
    pub get_money: Option<unsafe extern "C" fn(PierStr) -> i64>,
    pub set_money: Option<unsafe extern "C" fn(PierStr, i64) -> bool>,
    pub add_money: Option<unsafe extern "C" fn(PierStr, i64) -> bool>,
    pub reduce_money: Option<unsafe extern "C" fn(PierStr, i64) -> bool>,
    pub trans_money: Option<unsafe extern "C" fn(PierStr, PierStr, i64, PierStr) -> bool>,
    pub money_get_hist: Option<unsafe extern "C" fn(PierStr, i32, *mut c_void, PierStrSink)>,
    pub money_clear_hist: Option<unsafe extern "C" fn(i32)>,
    pub money_listen_before_event: Option<unsafe extern "C" fn(PierMoneyCb)>,
    pub money_listen_after_event: Option<unsafe extern "C" fn(PierMoneyCb)>,
    pub money_ranking: Option<unsafe extern "C" fn(u16, *mut c_void, PierStrSink)>,
    pub player_get_carried_item:
        Option<unsafe extern "C" fn(PierPlayerSel, *mut c_void, PierStrSink) -> bool>,
    pub player_get_item:
        Option<unsafe extern "C" fn(PierPlayerSel, i32, *mut c_void, PierStrSink) -> bool>,
    pub player_set_item: Option<unsafe extern "C" fn(PierPlayerSel, i32, PierStr) -> bool>,
    pub player_get_equipment:
        Option<unsafe extern "C" fn(PierPlayerSel, *mut c_void, PierStrSink) -> bool>,
    pub player_get_cooldown: Option<unsafe extern "C" fn(PierPlayerSel, PierStr) -> i32>,
    pub player_start_cooldown: Option<unsafe extern "C" fn(PierPlayerSel, PierStr, i32) -> bool>,
    pub player_get_network_status:
        Option<unsafe extern "C" fn(PierPlayerSel, *mut c_void, PierStrSink) -> bool>,
    pub actor_get_vehicle: Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,
    pub actor_get_first_passenger:
        Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,
    pub actor_get_owner: Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,

    // Appended: API gap fill, gated by struct_size.

    // ── Player: equipment, cooldown, network (dedicated fns)
    pub actor_get_target: Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,
    pub actor_get_equipped_item:
        Option<unsafe extern "C" fn(PierActorId, i32, *mut c_void, PierStrSink) -> bool>,
    pub actor_set_equipped_item: Option<unsafe extern "C" fn(PierActorId, i32, PierStr) -> bool>,
    pub actor_get_effects:
        Option<unsafe extern "C" fn(PierActorId, *mut c_void, PierStrSink) -> bool>,
    pub actor_get_status_flag: Option<unsafe extern "C" fn(PierActorId, i32) -> bool>,
    pub actor_set_status_flag: Option<unsafe extern "C" fn(PierActorId, i32, bool) -> bool>,
    pub actor_trace_ray: Option<
        unsafe extern "C" fn(PierActorId, f32, bool, bool, *mut c_void, PierStrSink) -> bool,
    >,

    // ── Actor: relationships, equipment, effects, geometry (dedicated fns)
    pub actor_distance_to: Option<unsafe extern "C" fn(PierActorId, PierActorId, *mut f64) -> bool>,
    pub actor_get_aabb: Option<unsafe extern "C" fn(PierActorId, *mut c_void, PierStrSink) -> bool>,
    pub actor_clone:
        Option<unsafe extern "C" fn(PierActorId, i32, f64, f64, f64, *mut PierActorId) -> bool>,
    pub block_get_state:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, *mut c_void, PierStrSink) -> bool>,
    pub block_set_state: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, PierStr) -> bool>,
    pub block_get_collision_shape:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub item_get_enchants: Option<unsafe extern "C" fn(PierStr, *mut c_void, PierStrSink) -> bool>,
    pub item_set_enchants:
        Option<unsafe extern "C" fn(PierStr, PierStr, *mut c_void, PierStrSink) -> bool>,
    pub item_matches: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,
    pub item_get_user_data: Option<unsafe extern "C" fn(PierStr, *mut c_void, PierStrSink) -> bool>,
    pub level_get_biome:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub level_get_default_spawn: Option<unsafe extern "C" fn(*mut i32, *mut i32, *mut i32) -> bool>,
    pub level_set_default_spawn: Option<unsafe extern "C" fn(i32, i32, i32) -> bool>,

    // ── Block: state get/set, collision shape (dedicated fns)
    pub level_save: Option<unsafe extern "C" fn() -> bool>,
    pub level_get_sleep_status: Option<unsafe extern "C" fn(*mut c_void, PierStrSink) -> bool>,
    pub level_update_weather: Option<unsafe extern "C" fn(f32, i32, f32, i32) -> bool>,

    // ── Item: enchants, matching, NBT (dedicated fns)
    pub level_find_path:
        Option<unsafe extern "C" fn(PierActorId, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub packet_hook_register: Option<
        unsafe extern "C" fn(PierModHandle, i32, PierPacketCb, *mut c_void) -> PierPacketHookHandle,
    >,
    pub packet_hook_unregister:
        Option<unsafe extern "C" fn(PierModHandle, PierPacketHookHandle) -> bool>,
    pub packet_conn_hook_register: Option<
        unsafe extern "C" fn(PierModHandle, PierConnCb, *mut c_void) -> PierPacketHookHandle,
    >,

    // ── Level: biome, spawn, save, weather, path, sleep (dedicated fns)
    pub packet_conn_hook_unregister:
        Option<unsafe extern "C" fn(PierModHandle, PierPacketHookHandle) -> bool>,
    pub client_get_local_player: Option<unsafe extern "C" fn(*mut c_void, PierStrSink) -> bool>,
    pub client_is_in_level: Option<unsafe extern "C" fn() -> bool>,
    pub client_get_screen_name: Option<unsafe extern "C" fn(*mut c_void, PierStrSink) -> bool>,
    pub client_register_key: Option<
        unsafe extern "C" fn(
            PierModHandle,
            PierStr,
            *const i32,
            i32,
            bool,
            PierKeyCb,
            PierKeyCb,
            *mut c_void,
        ) -> PierKeyHandle,
    >,
    pub client_unregister_key: Option<unsafe extern "C" fn(PierKeyHandle) -> bool>,
    pub client_get_key_codes:
        Option<unsafe extern "C" fn(PierKeyHandle, *mut c_void, PierStrSink) -> bool>,

    // Packet interception, appended and gated by struct_size.
    pub md_is_available: Option<unsafe extern "C" fn() -> bool>,
    pub md_add_simple_dimension: Option<unsafe extern "C" fn(PierStr, u32, i32) -> i32>,
    pub md_set_dimension_rule: Option<unsafe extern "C" fn(i32, i32, bool)>,
    pub md_get_dimension_rule: Option<unsafe extern "C" fn(i32, i32, *mut bool) -> bool>,
    pub md_clear_dimension_rules: Option<unsafe extern "C" fn(i32)>,

    // Capability group: client (client_*). All NULL on a server host.
    pub md_get_dimension_id: Option<unsafe extern "C" fn(PierStr) -> i32>,
    pub md_add_plot_dimension: Option<unsafe extern "C" fn(PierStr, u32, PierStr) -> i32>,
    pub schedule_for: Option<unsafe extern "C" fn(PierModHandle, PierTaskCb, *mut c_void) -> u64>,
    pub schedule_after_for:
        Option<unsafe extern "C" fn(PierModHandle, PierTaskCb, *mut c_void, u64) -> u64>,
    pub schedule_cancel: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub schedule_pending_count: Option<unsafe extern "C" fn(PierModHandle) -> u32>,
    pub container_refresh: Option<unsafe extern "C" fn(PierContainerRef) -> bool>,

    // Capability group: custom dimensions (md_*). All NULL when pier-dimensions was
    pub player_send_title:
        Option<unsafe extern "C" fn(PierPlayerSel, i32, PierStr, i32, i32, i32) -> bool>,
    pub bus_subscribe:
        Option<unsafe extern "C" fn(PierModHandle, PierStr, PierBusCb, *mut c_void) -> u64>,
    pub bus_unsubscribe: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub bus_publish: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierStr) -> u32>,
    pub bus_publish_vetoable:
        Option<unsafe extern "C" fn(PierModHandle, PierStr, PierStr, *mut u32) -> bool>,
    pub bus_subscriber_count: Option<unsafe extern "C" fn(PierStr) -> u32>,
    pub md_set_plot_grid: Option<unsafe extern "C" fn(i32, i32, i32)>,
    pub md_clear_plot_grid: Option<unsafe extern "C" fn(i32)>,
    pub md_set_plot_merges: Option<unsafe extern "C" fn(i32, *const i32, i32)>,
    pub service_register:
        Option<unsafe extern "C" fn(PierModHandle, PierStr, PierServiceCb, *mut c_void) -> u64>,

    // The tail of the append area, gated by struct_size.

    // ── Mod-scoped scheduling
    pub service_unregister: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub service_call: Option<
        unsafe extern "C" fn(PierModHandle, PierStr, PierStr, *mut c_void, PierStrSink) -> i32,
    >,
    pub service_list: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub edit_set_block_nbt: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, i32) -> bool>,

    // ── Client-side container resync
    pub edit_set_block_states:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, PierStr, i32) -> bool>,
    pub edit_set_block_entity: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr) -> bool>,

    // ── Titles
    pub edit_spawn_entity_nbt:
        Option<unsafe extern "C" fn(i32, PierStr, bool, f64, f64, f64, *mut PierActorId) -> bool>,
    pub edit_trace_ray: Option<
        unsafe extern "C" fn(PierActorId, f32, bool, bool, *mut c_void, PierStrSink) -> bool,
    >,
    pub lane_publish:
        Option<unsafe extern "C" fn(PierModHandle, PierStr, *const PierLaneDesc) -> u64>,
    pub lane_unpublish: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,

    // ── Cross-mod event bus
    pub lane_acquire:
        Option<unsafe extern "C" fn(PierModHandle, PierStr, u64, *mut PierLaneRef) -> i32>,
    pub lane_release: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub lane_list: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub md_list_dimensions: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub level_delete_chunk_keys: Option<unsafe extern "C" fn(i32, i32, i32) -> i32>,
    pub level_chunks_loaded: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32) -> i32>,
    pub player_conn_id: Option<unsafe extern "C" fn(PierPlayerSel) -> u64>,

    // ── Plot-boundary confinement
    pub level_chunk_keys:
        Option<unsafe extern "C" fn(i32, i32, i32, *mut c_void, PierStrSink) -> i32>,
    pub level_delete_key: Option<unsafe extern "C" fn(PierStr) -> bool>,
    pub level_set_biome: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, PierStr) -> i32>,
    pub get_extra_block:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub set_extra_block: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, i32) -> bool>,

    // ── Tick statistics
    pub get_tps: Option<unsafe extern "C" fn(i32) -> f64>,
    pub get_mspt: Option<unsafe extern "C" fn(i32) -> f64>,

    // ── Packet interception filtered by id
    pub packet_hook_register_ids: Option<
        unsafe extern "C" fn(
            PierModHandle,
            i32,
            *const i32,
            usize,
            PierPacketCb,
            *mut c_void,
        ) -> PierPacketHookHandle,
    >,

    // ── Bulk block reads and writes
    pub scan_region_indexed: Option<
        unsafe extern "C" fn(
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            *mut c_void,
            PierPaletteSink,
            PierCellSink,
        ) -> bool,
    >,
    pub edit_fill_region:
        Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, i32, i32, PierStr, i32) -> i64>,
    pub edit_set_blocks: Option<
        unsafe extern "C" fn(i32, *const PierStr, u32, *const PierBlockCell, usize, i32) -> i64,
    >,
    pub container_get_items:
        Option<unsafe extern "C" fn(PierContainerRef, *mut c_void, PierSlotSink) -> bool>,
}

impl PierApi {
    /// Whether the host table is long enough to cover a given byte offset.
    ///
    /// This is the only basis for forward compatibility (contract §2.2): with an old host
    /// and a new mod, the slots the new mod cannot reach are simply not in the memory the
    /// host allocated. Length is checked first, then the slot for non-null.
    ///
    /// `offset_of!` is used rather than counting bytes by hand, because a hand-counted
    /// number goes quietly wrong on the next append, and it goes wrong in the direction of
    /// believing the table is long enough.
    #[inline]
    pub fn covers(&self, offset: usize, size: usize) -> bool {
        (self.struct_size as usize) >= offset + size
    }

    /// Whether the host was built for the client target.
    #[inline]
    pub fn is_client_host(&self) -> bool {
        (self.host_flags & crate::PIER_FLAG_CLIENT) != 0
    }
}
