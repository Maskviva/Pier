# Go：玩家

## 函数 {#functions}

### `PlayerByName` {#PlayerByName}

```go
func PlayerByName(name string) Player
```

这个名字的玩家。关于权限、金钱或归属的判断要用 `PlayerByXuid`：名字会换主人。

- 参数：
    - name : `string`
- 返回值类型：`Player`

### `PlayerByXuid` {#PlayerByXuid}

```go
func PlayerByXuid(xuid string) Player
```

这个 xuid 的玩家。

- 参数：
    - xuid : `string`
- 返回值类型：`Player`

### `PlayerByUuid` {#PlayerByUuid}

```go
func PlayerByUuid(uuid string) Player
```

这个 uuid 的玩家。

- 参数：
    - uuid : `string`
- 返回值类型：`Player`

### `Broadcast` {#Broadcast}

```go
func Broadcast(msg string) error
```

给每个在线玩家发送一条消息。

- 参数：
    - msg : `string`
- 返回值类型：`error`
- 对应槽位：[`broadcast_message`](../cpp/player.md#broadcast_message)

### `ListPlayers` {#ListPlayers}

```go
func ListPlayers() ([]PlayerInfo, error)
```

列出在线玩家。解析不了的条目会被跳过，并记一条警告，所以一条坏数据不会让「谁在线」变得答不上来。

- 返回值类型：`([]PlayerInfo, error)`
- 对应槽位：[`list_players`](../cpp/player.md#list_players)、[`log`](../cpp/core.md#log)

### `ByName` {#ByName}

```go
func ByName(name string) PlayerSel
```

按名字选择一名玩家。

- 参数：
    - name : `string`
- 返回值类型：`PlayerSel`

### `ByXuid` {#ByXuid}

```go
func ByXuid(xuid string) PlayerSel
```

按 xuid 选择一名玩家。

- 参数：
    - xuid : `string`
- 返回值类型：`PlayerSel`

### `ByUuid` {#ByUuid}

```go
func ByUuid(uuid string) PlayerSel
```

按 uuid 选择一名玩家。

- 参数：
    - uuid : `string`
- 返回值类型：`PlayerSel`

## `Player` {#Player}

```go
type Player struct {
    Sel PlayerSel
}
```

一名玩家，由选择器指定。它的大部分方法由 abi.h 的常量表生成，每个属性和动作一个；其余是手写的方法。

### `Player.Resolve` {#Player.Resolve}

```go
func (p Player) Resolve() (Entity, error)
```

这名玩家对应的实体。返回错误表示没有人和选择器对得上。

- 返回值类型：`(Entity, error)`
- 对应槽位：[`player_resolve`](../cpp/player.md#player_resolve)

### `Player.IsOnline` {#Player.IsOnline}

```go
func (p Player) IsOnline() bool
```

判断选择器是否对得上一名在线玩家。每个 ABI v2 宿主都有它用到的那个槽位，所以 false 就表示没有人对得上。

- 返回值类型：`bool`
- 对应槽位：[`player_resolve`](../cpp/player.md#player_resolve)

### `Player.SendMessage` {#Player.SendMessage}

```go
func (p Player) SendMessage(msg string) error
```

给这名玩家发送一行聊天消息。

- 参数：
    - msg : `string`
- 返回值类型：`error`
- 对应槽位：[`player_send_message`](../cpp/player.md#player_send_message)

### `Player.SendMessageTyped` {#Player.SendMessageTyped}

```go
func (p Player) SendMessageTyped(msg string, kind int32) error
```

给这名玩家发送一条指定 `TextPacket` 类型的消息。

- 参数：
    - msg : `string`
    - kind : `int32`
- 返回值类型：`error`
- 对应槽位：[`player_send_message_typed`](../cpp/player.md#player_send_message_typed)

### `Player.SendTitle` {#Player.SendTitle}

```go
func (p Player) SendTitle(slot int32, text string, fadeIn, stay, fadeOut int32) error
```

显示一个标题：`slot` 为 0 是主标题，1 是副标题，2 是动作栏；时间的单位是刻。

- 参数：
    - slot : `int32`
    - text : `string`
    - fadeIn : `int32`
    - stay : `int32`
    - fadeOut : `int32`
- 返回值类型：`error`
- 对应槽位：[`player_send_title`](../cpp/player.md#player_send_title)

### `Player.Disconnect` {#Player.Disconnect}

```go
func (p Player) Disconnect(reason string) error
```

带着原因把这名玩家从服务器上移除。

- 参数：
    - reason : `string`
- 返回值类型：`error`
- 对应槽位：[`player_disconnect`](../cpp/player.md#player_disconnect)

### `Player.SetGameType` {#Player.SetGameType}

```go
func (p Player) SetGameType(mode int32) error
```

设置这名玩家的游戏模式，也就是 `GameType` 读到的那个值。

- 参数：
    - mode : `int32`
- 返回值类型：`error`
- 对应槽位：[`player_set_gamemode`](../cpp/player.md#player_set_gamemode)

### `Player.Teleport` {#Player.Teleport}

```go
func (p Player) Teleport(dim int32, x, y, z float64) error
```

把这名玩家移到任何已注册维度里的一个位置。

- 参数：
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- 返回值类型：`error`
- 对应槽位：[`player_teleport`](../cpp/player.md#player_teleport)

### `Player.CarriedItem` {#Player.CarriedItem}

```go
func (p Player) CarriedItem() (Item, error)
```

这名玩家手里拿着的物品。

- 返回值类型：`(Item, error)`
- 对应槽位：[`player_get_carried_item`](../cpp/player.md#player_get_carried_item)

### `Player.InventoryItem` {#Player.InventoryItem}

```go
func (p Player) InventoryItem(slot int32) (Item, error)
```

物品栏某一格里的物品。

- 参数：
    - slot : `int32`
- 返回值类型：`(Item, error)`
- 对应槽位：[`player_get_item`](../cpp/player.md#player_get_item)

### `Player.SetInventoryItem` {#Player.SetInventoryItem}

```go
func (p Player) SetInventoryItem(slot int32, item Item) error
```

把一个物品放进物品栏的某一格。

- 参数：
    - slot : `int32`
    - item : `Item`
- 返回值类型：`error`
- 对应槽位：[`player_set_item`](../cpp/player.md#player_set_item)

### `Player.ConnID` {#Player.ConnID}

```go
func (p Player) ConnID() (uint64, error)
```

这名玩家的网络连接 id。

- 返回值类型：`(uint64, error)`
- 对应槽位：[`player_conn_id`](../cpp/player.md#player_conn_id)

### `Player.Inventory` {#Player.Inventory}

```go
func (p Player) Inventory() Container
```

把这名玩家的物品栏当作容器。

- 返回值类型：`Container`

### `Player.EnderChest` {#Player.EnderChest}

```go
func (p Player) EnderChest() Container
```

把这名玩家的末影箱当作容器。

- 返回值类型：`Container`

### `Player.Armor` {#Player.Armor}

```go
func (p Player) Armor() Container
```

把这名玩家的盔甲栏当作容器。

- 返回值类型：`Container`

### `Player.Offhand` {#Player.Offhand}

```go
func (p Player) Offhand() Container
```

把这名玩家的副手栏当作容器。

- 返回值类型：`Container`

### `Player.GameType` {#Player.GameType}

```go
func (p Player) GameType() (float64, error)
```

读取 `PIER_PPROP_GAME_TYPE`：(G) `Player::getPlayerGameType`；写入用 `player_set_gamemode`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Level` {#Player.Level}

```go
func (p Player) Level() (float64, error)
```

读取 `PIER_PPROP_LEVEL`：(S) 属性 `Player::LEVEL()`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetLevel` {#Player.SetLevel}

```go
func (p Player) SetLevel(v float64) error
```

写入 `PIER_PPROP_LEVEL`：(S) 属性 `Player::LEVEL()`

- 参数：
    - v : `float64`
- 返回值类型：`error`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Experience` {#Player.Experience}

```go
func (p Player) Experience() (float64, error)
```

读取 `PIER_PPROP_EXPERIENCE`：(S) 属性 `Player::EXPERIENCE()`（经验条进度，0 到 1）

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetExperience` {#Player.SetExperience}

```go
func (p Player) SetExperience(v float64) error
```

写入 `PIER_PPROP_EXPERIENCE`：(S) 属性 `Player::EXPERIENCE()`（经验条进度，0 到 1）

- 参数：
    - v : `float64`
- 返回值类型：`error`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Hunger` {#Player.Hunger}

```go
func (p Player) Hunger() (float64, error)
```

读取 `PIER_PPROP_HUNGER`：(S) 属性 `Player::HUNGER()`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetHunger` {#Player.SetHunger}

```go
func (p Player) SetHunger(v float64) error
```

写入 `PIER_PPROP_HUNGER`：(S) 属性 `Player::HUNGER()`

- 参数：
    - v : `float64`
- 返回值类型：`error`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Saturation` {#Player.Saturation}

```go
func (p Player) Saturation() (float64, error)
```

读取 `PIER_PPROP_SATURATION`：(S) 属性 `Player::SATURATION()`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetSaturation` {#Player.SetSaturation}

```go
func (p Player) SetSaturation(v float64) error
```

写入 `PIER_PPROP_SATURATION`：(S) 属性 `Player::SATURATION()`

- 参数：
    - v : `float64`
- 返回值类型：`error`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Exhaustion` {#Player.Exhaustion}

```go
func (p Player) Exhaustion() (float64, error)
```

读取 `PIER_PPROP_EXHAUSTION`：(S) 属性 `Player::EXHAUSTION()`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetExhaustion` {#Player.SetExhaustion}

```go
func (p Player) SetExhaustion(v float64) error
```

写入 `PIER_PPROP_EXHAUSTION`：(S) 属性 `Player::EXHAUSTION()`

- 参数：
    - v : `float64`
- 返回值类型：`error`
- 对应槽位：[`player_set_num`](../cpp/player.md#player_set_num)

### `Player.XpNeededNextLevel` {#Player.XpNeededNextLevel}

```go
func (p Player) XpNeededNextLevel() (float64, error)
```

读取 `PIER_PPROP_XP_NEEDED_NEXT_LEVEL`：(G) `Player::getXpNeededForNextLevel`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Luck` {#Player.Luck}

```go
func (p Player) Luck() (float64, error)
```

读取 `PIER_PPROP_LUCK`：(G) `Player::getLuck`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SelectedSlot` {#Player.SelectedSlot}

```go
func (p Player) SelectedSlot() (float64, error)
```

读取 `PIER_PPROP_SELECTED_SLOT`：(G) `Player::getSelectedItemSlot`；设置用 `PIER_PACT_SET_SELECTED_SLOT`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsOperator` {#Player.IsOperator}

```go
func (p Player) IsOperator() (bool, error)
```

读取 `PIER_PPROP_IS_OPERATOR`：(G) `Player::isOperator`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanUseOperatorBlocks` {#Player.CanUseOperatorBlocks}

```go
func (p Player) CanUseOperatorBlocks() (bool, error)
```

读取 `PIER_PPROP_CAN_USE_OPERATOR_BLOCKS`：(G) `Player::canUseOperatorBlocks`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsFlying` {#Player.IsFlying}

```go
func (p Player) IsFlying() (bool, error)
```

读取 `PIER_PPROP_IS_FLYING`：(G) `Player::isFlying`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanJump` {#Player.CanJump}

```go
func (p Player) CanJump() (bool, error)
```

读取 `PIER_PPROP_CAN_JUMP`：(G) `Player::canJump`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsEmoting` {#Player.IsEmoting}

```go
func (p Player) IsEmoting() (bool, error)
```

读取 `PIER_PPROP_IS_EMOTING`：(G) `Player::isEmoting`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsInRaid` {#Player.IsInRaid}

```go
func (p Player) IsInRaid() (bool, error)
```

读取 `PIER_PPROP_IS_IN_RAID`：(G) `Player::isInRaid`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsHurt` {#Player.IsHurt}

```go
func (p Player) IsHurt() (bool, error)
```

读取 `PIER_PPROP_IS_HURT`：(G) `Player::isHurt`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsScoping` {#Player.IsScoping}

```go
func (p Player) IsScoping() (bool, error)
```

读取 `PIER_PPROP_IS_SCOPING`：(G) `Player::isScoping`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanSleep` {#Player.CanSleep}

```go
func (p Player) CanSleep() (bool, error)
```

读取 `PIER_PPROP_CAN_SLEEP`：(G) `Player::canSleep`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.HasRespawnPosition` {#Player.HasRespawnPosition}

```go
func (p Player) HasRespawnPosition() (bool, error)
```

读取 `PIER_PPROP_HAS_RESPAWN_POSITION`：(G) `Player::hasRespawnPosition`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.ClientSubId` {#Player.ClientSubId}

```go
func (p Player) ClientSubId() (float64, error)
```

读取 `PIER_PPROP_CLIENT_SUB_ID`：(G) `Player::getClientSubId`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanUseAbility` {#Player.CanUseAbility}

```go
func (p Player) CanUseAbility() (bool, error)
```

读取 `PIER_PPROP_CAN_USE_ABILITY`：(G) `Player::canUseAbility`；能力编号要经 `player_action` 的读取路径传入，见 `PIER_PACT_CAN_USE_ABILITY`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Direction` {#Player.Direction}

```go
func (p Player) Direction() (float64, error)
```

读取 `PIER_PPROP_DIRECTION`：(G) `Player::getDirection`（0=南，1=西，2=北，3=东）

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.ChunkRadius` {#Player.ChunkRadius}

```go
func (p Player) ChunkRadius() (float64, error)
```

读取 `PIER_PPROP_CHUNK_RADIUS`：(G) `Player::getChunkRadius`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.NetworkRtt` {#Player.NetworkRtt}

```go
func (p Player) NetworkRtt() (float64, error)
```

读取 `PIER_PPROP_NETWORK_RTT`：(G) `getNetworkStatus().mPing`（毫秒）

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Platform` {#Player.Platform}

```go
func (p Player) Platform() (float64, error)
```

读取 `PIER_PPROP_PLATFORM`：(G) `Player::getPlatform`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.EnchantmentSeed` {#Player.EnchantmentSeed}

```go
func (p Player) EnchantmentSeed() (float64, error)
```

读取 `PIER_PPROP_ENCHANTMENT_SEED`：(G) `Player::getEnchantmentSeed`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsUsingItem` {#Player.IsUsingItem}

```go
func (p Player) IsUsingItem() (bool, error)
```

读取 `PIER_PPROP_IS_USING_ITEM`：(G) `Player::isUsingItem`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsBlocking` {#Player.IsBlocking}

```go
func (p Player) IsBlocking() (bool, error)
```

读取 `PIER_PPROP_IS_BLOCKING`：(G) `Player::isBlocking`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsGliding` {#Player.IsGliding}

```go
func (p Player) IsGliding() (bool, error)
```

读取 `PIER_PPROP_IS_GLIDING`：(G) `Player::isGliding`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsSwimming` {#Player.IsSwimming}

```go
func (p Player) IsSwimming() (bool, error)
```

读取 `PIER_PPROP_IS_SWIMMING`：(G) `Player::isSwimming`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.PermissionLevel` {#Player.PermissionLevel}

```go
func (p Player) PermissionLevel() (float64, error)
```

读取 `PIER_PPROP_PERMISSION_LEVEL`：(G) `Player::getPlayerPermissionLevel`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Score` {#Player.Score}

```go
func (p Player) Score() (float64, error)
```

读取 `PIER_PPROP_SCORE`：(G) `Player::getScore`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.FallDistance` {#Player.FallDistance}

```go
func (p Player) FallDistance() (float64, error)
```

读取 `PIER_PPROP_FALL_DISTANCE`：(G) `Actor::getFallDistance`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsDead` {#Player.IsDead}

```go
func (p Player) IsDead() (bool, error)
```

读取 `PIER_PPROP_IS_DEAD`：(G) `Actor::isDead`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.HasDiedBefore` {#Player.HasDiedBefore}

```go
func (p Player) HasDiedBefore() (bool, error)
```

读取 `PIER_PPROP_HAS_DIED_BEFORE`：(G) `Player::hasDiedBefore`

- 返回值类型：`(bool, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Dimension` {#Player.Dimension}

```go
func (p Player) Dimension() (float64, error)
```

读取 `PIER_PPROP_DIMENSION`：(G) `Actor::getDimensionId`

- 返回值类型：`(float64, error)`
- 对应槽位：[`player_get_num`](../cpp/player.md#player_get_num)

### `Player.RealName` {#Player.RealName}

```go
func (p Player) RealName() (string, error)
```

读取 `PIER_PSTR_REAL_NAME`：取自 `Player::getRealName`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.Uuid` {#Player.Uuid}

```go
func (p Player) Uuid() (string, error)
```

读取 `PIER_PSTR_UUID`：取自 `Player::getUuid().asString()`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.Xuid` {#Player.Xuid}

```go
func (p Player) Xuid() (string, error)
```

读取 `PIER_PSTR_XUID`：取自 `Player::getXuid`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.IpAndPort` {#Player.IpAndPort}

```go
func (p Player) IpAndPort() (string, error)
```

读取 `PIER_PSTR_IP_AND_PORT`：取自 `Player::getIPAndPort`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.LocaleCode` {#Player.LocaleCode}

```go
func (p Player) LocaleCode() (string, error)
```

读取 `PIER_PSTR_LOCALE_CODE`：取自 `Player::getLocaleCode`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.NameTag` {#Player.NameTag}

```go
func (p Player) NameTag() (string, error)
```

读取 `PIER_PSTR_NAME_TAG`：取自 `Actor::getNameTag`（显示名）

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.LastDeathPos` {#Player.LastDeathPos}

```go
func (p Player) LastDeathPos() (string, error)
```

读取 `PIER_PSTR_LAST_DEATH_POS`：SNBT `{x,y,z}`；没有时为 `""`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.LastDeathDimension` {#Player.LastDeathDimension}

```go
func (p Player) LastDeathDimension() (string, error)
```

读取 `PIER_PSTR_LAST_DEATH_DIMENSION`：维度 id，以字符串表示

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.NetworkStatus` {#Player.NetworkStatus}

```go
func (p Player) NetworkStatus() (string, error)
```

读取 `PIER_PSTR_NETWORK_STATUS`：SNBT `{ping,avg_ping,packet_loss,max_ping}`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.PlatformOnlineId` {#Player.PlatformOnlineId}

```go
func (p Player) PlatformOnlineId() (string, error)
```

读取 `PIER_PSTR_PLATFORM_ONLINE_ID`：取自 `Player::getPlatformOnlineId`

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.RespawnPos` {#Player.RespawnPos}

```go
func (p Player) RespawnPos() (string, error)
```

读取 `PIER_PSTR_RESPAWN_POS`：SNBT `{x,y,z,dim}`：这名玩家会在哪里重生。不会为空：没有床的玩家报告的是世界出生点，这两种情况区分不开。

- 返回值类型：`(string, error)`
- 对应槽位：[`player_get_str`](../cpp/player.md#player_get_str)

### `Player.SetAbility` {#Player.SetAbility}

```go
func (p Player) SetAbility(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SET_ABILITY`：`a` 为 `AbilitiesIndex`，`b` 为 0 或 1（布尔类的能力）或者浮点数（`FlySpeed` 等）。写完以后会把 `PlayerPermissionLevel` 恢复成写之前的值：引擎的 `LayeredAbilities::setAbility` 走的是「切换到自定义权限」那条路，会把玩家推到 Custom，而这个等级会和能力层一起放进 `UpdateAbilitiesPacket` 发给客户端。要改权限等级，用 `PIER_PACT_SET_PERMISSION_LEVEL`。玩家加入完成之前（`Player::isPlayerInitialized`）调用会被拒绝，返回 false：客户端还在加载时写入的能力会一直不同步，直到玩家重新进入。模拟玩家不受这个限制。

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.CanUseAbilityAction` {#Player.CanUseAbilityAction}

```go
func (p Player) CanUseAbilityAction(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_CAN_USE_ABILITY`：`a` 为 `AbilitiesIndex`，输出 `"0"` 或 `"1"`，调用 `Player::canUseAbility`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SetSelectedSlot` {#Player.SetSelectedSlot}

```go
func (p Player) SetSelectedSlot(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SET_SELECTED_SLOT`：`a` 为槽位，调用 `Player::setSelectedSlot`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.GiveItem` {#Player.GiveItem}

```go
func (p Player) GiveItem(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_GIVE_ITEM`：`sarg` 为物品 SNBT，经 `ItemStack::fromTag` 和 `Player::addAndRefresh` 给出

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SetSpawnPoint` {#Player.SetSpawnPoint}

```go
func (p Player) SetSpawnPoint(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SET_SPAWN_POINT`：`a`、`b`、`c` 为坐标，`sarg` 为维度 id（任何已注册的维度都可以）；原生调用 `Player::setRespawnPosition`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.ClearTitle` {#Player.ClearTitle}

```go
func (p Player) ClearTitle(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_CLEAR_TITLE`：原生发送 `SetTitlePacket(Clear)`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SetTitle` {#Player.SetTitle}

```go
func (p Player) SetTitle(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SET_TITLE`：`sarg` 为文本，`a` 为位置（0 主标题，1 副标题，2 动作栏）；原生发送 `SetTitlePacket`，文本原样发送

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.AddExperience` {#Player.AddExperience}

```go
func (p Player) AddExperience(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_ADD_EXPERIENCE`：`a` 为经验值，调用 `Player::addExperience`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.AddLevels` {#Player.AddLevels}

```go
func (p Player) AddLevels(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_ADD_LEVELS`：`a` 为等级数，调用 `Player::addLevels`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.StartCooldown` {#Player.StartCooldown}

```go
func (p Player) StartCooldown(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_START_COOLDOWN`：`sarg` 为物品名，`a` 为刻数，调用 `Player::startItemCooldown`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.StartRiding` {#Player.StartRiding}

```go
func (p Player) StartRiding(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_START_RIDING`：`a` 为坐骑的 `ActorUniqueID`（低 64 位），调用 `Player::startRiding`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.StopRiding` {#Player.StopRiding}

```go
func (p Player) StopRiding(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_STOP_RIDING`：调用 `Player::stopRiding`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.Attack` {#Player.Attack}

```go
func (p Player) Attack(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_ATTACK`：`a` 为目标的 `ActorUniqueID`（低 64 位），调用 `Player::attack`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.Drop` {#Player.Drop}

```go
func (p Player) Drop(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_DROP`：`sarg` 为物品 SNBT，`a` 为是否随机抛出（0 或 1），调用 `Player::drop`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.Interact` {#Player.Interact}

```go
func (p Player) Interact(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_INTERACT`：`a` 为目标的 `ActorUniqueID`，调用 `Player::interact`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.StartUsingItem` {#Player.StartUsingItem}

```go
func (p Player) StartUsingItem(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_START_USING_ITEM`：`sarg` 为物品 SNBT，`a` 为持续时间，调用 `Player::startUsingItem`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.StopUsingItem` {#Player.StopUsingItem}

```go
func (p Player) StopUsingItem(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_STOP_USING_ITEM`：调用 `Player::stopUsingItem`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SetChunkRadius` {#Player.SetChunkRadius}

```go
func (p Player) SetChunkRadius(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SET_CHUNK_RADIUS`：`a` 为半径，调用 `Player::setChunkRadius`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SetEnchantmentSeed` {#Player.SetEnchantmentSeed}

```go
func (p Player) SetEnchantmentSeed(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SET_ENCHANTMENT_SEED`：`a` 为种子，调用 `Player::setEnchantmentSeed`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.RegisterTrackedBoss` {#Player.RegisterTrackedBoss}

```go
func (p Player) RegisterTrackedBoss(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_REGISTER_TRACKED_BOSS`：`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::registerTrackedBoss`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.UnregisterTrackedBoss` {#Player.UnregisterTrackedBoss}

```go
func (p Player) UnregisterTrackedBoss(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_UNREGISTER_TRACKED_BOSS`：`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::unRegisterTrackedBoss`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.PlayEmote` {#Player.PlayEmote}

```go
func (p Player) PlayEmote(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_PLAY_EMOTE`：`sarg` 为表情的 id，调用 `Player::playEmote`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.ResendAllChunks` {#Player.ResendAllChunks}

```go
func (p Player) ResendAllChunks(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_RESEND_ALL_CHUNKS`：调用 `Player::resendAllChunks`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.OpenInventory` {#Player.OpenInventory}

```go
func (p Player) OpenInventory(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_OPEN_INVENTORY`：调用 `Player::openInventory`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SidebarSet` {#Player.SidebarSet}

```go
func (p Player) SidebarSet(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SIDEBAR_SET`：`sarg` 为 `"obj\ntitle\nline…"`，只对这名玩家显示的侧边栏

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SidebarClear` {#Player.SidebarClear}

```go
func (p Player) SidebarClear(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SIDEBAR_CLEAR`：`sarg` 为计分项，发送 `RemoveObjectivePacket`

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

### `Player.SetPermissionLevel` {#Player.SetPermissionLevel}

```go
func (p Player) SetPermissionLevel(sarg string, a, b, c float64) (string, error)
```

执行 `PIER_PACT_SET_PERMISSION_LEVEL`：`a` 为 `PlayerPermissionLevel`（0 Visitor，1 Member，2 Operator，3 Custom）。调用 `LayeredAbilities::setPlayerPermissions` 并发送 `UpdateAbilitiesPacket`。读取一侧是 `PIER_PPROP_PERMISSION_LEVEL`。和 `PIER_PACT_SET_ABILITY` 一样，玩家加入完成之前调用会被拒绝。

这个动作有输出时，输出就是返回值；每个参数的含义写在 abi.h 里这个常量的注释中。

- 参数：
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- 返回值类型：`(string, error)`
- 对应槽位：[`player_action`](../cpp/player.md#player_action)

## `PlayerInfo` {#PlayerInfo}

```go
type PlayerInfo struct {
    Name, Xuid, Uuid string
    Dim              int32
    HasDim           bool
    X, Y, Z          float64
    HasPos           bool
}
```

`ListPlayers` 报告的一名在线玩家。`Dim` 和位置只在有的时候才读取，`HasDim` 和 `HasPos` 说明它们有没有。

## `SelKind` {#SelKind}

```go
type SelKind int32
```

说明 `PlayerSel` 用什么来指定玩家。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="SelName"></span>`SelName` | `0` |  |
| <span id="SelXuid"></span>`SelXuid` | `1` |  |
| <span id="SelUuid"></span>`SelUuid` | `2` |  |

## `PlayerSel` {#PlayerSel}

```go
type PlayerSel struct {
    Kind  SelKind
    Value string
}
```

按名字、xuid 或 uuid 指定一名玩家。

## `PlayerPos` {#PlayerPos}

```go
type PlayerPos struct {
    X, Y, Z   float64
    Dimension int32
    Found     bool
}
```

一名玩家所在的位置；没有人对得上时 `Found` 为 false。

## `PProp*` {#PProp}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PPropGameType"></span>`PPropGameType` | `0` | 即 `PIER_PPROP_GAME_TYPE`：(G) `Player::getPlayerGameType`；写入用 `player_set_gamemode` |
| <span id="PPropLevel"></span>`PPropLevel` | `1` | 即 `PIER_PPROP_LEVEL`：(S) 属性 `Player::LEVEL()` |
| <span id="PPropExperience"></span>`PPropExperience` | `2` | 即 `PIER_PPROP_EXPERIENCE`：(S) 属性 `Player::EXPERIENCE()`（经验条进度，0 到 1） |
| <span id="PPropHunger"></span>`PPropHunger` | `3` | 即 `PIER_PPROP_HUNGER`：(S) 属性 `Player::HUNGER()` |
| <span id="PPropSaturation"></span>`PPropSaturation` | `4` | 即 `PIER_PPROP_SATURATION`：(S) 属性 `Player::SATURATION()` |
| <span id="PPropExhaustion"></span>`PPropExhaustion` | `5` | 即 `PIER_PPROP_EXHAUSTION`：(S) 属性 `Player::EXHAUSTION()` |
| <span id="PPropXpNeededNextLevel"></span>`PPropXpNeededNextLevel` | `6` | 即 `PIER_PPROP_XP_NEEDED_NEXT_LEVEL`：(G) `Player::getXpNeededForNextLevel` |
| <span id="PPropLuck"></span>`PPropLuck` | `7` | 即 `PIER_PPROP_LUCK`：(G) `Player::getLuck` |
| <span id="PPropSelectedSlot"></span>`PPropSelectedSlot` | `8` | 即 `PIER_PPROP_SELECTED_SLOT`：(G) `Player::getSelectedItemSlot`；设置用 `PIER_PACT_SET_SELECTED_SLOT` |
| <span id="PPropIsOperator"></span>`PPropIsOperator` | `9` | 即 `PIER_PPROP_IS_OPERATOR`：(G) `Player::isOperator` |
| <span id="PPropCanUseOperatorBlocks"></span>`PPropCanUseOperatorBlocks` | `10` | 即 `PIER_PPROP_CAN_USE_OPERATOR_BLOCKS`：(G) `Player::canUseOperatorBlocks` |
| <span id="PPropIsFlying"></span>`PPropIsFlying` | `11` | 即 `PIER_PPROP_IS_FLYING`：(G) `Player::isFlying` |
| <span id="PPropCanJump"></span>`PPropCanJump` | `12` | 即 `PIER_PPROP_CAN_JUMP`：(G) `Player::canJump` |
| <span id="PPropIsEmoting"></span>`PPropIsEmoting` | `13` | 即 `PIER_PPROP_IS_EMOTING`：(G) `Player::isEmoting` |
| <span id="PPropIsInRaid"></span>`PPropIsInRaid` | `14` | 即 `PIER_PPROP_IS_IN_RAID`：(G) `Player::isInRaid` |
| <span id="PPropIsHurt"></span>`PPropIsHurt` | `15` | 即 `PIER_PPROP_IS_HURT`：(G) `Player::isHurt` |
| <span id="PPropIsScoping"></span>`PPropIsScoping` | `16` | 即 `PIER_PPROP_IS_SCOPING`：(G) `Player::isScoping` |
| <span id="PPropCanSleep"></span>`PPropCanSleep` | `17` | 即 `PIER_PPROP_CAN_SLEEP`：(G) `Player::canSleep` |
| <span id="PPropHasRespawnPosition"></span>`PPropHasRespawnPosition` | `18` | 即 `PIER_PPROP_HAS_RESPAWN_POSITION`：(G) `Player::hasRespawnPosition` |
| <span id="PPropClientSubId"></span>`PPropClientSubId` | `19` | 即 `PIER_PPROP_CLIENT_SUB_ID`：(G) `Player::getClientSubId` |
| <span id="PPropCanUseAbility"></span>`PPropCanUseAbility` | `20` | 即 `PIER_PPROP_CAN_USE_ABILITY`：(G) `Player::canUseAbility`；能力编号要经 `player_action` 的读取路径传入，见 `PIER_PACT_CAN_USE_ABILITY` |
| <span id="PPropDirection"></span>`PPropDirection` | `21` | 即 `PIER_PPROP_DIRECTION`：(G) `Player::getDirection`（0=南，1=西，2=北，3=东） |
| <span id="PPropChunkRadius"></span>`PPropChunkRadius` | `22` | 即 `PIER_PPROP_CHUNK_RADIUS`：(G) `Player::getChunkRadius` |
| <span id="PPropNetworkRtt"></span>`PPropNetworkRtt` | `23` | 即 `PIER_PPROP_NETWORK_RTT`：(G) `getNetworkStatus().mPing`（毫秒） |
| <span id="PPropPlatform"></span>`PPropPlatform` | `24` | 即 `PIER_PPROP_PLATFORM`：(G) `Player::getPlatform` |
| <span id="PPropEnchantmentSeed"></span>`PPropEnchantmentSeed` | `25` | 即 `PIER_PPROP_ENCHANTMENT_SEED`：(G) `Player::getEnchantmentSeed` |
| <span id="PPropIsUsingItem"></span>`PPropIsUsingItem` | `26` | 即 `PIER_PPROP_IS_USING_ITEM`：(G) `Player::isUsingItem` |
| <span id="PPropIsBlocking"></span>`PPropIsBlocking` | `27` | 即 `PIER_PPROP_IS_BLOCKING`：(G) `Player::isBlocking` |
| <span id="PPropIsGliding"></span>`PPropIsGliding` | `28` | 即 `PIER_PPROP_IS_GLIDING`：(G) `Player::isGliding` |
| <span id="PPropIsSwimming"></span>`PPropIsSwimming` | `29` | 即 `PIER_PPROP_IS_SWIMMING`：(G) `Player::isSwimming` |
| <span id="PPropPermissionLevel"></span>`PPropPermissionLevel` | `30` | 即 `PIER_PPROP_PERMISSION_LEVEL`：(G) `Player::getPlayerPermissionLevel` |
| <span id="PPropScore"></span>`PPropScore` | `31` | 即 `PIER_PPROP_SCORE`：(G) `Player::getScore` |
| <span id="PPropFallDistance"></span>`PPropFallDistance` | `32` | 即 `PIER_PPROP_FALL_DISTANCE`：(G) `Actor::getFallDistance` |
| <span id="PPropIsDead"></span>`PPropIsDead` | `33` | 即 `PIER_PPROP_IS_DEAD`：(G) `Actor::isDead` |
| <span id="PPropHasDiedBefore"></span>`PPropHasDiedBefore` | `34` | 即 `PIER_PPROP_HAS_DIED_BEFORE`：(G) `Player::hasDiedBefore` |
| <span id="PPropDimension"></span>`PPropDimension` | `35` | 即 `PIER_PPROP_DIMENSION`：(G) `Actor::getDimensionId` |

## `PStr*` {#PStr}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PStrRealName"></span>`PStrRealName` | `0` | 即 `PIER_PSTR_REAL_NAME`：取自 `Player::getRealName` |
| <span id="PStrUuid"></span>`PStrUuid` | `1` | 即 `PIER_PSTR_UUID`：取自 `Player::getUuid().asString()` |
| <span id="PStrXuid"></span>`PStrXuid` | `2` | 即 `PIER_PSTR_XUID`：取自 `Player::getXuid` |
| <span id="PStrIpAndPort"></span>`PStrIpAndPort` | `3` | 即 `PIER_PSTR_IP_AND_PORT`：取自 `Player::getIPAndPort` |
| <span id="PStrLocaleCode"></span>`PStrLocaleCode` | `4` | 即 `PIER_PSTR_LOCALE_CODE`：取自 `Player::getLocaleCode` |
| <span id="PStrNameTag"></span>`PStrNameTag` | `5` | 即 `PIER_PSTR_NAME_TAG`：取自 `Actor::getNameTag`（显示名） |
| <span id="PStrLastDeathPos"></span>`PStrLastDeathPos` | `6` | 即 `PIER_PSTR_LAST_DEATH_POS`：SNBT `{x,y,z}`；没有时为 `""` |
| <span id="PStrLastDeathDimension"></span>`PStrLastDeathDimension` | `7` | 即 `PIER_PSTR_LAST_DEATH_DIMENSION`：维度 id，以字符串表示 |
| <span id="PStrNetworkStatus"></span>`PStrNetworkStatus` | `8` | 即 `PIER_PSTR_NETWORK_STATUS`：SNBT `{ping,avg_ping,packet_loss,max_ping}` |
| <span id="PStrPlatformOnlineId"></span>`PStrPlatformOnlineId` | `9` | 即 `PIER_PSTR_PLATFORM_ONLINE_ID`：取自 `Player::getPlatformOnlineId` |
| <span id="PStrRespawnPos"></span>`PStrRespawnPos` | `10` | 即 `PIER_PSTR_RESPAWN_POS`：SNBT `{x,y,z,dim}`：这名玩家会在哪里重生。不会为空：没有床的玩家报告的是世界出生点，这两种情况区分不开。 |

## `PAct*` {#PAct}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PActSetAbility"></span>`PActSetAbility` | `0` | 即 `PIER_PACT_SET_ABILITY`：`a` 为 `AbilitiesIndex`，`b` 为 0 或 1（布尔类的能力）或者浮点数（`FlySpeed` 等）。写完以后会把 `PlayerPermissionLevel` 恢复成写之前的值：引擎的 `LayeredAbilities::setAbility` 走的是「切换到自定义权限」那条路，会把玩家推到 Custom，而这个等级会和能力层一起放进 `UpdateAbilitiesPacket` 发给客户端。要改权限等级，用 `PIER_PACT_SET_PERMISSION_LEVEL`。玩家加入完成之前（`Player::isPlayerInitialized`）调用会被拒绝，返回 false：客户端还在加载时写入的能力会一直不同步，直到玩家重新进入。模拟玩家不受这个限制。 |
| <span id="PActCanUseAbility"></span>`PActCanUseAbility` | `1` | 即 `PIER_PACT_CAN_USE_ABILITY`：`a` 为 `AbilitiesIndex`，输出 `"0"` 或 `"1"`，调用 `Player::canUseAbility` |
| <span id="PActSetSelectedSlot"></span>`PActSetSelectedSlot` | `2` | 即 `PIER_PACT_SET_SELECTED_SLOT`：`a` 为槽位，调用 `Player::setSelectedSlot` |
| <span id="PActGiveItem"></span>`PActGiveItem` | `3` | 即 `PIER_PACT_GIVE_ITEM`：`sarg` 为物品 SNBT，经 `ItemStack::fromTag` 和 `Player::addAndRefresh` 给出 |
| <span id="PActSetSpawnPoint"></span>`PActSetSpawnPoint` | `4` | 即 `PIER_PACT_SET_SPAWN_POINT`：`a`、`b`、`c` 为坐标，`sarg` 为维度 id（任何已注册的维度都可以）；原生调用 `Player::setRespawnPosition` |
| <span id="PActClearTitle"></span>`PActClearTitle` | `5` | 即 `PIER_PACT_CLEAR_TITLE`：原生发送 `SetTitlePacket(Clear)` |
| <span id="PActSetTitle"></span>`PActSetTitle` | `6` | 即 `PIER_PACT_SET_TITLE`：`sarg` 为文本，`a` 为位置（0 主标题，1 副标题，2 动作栏）；原生发送 `SetTitlePacket`，文本原样发送 |
| <span id="PActAddExperience"></span>`PActAddExperience` | `7` | 即 `PIER_PACT_ADD_EXPERIENCE`：`a` 为经验值，调用 `Player::addExperience` |
| <span id="PActAddLevels"></span>`PActAddLevels` | `8` | 即 `PIER_PACT_ADD_LEVELS`：`a` 为等级数，调用 `Player::addLevels` |
| <span id="PActStartCooldown"></span>`PActStartCooldown` | `9` | 即 `PIER_PACT_START_COOLDOWN`：`sarg` 为物品名，`a` 为刻数，调用 `Player::startItemCooldown` |
| <span id="PActStartRiding"></span>`PActStartRiding` | `10` | 即 `PIER_PACT_START_RIDING`：`a` 为坐骑的 `ActorUniqueID`（低 64 位），调用 `Player::startRiding` |
| <span id="PActStopRiding"></span>`PActStopRiding` | `11` | 即 `PIER_PACT_STOP_RIDING`：调用 `Player::stopRiding` |
| <span id="PActAttack"></span>`PActAttack` | `12` | 即 `PIER_PACT_ATTACK`：`a` 为目标的 `ActorUniqueID`（低 64 位），调用 `Player::attack` |
| <span id="PActDrop"></span>`PActDrop` | `13` | 即 `PIER_PACT_DROP`：`sarg` 为物品 SNBT，`a` 为是否随机抛出（0 或 1），调用 `Player::drop` |
| <span id="PActInteract"></span>`PActInteract` | `14` | 即 `PIER_PACT_INTERACT`：`a` 为目标的 `ActorUniqueID`，调用 `Player::interact` |
| <span id="PActStartUsingItem"></span>`PActStartUsingItem` | `15` | 即 `PIER_PACT_START_USING_ITEM`：`sarg` 为物品 SNBT，`a` 为持续时间，调用 `Player::startUsingItem` |
| <span id="PActStopUsingItem"></span>`PActStopUsingItem` | `16` | 即 `PIER_PACT_STOP_USING_ITEM`：调用 `Player::stopUsingItem` |
| <span id="PActSetChunkRadius"></span>`PActSetChunkRadius` | `17` | 即 `PIER_PACT_SET_CHUNK_RADIUS`：`a` 为半径，调用 `Player::setChunkRadius` |
| <span id="PActSetEnchantmentSeed"></span>`PActSetEnchantmentSeed` | `18` | 即 `PIER_PACT_SET_ENCHANTMENT_SEED`：`a` 为种子，调用 `Player::setEnchantmentSeed` |
| <span id="PActRegisterTrackedBoss"></span>`PActRegisterTrackedBoss` | `19` | 即 `PIER_PACT_REGISTER_TRACKED_BOSS`：`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::registerTrackedBoss` |
| <span id="PActUnregisterTrackedBoss"></span>`PActUnregisterTrackedBoss` | `20` | 即 `PIER_PACT_UNREGISTER_TRACKED_BOSS`：`a` 为 Boss 的 `ActorUniqueID`，调用 `Player::unRegisterTrackedBoss` |
| <span id="PActPlayEmote"></span>`PActPlayEmote` | `21` | 即 `PIER_PACT_PLAY_EMOTE`：`sarg` 为表情的 id，调用 `Player::playEmote` |
| <span id="PActResendAllChunks"></span>`PActResendAllChunks` | `22` | 即 `PIER_PACT_RESEND_ALL_CHUNKS`：调用 `Player::resendAllChunks` |
| <span id="PActOpenInventory"></span>`PActOpenInventory` | `23` | 即 `PIER_PACT_OPEN_INVENTORY`：调用 `Player::openInventory` |
| <span id="PActSidebarSet"></span>`PActSidebarSet` | `24` | 即 `PIER_PACT_SIDEBAR_SET`：`sarg` 为 `"obj\ntitle\nline…"`，只对这名玩家显示的侧边栏 |
| <span id="PActSidebarClear"></span>`PActSidebarClear` | `25` | 即 `PIER_PACT_SIDEBAR_CLEAR`：`sarg` 为计分项，发送 `RemoveObjectivePacket` |
| <span id="PActSetPermissionLevel"></span>`PActSetPermissionLevel` | `26` | 即 `PIER_PACT_SET_PERMISSION_LEVEL`：`a` 为 `PlayerPermissionLevel`（0 Visitor，1 Member，2 Operator，3 Custom）。调用 `LayeredAbilities::setPlayerPermissions` 并发送 `UpdateAbilitiesPacket`。读取一侧是 `PIER_PPROP_PERMISSION_LEVEL`。和 `PIER_PACT_SET_ABILITY` 一样，玩家加入完成之前调用会被拒绝。 |
