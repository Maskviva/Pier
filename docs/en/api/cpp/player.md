# Players

??? note "Section notes in abi.h"

    **Appended**

    **§B player management**

    **§I NBT binary, KvDb (thread-safe), system & server info**

    **Player: equipment, cooldown, network (dedicated fns)**

    **Titles**

    `PACT_SET_TITLE` (`player_action` opcode 6) reaches the client by running the console command `title "<name>" title <text>`. Three things are wrong with that and none of them are theoretical:

    ```text
    - the text is pasted into a command line unquoted, so a plot named
      `He said "hi"` truncates the command;
    - `title`'s text parameter is a `message`, which expands selectors —
      a plot named `@e` is a command injection, not a name;
    - `/title` has no way to set fade/stay for the same call, so timing is
      whatever the client last stored.
    ```

    This slot builds a real SetTitlePacket instead. No wire format crosses the FFI (the packet is constructed field-by-field on this side), so it survives protocol bumps the way `spawn_particle_for` does.

    `type` is `SetTitlePacketPayload::TitleType`:

    ```text
    0 Clear · 1 Reset · 2 Title · 3 Subtitle · 4 Actionbar · 5 Times
    ```

    The TextObject variants (6..8) need a ResolvedTextObject and are refused. `text` is ignored for Clear/Reset/Times.

    Durations are in TICKS. For 2/3/4, when all three are &gt;= 0 a Times packet is sent first so the timing is deterministic rather than inherited from whatever the client last stored; pass -1 for all three to keep the client's current timing. Mixing (-1 with &gt;=0) is refused rather than guessed at — a half-specified duration set has no sane meaning. Server thread only.

    **Same-toolchain fast lane, appended and struct\_size-gated.**

    Five slots appended without touching `PIER_ABI_VERSION`: a pure append is not a version change, and `struct_size` is the precise gate.

    Both directions hold. A new loader running an old mod: the old table is a byte-identical prefix of the new one, the mod cannot reach these five slots, and it works unchanged. A new mod on an old loader: SDK runtime init compares `struct_size`, finds the loader's table shorter than the one it was compiled against, and refuses to load. That is the right outcome, since a mod that reads the `lane_publish` cell on a loader without it would read out of bounds.

    In short: the version number tracks "semantics changed", `struct_size` tracks "the table grew". This change is only the latter.

    See the long comment at PierLaneDesc above. In one line: service is the cross-language (name, JSON) -&gt; JSON channel, while this is a direct function-table call that holds only when both sides were built by the same toolchain; a fingerprint mismatch yields no pointer and the consumer falls back to service.

    Server thread only.

## Slots {#slots}

### `get_player_position` {#get_player_position}

```c
PierPlayerPos (*get_player_position)(PierStr name);
```

Look up a connected player's feet position and dimension by name. Used to pick selection corners from where the player is standing. Server thread only.

- Call: `api->get_player_position(name)`
- Parameters:
    - name : `PierStr`
