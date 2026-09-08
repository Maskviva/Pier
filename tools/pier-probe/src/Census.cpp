/**
 * Census.cpp — is every slot the mod was compiled against actually there.
 *
 * The report in ReadOnly.cpp calls slots. This one does not call anything: it walks the
 * whole table and records, for each slot, whether the host provides it. That answers a
 * different question, and it is the question a mod author has when a capability does
 * nothing: is the slot missing, or is it present and refusing?
 *
 * Two ways a slot can be unavailable, and they need different fixes:
 *   past struct_size  the host is older than this mod. Nothing to do but use an older
 *                     SDK or a newer host.
 *   NULL              the host is new enough but was built without the package that
 *                     fills the slot. Contract §1 rule 4 makes pier-dimensions and
 *                     pier-lane droppable, and a client build fills a different set.
 *
 * The list covers every slot the contract declares and is generated from abi.h. A
 * hand-typed list has to be right about two hundred names, and a name that is wrong but
 * happens to exist elsewhere in the table compiles and reports the wrong slot. The cost
 * is that the list follows abi.h automatically and cannot notice a slot the contract
 * declares and no host ever fills; sources-are-built and sys-mirrors-abi speak to that.
 */
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "probe.h"

namespace
{
    struct Entry
    {
        char const* group;
        char const* name;
        std::size_t endOffset; // offset of the slot plus its size
        void* const* slot;     // where the pointer lives in the host table
    };

    /** offsetof plus sizeof, which is the first byte past the slot. covers() compares
     *  against this so a table that ends exactly at the slot still counts as having it. */
#define ENT(group, member)                                                                         \
    Entry                                                                                          \
    {                                                                                              \
        group, #member, offsetof(PierApi, member) + sizeof(void*),                                 \
            reinterpret_cast<void* const*>(&a->member)                                             \
    }

