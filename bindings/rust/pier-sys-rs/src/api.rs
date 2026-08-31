//! `PierApi` —— 宿主交给模组的那张函数表，**逐格**对着 `sdk/abi.h`。
//!
//! 手写而不是生成：`abi.h` 的价值有一半在逐槽的注释里，生成器搬得动签名
//! 搬不动理由。代价是它有一种失效方式，而那是最坏的一种 —— 表尾追加了一个
//! 槽而镜像忘了跟，之后每一槽错位一格，调 `bus_publish` 打到别的函数指针上，
//! **没有任何诊断**，两侧还都编得过。`sys-mirrors-abi` 机检就是为它存在的。
//!
//! **没有任何条件编译。** 契约 §2.1：布局在所有构建目标下相同，能力缺席 =
//! 槽位 NULL。`client_*` 与 `md_*` 永远占位，对应能力包没编进宿主时为 `None`。
//! 镜像出现 `#[cfg]` 会被机检拒绝。
//!
//! 每个槽是 `Option<fn>`：空槽是「这个能力没编进宿主」的**正式表示**，不是
//! 异常。它和裸函数指针布局相同（空指针优化），所以非空检查不花一个字节。
//!
//! 调用之前还要查 `struct_size` —— 两道闸缺一不可，见 `pier-rs` 的
//! `require_slot!` 与契约 §2.2。

#![allow(clippy::type_complexity)]

use core::ffi::c_void;

use crate::types::*;

