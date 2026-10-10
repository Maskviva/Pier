# Zig: Players

## `Player` {#Player}

```zig
pub const Player = struct {
    sel_kind: SelKind,
    sel_value: []const u8,
    // ...
};
```

One player, named by a selector.

### `Player.byName` {#Player.byName}

```zig
pub fn byName(sel_name: []const u8) Player
```

- Parameters:
    - sel_name : `[]const u8`
- Return type: `Player`

### `Player.byXuid` {#Player.byXuid}

```zig
pub fn byXuid(sel_xuid: []const u8) Player
```

- Parameters:
    - sel_xuid : `[]const u8`
- Return type: `Player`

### `Player.byUuid` {#Player.byUuid}

```zig
pub fn byUuid(sel_uuid: []const u8) Player
```

- Parameters:
    - sel_uuid : `[]const u8`
- Return type: `Player`

### `Player.cSel` {#Player.cSel}

```zig
pub fn cSel(self: Player) core.c.PierPlayerSel
```

- Return type: `core.c.PierPlayerSel`

### `Player.gameType` {#Player.gameType}

```zig
pub fn gameType(self: Player) core.Error!f64
```

`PIER_PPROP_GAME_TYPE`: (G) `Player::getPlayerGameType`; write via `player_set_gamemode`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.level` {#Player.level}

```zig
pub fn level(self: Player) core.Error!f64
```

`PIER_PPROP_LEVEL`: (S) attribute `Player::LEVEL()`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setLevel` {#Player.setLevel}

```zig
pub fn setLevel(self: Player, new_value: f64) core.Error!void
```

Writes `PIER_PPROP_LEVEL`: (S) attribute `Player::LEVEL()`

- Parameters:
    - new_value : `f64`
- Return type: `core.Error!void`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.experience` {#Player.experience}

```zig
pub fn experience(self: Player) core.Error!f64
```

`PIER_PPROP_EXPERIENCE`: (S) attribute `Player::EXPERIENCE()` (progress 0..1)

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setExperience` {#Player.setExperience}

```zig
pub fn setExperience(self: Player, new_value: f64) core.Error!void
```

Writes `PIER_PPROP_EXPERIENCE`: (S) attribute `Player::EXPERIENCE()` (progress 0..1)

- Parameters:
    - new_value : `f64`
- Return type: `core.Error!void`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.hunger` {#Player.hunger}

```zig
pub fn hunger(self: Player) core.Error!f64
```

`PIER_PPROP_HUNGER`: (S) attribute `Player::HUNGER()`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setHunger` {#Player.setHunger}

```zig
pub fn setHunger(self: Player, new_value: f64) core.Error!void
```

Writes `PIER_PPROP_HUNGER`: (S) attribute `Player::HUNGER()`

- Parameters:
    - new_value : `f64`
- Return type: `core.Error!void`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.saturation` {#Player.saturation}

```zig
pub fn saturation(self: Player) core.Error!f64
```

`PIER_PPROP_SATURATION`: (S) attribute `Player::SATURATION()`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setSaturation` {#Player.setSaturation}

```zig
pub fn setSaturation(self: Player, new_value: f64) core.Error!void
```

Writes `PIER_PPROP_SATURATION`: (S) attribute `Player::SATURATION()`

- Parameters:
    - new_value : `f64`
- Return type: `core.Error!void`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.exhaustion` {#Player.exhaustion}

```zig
pub fn exhaustion(self: Player) core.Error!f64
```

`PIER_PPROP_EXHAUSTION`: (S) attribute `Player::EXHAUSTION()`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setExhaustion` {#Player.setExhaustion}

```zig
pub fn setExhaustion(self: Player, new_value: f64) core.Error!void
```

Writes `PIER_PPROP_EXHAUSTION`: (S) attribute `Player::EXHAUSTION()`

- Parameters:
    - new_value : `f64`
- Return type: `core.Error!void`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.xpNeededNextLevel` {#Player.xpNeededNextLevel}

```zig
pub fn xpNeededNextLevel(self: Player) core.Error!f64
```

`PIER_PPROP_XP_NEEDED_NEXT_LEVEL`: (G) `Player::getXpNeededForNextLevel`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.luck` {#Player.luck}

```zig
pub fn luck(self: Player) core.Error!f64
```

`PIER_PPROP_LUCK`: (G) `Player::getLuck`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.selectedSlot` {#Player.selectedSlot}

```zig
pub fn selectedSlot(self: Player) core.Error!f64
```

`PIER_PPROP_SELECTED_SLOT`: (G) `Player::getSelectedItemSlot`; set via `PIER_PACT_SET_SELECTED_SLOT`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isOperator` {#Player.isOperator}

```zig
pub fn isOperator(self: Player) core.Error!bool
```

`PIER_PPROP_IS_OPERATOR`: (G) `Player::isOperator`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canUseOperatorBlocks` {#Player.canUseOperatorBlocks}

```zig
pub fn canUseOperatorBlocks(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_USE_OPERATOR_BLOCKS`: (G) `Player::canUseOperatorBlocks`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isFlying` {#Player.isFlying}

```zig
pub fn isFlying(self: Player) core.Error!bool
```

`PIER_PPROP_IS_FLYING`: (G) `Player::isFlying`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canJump` {#Player.canJump}

```zig
pub fn canJump(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_JUMP`: (G) `Player::canJump`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isEmoting` {#Player.isEmoting}

```zig
pub fn isEmoting(self: Player) core.Error!bool
```

`PIER_PPROP_IS_EMOTING`: (G) `Player::isEmoting`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isInRaid` {#Player.isInRaid}

```zig
pub fn isInRaid(self: Player) core.Error!bool
```

`PIER_PPROP_IS_IN_RAID`: (G) `Player::isInRaid`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isHurt` {#Player.isHurt}

```zig
pub fn isHurt(self: Player) core.Error!bool
```

`PIER_PPROP_IS_HURT`: (G) `Player::isHurt`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isScoping` {#Player.isScoping}

```zig
pub fn isScoping(self: Player) core.Error!bool
```

`PIER_PPROP_IS_SCOPING`: (G) `Player::isScoping`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canSleep` {#Player.canSleep}

```zig
pub fn canSleep(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_SLEEP`: (G) `Player::canSleep`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.hasRespawnPosition` {#Player.hasRespawnPosition}

```zig
pub fn hasRespawnPosition(self: Player) core.Error!bool
```

`PIER_PPROP_HAS_RESPAWN_POSITION`: (G) `Player::hasRespawnPosition`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.clientSubId` {#Player.clientSubId}

```zig
pub fn clientSubId(self: Player) core.Error!f64
```

`PIER_PPROP_CLIENT_SUB_ID`: (G) `Player::getClientSubId`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canUseAbility` {#Player.canUseAbility}

```zig
pub fn canUseAbility(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_USE_ABILITY`: (G) `Player::canUseAbility`; the ability index is passed through the `player_action` GET path, see `PIER_PACT_CAN_USE_ABILITY`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.direction` {#Player.direction}

```zig
pub fn direction(self: Player) core.Error!f64
```

`PIER_PPROP_DIRECTION`: (G) `Player::getDirection` (0=S,1=W,2=N,3=E)

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.chunkRadius` {#Player.chunkRadius}

```zig
pub fn chunkRadius(self: Player) core.Error!f64
```

`PIER_PPROP_CHUNK_RADIUS`: (G) `Player::getChunkRadius`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.networkRtt` {#Player.networkRtt}

```zig
pub fn networkRtt(self: Player) core.Error!f64
```

`PIER_PPROP_NETWORK_RTT`: (G) getNetworkStatus().mPing (ms)

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.platform` {#Player.platform}

```zig
pub fn platform(self: Player) core.Error!f64
```

`PIER_PPROP_PLATFORM`: (G) `Player::getPlatform`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.enchantmentSeed` {#Player.enchantmentSeed}

```zig
pub fn enchantmentSeed(self: Player) core.Error!f64
```

`PIER_PPROP_ENCHANTMENT_SEED`: (G) `Player::getEnchantmentSeed`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isUsingItem` {#Player.isUsingItem}

```zig
pub fn isUsingItem(self: Player) core.Error!bool
```

`PIER_PPROP_IS_USING_ITEM`: (G) `Player::isUsingItem`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isBlocking` {#Player.isBlocking}

```zig
pub fn isBlocking(self: Player) core.Error!bool
```

`PIER_PPROP_IS_BLOCKING`: (G) `Player::isBlocking`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isGliding` {#Player.isGliding}

```zig
pub fn isGliding(self: Player) core.Error!bool
```

`PIER_PPROP_IS_GLIDING`: (G) `Player::isGliding`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isSwimming` {#Player.isSwimming}

```zig
pub fn isSwimming(self: Player) core.Error!bool
```

`PIER_PPROP_IS_SWIMMING`: (G) `Player::isSwimming`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.permissionLevel` {#Player.permissionLevel}

```zig
pub fn permissionLevel(self: Player) core.Error!f64
```

`PIER_PPROP_PERMISSION_LEVEL`: (G) `Player::getPlayerPermissionLevel`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.score` {#Player.score}

```zig
pub fn score(self: Player) core.Error!f64
```

`PIER_PPROP_SCORE`: (G) `Player::getScore`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.fallDistance` {#Player.fallDistance}

```zig
pub fn fallDistance(self: Player) core.Error!f64
```

`PIER_PPROP_FALL_DISTANCE`: (G) `Actor::getFallDistance`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isDead` {#Player.isDead}

```zig
pub fn isDead(self: Player) core.Error!bool
```

`PIER_PPROP_IS_DEAD`: (G) `Actor::isDead`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.hasDiedBefore` {#Player.hasDiedBefore}

```zig
pub fn hasDiedBefore(self: Player) core.Error!bool
```

`PIER_PPROP_HAS_DIED_BEFORE`: (G) `Player::hasDiedBefore`

- Return type: `core.Error!bool`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.dimension` {#Player.dimension}

```zig
pub fn dimension(self: Player) core.Error!f64
```

`PIER_PPROP_DIMENSION`: (G) `Actor::getDimensionId`

- Return type: `core.Error!f64`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.realName` {#Player.realName}

```zig
pub fn realName(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_REAL_NAME`: `Player::getRealName`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.uuid` {#Player.uuid}

```zig
pub fn uuid(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_UUID`: `Player::getUuid()`.asString()

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.xuid` {#Player.xuid}

```zig
pub fn xuid(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_XUID`: `Player::getXuid`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.ipAndPort` {#Player.ipAndPort}

```zig
pub fn ipAndPort(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_IP_AND_PORT`: `Player::getIPAndPort`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.localeCode` {#Player.localeCode}

```zig
pub fn localeCode(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_LOCALE_CODE`: `Player::getLocaleCode`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.nameTag` {#Player.nameTag}

```zig
pub fn nameTag(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_NAME_TAG`: `Actor::getNameTag` (display name)

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.lastDeathPos` {#Player.lastDeathPos}

```zig
pub fn lastDeathPos(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_LAST_DEATH_POS`: SNBT {x,y,z} or "" if none

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.lastDeathDimension` {#Player.lastDeathDimension}

```zig
pub fn lastDeathDimension(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_LAST_DEATH_DIMENSION`: dimension id as string

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.networkStatus` {#Player.networkStatus}

```zig
pub fn networkStatus(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_NETWORK_STATUS`: SNBT {ping,`avg_ping`,`packet_loss`,`max_ping`}

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.platformOnlineId` {#Player.platformOnlineId}

```zig
pub fn platformOnlineId(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_PLATFORM_ONLINE_ID`: `Player::getPlatformOnlineId`

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.respawnPos` {#Player.respawnPos}

```zig
pub fn respawnPos(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_RESPAWN_POS`: SNBT {x,y,z,dim}: where this player would respawn. Never empty; a player with no bed reports the world spawn, and the two are not distinguishable.

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `core.Error![]u8`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.setAbility` {#Player.setAbility}

```zig
pub fn setAbility(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_ABILITY`: a=AbilitiesIndex, b=0/1 (bool slots) or float (FlySpeed etc.). Restores PlayerPermissionLevel to its pre-write value afterwards: the engine's `LayeredAbilities::setAbility` is the "switch to custom permissions" path and pushes the player to Custom, and that level ships to the client inside UpdateAbilitiesPacket together with the ability layer. To change the level, use `PIER_PACT_SET_PERMISSION_LEVEL`. Refused (false) until the player has finished joining (`Player::isPlayerInitialized`): an ability written while the client is still loading desynchronizes until the player rejoins. Simulated players are exempt.

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.canUseAbilityAction` {#Player.canUseAbilityAction}

```zig
pub fn canUseAbilityAction(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_CAN_USE_ABILITY`: a=AbilitiesIndex → out "0"/"1" `Player::canUseAbility`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.setSelectedSlot` {#Player.setSelectedSlot}

```zig
pub fn setSelectedSlot(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_SELECTED_SLOT`: a=slot `Player::setSelectedSlot`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.giveItem` {#Player.giveItem}

```zig
pub fn giveItem(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_GIVE_ITEM`: sarg=item SNBT `ItemStack::fromTag` + `Player::addAndRefresh`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.setSpawnPoint` {#Player.setSpawnPoint}

```zig
pub fn setSpawnPoint(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_SPAWN_POINT`: a,b,c=pos, sarg=dim id (any registered dim); native `Player::setRespawnPosition`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.clearTitle` {#Player.clearTitle}

```zig
pub fn clearTitle(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_CLEAR_TITLE`: native SetTitlePacket(Clear)

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.setTitle` {#Player.setTitle}

```zig
pub fn setTitle(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_TITLE`: sarg=text, a=slot(0 title,1 subtitle,2 actionbar); native SetTitlePacket, text sent verbatim

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.addExperience` {#Player.addExperience}

```zig
pub fn addExperience(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_ADD_EXPERIENCE`: a=xp `Player::addExperience`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.addLevels` {#Player.addLevels}

```zig
pub fn addLevels(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_ADD_LEVELS`: a=levels `Player::addLevels`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.startCooldown` {#Player.startCooldown}

```zig
pub fn startCooldown(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_START_COOLDOWN`: sarg=item name, a=ticks `Player::startItemCooldown`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.startRiding` {#Player.startRiding}

```zig
pub fn startRiding(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_START_RIDING`: a=vehicle ActorUniqueID (lower 64b) `Player::startRiding`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.stopRiding` {#Player.stopRiding}

```zig
pub fn stopRiding(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_STOP_RIDING`: `Player::stopRiding`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.attack` {#Player.attack}

```zig
pub fn attack(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_ATTACK`: a=target ActorUniqueID (lower 64b) `Player::attack`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.drop` {#Player.drop}

```zig
pub fn drop(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_DROP`: sarg=item SNBT, a=random(0/1) `Player::drop`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.interact` {#Player.interact}

```zig
pub fn interact(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_INTERACT`: a=target ActorUniqueID `Player::interact`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.startUsingItem` {#Player.startUsingItem}

```zig
pub fn startUsingItem(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_START_USING_ITEM`: sarg=item SNBT, a=duration `Player::startUsingItem`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.stopUsingItem` {#Player.stopUsingItem}

```zig
pub fn stopUsingItem(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_STOP_USING_ITEM`: `Player::stopUsingItem`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.setChunkRadius` {#Player.setChunkRadius}

```zig
pub fn setChunkRadius(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_CHUNK_RADIUS`: a=radius `Player::setChunkRadius`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.setEnchantmentSeed` {#Player.setEnchantmentSeed}

```zig
pub fn setEnchantmentSeed(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_ENCHANTMENT_SEED`: a=seed `Player::setEnchantmentSeed`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.registerTrackedBoss` {#Player.registerTrackedBoss}

```zig
pub fn registerTrackedBoss(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_REGISTER_TRACKED_BOSS`: a=boss ActorUniqueID `Player::registerTrackedBoss`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.unregisterTrackedBoss` {#Player.unregisterTrackedBoss}

```zig
pub fn unregisterTrackedBoss(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_UNREGISTER_TRACKED_BOSS`: a=boss ActorUniqueID `Player::unRegisterTrackedBoss`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.playEmote` {#Player.playEmote}

```zig
pub fn playEmote(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_PLAY_EMOTE`: sarg=piece id `Player::playEmote`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.resendAllChunks` {#Player.resendAllChunks}

```zig
pub fn resendAllChunks(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_RESEND_ALL_CHUNKS`: `Player::resendAllChunks`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.openInventory` {#Player.openInventory}

```zig
pub fn openInventory(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_OPEN_INVENTORY`: `Player::openInventory`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.sidebarSet` {#Player.sidebarSet}

```zig
pub fn sidebarSet(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SIDEBAR_SET`: sarg="obj\\ntitle\\nline…" per-player sidebar

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.sidebarClear` {#Player.sidebarClear}

```zig
pub fn sidebarClear(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SIDEBAR_CLEAR`: sarg=objective RemoveObjectivePacket

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.setPermissionLevel` {#Player.setPermissionLevel}

```zig
pub fn setPermissionLevel(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_PERMISSION_LEVEL`: a=PlayerPermissionLevel (0 Visitor, 1 Member, 2 Operator, 3 Custom). `LayeredAbilities::setPlayerPermissions` plus UpdateAbilitiesPacket. The read side is `PIER_PPROP_PERMISSION_LEVEL`. Refused until the player has finished joining, like `PIER_PACT_SET_ABILITY`.

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- Return type: `core.Error![]u8`
- Slots: [`player_action`](../cpp/player.md#player_action)

## `SelKind` {#SelKind}

```zig
pub const SelKind = enum(i32) {
    name = 0, xuid = 1, uuid = 2
};
```

How a Player selector names its player. A decision about permissions, money or ownership keys on the xuid: a name can change hands.
