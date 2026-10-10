# levilamina::player

Players: addressed by selector and resolved again on every call.

**Only an xuid may be used as a key**

When `PlayerSel::Name` matches no account name on the host side it falls back to the display
name, which another mod can change. A player setting their display name to the account name of
an offline player redirects every by-name call onto themselves. Permission, economy and
ownership decisions all use [`Player::by_xuid`](player.md#Player.by_xuid); see the module documentation of [`crate::sel`](player.md#PlayerSel).

**A player is an actor too**

[`Player::as_entity`](player.md#Player.as_entity) goes through `player_resolve` for an `ActorUniqueID`, after which the
whole of [`crate::entity::Entity`](entity.md#Entity) applies. The two APIs are complementary: player-specific
things such as inventory, ability bits, titles and kicking are here, and actor-general ones such
as health, teleporting and tags are there.

## `Player` {#Player}

```rust
pub struct Player {
    // private fields
}
```

One player. It holds a selector and no pointer.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Hash`, `Display`, `From`

### `Player::by_name` {#Player.by_name}

```rust
pub fn by_name(name: impl Into<String>) -> Player
```

By name. This goes through the display-name fallback, so identity uses
[`Player::by_xuid`](player.md#Player.by_xuid).

- Parameters:
    - name : `impl Into<String>`
- Return type: `Player`

### `Player::by_xuid` {#Player.by_xuid}

```rust
pub fn by_xuid(xuid: impl Into<String>) -> Player
```

By xuid: unique, unforgeable and unchangeable by the player.

- Parameters:
    - xuid : `impl Into<String>`
- Return type: `Player`

### `Player::by_uuid` {#Player.by_uuid}

```rust
pub fn by_uuid(uuid: impl Into<String>) -> Player
```

- Parameters:
    - uuid : `impl Into<String>`
- Return type: `Player`

### `Player::from_sel` {#Player.from_sel}

```rust
pub fn from_sel(sel: PlayerSel) -> Player
```

- Parameters:
    - sel : `PlayerSel`
- Return type: `Player`

### `Player::sel` {#Player.sel}

```rust
pub fn sel(&self) -> &PlayerSel
```

- Return type: `&PlayerSel`

### `Player::list` {#Player.list}

```rust
pub fn list() -> Vec<PlayerInfo>
```

The list of online players.

The host sinks one SNBT per player. An unparsable entry is skipped with a warning
rather than emptying the whole table: one bad entry should not make who is on the
server unanswerable. Every ABI v2 host fills `list_players`, server and client alike,
so an empty list means that nobody is online.

- Return type: `Vec<PlayerInfo>`
- Slots: [`list_players`](../cpp/player.md#list_players), [`list_players`](../cpp/player.md#list_players)

### `Player::broadcast` {#Player.broadcast}

```rust
pub fn broadcast(msg: &str) -> Result<()>
```

Sends one message to every online player.

- Parameters:
    - msg : `&str`
- Return type: `Result<()>`
- Slots: [`broadcast_message`](../cpp/player.md#broadcast_message)

### `Player::is_online` {#Player.is_online}

```rust
pub fn is_online(&self) -> bool
```

Whether this selector currently resolves to anyone. Every ABI v2 host fills
`player_resolve`, server and client alike, so false here means nobody matches, never
a host that cannot tell.

- Return type: `bool`
- Slots: [`player_resolve`](../cpp/player.md#player_resolve)

### `Player::as_entity` {#Player.as_entity}

```rust
pub fn as_entity(&self) -> Result<Entity>
```

Uses it as an actor, giving the full set of [`Entity`](entity.md#Entity) capabilities.

- Return type: `Result<Entity>`
- Slots: [`player_resolve`](../cpp/player.md#player_resolve)

### `Player::num` {#Player.num}

```rust
pub fn num(&self, prop: i32) -> Result<f64>
```

Reads a `PIER_PPROP_*` numeric property.

- Parameters:
    - prop : `i32`
- Return type: `Result<f64>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::set_num` {#Player.set_num}

```rust
pub fn set_num(&self, prop: i32, v: f64) -> Result<()>
```

Writes a `PIER_PPROP_*` numeric property. Only the ones marked (S) are writable.

- Parameters:
    - prop : `i32`
    - v : `f64`
- Return type: `Result<()>`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player::text` {#Player.text}

```rust
pub fn text(&self, prop: i32) -> Result<String>
```

Reads a `PIER_PSTR_*` string property.

- Parameters:
    - prop : `i32`
- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::last_death_pos` {#Player.last_death_pos}

```rust
pub fn last_death_pos(&self) -> Result<Option<(PositionF64, i32)>>
```

The position of the last death. Never having died gives `Ok(None)`, since the host
sends an empty string.

- Return type: `Result<Option<(PositionF64, i32)>>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::game_type` {#Player.game_type}

```rust
pub fn game_type(&self) -> Result<GameMode>
```

- Return type: `Result<GameMode>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::permission_level` {#Player.permission_level}

```rust
pub fn permission_level(&self) -> Result<PlayerPermission>
```

- Return type: `Result<PlayerPermission>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::set_level` {#Player.set_level}

```rust
pub fn set_level(&self, level: i32) -> Result<()>
```

- Parameters:
    - level : `i32`
- Return type: `Result<()>`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_experience` {#Player.set_experience}

```rust
pub fn set_experience(&self, progress: f64) -> Result<()>
```

The experience bar progress, from 0 to 1.

- Parameters:
    - progress : `f64`
- Return type: `Result<()>`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_hunger` {#Player.set_hunger}

```rust
pub fn set_hunger(&self, v: f64) -> Result<()>
```

- Parameters:
    - v : `f64`
- Return type: `Result<()>`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_saturation` {#Player.set_saturation}

```rust
pub fn set_saturation(&self, v: f64) -> Result<()>
```

- Parameters:
    - v : `f64`
- Return type: `Result<()>`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player::set_exhaustion` {#Player.set_exhaustion}

```rust
pub fn set_exhaustion(&self, v: f64) -> Result<()>
```

- Parameters:
    - v : `f64`
- Return type: `Result<()>`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player::position` {#Player.position}

```rust
pub fn position(&self) -> Result<(PositionF64, i32)>
```

The position. It goes through a dedicated slot rather than a property number, because
one call gives all three axes plus the dimension, while a player may have moved between
three separate property calls.

- Return type: `Result<(PositionF64, i32)>`
- Slots: [`get_player_position`](../cpp/player.md#get_player_position), [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::network_status` {#Player.network_status}

```rust
pub fn network_status(&self) -> Result<NetworkStatus>
```

The network status in detail.

- Return type: `Result<NetworkStatus>`
- Slots: [`player_get_network_status`](../cpp/player.md#player_get_network_status)

### `Player::conn_id` {#Player.conn_id}

```rust
pub fn conn_id(&self) -> Result<u64>
```

The connection id of this player, the same number a packet interceptor sees.

A 0 means offline or no network identity available. On the ABI 0 is not a valid
connection id, so this reports it truthfully as an `Err` rather than handing over a 0.

- Return type: `Result<u64>`
- Slots: [`player_conn_id`](../cpp/player.md#player_conn_id)

### `Player::disconnect` {#Player.disconnect}

```rust
pub fn disconnect(&self, reason: &str) -> Result<()>
```

- Parameters:
    - reason : `&str`
- Return type: `Result<()>`
- Slots: [`player_disconnect`](../cpp/player.md#player_disconnect)

### `Player::set_gamemode` {#Player.set_gamemode}

```rust
pub fn set_gamemode(&self, mode: GameMode) -> Result<()>
```

- Parameters:
    - mode : `GameMode`
- Return type: `Result<()>`
- Slots: [`player_set_gamemode`](../cpp/player.md#player_set_gamemode)

### `Player::teleport` {#Player.teleport}

```rust
pub fn teleport(&self, dim: i32, x: f64, y: f64, z: f64) -> Result<()>
```

Teleports. A custom dimension, with an id of 3 or above, goes through here too, and
when the dimension bridge cannot build a matching instance the host fails rather than
dropping the person into a mismatched dimension.

- Parameters:
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`player_teleport`](../cpp/player.md#player_teleport)

### `Player::act` {#Player.act}

```rust
pub fn act(&self, action: i32, sarg: &str, a: f64, b: f64, c: f64) -> Result<String>
```

Runs a `PIER_PACT_*` action and returns its output.

- Parameters:
    - action : `i32`
    - sarg : `&str`
    - a : `f64`
    - b : `f64`
    - c : `f64`
- Return type: `Result<String>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_ability` {#Player.set_ability}

```rust
pub fn set_ability<V: AbilityValue>(&self, ability: Ability, value: V) -> Result<()>
```

Sets one ability bit.

Passing a boolean ability where a floating-point one belongs raises no error and is
simply written under the other interpretation, so [`Ability::is_float`](types.md#Ability.is_float) stops it first.

- Parameters:
    - ability : `Ability`
    - value : `V`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_ability_raw` {#Player.set_ability_raw}

```rust
pub fn set_ability_raw(&self, index: i32, value: f64) -> Result<()>
```

Sets an ability bit by index, for when the host is newer than this layer and has extra
bits.

- Parameters:
    - index : `i32`
    - value : `f64`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::can_use_ability` {#Player.can_use_ability}

```rust
pub fn can_use_ability(&self, ability: Ability) -> Result<bool>
```

- Parameters:
    - ability : `Ability`
- Return type: `Result<bool>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_permission_level` {#Player.set_permission_level}

```rust
pub fn set_permission_level(&self, level: PlayerPermission) -> Result<()>
```

- Parameters:
    - level : `PlayerPermission`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_selected_slot` {#Player.set_selected_slot}

```rust
pub fn set_selected_slot(&self, slot: i32) -> Result<()>
```

- Parameters:
    - slot : `i32`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::give_item` {#Player.give_item}

```rust
pub fn give_item(&self, item: &ItemStack) -> Result<()>
```

- Parameters:
    - item : `&ItemStack`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_spawn_point` {#Player.set_spawn_point}

```rust
pub fn set_spawn_point(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<()>
```

- Parameters:
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::add_experience` {#Player.add_experience}

```rust
pub fn add_experience(&self, xp: i32) -> Result<()>
```

- Parameters:
    - xp : `i32`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::add_levels` {#Player.add_levels}

```rust
pub fn add_levels(&self, levels: i32) -> Result<()>
```

- Parameters:
    - levels : `i32`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_chunk_radius` {#Player.set_chunk_radius}

```rust
pub fn set_chunk_radius(&self, radius: i32) -> Result<()>
```

- Parameters:
    - radius : `i32`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_enchantment_seed` {#Player.set_enchantment_seed}

```rust
pub fn set_enchantment_seed(&self, seed: i32) -> Result<()>
```

- Parameters:
    - seed : `i32`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::play_emote` {#Player.play_emote}

```rust
pub fn play_emote(&self, piece_id: &str) -> Result<()>
```

- Parameters:
    - piece_id : `&str`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::resend_all_chunks` {#Player.resend_all_chunks}

```rust
pub fn resend_all_chunks(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::open_inventory` {#Player.open_inventory}

```rust
pub fn open_inventory(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::start_riding` {#Player.start_riding}

```rust
pub fn start_riding(&self, vehicle: Entity) -> Result<()>
```

- Parameters:
    - vehicle : `Entity`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::stop_riding` {#Player.stop_riding}

```rust
pub fn stop_riding(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::attack` {#Player.attack}

```rust
pub fn attack(&self, target: Entity) -> Result<()>
```

- Parameters:
    - target : `Entity`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::interact` {#Player.interact}

```rust
pub fn interact(&self, target: Entity) -> Result<()>
```

- Parameters:
    - target : `Entity`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::drop_item` {#Player.drop_item}

```rust
pub fn drop_item(&self, item: &ItemStack, random: bool) -> Result<()>
```

- Parameters:
    - item : `&ItemStack`
    - random : `bool`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::set_sidebar` {#Player.set_sidebar}

```rust
pub fn set_sidebar(&self, objective: &str, title: &str, lines: &[String]) -> Result<()>
```

A per-player sidebar. `lines` runs from top to bottom.

- Parameters:
    - objective : `&str`
    - title : `&str`
    - lines : `&[String]`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::clear_sidebar` {#Player.clear_sidebar}

```rust
pub fn clear_sidebar(&self, objective: &str) -> Result<()>
```

- Parameters:
    - objective : `&str`
- Return type: `Result<()>`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player::send_message` {#Player.send_message}

```rust
pub fn send_message(&self, msg: &str) -> Result<()>
```

- Parameters:
    - msg : `&str`
- Return type: `Result<()>`
- Slots: [`player_send_message`](../cpp/player.md#player_send_message)

### `Player::tell` {#Player.tell}

```rust
pub fn tell(&self, msg: &str, kind: MessageType) -> Result<()>
```

Sends one with a given `TextPacketType`.

- Parameters:
    - msg : `&str`
    - kind : `MessageType`
- Return type: `Result<()>`
- Slots: [`player_send_message_typed`](../cpp/player.md#player_send_message_typed)

### `Player::send_title` {#Player.send_title}

```rust
pub fn send_title(&self, kind: TitleKind, text: &str, times: Option<TitleTimes>) -> Result<()>
```

Sends one title.

It goes through a real `SetTitlePacket` and not an assembled `/title` command, which
would paste the text into a command line verbatim and turn a plot whose name contains a
quote or an `@e` into a command injection.

A given `times` sends a Times packet first so the timing is definite, and omitting it
reuses the durations the client stored last. The three durations cannot be given in
part, a combination the host refuses outright.

- Parameters:
    - kind : `TitleKind`
    - text : `&str`
    - times : `Option<TitleTimes>`
- Return type: `Result<()>`
- Slots: [`player_send_title`](../cpp/player.md#player_send_title)

### `Player::set_title` {#Player.set_title}

```rust
pub fn set_title(&self, text: &str) -> Result<()>
```

- Parameters:
    - text : `&str`
- Return type: `Result<()>`
- Slots: [`player_send_title`](../cpp/player.md#player_send_title)

### `Player::set_subtitle` {#Player.set_subtitle}

```rust
pub fn set_subtitle(&self, text: &str) -> Result<()>
```

- Parameters:
    - text : `&str`
- Return type: `Result<()>`
- Slots: [`player_send_title`](../cpp/player.md#player_send_title)

### `Player::set_actionbar` {#Player.set_actionbar}

```rust
pub fn set_actionbar(&self, text: &str) -> Result<()>
```

- Parameters:
    - text : `&str`
- Return type: `Result<()>`
- Slots: [`player_send_title`](../cpp/player.md#player_send_title)

### `Player::clear_title` {#Player.clear_title}

```rust
pub fn clear_title(&self) -> Result<()>
```

- Return type: `Result<()>`
- Slots: [`player_send_title`](../cpp/player.md#player_send_title)

### `Player::spawn_particle` {#Player.spawn_particle}

```rust
pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()>
```

Spawns a particle for this one player only.

Unlike `World::spawn_particle`, nobody else sees it, since that one broadcasts across
the whole dimension. Something like a selection highlight has to use this one, otherwise
the whole server sees it.

- Parameters:
    - dim : `i32`
    - effect : `&str`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`spawn_particle_for`](../cpp/world.md#spawn_particle_for)

### `Player::send_packet` {#Player.send_packet}

```rust
pub fn send_packet(&self, packet_id: i32, body: &[u8]) -> Result<()>
```

Pushes a raw packet onto the connection of this player.

An escape hatch: `body` is the wire format of the current game version and has to
follow every version change, which is the caller's responsibility. A named entry point
is preferred wherever one exists.

- Parameters:
    - packet_id : `i32`
    - body : `&[u8]`
- Return type: `Result<()>`
- Slots: [`send_packet`](../cpp/packet.md#send_packet)

### `Player::inventory` {#Player.inventory}

```rust
pub fn inventory(&self) -> Container
```

- Return type: `Container`

### `Player::ender_chest` {#Player.ender_chest}

```rust
pub fn ender_chest(&self) -> Container
```

- Return type: `Container`

### `Player::armor` {#Player.armor}

```rust
pub fn armor(&self) -> Container
```

- Return type: `Container`

### `Player::offhand_container` {#Player.offhand_container}

```rust
pub fn offhand_container(&self) -> Container
```

- Return type: `Container`

### `Player::offhand` {#Player.offhand}

```rust
pub fn offhand(&self) -> Result<ItemStack>
```

The item in the off hand. An empty hand gives the air item and is not an error.

- Return type: `Result<ItemStack>`

### `Player::set_offhand` {#Player.set_offhand}

```rust
pub fn set_offhand(&self, item: &ItemStack) -> Result<()>
```

Writes the off hand. Remember [`Container::refresh`](container.md#Container.refresh) afterwards, otherwise the client
keeps showing the old item.

- Parameters:
    - item : `&ItemStack`
- Return type: `Result<()>`

### `Player::carried_item` {#Player.carried_item}

```rust
pub fn carried_item(&self) -> Result<ItemStack>
```

The item in hand.

- Return type: `Result<ItemStack>`
- Slots: [`player_get_carried_item`](../cpp/player.md#player_get_carried_item)

### `Player::item` {#Player.item}

```rust
pub fn item(&self, slot: i32) -> Result<ItemStack>
```

One inventory slot.

- Parameters:
    - slot : `i32`
- Return type: `Result<ItemStack>`
- Slots: [`player_get_item`](../cpp/player.md#player_get_item)

### `Player::set_item` {#Player.set_item}

```rust
pub fn set_item(&self, slot: i32, item: &ItemStack) -> Result<()>
```

- Parameters:
    - slot : `i32`
    - item : `&ItemStack`
- Return type: `Result<()>`
- Slots: [`player_set_item`](../cpp/player.md#player_set_item)

### `Player::equipment` {#Player.equipment}

```rust
pub fn equipment(&self) -> Result<Vec<(i32, ItemStack)>>
```

The full equipment set. For the `slot` numbering see [`crate::types::EquipSlot`](types.md#EquipSlot).

- Return type: `Result<Vec<(i32, ItemStack)>>`
- Slots: [`player_get_equipment`](../cpp/player.md#player_get_equipment)

### `Player::cooldown` {#Player.cooldown}

```rust
pub fn cooldown(&self, item_name: &str) -> Result<i32>
```

How many ticks of cooldown one item has left.

On the ABI a -1 means both not on cooldown and the player being offline. It is handed
over unchanged and stated rather than guessed at on the caller's behalf
(contract §5.2). Telling them apart starts with
[`Player::is_online`](player.md#Player.is_online).

- Parameters:
    - item_name : `&str`
- Return type: `Result<i32>`
- Slots: [`player_get_cooldown`](../cpp/player.md#player_get_cooldown)

### `Player::start_cooldown` {#Player.start_cooldown}

```rust
pub fn start_cooldown(&self, item_name: &str, ticks: i32) -> Result<()>
```

- Parameters:
    - item_name : `&str`
    - ticks : `i32`
- Return type: `Result<()>`
- Slots: [`player_start_cooldown`](../cpp/player.md#player_start_cooldown)

### `Player::real_name` {#Player.real_name}

```rust
pub fn real_name(&self) -> Result<String>
```

The account name.

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::uuid` {#Player.uuid}

```rust
pub fn uuid(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::xuid` {#Player.xuid}

```rust
pub fn xuid(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::ip_and_port` {#Player.ip_and_port}

```rust
pub fn ip_and_port(&self) -> Result<String>
```

`address:port`. IPv6 has the same shape, so it must not be split on the last colon.

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::locale_code` {#Player.locale_code}

```rust
pub fn locale_code(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::respawn_pos` {#Player.respawn_pos}

```rust
pub fn respawn_pos(&self) -> Result<String>
```

`{x,y,z,dim}` as SNBT: where this player would respawn. Never empty -- a player
with no bed reports the world spawn, and the two cannot be told apart.

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::name_tag` {#Player.name_tag}

```rust
pub fn name_tag(&self) -> Result<String>
```

The name shown above the head, which can be changed. An identity decision uses
[`Player::xuid`](player.md#Player.xuid).

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::platform_online_id` {#Player.platform_online_id}

```rust
pub fn platform_online_id(&self) -> Result<String>
```

- Return type: `Result<String>`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player::dimension` {#Player.dimension}

```rust
pub fn dimension(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::level` {#Player.level}

```rust
pub fn level(&self) -> Result<i32>
```

The experience level.

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::experience` {#Player.experience}

```rust
pub fn experience(&self) -> Result<f64>
```

The progress of the experience bar, from 0 to 1. It is not the accumulated experience.

- Return type: `Result<f64>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::hunger` {#Player.hunger}

```rust
pub fn hunger(&self) -> Result<f64>
```

- Return type: `Result<f64>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::saturation` {#Player.saturation}

```rust
pub fn saturation(&self) -> Result<f64>
```

The saturation; hunger only starts dropping once it is spent.

- Return type: `Result<f64>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::exhaustion` {#Player.exhaustion}

```rust
pub fn exhaustion(&self) -> Result<f64>
```

The exhaustion; filling one unit costs a point of saturation.

- Return type: `Result<f64>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::xp_needed_for_next_level` {#Player.xp_needed_for_next_level}

```rust
pub fn xp_needed_for_next_level(&self) -> Result<i32>
```

How much experience remains to the next level.

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::luck` {#Player.luck}

```rust
pub fn luck(&self) -> Result<f64>
```

- Return type: `Result<f64>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::selected_slot` {#Player.selected_slot}

```rust
pub fn selected_slot(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::score` {#Player.score}

```rust
pub fn score(&self) -> Result<i32>
```

The `score` pseudo-objective of the scoreboard, not any custom objective.

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::chunk_radius` {#Player.chunk_radius}

```rust
pub fn chunk_radius(&self) -> Result<i32>
```

The view distance the client requested, in chunks.

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::enchantment_seed` {#Player.enchantment_seed}

```rust
pub fn enchantment_seed(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::platform` {#Player.platform}

```rust
pub fn platform(&self) -> Result<i32>
```

The value of `BuildPlatform`, not an operating system name.

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::direction` {#Player.direction}

```rust
pub fn direction(&self) -> Result<i32>
```

The facing: 0 is south, 1 west, 2 north and 3 east.

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::ping` {#Player.ping}

```rust
pub fn ping(&self) -> Result<i32>
```

The round-trip latency in milliseconds. For the detail see [`Player::network_status`](player.md#Player.network_status).

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::client_sub_id` {#Player.client_sub_id}

```rust
pub fn client_sub_id(&self) -> Result<i32>
```

- Return type: `Result<i32>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::fall_distance` {#Player.fall_distance}

```rust
pub fn fall_distance(&self) -> Result<f64>
```

- Return type: `Result<f64>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_operator` {#Player.is_operator}

```rust
pub fn is_operator(&self) -> Result<bool>
```

Listed as an operator in `permissions.json`. Not the same thing as
[`Player::permission_level`](player.md#Player.permission_level), which can change at runtime.

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::can_use_operator_blocks` {#Player.can_use_operator_blocks}

```rust
pub fn can_use_operator_blocks(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_flying` {#Player.is_flying}

```rust
pub fn is_flying(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::can_jump` {#Player.can_jump}

```rust
pub fn can_jump(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_emoting` {#Player.is_emoting}

```rust
pub fn is_emoting(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_in_raid` {#Player.is_in_raid}

```rust
pub fn is_in_raid(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_hurt` {#Player.is_hurt}

```rust
pub fn is_hurt(&self) -> Result<bool>
```

Currently inside the invulnerability frames after taking damage.

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_scoping` {#Player.is_scoping}

```rust
pub fn is_scoping(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::can_sleep` {#Player.can_sleep}

```rust
pub fn can_sleep(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::has_respawn_position` {#Player.has_respawn_position}

```rust
pub fn has_respawn_position(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_using_item` {#Player.is_using_item}

```rust
pub fn is_using_item(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_blocking` {#Player.is_blocking}

```rust
pub fn is_blocking(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_gliding` {#Player.is_gliding}

```rust
pub fn is_gliding(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_swimming` {#Player.is_swimming}

```rust
pub fn is_swimming(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::is_dead` {#Player.is_dead}

```rust
pub fn is_dead(&self) -> Result<bool>
```

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player::has_died_before` {#Player.has_died_before}

```rust
pub fn has_died_before(&self) -> Result<bool>
```

Has died at least once in this save.

- Return type: `Result<bool>`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

## `PlayerSel` {#PlayerSel}

```rust
pub enum PlayerSel {
        /// By name. The account name is matched first and the host falls back to the display
        /// name on a miss; see the module documentation and do not use it as an identity.
        Name(String),
        /// By xuid: unique, unforgeable and unchangeable by the player. This is the one to use
        /// as a key.
        /// It may be an empty string on an offline-mode server, where only `Name` remains.
        Xuid(String),
        /// By uuid in its canonical string form. Equally stable and suited to a save key.
        Uuid(String),
}
```

How to point at a player.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Hash`, `From`, `Display`

### `PlayerSel::name` {#PlayerSel.name}

```rust
pub fn name(v: impl Into<String>) -> PlayerSel
```

- Parameters:
    - v : `impl Into<String>`
- Return type: `PlayerSel`

### `PlayerSel::xuid` {#PlayerSel.xuid}

```rust
pub fn xuid(v: impl Into<String>) -> PlayerSel
```

- Parameters:
    - v : `impl Into<String>`
- Return type: `PlayerSel`

### `PlayerSel::uuid` {#PlayerSel.uuid}

```rust
pub fn uuid(v: impl Into<String>) -> PlayerSel
```

- Parameters:
    - v : `impl Into<String>`
- Return type: `PlayerSel`

### `PlayerSel::kind` {#PlayerSel.kind}

```rust
pub fn kind(&self) -> i32
```

The underlying `kind` value, 0, 1 or 2, aligned with `abi.h`.

- Return type: `i32`

### `PlayerSel::value` {#PlayerSel.value}

```rust
pub fn value(&self) -> &str
```

- Return type: `&str`

### `PlayerSel::is_stable` {#PlayerSel.is_stable}

```rust
pub fn is_stable(&self) -> bool
```

Whether this selector is a reliable identity, meaning an xuid or a uuid.

A permission, economy or ownership decision receiving `false` should take care; see
the module documentation.

- Return type: `bool`

### `PlayerSel::is_empty` {#PlayerSel.is_empty}

```rust
pub fn is_empty(&self) -> bool
```

An emptiness check: an empty selector resolves to nobody, and finding that early beats
seeing an inexplicable `false` at the call site.

- Return type: `bool`

## `PlayerInfo` {#PlayerInfo}

```rust
pub struct PlayerInfo {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
    pub dimension: Option<i32>,
    pub pos: Option<PositionF64>,
}
```

One entry `list_players` reports.

`dimension` and `pos` are `Option` and not bare values. The host omits those keys when
the level is not ready, or while that player is midway through a dimension change, and
filling them with 0 would say they are at the origin of the overworld. The land
protection bypass contract §5.1 records has exactly that shape: an event in a custom
dimension could not read `dim`, the consumer wrote `unwrap_or(0)`, and everything was
allowed as the overworld.

A caller that wants a fallback writes `.unwrap_or(0)` itself, and is then the one
answerable for that default.

- Implements: `Debug`, `Clone`, `Default`, `PartialEq`

### `PlayerInfo::selector` {#PlayerInfo.selector}

```rust
pub fn selector(&self) -> PlayerSel
```

Builds a stable selector from an xuid. An empty xuid, on an offline-mode server, falls
back to the name and says so, so a caller knows the key is unreliable.

- Return type: `PlayerSel`

## `NetworkStatus` {#NetworkStatus}

```rust
pub struct NetworkStatus {
    pub ping: i32,
    pub avg_ping: i32,
    pub max_ping: i32,
    /// Per mille, exactly as the host gives it.
    pub packet_loss: i32,
}
```

The network status of a player, from `PIER_PSTR_NETWORK_STATUS`.

- Implements: `Debug`, `Clone`, `Copy`, `Default`, `PartialEq`, `Eq`