/// 宿主在装载时交给模组的函数表。指针在模组的整个生命周期内有效。
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
    pub subscribe_event: Option<unsafe extern "C" fn(PierModHandle, PierStr, i32, PierEventCb, *mut c_void) -> PierListenerHandle>,
    pub unsubscribe_event: Option<unsafe extern "C" fn(PierModHandle, PierListenerHandle) -> bool>,
    pub list_events: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub execute_command: Option<unsafe extern "C" fn(PierStr, *mut c_void, PierCmdOutputSink) -> bool>,
    pub register_command: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierStr, i32, PierCommandCb, *mut c_void) -> bool>,
    pub get_current_tick: Option<unsafe extern "C" fn() -> u64>,
    pub get_tick_delta_time: Option<unsafe extern "C" fn() -> f64>,
    pub get_player_count: Option<unsafe extern "C" fn() -> i32>,
    pub get_sim_paused: Option<unsafe extern "C" fn() -> bool>,
    pub spawn_particle: Option<unsafe extern "C" fn(i32, PierStr, f64, f64, f64) -> bool>,
    pub get_player_position: Option<unsafe extern "C" fn(PierStr) -> PierPlayerPos>,

    // ── 追加
    pub scan_region: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, i32, i32, *mut c_void, PierBlockSink, PierEntitySink) -> bool>,
    pub get_block: Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierBlockSink) -> bool>,
    pub set_block: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr) -> bool>,
    pub get_time: Option<unsafe extern "C" fn(*mut i64) -> bool>,

    // ── 追加区 —— 只追加，不重排

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
    pub player_get_str: Option<unsafe extern "C" fn(PierPlayerSel, i32, *mut c_void, PierStrSink) -> bool>,
    pub player_set_num: Option<unsafe extern "C" fn(PierPlayerSel, i32, f64) -> bool>,
    pub player_action: Option<unsafe extern "C" fn(PierPlayerSel, i32, PierStr, f64, f64, f64, *mut c_void, PierStrSink) -> bool>,
    pub list_actors: Option<unsafe extern "C" fn(i32, *mut c_void, PierActorSink)>,
    pub actor_snapshot: Option<unsafe extern "C" fn(PierActorId, *mut c_void, PierStrSink) -> bool>,
    pub actor_get_num: Option<unsafe extern "C" fn(PierActorId, i32, *mut f64) -> bool>,

    // ── §C actors (players resolve here too, via player_resolve)
    pub actor_get_str: Option<unsafe extern "C" fn(PierActorId, i32, *mut c_void, PierStrSink) -> bool>,
    pub actor_action: Option<unsafe extern "C" fn(PierActorId, i32, PierStr, f64, f64, f64, *mut c_void, PierStrSink) -> bool>,
    pub spawn_mob: Option<unsafe extern "C" fn(i32, PierStr, f64, f64, f64, *mut PierActorId) -> bool>,
    pub explode: Option<unsafe extern "C" fn(i32, f64, f64, f64, f32, f32, PierActorId, bool, bool, bool) -> bool>,
    pub block_get_num: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, *mut f64) -> bool>,
    pub block_get_str: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub block_action: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, PierStr, *mut c_void, PierStrSink) -> bool>,

    // ── §D blocks & block entities
    pub block_entity_snbt: Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub item_get_num: Option<unsafe extern "C" fn(PierStr, i32, *mut f64) -> bool>,
    pub item_get_str: Option<unsafe extern "C" fn(PierStr, i32, *mut c_void, PierStrSink) -> bool>,
    pub item_transform: Option<unsafe extern "C" fn(PierStr, i32, PierStr, f64, *mut c_void, PierStrSink) -> bool>,

    // ── §E items (SNBT value objects) & containers
    pub container_size: Option<unsafe extern "C" fn(PierContainerRef, *mut i32) -> bool>,
    pub container_get_item: Option<unsafe extern "C" fn(PierContainerRef, i32, *mut c_void, PierStrSink) -> bool>,
    pub container_set_item: Option<unsafe extern "C" fn(PierContainerRef, i32, PierStr) -> bool>,
    pub container_add_item: Option<unsafe extern "C" fn(PierContainerRef, PierStr) -> bool>,
    pub container_remove_item: Option<unsafe extern "C" fn(PierContainerRef, i32, i32) -> bool>,
    pub container_clear: Option<unsafe extern "C" fn(PierContainerRef) -> bool>,
    pub scoreboard_op: Option<unsafe extern "C" fn(i32, PierStr, PierStr, i64, *mut c_void, PierStrSink) -> bool>,
    pub form_send: Option<unsafe extern "C" fn(PierModHandle, PierPlayerSel, i32, PierStr, PierFormResultCb, *mut c_void) -> bool>,
    pub register_command_ex: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierStr, i32, PierStr, PierCommandCb, *mut c_void) -> bool>,

    // ── §F scoreboard
    pub register_command_enum: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,

    // ── §G forms (async result callback)
    pub register_command_soft_enum: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,

    // ── §H parameterized commands & enums
    pub update_command_soft_enum: Option<unsafe extern "C" fn(PierStr, i32, PierStr) -> bool>,
    pub nbt_snbt_to_binary: Option<unsafe extern "C" fn(PierStr, i32, *mut c_void, PierBytesSink) -> bool>,
    pub nbt_binary_to_snbt: Option<unsafe extern "C" fn(*const u8, usize, i32, *mut c_void, PierStrSink) -> bool>,
    pub kvdb_open: Option<unsafe extern "C" fn(PierModHandle, PierStr, bool) -> PierKvDbHandle>,

    // ── §I NBT binary, KvDb (thread-safe), system & server info
    pub kvdb_close: Option<unsafe extern "C" fn(PierKvDbHandle)>,
    pub kvdb_get: Option<unsafe extern "C" fn(PierKvDbHandle, PierStr, *mut c_void, PierStrSink) -> bool>,
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
    pub spawn_particle_for: Option<unsafe extern "C" fn(PierPlayerSel, i32, PierStr, f64, f64, f64) -> bool>,
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
    pub structures_near: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, *mut c_void, PierStrSink)>,
    pub player_send_message_typed: Option<unsafe extern "C" fn(PierPlayerSel, PierStr, i32) -> bool>,
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
    pub player_get_carried_item: Option<unsafe extern "C" fn(PierPlayerSel, *mut c_void, PierStrSink) -> bool>,
    pub player_get_item: Option<unsafe extern "C" fn(PierPlayerSel, i32, *mut c_void, PierStrSink) -> bool>,
    pub player_set_item: Option<unsafe extern "C" fn(PierPlayerSel, i32, PierStr) -> bool>,
    pub player_get_equipment: Option<unsafe extern "C" fn(PierPlayerSel, *mut c_void, PierStrSink) -> bool>,
    pub player_get_cooldown: Option<unsafe extern "C" fn(PierPlayerSel, PierStr) -> i32>,
    pub player_start_cooldown: Option<unsafe extern "C" fn(PierPlayerSel, PierStr, i32) -> bool>,
    pub player_get_network_status: Option<unsafe extern "C" fn(PierPlayerSel, *mut c_void, PierStrSink) -> bool>,
    pub actor_get_vehicle: Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,
    pub actor_get_first_passenger: Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,
    pub actor_get_owner: Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,

    // ── 追加 —— API 补齐（struct_size 把关）

    // ── Player: equipment, cooldown, network (dedicated fns)
    pub actor_get_target: Option<unsafe extern "C" fn(PierActorId, *mut PierActorId) -> bool>,
    pub actor_get_equipped_item: Option<unsafe extern "C" fn(PierActorId, i32, *mut c_void, PierStrSink) -> bool>,
    pub actor_set_equipped_item: Option<unsafe extern "C" fn(PierActorId, i32, PierStr) -> bool>,
    pub actor_get_effects: Option<unsafe extern "C" fn(PierActorId, *mut c_void, PierStrSink) -> bool>,
    pub actor_get_status_flag: Option<unsafe extern "C" fn(PierActorId, i32) -> bool>,
    pub actor_set_status_flag: Option<unsafe extern "C" fn(PierActorId, i32, bool) -> bool>,
    pub actor_trace_ray: Option<unsafe extern "C" fn(PierActorId, f32, bool, bool, *mut c_void, PierStrSink) -> bool>,

    // ── Actor: relationships, equipment, effects, geometry (dedicated fns)
    pub actor_distance_to: Option<unsafe extern "C" fn(PierActorId, PierActorId, *mut f64) -> bool>,
    pub actor_get_aabb: Option<unsafe extern "C" fn(PierActorId, *mut c_void, PierStrSink) -> bool>,
    pub actor_clone: Option<unsafe extern "C" fn(PierActorId, i32, f64, f64, f64, *mut PierActorId) -> bool>,
    pub block_get_state: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, *mut c_void, PierStrSink) -> bool>,
    pub block_set_state: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, PierStr) -> bool>,
    pub block_get_collision_shape: Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub item_get_enchants: Option<unsafe extern "C" fn(PierStr, *mut c_void, PierStrSink) -> bool>,
    pub item_set_enchants: Option<unsafe extern "C" fn(PierStr, PierStr, *mut c_void, PierStrSink) -> bool>,
    pub item_matches: Option<unsafe extern "C" fn(PierStr, PierStr) -> bool>,
    pub item_get_user_data: Option<unsafe extern "C" fn(PierStr, *mut c_void, PierStrSink) -> bool>,
    pub level_get_biome: Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub level_get_default_spawn: Option<unsafe extern "C" fn(*mut i32, *mut i32, *mut i32) -> bool>,
    pub level_set_default_spawn: Option<unsafe extern "C" fn(i32, i32, i32) -> bool>,

    // ── Block: state get/set, collision shape (dedicated fns)
    pub level_save: Option<unsafe extern "C" fn() -> bool>,
    pub level_get_sleep_status: Option<unsafe extern "C" fn(*mut c_void, PierStrSink) -> bool>,
    pub level_update_weather: Option<unsafe extern "C" fn(f32, i32, f32, i32) -> bool>,

    // ── Item: enchants, matching, NBT (dedicated fns)
    pub level_find_path: Option<unsafe extern "C" fn(PierActorId, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub packet_hook_register: Option<unsafe extern "C" fn(PierModHandle, i32, PierPacketCb, *mut c_void) -> PierPacketHookHandle>,
    pub packet_hook_unregister: Option<unsafe extern "C" fn(PierModHandle, PierPacketHookHandle) -> bool>,
    pub packet_conn_hook_register: Option<unsafe extern "C" fn(PierModHandle, PierConnCb, *mut c_void) -> PierPacketHookHandle>,

    // ── Level: biome, spawn, save, weather, path, sleep (dedicated fns)
    pub packet_conn_hook_unregister: Option<unsafe extern "C" fn(PierModHandle, PierPacketHookHandle) -> bool>,
    pub client_get_local_player: Option<unsafe extern "C" fn(*mut c_void, PierStrSink) -> bool>,
    pub client_is_in_level: Option<unsafe extern "C" fn() -> bool>,
    pub client_get_screen_name: Option<unsafe extern "C" fn(*mut c_void, PierStrSink) -> bool>,
    pub client_register_key: Option<unsafe extern "C" fn(PierModHandle, PierStr, *const i32, i32, bool, PierKeyCb, PierKeyCb, *mut c_void) -> PierKeyHandle>,
    pub client_unregister_key: Option<unsafe extern "C" fn(PierKeyHandle) -> bool>,
    pub client_get_key_codes: Option<unsafe extern "C" fn(PierKeyHandle, *mut c_void, PierStrSink) -> bool>,

    // ── 数据包拦截（追加，struct_size 把关）
    pub md_is_available: Option<unsafe extern "C" fn() -> bool>,
    pub md_add_simple_dimension: Option<unsafe extern "C" fn(PierStr, u32, i32) -> i32>,
    pub md_set_dimension_rule: Option<unsafe extern "C" fn(i32, i32, bool)>,
    pub md_get_dimension_rule: Option<unsafe extern "C" fn(i32, i32, *mut bool) -> bool>,
    pub md_clear_dimension_rules: Option<unsafe extern "C" fn(i32)>,

    // ── 能力组：客户端（client_*）。服务端宿主全为 NULL。
    pub md_get_dimension_id: Option<unsafe extern "C" fn(PierStr) -> i32>,
    pub md_add_plot_dimension: Option<unsafe extern "C" fn(PierStr, u32, PierStr) -> i32>,
    pub schedule_for: Option<unsafe extern "C" fn(PierModHandle, PierTaskCb, *mut c_void) -> u64>,
    pub schedule_after_for: Option<unsafe extern "C" fn(PierModHandle, PierTaskCb, *mut c_void, u64) -> u64>,
    pub schedule_cancel: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub schedule_pending_count: Option<unsafe extern "C" fn(PierModHandle) -> u32>,
    pub container_refresh: Option<unsafe extern "C" fn(PierContainerRef) -> bool>,

    // ── 能力组：自定义维度（md_*）。pier-dimensions 没编进宿主时全为
    pub player_send_title: Option<unsafe extern "C" fn(PierPlayerSel, i32, PierStr, i32, i32, i32) -> bool>,
    pub bus_subscribe: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierBusCb, *mut c_void) -> u64>,
    pub bus_unsubscribe: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub bus_publish: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierStr) -> u32>,
    pub bus_publish_vetoable: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierStr, *mut u32) -> bool>,
    pub bus_subscriber_count: Option<unsafe extern "C" fn(PierStr) -> u32>,
    pub md_set_plot_grid: Option<unsafe extern "C" fn(i32, i32, i32)>,
    pub md_clear_plot_grid: Option<unsafe extern "C" fn(i32)>,
    pub md_set_plot_merges: Option<unsafe extern "C" fn(i32, *const i32, i32)>,
    pub service_register: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierServiceCb, *mut c_void) -> u64>,

    // ── 追加尾（struct_size 把关）

    // ── Mod-scoped scheduling
    pub service_unregister: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub service_call: Option<unsafe extern "C" fn(PierModHandle, PierStr, PierStr, *mut c_void, PierStrSink) -> i32>,
    pub service_list: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub edit_set_block_nbt: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, i32) -> bool>,

    // ── Client-side container resync
    pub edit_set_block_states: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, PierStr, i32) -> bool>,
    pub edit_set_block_entity: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr) -> bool>,

    // ── Titles
    pub edit_spawn_entity_nbt: Option<unsafe extern "C" fn(i32, PierStr, bool, f64, f64, f64, *mut PierActorId) -> bool>,
    pub edit_trace_ray: Option<unsafe extern "C" fn(PierActorId, f32, bool, bool, *mut c_void, PierStrSink) -> bool>,
    pub lane_publish: Option<unsafe extern "C" fn(PierModHandle, PierStr, *const PierLaneDesc) -> u64>,
    pub lane_unpublish: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,

    // ── Cross-mod event bus
    pub lane_acquire: Option<unsafe extern "C" fn(PierModHandle, PierStr, u64, *mut PierLaneRef) -> i32>,
    pub lane_release: Option<unsafe extern "C" fn(PierModHandle, u64) -> bool>,
    pub lane_list: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub md_list_dimensions: Option<unsafe extern "C" fn(*mut c_void, PierStrSink)>,
    pub level_delete_chunk_keys: Option<unsafe extern "C" fn(i32, i32, i32) -> i32>,
    pub level_chunks_loaded: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32) -> i32>,
    pub player_conn_id: Option<unsafe extern "C" fn(PierPlayerSel) -> u64>,

    // ── Plot-boundary confinement
    pub level_chunk_keys: Option<unsafe extern "C" fn(i32, i32, i32, *mut c_void, PierStrSink) -> i32>,
    pub level_delete_key: Option<unsafe extern "C" fn(PierStr) -> bool>,
    pub level_set_biome: Option<unsafe extern "C" fn(i32, i32, i32, i32, i32, PierStr) -> i32>,
    pub get_extra_block: Option<unsafe extern "C" fn(i32, i32, i32, i32, *mut c_void, PierStrSink) -> bool>,
    pub set_extra_block: Option<unsafe extern "C" fn(i32, i32, i32, i32, PierStr, i32) -> bool>,
}

impl PierApi {
    /// 宿主的表是否长到覆盖了某个字节偏移。
    ///
    /// 这是前向兼容的**唯一**依据（契约 §2.2）：老宿主 + 新模组时，新模组
    /// 够不到的那些槽根本不在宿主分配的内存里。先查长度，再查槽位非空。
    ///
    /// 用 `offset_of!` 而不是手数字节：手数的那个数字会在下一次追加时悄悄
    /// 变错，而错的方向恰好是「以为够长」。
    #[inline]
    pub fn covers(&self, offset: usize, size: usize) -> bool {
        (self.struct_size as usize) >= offset + size
    }

    /// 宿主是不是按客户端目标编的。
    #[inline]
    pub fn is_client_host(&self) -> bool {
        (self.host_flags & crate::PIER_FLAG_CLIENT) != 0
    }
}