- Return type: `PierPlayerPos`
- Section of abi.h: Appended
- Position in the table: slot 14, counting from 0
- Callers in each binding:
    - Rust: [`Player::position`](../rust/player.md#Player.position)
    - Go: [`Raw.GetPlayerPosition`](../go/raw.md#Raw.GetPlayerPosition)

### `list_players` {#list_players}

```c
void (*list_players)(void* ctx, PierStrSink snbt_sink);
```

One SNBT per online player: {name,xuid,uuid,dim,x,y,z}.

- Call: `api->list_players(ctx, snbt_sink)`
- Parameters:
    - ctx : `void*`
    - snbt_sink : `PierStrSink`
- Section of abi.h: §B player management
- Position in the table: slot 21, counting from 0
- Callers in each binding:
    - Rust: [`Player::list`](../rust/player.md#Player.list), [`Player::list`](../rust/player.md#Player.list)
    - Go: [`ListPlayers`](../go/player.md#ListPlayers), [`Raw.ListPlayers`](../go/raw.md#Raw.ListPlayers)

### `player_resolve` {#player_resolve}

```c
bool (*player_resolve)(PierPlayerSel sel, PierActorId* out);
```

Resolve a player selector to their ActorUniqueID (bridges into the actor\_\* API).

- Call: `api->player_resolve(sel, out)`
- Parameters:
    - sel : `PierPlayerSel`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 22, counting from 0
- Callers in each binding:
    - Rust: [`Player::is_online`](../rust/player.md#Player.is_online), [`Player::as_entity`](../rust/player.md#Player.as_entity)
    - Go: [`Player.Resolve`](../go/player.md#Player.Resolve), [`Player.IsOnline`](../go/player.md#Player.IsOnline), [`Raw.PlayerResolve`](../go/raw.md#Raw.PlayerResolve)

### `player_send_message` {#player_send_message}

```c
bool (*player_send_message)(PierPlayerSel sel, PierStr msg);
```

- Call: `api->player_send_message(sel, msg)`
- Parameters:
    - sel : `PierPlayerSel`
    - msg : `PierStr`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 23, counting from 0
- Callers in each binding:
    - Rust: [`Player::send_message`](../rust/player.md#Player.send_message)
    - Go: [`Player.SendMessage`](../go/player.md#Player.SendMessage), [`Raw.PlayerSendMessage`](../go/raw.md#Raw.PlayerSendMessage)

### `player_disconnect` {#player_disconnect}

```c
bool (*player_disconnect)(PierPlayerSel sel, PierStr reason);
```

- Call: `api->player_disconnect(sel, reason)`
- Parameters:
    - sel : `PierPlayerSel`
    - reason : `PierStr`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 24, counting from 0
- Callers in each binding:
    - Rust: [`Player::disconnect`](../rust/player.md#Player.disconnect)
    - Go: [`Player.Disconnect`](../go/player.md#Player.Disconnect), [`Raw.PlayerDisconnect`](../go/raw.md#Raw.PlayerDisconnect)

### `broadcast_message` {#broadcast_message}

```c
void (*broadcast_message)(PierStr msg);
```

sendMessage to every online player.

- Call: `api->broadcast_message(msg)`
- Parameters:
    - msg : `PierStr`
- Section of abi.h: §B player management
- Position in the table: slot 25, counting from 0
- Callers in each binding:
    - Rust: [`Player::broadcast`](../rust/player.md#Player.broadcast)
    - Go: [`Broadcast`](../go/player.md#Broadcast), [`Raw.BroadcastMessage`](../go/raw.md#Raw.BroadcastMessage)

### `player_set_gamemode` {#player_set_gamemode}

```c
bool (*player_set_gamemode)(PierPlayerSel sel, int32_t mode);
```

0=survival 1=creative 2=adventure 6=spectator, native (`Player::setPlayerGameType`).

- Call: `api->player_set_gamemode(sel, mode)`
- Parameters:
    - sel : `PierPlayerSel`
    - mode : `int32_t`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 26, counting from 0
- Callers in each binding:
    - Rust: [`Player::set_gamemode`](../rust/player.md#Player.set_gamemode)
    - Go: [`Player.SetGameType`](../go/player.md#Player.SetGameType), [`Raw.PlayerSetGamemode`](../go/raw.md#Raw.PlayerSetGamemode)

### `player_teleport` {#player_teleport}

```c
bool (*player_teleport)(PierPlayerSel sel, int32_t dim, double x, double y, double z);
```

Teleport natively (`Actor::teleport`). Custom dimensions (id &gt;= 3) are allowed; the dimension bridge must produce an engine instance whose id matches, or the call fails instead of sending the player into a mismatched dimension.

- Call: `api->player_teleport(sel, dim, x, y, z)`
- Parameters:
    - sel : `PierPlayerSel`
    - dim : `int32_t`
    - x : `double`
    - y : `double`
    - z : `double`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 27, counting from 0
- Callers in each binding:
    - Rust: [`Player::teleport`](../rust/player.md#Player.teleport)
    - Go: [`Player.Teleport`](../go/player.md#Player.Teleport), [`Raw.PlayerTeleport`](../go/raw.md#Raw.PlayerTeleport)

### `player_get_num` {#player_get_num}

```c
bool (*player_get_num)(PierPlayerSel sel, int32_t prop, double* out);
```

The four \*\_get\_num slots share one convention. The return is whether the host has an answer, and \*out is the answer; a false leaves \*out untouched.

False and "the value is zero" are different results and a caller must not collapse them (contract §5.2). False has two causes it does not distinguish: the subject was not found, and the property is one this engine version does not expose. Neither is an error the host logs, because a caller polling a property it cannot read would fill the log with it; the list of properties that always answer false on a given version is in CHANGELOG.md under the release.

- Call: `api->player_get_num(sel, prop, out)`
- Parameters:
    - sel : `PierPlayerSel`
    - prop : `int32_t`
    - out : `double*`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 28, counting from 0
- Callers in each binding:
    - Rust: [`Player::num`](../rust/player.md#Player.num), [`Player::game_type`](../rust/player.md#Player.game_type), [`Player::permission_level`](../rust/player.md#Player.permission_level), [`Player::dimension`](../rust/player.md#Player.dimension), [`Player::level`](../rust/player.md#Player.level), [`Player::experience`](../rust/player.md#Player.experience) and more, 36 in all
    - Go: [`Player.GameType`](../go/player.md#Player.GameType), [`Player.Level`](../go/player.md#Player.Level), [`Player.Experience`](../go/player.md#Player.Experience), [`Player.Hunger`](../go/player.md#Player.Hunger), [`Player.Saturation`](../go/player.md#Player.Saturation), [`Player.Exhaustion`](../go/player.md#Player.Exhaustion) and more, 37 in all
    - Zig: [`Player.gameType`](../zig/player.md#Player.gameType), [`Player.level`](../zig/player.md#Player.level), [`Player.experience`](../zig/player.md#Player.experience), [`Player.hunger`](../zig/player.md#Player.hunger), [`Player.saturation`](../zig/player.md#Player.saturation), [`Player.exhaustion`](../zig/player.md#Player.exhaustion) and more, 36 in all

### `player_get_str` {#player_get_str}

```c
bool (*player_get_str)(PierPlayerSel sel, int32_t prop, void* ctx, PierStrSink sink);
```

- Call: `api->player_get_str(sel, prop, ctx, sink)`
- Parameters:
    - sel : `PierPlayerSel`
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 29, counting from 0
- Callers in each binding:
    - Rust: [`Player::text`](../rust/player.md#Player.text), [`Player::last_death_pos`](../rust/player.md#Player.last_death_pos), [`Player::position`](../rust/player.md#Player.position), [`Player::real_name`](../rust/player.md#Player.real_name), [`Player::uuid`](../rust/player.md#Player.uuid), [`Player::xuid`](../rust/player.md#Player.xuid) and more, 11 in all
    - Go: [`Player.RealName`](../go/player.md#Player.RealName), [`Player.Uuid`](../go/player.md#Player.Uuid), [`Player.Xuid`](../go/player.md#Player.Xuid), [`Player.IpAndPort`](../go/player.md#Player.IpAndPort), [`Player.LocaleCode`](../go/player.md#Player.LocaleCode), [`Player.NameTag`](../go/player.md#Player.NameTag) and more, 12 in all
    - Zig: [`Player.realName`](../zig/player.md#Player.realName), [`Player.uuid`](../zig/player.md#Player.uuid), [`Player.xuid`](../zig/player.md#Player.xuid), [`Player.ipAndPort`](../zig/player.md#Player.ipAndPort), [`Player.localeCode`](../zig/player.md#Player.localeCode), [`Player.nameTag`](../zig/player.md#Player.nameTag) and more, 11 in all

### `player_set_num` {#player_set_num}

```c
bool (*player_set_num)(PierPlayerSel sel, int32_t prop, double v);
```

- Call: `api->player_set_num(sel, prop, v)`
- Parameters:
    - sel : `PierPlayerSel`
    - prop : `int32_t`
    - v : `double`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 30, counting from 0
- Callers in each binding:
    - Rust: [`Player::set_num`](../rust/player.md#Player.set_num), [`Player::set_level`](../rust/player.md#Player.set_level), [`Player::set_experience`](../rust/player.md#Player.set_experience), [`Player::set_hunger`](../rust/player.md#Player.set_hunger), [`Player::set_saturation`](../rust/player.md#Player.set_saturation), [`Player::set_exhaustion`](../rust/player.md#Player.set_exhaustion)
    - Go: [`Player.SetLevel`](../go/player.md#Player.SetLevel), [`Player.SetExperience`](../go/player.md#Player.SetExperience), [`Player.SetHunger`](../go/player.md#Player.SetHunger), [`Player.SetSaturation`](../go/player.md#Player.SetSaturation), [`Player.SetExhaustion`](../go/player.md#Player.SetExhaustion), [`Raw.PlayerSetNum`](../go/raw.md#Raw.PlayerSetNum)
    - Zig: [`Player.setLevel`](../zig/player.md#Player.setLevel), [`Player.setExperience`](../zig/player.md#Player.setExperience), [`Player.setHunger`](../zig/player.md#Player.setHunger), [`Player.setSaturation`](../zig/player.md#Player.setSaturation), [`Player.setExhaustion`](../zig/player.md#Player.setExhaustion)

### `player_action` {#player_action}

```c
bool (*player_action)(
    PierPlayerSel sel,
    int32_t action,
    PierStr sarg,
    double a,
    double b,
    double c,
    void* ctx,
    PierStrSink out
);
```

- Call: `api->player_action(sel, action, sarg, a, b, c, ctx, out)`
- Parameters:
    - sel : `PierPlayerSel`
    - action : `int32_t`
    - sarg : `PierStr`
    - a : `double`
    - b : `double`
    - c : `double`
    - ctx : `void*`
    - out : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §B player management
- Position in the table: slot 31, counting from 0
- Callers in each binding:
    - Rust: [`Player::act`](../rust/player.md#Player.act), [`Player::set_ability`](../rust/player.md#Player.set_ability), [`Player::set_ability_raw`](../rust/player.md#Player.set_ability_raw), [`Player::can_use_ability`](../rust/player.md#Player.can_use_ability), [`Player::set_permission_level`](../rust/player.md#Player.set_permission_level), [`Player::set_selected_slot`](../rust/player.md#Player.set_selected_slot) and more, 22 in all
    - Go: [`Player.SetAbility`](../go/player.md#Player.SetAbility), [`Player.CanUseAbilityAction`](../go/player.md#Player.CanUseAbilityAction), [`Player.SetSelectedSlot`](../go/player.md#Player.SetSelectedSlot), [`Player.GiveItem`](../go/player.md#Player.GiveItem), [`Player.SetSpawnPoint`](../go/player.md#Player.SetSpawnPoint), [`Player.ClearTitle`](../go/player.md#Player.ClearTitle) and more, 28 in all
    - Zig: [`Player.setAbility`](../zig/player.md#Player.setAbility), [`Player.canUseAbilityAction`](../zig/player.md#Player.canUseAbilityAction), [`Player.setSelectedSlot`](../zig/player.md#Player.setSelectedSlot), [`Player.giveItem`](../zig/player.md#Player.giveItem), [`Player.setSpawnPoint`](../zig/player.md#Player.setSpawnPoint), [`Player.clearTitle`](../zig/player.md#Player.clearTitle) and more, 27 in all

### `player_send_message_typed` {#player_send_message_typed}

```c
bool (*player_send_message_typed)(PierPlayerSel sel, PierStr msg, int32_t type);
```

!!! note "Group note"

    Send a message of a specific TextPacketType to one player (additive, gated by `struct_size`). `type` is a TextPacketType value:

    ```text
    0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip ·
    6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper ·
    10 TextObject · 11 TextObjectAnnouncement.
    ```

    Out-of-range falls back to Raw. Single-string body (like LSE tell): the author/param kinds (Chat/Whisper/Translate) arrive as plain text. plain `player_send_message` remains the Raw/Chat convenience path.

- Call: `api->player_send_message_typed(sel, msg, type)`
- Parameters:
    - sel : `PierPlayerSel`
    - msg : `PierStr`
    - type : `int32_t`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 91, counting from 0
- Callers in each binding:
    - Rust: [`Player::tell`](../rust/player.md#Player.tell)
    - Go: [`Player.SendMessageTyped`](../go/player.md#Player.SendMessageTyped), [`Raw.PlayerSendMessageTyped`](../go/raw.md#Raw.PlayerSendMessageTyped)

### `player_get_carried_item` {#player_get_carried_item}

```c
bool (*player_get_carried_item)(PierPlayerSel sel, void* ctx, PierStrSink sink);
```

- Call: `api->player_get_carried_item(sel, ctx, sink)`
- Parameters:
    - sel : `PierPlayerSel`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Player: equipment, cooldown, network (dedicated fns)
- Position in the table: slot 102, counting from 0
- Callers in each binding:
    - Rust: [`Player::carried_item`](../rust/player.md#Player.carried_item)
    - Go: [`Player.CarriedItem`](../go/player.md#Player.CarriedItem), [`Raw.PlayerGetCarriedItem`](../go/raw.md#Raw.PlayerGetCarriedItem)

### `player_get_item` {#player_get_item}

```c
bool (*player_get_item)(PierPlayerSel sel, int32_t slot, void* ctx, PierStrSink sink);
```

- Call: `api->player_get_item(sel, slot, ctx, sink)`
- Parameters:
    - sel : `PierPlayerSel`
    - slot : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Player: equipment, cooldown, network (dedicated fns)
- Position in the table: slot 103, counting from 0
- Callers in each binding:
    - Rust: [`Player::item`](../rust/player.md#Player.item)
    - Go: [`Player.InventoryItem`](../go/player.md#Player.InventoryItem), [`Raw.PlayerGetItem`](../go/raw.md#Raw.PlayerGetItem)

### `player_set_item` {#player_set_item}

```c
bool (*player_set_item)(PierPlayerSel sel, int32_t slot, PierStr item_snbt);
```

- Call: `api->player_set_item(sel, slot, item_snbt)`
- Parameters:
    - sel : `PierPlayerSel`
    - slot : `int32_t`
    - item_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: Player: equipment, cooldown, network (dedicated fns)
- Position in the table: slot 104, counting from 0
- Callers in each binding:
    - Rust: [`Player::set_item`](../rust/player.md#Player.set_item)
    - Go: [`Player.SetInventoryItem`](../go/player.md#Player.SetInventoryItem), [`Raw.PlayerSetItem`](../go/raw.md#Raw.PlayerSetItem)

### `player_get_equipment` {#player_get_equipment}

```c
bool (*player_get_equipment)(PierPlayerSel sel, void* ctx, PierStrSink sink);
```

All equipment as SNBT: \[{slot, `item_snbt`},…\] slot: 0=mainhand 1=offhand 2-5=armor

- Call: `api->player_get_equipment(sel, ctx, sink)`
- Parameters:
    - sel : `PierPlayerSel`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Player: equipment, cooldown, network (dedicated fns)
- Position in the table: slot 105, counting from 0
- Callers in each binding:
    - Rust: [`Player::equipment`](../rust/player.md#Player.equipment)
    - Go: [`Raw.PlayerGetEquipment`](../go/raw.md#Raw.PlayerGetEquipment)

### `player_get_cooldown` {#player_get_cooldown}

```c
int32_t (*player_get_cooldown)(PierPlayerSel sel, PierStr item_name);
```

Ticks remaining for an item cooldown (-1 if not on cooldown / player offline).

- Call: `api->player_get_cooldown(sel, item_name)`
- Parameters:
    - sel : `PierPlayerSel`
    - item_name : `PierStr`
- Return type: `int32_t`
- Section of abi.h: Player: equipment, cooldown, network (dedicated fns)
- Position in the table: slot 106, counting from 0
- Callers in each binding:
    - Rust: [`Player::cooldown`](../rust/player.md#Player.cooldown)
    - Go: [`Raw.PlayerGetCooldown`](../go/raw.md#Raw.PlayerGetCooldown)

### `player_start_cooldown` {#player_start_cooldown}

```c
bool (*player_start_cooldown)(PierPlayerSel sel, PierStr item_name, int32_t ticks);
```

- Call: `api->player_start_cooldown(sel, item_name, ticks)`
- Parameters:
    - sel : `PierPlayerSel`
    - item_name : `PierStr`
    - ticks : `int32_t`
- Return type: `bool`
- Section of abi.h: Player: equipment, cooldown, network (dedicated fns)
- Position in the table: slot 107, counting from 0
- Callers in each binding:
    - Rust: [`Player::start_cooldown`](../rust/player.md#Player.start_cooldown)
    - Go: [`Raw.PlayerStartCooldown`](../go/raw.md#Raw.PlayerStartCooldown)

### `player_get_network_status` {#player_get_network_status}

```c
bool (*player_get_network_status)(PierPlayerSel sel, void* ctx, PierStrSink sink);
```

- Call: `api->player_get_network_status(sel, ctx, sink)`
- Parameters:
    - sel : `PierPlayerSel`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Player: equipment, cooldown, network (dedicated fns)
- Position in the table: slot 108, counting from 0
- Callers in each binding:
    - Rust: [`Player::network_status`](../rust/player.md#Player.network_status)
    - Go: [`Raw.PlayerGetNetworkStatus`](../go/raw.md#Raw.PlayerGetNetworkStatus)

### `player_send_title` {#player_send_title}

```c
bool (*player_send_title)(
    PierPlayerSel sel, int32_t type, PierStr text, int32_t fade_in_ticks,
    int32_t stay_ticks, int32_t fade_out_ticks);
```

- Call: `api->player_send_title(sel, type, text, fade_in_ticks, stay_ticks, fade_out_ticks)`
- Parameters:
    - sel : `PierPlayerSel`
    - type : `int32_t`
    - text : `PierStr`
    - fade_in_ticks : `int32_t`
    - stay_ticks : `int32_t`
    - fade_out_ticks : `int32_t`
- Return type: `bool`
- Section of abi.h: Titles
- Position in the table: slot 156, counting from 0
- Callers in each binding:
    - Rust: [`Player::send_title`](../rust/player.md#Player.send_title), [`Player::set_title`](../rust/player.md#Player.set_title), [`Player::set_subtitle`](../rust/player.md#Player.set_subtitle), [`Player::set_actionbar`](../rust/player.md#Player.set_actionbar), [`Player::clear_title`](../rust/player.md#Player.clear_title)
    - Go: [`Player.SendTitle`](../go/player.md#Player.SendTitle), [`Raw.PlayerSendTitle`](../go/raw.md#Raw.PlayerSendTitle)

### `player_conn_id` {#player_conn_id}

```c
uint64_t (*player_conn_id)(PierPlayerSel who);
```

This player's connection id — the same number packet interceptors see in the packet context.

A packet callback has only `conn_id`, not a player, so per-player rewriting of outbound packets is impossible without this: locking the sky color needs the dimension of the person on that connection, which needs to know who they are.

The alternative is to periodically send packets that override the server's, and that is wrong: the server sends real time while the mod sends locked time, the two kinds interleave, and the client's sky flickers between them. Rewriting is correct, and rewriting needs this slot.

@return the connection id; 0 if the player is offline or their network

```text
identifier is unavailable.
```

Pure append: `PIER_ABI_VERSION` is unchanged, `struct_size` is the gate.

- Call: `api->player_conn_id(who)`
- Parameters:
    - who : `PierPlayerSel`
- Return type: `uint64_t`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 180, counting from 0
- Callers in each binding:
    - Rust: [`Player::conn_id`](../rust/player.md#Player.conn_id)
    - Go: [`Player.ConnID`](../go/player.md#Player.ConnID), [`Raw.PlayerConnId`](../go/raw.md#Raw.PlayerConnId)

## `PierPlayerNumProp` {#PierPlayerNumProp}

`player_get_num` / `player_set_num` keys. (G)=get-only, (S)=settable.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_PPROP_GAME_TYPE"></span>`PIER_PPROP_GAME_TYPE` | `0` | (G) `Player::getPlayerGameType`; write via `player_set_gamemode` |
| <span id="PIER_PPROP_LEVEL"></span>`PIER_PPROP_LEVEL` | `1` | (S) attribute `Player::LEVEL()` |
| <span id="PIER_PPROP_EXPERIENCE"></span>`PIER_PPROP_EXPERIENCE` | `2` | (S) attribute `Player::EXPERIENCE()` (progress 0..1) |
| <span id="PIER_PPROP_HUNGER"></span>`PIER_PPROP_HUNGER` | `3` | (S) attribute `Player::HUNGER()` |
| <span id="PIER_PPROP_SATURATION"></span>`PIER_PPROP_SATURATION` | `4` | (S) attribute `Player::SATURATION()` |
| <span id="PIER_PPROP_EXHAUSTION"></span>`PIER_PPROP_EXHAUSTION` | `5` | (S) attribute `Player::EXHAUSTION()` |
| <span id="PIER_PPROP_XP_NEEDED_NEXT_LEVEL"></span>`PIER_PPROP_XP_NEEDED_NEXT_LEVEL` | `6` | (G) `Player::getXpNeededForNextLevel` |
| <span id="PIER_PPROP_LUCK"></span>`PIER_PPROP_LUCK` | `7` | (G) `Player::getLuck` |
| <span id="PIER_PPROP_SELECTED_SLOT"></span>`PIER_PPROP_SELECTED_SLOT` | `8` | (G) `Player::getSelectedItemSlot`; set via `PIER_PACT_SET_SELECTED_SLOT` |
| <span id="PIER_PPROP_IS_OPERATOR"></span>`PIER_PPROP_IS_OPERATOR` | `9` | (G) `Player::isOperator` |
| <span id="PIER_PPROP_CAN_USE_OPERATOR_BLOCKS"></span>`PIER_PPROP_CAN_USE_OPERATOR_BLOCKS` | `10` | (G) `Player::canUseOperatorBlocks` |
| <span id="PIER_PPROP_IS_FLYING"></span>`PIER_PPROP_IS_FLYING` | `11` | (G) `Player::isFlying` |
| <span id="PIER_PPROP_CAN_JUMP"></span>`PIER_PPROP_CAN_JUMP` | `12` | (G) `Player::canJump` |
| <span id="PIER_PPROP_IS_EMOTING"></span>`PIER_PPROP_IS_EMOTING` | `13` | (G) `Player::isEmoting` |
| <span id="PIER_PPROP_IS_IN_RAID"></span>`PIER_PPROP_IS_IN_RAID` | `14` | (G) `Player::isInRaid` |
| <span id="PIER_PPROP_IS_HURT"></span>`PIER_PPROP_IS_HURT` | `15` | (G) `Player::isHurt` |
| <span id="PIER_PPROP_IS_SCOPING"></span>`PIER_PPROP_IS_SCOPING` | `16` | (G) `Player::isScoping` |
| <span id="PIER_PPROP_CAN_SLEEP"></span>`PIER_PPROP_CAN_SLEEP` | `17` | (G) `Player::canSleep` |
| <span id="PIER_PPROP_HAS_RESPAWN_POSITION"></span>`PIER_PPROP_HAS_RESPAWN_POSITION` | `18` | (G) `Player::hasRespawnPosition` |
| <span id="PIER_PPROP_CLIENT_SUB_ID"></span>`PIER_PPROP_CLIENT_SUB_ID` | `19` | (G) `Player::getClientSubId` |
| <span id="PIER_PPROP_CAN_USE_ABILITY"></span>`PIER_PPROP_CAN_USE_ABILITY` | `20` | (G) `Player::canUseAbility`; the ability index is passed through the `player_action` GET path, see `PIER_PACT_CAN_USE_ABILITY` |
| <span id="PIER_PPROP_DIRECTION"></span>`PIER_PPROP_DIRECTION` | `21` | (G) `Player::getDirection` (0=S,1=W,2=N,3=E) |
| <span id="PIER_PPROP_CHUNK_RADIUS"></span>`PIER_PPROP_CHUNK_RADIUS` | `22` | (G) `Player::getChunkRadius` |
| <span id="PIER_PPROP_NETWORK_RTT"></span>`PIER_PPROP_NETWORK_RTT` | `23` | (G) getNetworkStatus().mPing (ms) |
| <span id="PIER_PPROP_PLATFORM"></span>`PIER_PPROP_PLATFORM` | `24` | (G) `Player::getPlatform` |
| <span id="PIER_PPROP_ENCHANTMENT_SEED"></span>`PIER_PPROP_ENCHANTMENT_SEED` | `25` | (G) `Player::getEnchantmentSeed` |
| <span id="PIER_PPROP_IS_USING_ITEM"></span>`PIER_PPROP_IS_USING_ITEM` | `26` | (G) `Player::isUsingItem` |
| <span id="PIER_PPROP_IS_BLOCKING"></span>`PIER_PPROP_IS_BLOCKING` | `27` | (G) `Player::isBlocking` |
| <span id="PIER_PPROP_IS_GLIDING"></span>`PIER_PPROP_IS_GLIDING` | `28` | (G) `Player::isGliding` |
| <span id="PIER_PPROP_IS_SWIMMING"></span>`PIER_PPROP_IS_SWIMMING` | `29` | (G) `Player::isSwimming` |
| <span id="PIER_PPROP_PERMISSION_LEVEL"></span>`PIER_PPROP_PERMISSION_LEVEL` | `30` | (G) `Player::getPlayerPermissionLevel` |
| <span id="PIER_PPROP_SCORE"></span>`PIER_PPROP_SCORE` | `31` | (G) `Player::getScore` |
| <span id="PIER_PPROP_FALL_DISTANCE"></span>`PIER_PPROP_FALL_DISTANCE` | `32` | (G) `Actor::getFallDistance` |
| <span id="PIER_PPROP_IS_DEAD"></span>`PIER_PPROP_IS_DEAD` | `33` | (G) `Actor::isDead` |
| <span id="PIER_PPROP_HAS_DIED_BEFORE"></span>`PIER_PPROP_HAS_DIED_BEFORE` | `34` | (G) `Player::hasDiedBefore` |
| <span id="PIER_PPROP_DIMENSION"></span>`PIER_PPROP_DIMENSION` | `35` | (G) `Actor::getDimensionId` |

## `PierPlayerStrProp` {#PierPlayerStrProp}

`player_get_str` keys.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_PSTR_REAL_NAME"></span>`PIER_PSTR_REAL_NAME` | `0` | `Player::getRealName` |
| <span id="PIER_PSTR_UUID"></span>`PIER_PSTR_UUID` | `1` | `Player::getUuid()`.asString() |
| <span id="PIER_PSTR_XUID"></span>`PIER_PSTR_XUID` | `2` | `Player::getXuid` |
| <span id="PIER_PSTR_IP_AND_PORT"></span>`PIER_PSTR_IP_AND_PORT` | `3` | `Player::getIPAndPort` |
| <span id="PIER_PSTR_LOCALE_CODE"></span>`PIER_PSTR_LOCALE_CODE` | `4` | `Player::getLocaleCode` |
| <span id="PIER_PSTR_NAME_TAG"></span>`PIER_PSTR_NAME_TAG` | `5` | `Actor::getNameTag` (display name) |
| <span id="PIER_PSTR_LAST_DEATH_POS"></span>`PIER_PSTR_LAST_DEATH_POS` | `6` | SNBT {x,y,z} or "" if none |
| <span id="PIER_PSTR_LAST_DEATH_DIMENSION"></span>`PIER_PSTR_LAST_DEATH_DIMENSION` | `7` | dimension id as string |
| <span id="PIER_PSTR_NETWORK_STATUS"></span>`PIER_PSTR_NETWORK_STATUS` | `8` | SNBT {ping,`avg_ping`,`packet_loss`,`max_ping`} |
| <span id="PIER_PSTR_PLATFORM_ONLINE_ID"></span>`PIER_PSTR_PLATFORM_ONLINE_ID` | `9` | `Player::getPlatformOnlineId` |
| <span id="PIER_PSTR_RESPAWN_POS"></span>`PIER_PSTR_RESPAWN_POS` | `10` | SNBT {x,y,z,dim}: where this player would respawn. Never empty; a player with no bed reports the world spawn, and the two are not distinguishable. |

## `PierPlayerAction` {#PierPlayerAction}

`player_action` verbs.  Args are (sarg, a, b, c); unused args are ignored. `out` (when non-NULL) receives a result string where noted.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_PACT_SET_ABILITY"></span>`PIER_PACT_SET_ABILITY` | `0` | a=AbilitiesIndex, b=0/1 (bool slots) or float (FlySpeed etc.). Restores PlayerPermissionLevel to its pre-write value afterwards: the engine's `LayeredAbilities::setAbility` is the "switch to custom permissions" path and pushes the player to Custom, and that level ships to the client inside UpdateAbilitiesPacket together with the ability layer. To change the level, use `PIER_PACT_SET_PERMISSION_LEVEL`. Refused (false) until the player has finished joining (`Player::isPlayerInitialized`): an ability written while the client is still loading desynchronizes until the player rejoins. Simulated players are exempt. |
| <span id="PIER_PACT_CAN_USE_ABILITY"></span>`PIER_PACT_CAN_USE_ABILITY` | `1` | a=AbilitiesIndex → out "0"/"1" `Player::canUseAbility` |
| <span id="PIER_PACT_SET_SELECTED_SLOT"></span>`PIER_PACT_SET_SELECTED_SLOT` | `2` | a=slot `Player::setSelectedSlot` |
| <span id="PIER_PACT_GIVE_ITEM"></span>`PIER_PACT_GIVE_ITEM` | `3` | sarg=item SNBT `ItemStack::fromTag` + `Player::addAndRefresh` |
| <span id="PIER_PACT_SET_SPAWN_POINT"></span>`PIER_PACT_SET_SPAWN_POINT` | `4` | a,b,c=pos, sarg=dim id (any registered dim); native `Player::setRespawnPosition` |
| <span id="PIER_PACT_CLEAR_TITLE"></span>`PIER_PACT_CLEAR_TITLE` | `5` | native SetTitlePacket(Clear) |
| <span id="PIER_PACT_SET_TITLE"></span>`PIER_PACT_SET_TITLE` | `6` | sarg=text, a=slot(0 title,1 subtitle,2 actionbar); native SetTitlePacket, text sent verbatim |
| <span id="PIER_PACT_ADD_EXPERIENCE"></span>`PIER_PACT_ADD_EXPERIENCE` | `7` | a=xp `Player::addExperience` |
| <span id="PIER_PACT_ADD_LEVELS"></span>`PIER_PACT_ADD_LEVELS` | `8` | a=levels `Player::addLevels` |
| <span id="PIER_PACT_START_COOLDOWN"></span>`PIER_PACT_START_COOLDOWN` | `9` | sarg=item name, a=ticks `Player::startItemCooldown` |
| <span id="PIER_PACT_START_RIDING"></span>`PIER_PACT_START_RIDING` | `10` | a=vehicle ActorUniqueID (lower 64b) `Player::startRiding` |
| <span id="PIER_PACT_STOP_RIDING"></span>`PIER_PACT_STOP_RIDING` | `11` | `Player::stopRiding` |
| <span id="PIER_PACT_ATTACK"></span>`PIER_PACT_ATTACK` | `12` | a=target ActorUniqueID (lower 64b) `Player::attack` |
| <span id="PIER_PACT_DROP"></span>`PIER_PACT_DROP` | `13` | sarg=item SNBT, a=random(0/1) `Player::drop` |
| <span id="PIER_PACT_INTERACT"></span>`PIER_PACT_INTERACT` | `14` | a=target ActorUniqueID `Player::interact` |
| <span id="PIER_PACT_START_USING_ITEM"></span>`PIER_PACT_START_USING_ITEM` | `15` | sarg=item SNBT, a=duration `Player::startUsingItem` |
| <span id="PIER_PACT_STOP_USING_ITEM"></span>`PIER_PACT_STOP_USING_ITEM` | `16` | `Player::stopUsingItem` |
| <span id="PIER_PACT_SET_CHUNK_RADIUS"></span>`PIER_PACT_SET_CHUNK_RADIUS` | `17` | a=radius `Player::setChunkRadius` |
| <span id="PIER_PACT_SET_ENCHANTMENT_SEED"></span>`PIER_PACT_SET_ENCHANTMENT_SEED` | `18` | a=seed `Player::setEnchantmentSeed` |
| <span id="PIER_PACT_REGISTER_TRACKED_BOSS"></span>`PIER_PACT_REGISTER_TRACKED_BOSS` | `19` | a=boss ActorUniqueID `Player::registerTrackedBoss` |
| <span id="PIER_PACT_UNREGISTER_TRACKED_BOSS"></span>`PIER_PACT_UNREGISTER_TRACKED_BOSS` | `20` | a=boss ActorUniqueID `Player::unRegisterTrackedBoss` |
| <span id="PIER_PACT_PLAY_EMOTE"></span>`PIER_PACT_PLAY_EMOTE` | `21` | sarg=piece id `Player::playEmote` |
| <span id="PIER_PACT_RESEND_ALL_CHUNKS"></span>`PIER_PACT_RESEND_ALL_CHUNKS` | `22` | `Player::resendAllChunks` |
| <span id="PIER_PACT_OPEN_INVENTORY"></span>`PIER_PACT_OPEN_INVENTORY` | `23` | `Player::openInventory` |
| <span id="PIER_PACT_SIDEBAR_SET"></span>`PIER_PACT_SIDEBAR_SET` | `24` | sarg="obj\\ntitle\\nline…" per-player sidebar |
| <span id="PIER_PACT_SIDEBAR_CLEAR"></span>`PIER_PACT_SIDEBAR_CLEAR` | `25` | sarg=objective RemoveObjectivePacket |
| <span id="PIER_PACT_SET_PERMISSION_LEVEL"></span>`PIER_PACT_SET_PERMISSION_LEVEL` | `26` | a=PlayerPermissionLevel (0 Visitor, 1 Member, 2 Operator, 3 Custom). `LayeredAbilities::setPlayerPermissions` plus UpdateAbilitiesPacket. The read side is `PIER_PPROP_PERMISSION_LEVEL`. Refused until the player has finished joining, like `PIER_PACT_SET_ABILITY`. |