    std::vector<Entry> table(PierApi const* a)
    {
        return {
        ENT("core", log),
        ENT("core", gaming_status),
        ENT("core", schedule),
        ENT("core", schedule_after),
        ENT("core", subscribe_event),
        ENT("core", unsubscribe_event),
        ENT("world", list_events),
        ENT("core", execute_command),
        ENT("core", register_command),
        ENT("world", get_current_tick),
        ENT("world", get_tick_delta_time),
        ENT("world", get_player_count),
        ENT("world", get_sim_paused),
        ENT("world", spawn_particle),
        ENT("world", get_player_position),
        ENT("world", scan_region),
        ENT("world", get_block),
        ENT("world", set_block),
        ENT("world", get_time),
        ENT("world", set_time),
        ENT("world", set_weather),
        ENT("world", list_players),
        ENT("player", player_resolve),
        ENT("player", player_send_message),
        ENT("player", player_disconnect),
        ENT("core", broadcast_message),
        ENT("player", player_set_gamemode),
        ENT("player", player_teleport),
        ENT("player", player_get_num),
        ENT("player", player_get_str),
        ENT("player", player_set_num),
        ENT("player", player_action),
        ENT("world", list_actors),
        ENT("actor", actor_snapshot),
        ENT("actor", actor_get_num),
        ENT("actor", actor_get_str),
        ENT("actor", actor_action),
        ENT("world", spawn_mob),
        ENT("world", explode),
        ENT("world", block_get_num),
        ENT("world", block_get_str),
        ENT("world", block_action),
        ENT("world", block_entity_snbt),
        ENT("item", item_get_num),
        ENT("item", item_get_str),
        ENT("item", item_transform),
        ENT("container", container_size),
        ENT("container", container_get_item),
        ENT("container", container_set_item),
        ENT("container", container_add_item),
        ENT("container", container_remove_item),
        ENT("container", container_clear),
        ENT("scoreboard", scoreboard_op),
        ENT("player", form_send),
        ENT("core", register_command_ex),
        ENT("core", register_command_enum),
        ENT("core", register_command_soft_enum),
        ENT("world", update_command_soft_enum),
        ENT("nbt", nbt_snbt_to_binary),
        ENT("nbt", nbt_binary_to_snbt),
        ENT("kvdb", kvdb_open),
        ENT("kvdb", kvdb_close),
        ENT("kvdb", kvdb_get),
        ENT("kvdb", kvdb_set),
        ENT("kvdb", kvdb_del),
        ENT("kvdb", kvdb_has),
        ENT("kvdb", kvdb_is_empty),
        ENT("kvdb", kvdb_iter),
        ENT("sys", sys_info_str),
        ENT("sys", sys_get_env),
        ENT("sys", sys_set_env),
        ENT("sys", sys_is_wine),
        ENT("world", get_difficulty),
        ENT("world", set_difficulty),
        ENT("world", get_seed),
        ENT("server", game_rule_get),
        ENT("server", game_rule_set),
        ENT("server", server_info_str),
        ENT("world", spawn_particle_for),
        ENT("player", send_packet),
        ENT("server", tick_freeze),
        ENT("server", tick_step),
        ENT("server", tick_warp),
        ENT("server", profile_begin),
        ENT("server", profile_take),
        ENT("sim", sim_spawn),
        ENT("sim", sim_do),
        ENT("sim", sim_is),
        ENT("sim", sim_list),
        ENT("world", villages),
        ENT("world", structures_near),
        ENT("player", player_send_message_typed),
        ENT("world", get_money),
        ENT("world", set_money),
        ENT("money", add_money),
        ENT("money", reduce_money),
        ENT("money", trans_money),
        ENT("money", money_get_hist),
        ENT("money", money_clear_hist),
        ENT("money", money_listen_before_event),
        ENT("money", money_listen_after_event),
        ENT("money", money_ranking),
        ENT("player", player_get_carried_item),
        ENT("player", player_get_item),
        ENT("player", player_set_item),
        ENT("player", player_get_equipment),
        ENT("player", player_get_cooldown),
        ENT("player", player_start_cooldown),
        ENT("player", player_get_network_status),
        ENT("actor", actor_get_vehicle),
        ENT("actor", actor_get_first_passenger),
        ENT("actor", actor_get_owner),
        ENT("actor", actor_get_target),
        ENT("actor", actor_get_equipped_item),
        ENT("actor", actor_set_equipped_item),
        ENT("actor", actor_get_effects),
        ENT("actor", actor_get_status_flag),
        ENT("actor", actor_set_status_flag),
        ENT("actor", actor_trace_ray),
        ENT("actor", actor_distance_to),
        ENT("actor", actor_get_aabb),
        ENT("actor", actor_clone),
        ENT("world", block_get_state),
        ENT("world", block_set_state),
        ENT("world", block_get_collision_shape),
        ENT("item", item_get_enchants),
        ENT("item", item_set_enchants),
        ENT("item", item_matches),
        ENT("item", item_get_user_data),
        ENT("world", level_get_biome),
        ENT("world", level_get_default_spawn),
        ENT("world", level_set_default_spawn),
        ENT("world", level_save),
        ENT("world", level_get_sleep_status),
        ENT("world", level_update_weather),
        ENT("world", level_find_path),
        ENT("packet", packet_hook_register),
        ENT("packet", packet_hook_unregister),
        ENT("packet", packet_conn_hook_register),
        ENT("packet", packet_conn_hook_unregister),
        ENT("client", client_get_local_player),
        ENT("client", client_is_in_level),
        ENT("client", client_get_screen_name),
        ENT("client", client_register_key),
        ENT("client", client_unregister_key),
        ENT("client", client_get_key_codes),
        ENT("dimensions", md_is_available),
        ENT("dimensions", md_set_dimension_rule),
        ENT("dimensions", md_get_dimension_rule),
        ENT("dimensions", md_clear_dimension_rules),
        ENT("dimensions", md_get_dimension_id),
        ENT("core", schedule_for),
        ENT("core", schedule_after_for),
        ENT("core", schedule_cancel),
        ENT("core", schedule_pending_count),
        ENT("container", container_refresh),
        ENT("player", player_send_title),
        ENT("bus", bus_subscribe),
        ENT("bus", bus_unsubscribe),
        ENT("bus", bus_publish),
        ENT("bus", bus_publish_vetoable),
        ENT("bus", bus_subscriber_count),
        ENT("dimensions", md_set_plot_merges),
        ENT("service", service_register),
        ENT("service", service_unregister),
        ENT("service", service_call),
        ENT("service", service_list),
        ENT("world", edit_set_block_nbt),
        ENT("world", edit_set_block_states),
        ENT("world", edit_set_block_entity),
        ENT("world", edit_spawn_entity_nbt),
        ENT("world", edit_trace_ray),
        ENT("lane", lane_publish),
        ENT("lane", lane_unpublish),
        ENT("lane", lane_acquire),
        ENT("lane", lane_release),
        ENT("lane", lane_list),
        ENT("dimensions", md_list_dimensions),
        ENT("world", level_delete_chunk_keys),
        ENT("world", level_chunks_loaded),
        ENT("player", player_conn_id),
        ENT("world", level_chunk_keys),
        ENT("world", level_delete_key),
        ENT("world", level_set_biome),
        ENT("world", get_extra_block),
        ENT("world", set_extra_block),
        ENT("world", get_tps),
        ENT("world", get_mspt),
        ENT("packet", packet_hook_register_ids),
        ENT("world", scan_region_indexed),
        ENT("world", edit_fill_region),
        ENT("world", edit_set_blocks),
        ENT("container", container_get_items),
        ENT("dimensions", md_add_dimension),
        ENT("dimensions", md_add_dimension_pack),
        ENT("dimensions", md_pack_inspect),
        ENT("dimensions", md_retire_dimension),
        };
    }
#undef ENT
} // namespace

void probeCensus()
{
    auto const* a = probe::api();
    if (a == nullptr) return;

    int present = 0;
    int null = 0;
    int beyond = 0;

    for (auto const& e : table(a))
    {
        if (!probe::covers(e.endOffset))
        {
            probe::record(e.group, e.name, probe::Verdict::Absent, "past struct_size");
            beyond++;
            continue;
        }
        if (*e.slot == nullptr)
        {
            probe::record(e.group, e.name, probe::Verdict::Absent, "host provides no such capability");
            null++;
            continue;
        }
        probe::record(e.group, e.name, probe::Verdict::Ok, "present");
        present++;
    }

    char line[192];
    std::snprintf(line, sizeof line,
                  "census: %d present, %d NULL, %d past struct_size", present, null, beyond);
    probe::log(3, line);
}
