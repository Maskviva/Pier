# Zig：玩家

## `Player` {#Player}

```zig
pub const Player = struct {
    sel_kind: SelKind,
    sel_value: []const u8,
    // ...
};
```

一名玩家，由选择器指定。

### `Player.byName` {#Player.byName}

```zig
pub fn byName(sel_name: []const u8) Player
```

- 参数：
    - sel_name : `[]const u8`
- 返回值类型：`Player`

### `Player.byXuid` {#Player.byXuid}

```zig
pub fn byXuid(sel_xuid: []const u8) Player
```

- 参数：
    - sel_xuid : `[]const u8`
- 返回值类型：`Player`

### `Player.byUuid` {#Player.byUuid}

```zig
pub fn byUuid(sel_uuid: []const u8) Player
```

- 参数：
    - sel_uuid : `[]const u8`
- 返回值类型：`Player`

### `Player.cSel` {#Player.cSel}

```zig
pub fn cSel(self: Player) core.c.PierPlayerSel
```

- 返回值类型：`core.c.PierPlayerSel`

### `Player.gameType` {#Player.gameType}

```zig
pub fn gameType(self: Player) core.Error!f64
```

`PIER_PPROP_GAME_TYPE`：(G) `Player::getPlayerGameType`；写入用 `player_set_gamemode`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.level` {#Player.level}

```zig
pub fn level(self: Player) core.Error!f64
```

`PIER_PPROP_LEVEL`：(S) 属性 `Player::LEVEL()`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setLevel` {#Player.setLevel}

```zig
pub fn setLevel(self: Player, new_value: f64) core.Error!void
```

写入 `PIER_PPROP_LEVEL`：(S) 属性 `Player::LEVEL()`

- 参数：
    - new_value : `f64`
- 返回值类型：`core.Error!void`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.experience` {#Player.experience}

```zig
pub fn experience(self: Player) core.Error!f64
```

`PIER_PPROP_EXPERIENCE`：(S) 属性 `Player::EXPERIENCE()`（经验条进度，0 到 1）

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setExperience` {#Player.setExperience}

```zig
pub fn setExperience(self: Player, new_value: f64) core.Error!void
```

写入 `PIER_PPROP_EXPERIENCE`：(S) 属性 `Player::EXPERIENCE()`（经验条进度，0 到 1）

- 参数：
    - new_value : `f64`
- 返回值类型：`core.Error!void`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.hunger` {#Player.hunger}

```zig
pub fn hunger(self: Player) core.Error!f64
```

`PIER_PPROP_HUNGER`：(S) 属性 `Player::HUNGER()`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setHunger` {#Player.setHunger}

```zig
pub fn setHunger(self: Player, new_value: f64) core.Error!void
```

写入 `PIER_PPROP_HUNGER`：(S) 属性 `Player::HUNGER()`

- 参数：
    - new_value : `f64`
- 返回值类型：`core.Error!void`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.saturation` {#Player.saturation}

```zig
pub fn saturation(self: Player) core.Error!f64
```

`PIER_PPROP_SATURATION`：(S) 属性 `Player::SATURATION()`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setSaturation` {#Player.setSaturation}

```zig
pub fn setSaturation(self: Player, new_value: f64) core.Error!void
```

写入 `PIER_PPROP_SATURATION`：(S) 属性 `Player::SATURATION()`

- 参数：
    - new_value : `f64`
- 返回值类型：`core.Error!void`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.exhaustion` {#Player.exhaustion}

```zig
pub fn exhaustion(self: Player) core.Error!f64
```

`PIER_PPROP_EXHAUSTION`：(S) 属性 `Player::EXHAUSTION()`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.setExhaustion` {#Player.setExhaustion}

```zig
pub fn setExhaustion(self: Player, new_value: f64) core.Error!void
```

写入 `PIER_PPROP_EXHAUSTION`：(S) 属性 `Player::EXHAUSTION()`

- 参数：
    - new_value : `f64`
- 返回值类型：`core.Error!void`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.xpNeededNextLevel` {#Player.xpNeededNextLevel}

```zig
pub fn xpNeededNextLevel(self: Player) core.Error!f64
```

`PIER_PPROP_XP_NEEDED_NEXT_LEVEL`：(G) `Player::getXpNeededForNextLevel`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.luck` {#Player.luck}

```zig
pub fn luck(self: Player) core.Error!f64
```

`PIER_PPROP_LUCK`：(G) `Player::getLuck`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.selectedSlot` {#Player.selectedSlot}

```zig
pub fn selectedSlot(self: Player) core.Error!f64
```

`PIER_PPROP_SELECTED_SLOT`：(G) `Player::getSelectedItemSlot`；设置用 `PIER_PACT_SET_SELECTED_SLOT`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isOperator` {#Player.isOperator}

```zig
pub fn isOperator(self: Player) core.Error!bool
```

`PIER_PPROP_IS_OPERATOR`：(G) `Player::isOperator`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canUseOperatorBlocks` {#Player.canUseOperatorBlocks}

```zig
pub fn canUseOperatorBlocks(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_USE_OPERATOR_BLOCKS`：(G) `Player::canUseOperatorBlocks`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isFlying` {#Player.isFlying}

```zig
pub fn isFlying(self: Player) core.Error!bool
```

`PIER_PPROP_IS_FLYING`：(G) `Player::isFlying`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canJump` {#Player.canJump}

```zig
pub fn canJump(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_JUMP`：(G) `Player::canJump`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isEmoting` {#Player.isEmoting}

```zig
pub fn isEmoting(self: Player) core.Error!bool
```

`PIER_PPROP_IS_EMOTING`：(G) `Player::isEmoting`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isInRaid` {#Player.isInRaid}

```zig
pub fn isInRaid(self: Player) core.Error!bool
```

`PIER_PPROP_IS_IN_RAID`：(G) `Player::isInRaid`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isHurt` {#Player.isHurt}

```zig
pub fn isHurt(self: Player) core.Error!bool
```

`PIER_PPROP_IS_HURT`：(G) `Player::isHurt`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isScoping` {#Player.isScoping}

```zig
pub fn isScoping(self: Player) core.Error!bool
```

`PIER_PPROP_IS_SCOPING`：(G) `Player::isScoping`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canSleep` {#Player.canSleep}

```zig
pub fn canSleep(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_SLEEP`：(G) `Player::canSleep`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.hasRespawnPosition` {#Player.hasRespawnPosition}

```zig
pub fn hasRespawnPosition(self: Player) core.Error!bool
```

`PIER_PPROP_HAS_RESPAWN_POSITION`：(G) `Player::hasRespawnPosition`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.clientSubId` {#Player.clientSubId}

```zig
pub fn clientSubId(self: Player) core.Error!f64
```

`PIER_PPROP_CLIENT_SUB_ID`：(G) `Player::getClientSubId`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.canUseAbility` {#Player.canUseAbility}

```zig
pub fn canUseAbility(self: Player) core.Error!bool
```

`PIER_PPROP_CAN_USE_ABILITY`：(G) `Player::canUseAbility`；能力编号要经 `player_action` 的读取路径传入，见 `PIER_PACT_CAN_USE_ABILITY`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.direction` {#Player.direction}

```zig
pub fn direction(self: Player) core.Error!f64
```

`PIER_PPROP_DIRECTION`：(G) `Player::getDirection`（0=南，1=西，2=北，3=东）

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.chunkRadius` {#Player.chunkRadius}

```zig
pub fn chunkRadius(self: Player) core.Error!f64
```

`PIER_PPROP_CHUNK_RADIUS`：(G) `Player::getChunkRadius`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.networkRtt` {#Player.networkRtt}

```zig
pub fn networkRtt(self: Player) core.Error!f64
```

`PIER_PPROP_NETWORK_RTT`：(G) `getNetworkStatus().mPing`（毫秒）

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.platform` {#Player.platform}

```zig
pub fn platform(self: Player) core.Error!f64
```

`PIER_PPROP_PLATFORM`：(G) `Player::getPlatform`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.enchantmentSeed` {#Player.enchantmentSeed}

```zig
pub fn enchantmentSeed(self: Player) core.Error!f64
```

`PIER_PPROP_ENCHANTMENT_SEED`：(G) `Player::getEnchantmentSeed`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isUsingItem` {#Player.isUsingItem}

```zig
pub fn isUsingItem(self: Player) core.Error!bool
```

`PIER_PPROP_IS_USING_ITEM`：(G) `Player::isUsingItem`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isBlocking` {#Player.isBlocking}

```zig
pub fn isBlocking(self: Player) core.Error!bool
```

`PIER_PPROP_IS_BLOCKING`：(G) `Player::isBlocking`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isGliding` {#Player.isGliding}

```zig
pub fn isGliding(self: Player) core.Error!bool
```

`PIER_PPROP_IS_GLIDING`：(G) `Player::isGliding`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isSwimming` {#Player.isSwimming}

```zig
pub fn isSwimming(self: Player) core.Error!bool
```

`PIER_PPROP_IS_SWIMMING`：(G) `Player::isSwimming`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.permissionLevel` {#Player.permissionLevel}

```zig
pub fn permissionLevel(self: Player) core.Error!f64
```

`PIER_PPROP_PERMISSION_LEVEL`：(G) `Player::getPlayerPermissionLevel`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.score` {#Player.score}

```zig
pub fn score(self: Player) core.Error!f64
```

`PIER_PPROP_SCORE`：(G) `Player::getScore`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.fallDistance` {#Player.fallDistance}

```zig
pub fn fallDistance(self: Player) core.Error!f64
```

`PIER_PPROP_FALL_DISTANCE`：(G) `Actor::getFallDistance`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.isDead` {#Player.isDead}

```zig
pub fn isDead(self: Player) core.Error!bool
```

`PIER_PPROP_IS_DEAD`：(G) `Actor::isDead`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.hasDiedBefore` {#Player.hasDiedBefore}

```zig
pub fn hasDiedBefore(self: Player) core.Error!bool
```

`PIER_PPROP_HAS_DIED_BEFORE`：(G) `Player::hasDiedBefore`

- 返回值类型：`core.Error!bool`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.dimension` {#Player.dimension}

```zig
pub fn dimension(self: Player) core.Error!f64
```

`PIER_PPROP_DIMENSION`：(G) `Actor::getDimensionId`

- 返回值类型：`core.Error!f64`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.realName` {#Player.realName}

```zig
pub fn realName(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_REAL_NAME`：取自 `Player::getRealName`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.uuid` {#Player.uuid}

```zig
pub fn uuid(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_UUID`：取自 `Player::getUuid().asString()`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.xuid` {#Player.xuid}

```zig
pub fn xuid(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_XUID`：取自 `Player::getXuid`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.ipAndPort` {#Player.ipAndPort}

```zig
pub fn ipAndPort(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_IP_AND_PORT`：取自 `Player::getIPAndPort`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.localeCode` {#Player.localeCode}

```zig
pub fn localeCode(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_LOCALE_CODE`：取自 `Player::getLocaleCode`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.nameTag` {#Player.nameTag}

```zig
pub fn nameTag(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_NAME_TAG`：取自 `Actor::getNameTag`（显示名）

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.lastDeathPos` {#Player.lastDeathPos}

```zig
pub fn lastDeathPos(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_LAST_DEATH_POS`：SNBT `{x,y,z}`；没有时为 `""`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.lastDeathDimension` {#Player.lastDeathDimension}

```zig
pub fn lastDeathDimension(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_LAST_DEATH_DIMENSION`：维度 id，以字符串表示

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.networkStatus` {#Player.networkStatus}

```zig
pub fn networkStatus(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_NETWORK_STATUS`：SNBT `{ping,avg_ping,packet_loss,max_ping}`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.platformOnlineId` {#Player.platformOnlineId}

```zig
pub fn platformOnlineId(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_PLATFORM_ONLINE_ID`：取自 `Player::getPlatformOnlineId`

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.respawnPos` {#Player.respawnPos}

```zig
pub fn respawnPos(self: Player, allocator: std.mem.Allocator) core.Error![]u8
```

`PIER_PSTR_RESPAWN_POS`：SNBT `{x,y,z,dim}`：这名玩家会在哪里重生。不会为空：没有床的玩家报告的是世界出生点，这两种情况区分不开。

- 参数：
    - allocator : `std.mem.Allocator`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.setAbility` {#Player.setAbility}

```zig
pub fn setAbility(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_ABILITY`：`a` 为 `AbilitiesIndex`，`b` 为 0 或 1（布尔类的能力）或者浮点数（`FlySpeed` 等）。写完以后会把 `PlayerPermissionLevel` 恢复成写之前的值：引擎的 `LayeredAbilities::setAbility` 走的是「切换到自定义权限」那条路，会把玩家推到 Custom，而这个等级会和能力层一起放进 `UpdateAbilitiesPacket` 发给客户端。要改权限等级，用 `PIER_PACT_SET_PERMISSION_LEVEL`。玩家加入完成之前（`Player::isPlayerInitialized`）调用会被拒绝，返回 false：客户端还在加载时写入的能力会一直不同步，直到玩家重新进入。模拟玩家不受这个限制。

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.canUseAbilityAction` {#Player.canUseAbilityAction}

```zig
pub fn canUseAbilityAction(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_CAN_USE_ABILITY`：`a` 为 `AbilitiesIndex`，输出 `"0"` 或 `"1"`，调用 `Player::canUseAbility`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.setSelectedSlot` {#Player.setSelectedSlot}

```zig
pub fn setSelectedSlot(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_SELECTED_SLOT`：`a` 为槽位，调用 `Player::setSelectedSlot`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.giveItem` {#Player.giveItem}

```zig
pub fn giveItem(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_GIVE_ITEM`：`sarg` 为物品 SNBT，经 `ItemStack::fromTag` 和 `Player::addAndRefresh` 给出

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.setSpawnPoint` {#Player.setSpawnPoint}

```zig
pub fn setSpawnPoint(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_SPAWN_POINT`：`a`、`b`、`c` 为坐标，`sarg` 为维度 id（任何已注册的维度都可以）；原生调用 `Player::setRespawnPosition`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.clearTitle` {#Player.clearTitle}

```zig
pub fn clearTitle(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_CLEAR_TITLE`：原生发送 `SetTitlePacket(Clear)`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.setTitle` {#Player.setTitle}

```zig
pub fn setTitle(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_TITLE`：`sarg` 为文本，`a` 为位置（0 主标题，1 副标题，2 动作栏）；原生发送 `SetTitlePacket`，文本原样发送

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.addExperience` {#Player.addExperience}

```zig
pub fn addExperience(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_ADD_EXPERIENCE`：`a` 为经验值，调用 `Player::addExperience`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.addLevels` {#Player.addLevels}

```zig
pub fn addLevels(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_ADD_LEVELS`：`a` 为等级数，调用 `Player::addLevels`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.startCooldown` {#Player.startCooldown}

```zig
pub fn startCooldown(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_START_COOLDOWN`：`sarg` 为物品名，`a` 为刻数，调用 `Player::startItemCooldown`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.startRiding` {#Player.startRiding}

```zig
pub fn startRiding(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_START_RIDING`：`a` 为坐骑的 `ActorUniqueID`（低 64 位），调用 `Player::startRiding`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.stopRiding` {#Player.stopRiding}

```zig
pub fn stopRiding(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_STOP_RIDING`：调用 `Player::stopRiding`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.attack` {#Player.attack}

```zig
pub fn attack(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_ATTACK`：`a` 为目标的 `ActorUniqueID`（低 64 位），调用 `Player::attack`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.drop` {#Player.drop}

```zig
pub fn drop(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_DROP`：`sarg` 为物品 SNBT，`a` 为是否随机抛出（0 或 1），调用 `Player::drop`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.interact` {#Player.interact}

```zig
pub fn interact(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_INTERACT`：`a` 为目标的 `ActorUniqueID`，调用 `Player::interact`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.startUsingItem` {#Player.startUsingItem}

```zig
pub fn startUsingItem(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_START_USING_ITEM`：`sarg` 为物品 SNBT，`a` 为持续时间，调用 `Player::startUsingItem`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.stopUsingItem` {#Player.stopUsingItem}

```zig
pub fn stopUsingItem(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_STOP_USING_ITEM`：调用 `Player::stopUsingItem`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.setChunkRadius` {#Player.setChunkRadius}

```zig
pub fn setChunkRadius(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_CHUNK_RADIUS`：`a` 为半径，调用 `Player::setChunkRadius`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.setEnchantmentSeed` {#Player.setEnchantmentSeed}

```zig
pub fn setEnchantmentSeed(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_ENCHANTMENT_SEED`：`a` 为种子，调用 `Player::setEnchantmentSeed`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.registerTrackedBoss` {#Player.registerTrackedBoss}

```zig
pub fn registerTrackedBoss(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_REGISTER_TRACKED_BOSS`：`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::registerTrackedBoss`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.unregisterTrackedBoss` {#Player.unregisterTrackedBoss}

```zig
pub fn unregisterTrackedBoss(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_UNREGISTER_TRACKED_BOSS`：`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::unRegisterTrackedBoss`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.playEmote` {#Player.playEmote}

```zig
pub fn playEmote(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_PLAY_EMOTE`：`sarg` 为表情的 id，调用 `Player::playEmote`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.resendAllChunks` {#Player.resendAllChunks}

```zig
pub fn resendAllChunks(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_RESEND_ALL_CHUNKS`：调用 `Player::resendAllChunks`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.openInventory` {#Player.openInventory}

```zig
pub fn openInventory(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_OPEN_INVENTORY`：调用 `Player::openInventory`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.sidebarSet` {#Player.sidebarSet}

```zig
pub fn sidebarSet(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SIDEBAR_SET`：`sarg` 为 `"obj\ntitle\nline…"`，只对这名玩家显示的侧边栏

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.sidebarClear` {#Player.sidebarClear}

```zig
pub fn sidebarClear(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SIDEBAR_CLEAR`：`sarg` 为计分项，发送 `RemoveObjectivePacket`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.setPermissionLevel` {#Player.setPermissionLevel}

```zig
pub fn setPermissionLevel(self: Player, allocator: std.mem.Allocator, s_arg: []const u8, n_a: f64, n_b: f64, n_c: f64) core.Error![]u8
```

`PIER_PACT_SET_PERMISSION_LEVEL`：`a` 为 `PlayerPermissionLevel`（0 Visitor，1 Member，2 Operator，3 Custom）。调用 `LayeredAbilities::setPlayerPermissions` 并发送 `UpdateAbilitiesPacket`。读取一侧是 `PIER_PPROP_PERMISSION_LEVEL`。和 `PIER_PACT_SET_ABILITY` 一样，玩家加入完成之前调用会被拒绝。

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - allocator : `std.mem.Allocator`
    - s_arg : `[]const u8`
    - n_a : `f64`
    - n_b : `f64`
    - n_c : `f64`
- 返回值类型：`core.Error![]u8`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

## `SelKind` {#SelKind}

```zig
pub const SelKind = enum(i32) {
    name = 0, xuid = 1, uuid = 2
};
```

`Player` 选择器用什么来指定玩家。关于权限、金钱或归属的判断，要以 xuid 为键：名字会换主人。
