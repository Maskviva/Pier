# Go: Players

## Functions {#functions}

### `PlayerByName` {#PlayerByName}

```go
func PlayerByName(name string) Player
```

PlayerByName is the player with this name. A decision about permissions, money or ownership uses PlayerByXuid: a name can change hands.

- Parameters:
    - name : `string`
- Return type: `Player`

### `PlayerByXuid` {#PlayerByXuid}

```go
func PlayerByXuid(xuid string) Player
```

PlayerByXuid is the player with this xuid.

- Parameters:
    - xuid : `string`
- Return type: `Player`

### `PlayerByUuid` {#PlayerByUuid}

```go
func PlayerByUuid(uuid string) Player
```

PlayerByUuid is the player with this uuid.

- Parameters:
    - uuid : `string`
- Return type: `Player`

### `Broadcast` {#Broadcast}

```go
func Broadcast(msg string) error
```

Broadcast sends a message to every online player.

- Parameters:
    - msg : `string`
- Return type: `error`
- Slots: [`broadcast_message`](../cpp/player.md#broadcast_message)

### `ListPlayers` {#ListPlayers}

```go
func ListPlayers() ([]PlayerInfo, error)
```

ListPlayers lists the online players. An entry that does not parse is skipped with a warning, so one bad entry does not make who is online unanswerable.

- Return type: `([]PlayerInfo, error)`
- Slots: [`list_players`](../cpp/player.md#list_players), [`log`](../cpp/core.md#log)

### `ByName` {#ByName}

```go
func ByName(name string) PlayerSel
```

ByName selects a player by name.

- Parameters:
    - name : `string`
- Return type: `PlayerSel`

### `ByXuid` {#ByXuid}

```go
func ByXuid(xuid string) PlayerSel
```

ByXuid selects a player by xuid.

- Parameters:
    - xuid : `string`
- Return type: `PlayerSel`

### `ByUuid` {#ByUuid}

```go
func ByUuid(uuid string) PlayerSel
```

ByUuid selects a player by uuid.

- Parameters:
    - uuid : `string`
- Return type: `PlayerSel`

## `Player` {#Player}

```go
type Player struct {
    Sel PlayerSel
}
```

Player is one player, named by a selector. Most of its methods are generated from the constant tables of abi.h, one per property and verb; the rest are below.

### `Player.Resolve` {#Player.Resolve}

```go
func (p Player) Resolve() (Entity, error)
```

Resolve is the player's actor. An error means nobody matches the selector.

- Return type: `(Entity, error)`
- Slots: [`player_resolve`](../cpp/player.md#player_resolve)

### `Player.IsOnline` {#Player.IsOnline}

```go
func (p Player) IsOnline() bool
```

IsOnline reports whether the selector matches an online player. Every ABI v2 host has the slot it asks, so false means nobody matches.

- Return type: `bool`
- Slots: [`player_resolve`](../cpp/player.md#player_resolve)

### `Player.SendMessage` {#Player.SendMessage}

```go
func (p Player) SendMessage(msg string) error
```

SendMessage sends a chat line to the player.

- Parameters:
    - msg : `string`
- Return type: `error`
- Slots: [`player_send_message`](../cpp/player.md#player_send_message)

### `Player.SendMessageTyped` {#Player.SendMessageTyped}

```go
func (p Player) SendMessageTyped(msg string, kind int32) error
```

SendMessageTyped sends a message of a given TextPacket type to the player.

- Parameters:
    - msg : `string`
    - kind : `int32`
- Return type: `error`
- Slots: [`player_send_message_typed`](../cpp/player.md#player_send_message_typed)

### `Player.SendTitle` {#Player.SendTitle}

```go
func (p Player) SendTitle(slot int32, text string, fadeIn, stay, fadeOut int32) error
```

SendTitle shows a title: slot 0 title, 1 subtitle, 2 action bar; the times are in ticks.

- Parameters:
    - slot : `int32`
    - text : `string`
    - fadeIn : `int32`
    - stay : `int32`
    - fadeOut : `int32`
- Return type: `error`
- Slots: [`player_send_title`](../cpp/player.md#player_send_title)

### `Player.Disconnect` {#Player.Disconnect}

```go
func (p Player) Disconnect(reason string) error
```

Disconnect removes the player from the server with a reason.

- Parameters:
    - reason : `string`
- Return type: `error`
- Slots: [`player_disconnect`](../cpp/player.md#player_disconnect)

### `Player.SetGameType` {#Player.SetGameType}

```go
func (p Player) SetGameType(mode int32) error
```

SetGameType sets the player's game mode, the value GameType reads.

- Parameters:
    - mode : `int32`
- Return type: `error`
- Slots: [`player_set_gamemode`](../cpp/player.md#player_set_gamemode)

### `Player.Teleport` {#Player.Teleport}

```go
func (p Player) Teleport(dim int32, x, y, z float64) error
```

Teleport moves the player to a position of any registered dimension.

- Parameters:
    - dim : `int32`
    - x : `float64`
    - y : `float64`
    - z : `float64`
- Return type: `error`
- Slots: [`player_teleport`](../cpp/player.md#player_teleport)

### `Player.CarriedItem` {#Player.CarriedItem}

```go
func (p Player) CarriedItem() (Item, error)
```

CarriedItem is the item in the player's hand.

- Return type: `(Item, error)`
- Slots: [`player_get_carried_item`](../cpp/player.md#player_get_carried_item)

### `Player.InventoryItem` {#Player.InventoryItem}

```go
func (p Player) InventoryItem(slot int32) (Item, error)
```

InventoryItem is the item in one inventory slot.

- Parameters:
    - slot : `int32`
- Return type: `(Item, error)`
- Slots: [`player_get_item`](../cpp/player.md#player_get_item)

### `Player.SetInventoryItem` {#Player.SetInventoryItem}

```go
func (p Player) SetInventoryItem(slot int32, item Item) error
```

SetInventoryItem puts an item into one inventory slot.

- Parameters:
    - slot : `int32`
    - item : `Item`
- Return type: `error`
- Slots: [`player_set_item`](../cpp/player.md#player_set_item)

### `Player.ConnID` {#Player.ConnID}

```go
func (p Player) ConnID() (uint64, error)
```

ConnID is the player's network connection id.

- Return type: `(uint64, error)`
- Slots: [`player_conn_id`](../cpp/player.md#player_conn_id)

### `Player.Inventory` {#Player.Inventory}

```go
func (p Player) Inventory() Container
```

Inventory is the player's inventory as a container.

- Return type: `Container`

### `Player.EnderChest` {#Player.EnderChest}

```go
func (p Player) EnderChest() Container
```

EnderChest is the player's ender chest as a container.

- Return type: `Container`

### `Player.Armor` {#Player.Armor}

```go
func (p Player) Armor() Container
```

Armor is the player's armor slots as a container.

- Return type: `Container`

### `Player.Offhand` {#Player.Offhand}

```go
func (p Player) Offhand() Container
```

Offhand is the player's offhand slot as a container.

- Return type: `Container`

### `Player.GameType` {#Player.GameType}

```go
func (p Player) GameType() (float64, error)
```

GameType reads `PIER_PPROP_GAME_TYPE`: (G) `Player::getPlayerGameType`; write via `player_set_gamemode`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Level` {#Player.Level}

```go
func (p Player) Level() (float64, error)
```

Level reads `PIER_PPROP_LEVEL`: (S) attribute `Player::LEVEL()`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetLevel` {#Player.SetLevel}

```go
func (p Player) SetLevel(v float64) error
```

SetLevel writes `PIER_PPROP_LEVEL`: (S) attribute `Player::LEVEL()`

- Parameters:
    - v : `float64`
- Return type: `error`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Experience` {#Player.Experience}

```go
func (p Player) Experience() (float64, error)
```

Experience reads `PIER_PPROP_EXPERIENCE`: (S) attribute `Player::EXPERIENCE()` (progress 0..1)

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetExperience` {#Player.SetExperience}

```go
func (p Player) SetExperience(v float64) error
```

SetExperience writes `PIER_PPROP_EXPERIENCE`: (S) attribute `Player::EXPERIENCE()` (progress 0..1)

- Parameters:
    - v : `float64`
- Return type: `error`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Hunger` {#Player.Hunger}

```go
func (p Player) Hunger() (float64, error)
```

Hunger reads `PIER_PPROP_HUNGER`: (S) attribute `Player::HUNGER()`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetHunger` {#Player.SetHunger}

```go
func (p Player) SetHunger(v float64) error
```

SetHunger writes `PIER_PPROP_HUNGER`: (S) attribute `Player::HUNGER()`

- Parameters:
    - v : `float64`
- Return type: `error`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Saturation` {#Player.Saturation}

```go
func (p Player) Saturation() (float64, error)
```

Saturation reads `PIER_PPROP_SATURATION`: (S) attribute `Player::SATURATION()`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetSaturation` {#Player.SetSaturation}

```go
func (p Player) SetSaturation(v float64) error
```

SetSaturation writes `PIER_PPROP_SATURATION`: (S) attribute `Player::SATURATION()`

- Parameters:
    - v : `float64`
- Return type: `error`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.Exhaustion` {#Player.Exhaustion}

```go
func (p Player) Exhaustion() (float64, error)
```

Exhaustion reads `PIER_PPROP_EXHAUSTION`: (S) attribute `Player::EXHAUSTION()`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SetExhaustion` {#Player.SetExhaustion}

```go
func (p Player) SetExhaustion(v float64) error
```

SetExhaustion writes `PIER_PPROP_EXHAUSTION`: (S) attribute `Player::EXHAUSTION()`

- Parameters:
    - v : `float64`
- Return type: `error`
- Slots: [`player_set_num`](../cpp/player.md#player_set_num)

### `Player.XpNeededNextLevel` {#Player.XpNeededNextLevel}

```go
func (p Player) XpNeededNextLevel() (float64, error)
```

XpNeededNextLevel reads `PIER_PPROP_XP_NEEDED_NEXT_LEVEL`: (G) `Player::getXpNeededForNextLevel`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Luck` {#Player.Luck}

```go
func (p Player) Luck() (float64, error)
```

Luck reads `PIER_PPROP_LUCK`: (G) `Player::getLuck`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.SelectedSlot` {#Player.SelectedSlot}

```go
func (p Player) SelectedSlot() (float64, error)
```

SelectedSlot reads `PIER_PPROP_SELECTED_SLOT`: (G) `Player::getSelectedItemSlot`; set via `PIER_PACT_SET_SELECTED_SLOT`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsOperator` {#Player.IsOperator}

```go
func (p Player) IsOperator() (bool, error)
```

IsOperator reads `PIER_PPROP_IS_OPERATOR`: (G) `Player::isOperator`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanUseOperatorBlocks` {#Player.CanUseOperatorBlocks}

```go
func (p Player) CanUseOperatorBlocks() (bool, error)
```

CanUseOperatorBlocks reads `PIER_PPROP_CAN_USE_OPERATOR_BLOCKS`: (G) `Player::canUseOperatorBlocks`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsFlying` {#Player.IsFlying}

```go
func (p Player) IsFlying() (bool, error)
```

IsFlying reads `PIER_PPROP_IS_FLYING`: (G) `Player::isFlying`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanJump` {#Player.CanJump}

```go
func (p Player) CanJump() (bool, error)
```

CanJump reads `PIER_PPROP_CAN_JUMP`: (G) `Player::canJump`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsEmoting` {#Player.IsEmoting}

```go
func (p Player) IsEmoting() (bool, error)
```

IsEmoting reads `PIER_PPROP_IS_EMOTING`: (G) `Player::isEmoting`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsInRaid` {#Player.IsInRaid}

```go
func (p Player) IsInRaid() (bool, error)
```

IsInRaid reads `PIER_PPROP_IS_IN_RAID`: (G) `Player::isInRaid`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsHurt` {#Player.IsHurt}

```go
func (p Player) IsHurt() (bool, error)
```

IsHurt reads `PIER_PPROP_IS_HURT`: (G) `Player::isHurt`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsScoping` {#Player.IsScoping}

```go
func (p Player) IsScoping() (bool, error)
```

IsScoping reads `PIER_PPROP_IS_SCOPING`: (G) `Player::isScoping`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanSleep` {#Player.CanSleep}

```go
func (p Player) CanSleep() (bool, error)
```

CanSleep reads `PIER_PPROP_CAN_SLEEP`: (G) `Player::canSleep`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.HasRespawnPosition` {#Player.HasRespawnPosition}

```go
func (p Player) HasRespawnPosition() (bool, error)
```

HasRespawnPosition reads `PIER_PPROP_HAS_RESPAWN_POSITION`: (G) `Player::hasRespawnPosition`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.ClientSubId` {#Player.ClientSubId}

```go
func (p Player) ClientSubId() (float64, error)
```

ClientSubId reads `PIER_PPROP_CLIENT_SUB_ID`: (G) `Player::getClientSubId`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.CanUseAbility` {#Player.CanUseAbility}

```go
func (p Player) CanUseAbility() (bool, error)
```

CanUseAbility reads `PIER_PPROP_CAN_USE_ABILITY`: (G) `Player::canUseAbility`; the ability index is passed through the `player_action` GET path, see `PIER_PACT_CAN_USE_ABILITY`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Direction` {#Player.Direction}

```go
func (p Player) Direction() (float64, error)
```

Direction reads `PIER_PPROP_DIRECTION`: (G) `Player::getDirection` (0=S,1=W,2=N,3=E)

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.ChunkRadius` {#Player.ChunkRadius}

```go
func (p Player) ChunkRadius() (float64, error)
```

ChunkRadius reads `PIER_PPROP_CHUNK_RADIUS`: (G) `Player::getChunkRadius`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.NetworkRtt` {#Player.NetworkRtt}

```go
func (p Player) NetworkRtt() (float64, error)
```

NetworkRtt reads `PIER_PPROP_NETWORK_RTT`: (G) getNetworkStatus().mPing (ms)

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Platform` {#Player.Platform}

```go
func (p Player) Platform() (float64, error)
```

Platform reads `PIER_PPROP_PLATFORM`: (G) `Player::getPlatform`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.EnchantmentSeed` {#Player.EnchantmentSeed}

```go
func (p Player) EnchantmentSeed() (float64, error)
```

EnchantmentSeed reads `PIER_PPROP_ENCHANTMENT_SEED`: (G) `Player::getEnchantmentSeed`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsUsingItem` {#Player.IsUsingItem}

```go
func (p Player) IsUsingItem() (bool, error)
```

IsUsingItem reads `PIER_PPROP_IS_USING_ITEM`: (G) `Player::isUsingItem`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsBlocking` {#Player.IsBlocking}

```go
func (p Player) IsBlocking() (bool, error)
```

IsBlocking reads `PIER_PPROP_IS_BLOCKING`: (G) `Player::isBlocking`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsGliding` {#Player.IsGliding}

```go
func (p Player) IsGliding() (bool, error)
```

IsGliding reads `PIER_PPROP_IS_GLIDING`: (G) `Player::isGliding`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsSwimming` {#Player.IsSwimming}

```go
func (p Player) IsSwimming() (bool, error)
```

IsSwimming reads `PIER_PPROP_IS_SWIMMING`: (G) `Player::isSwimming`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.PermissionLevel` {#Player.PermissionLevel}

```go
func (p Player) PermissionLevel() (float64, error)
```

PermissionLevel reads `PIER_PPROP_PERMISSION_LEVEL`: (G) `Player::getPlayerPermissionLevel`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Score` {#Player.Score}

```go
func (p Player) Score() (float64, error)
```

Score reads `PIER_PPROP_SCORE`: (G) `Player::getScore`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.FallDistance` {#Player.FallDistance}

```go
func (p Player) FallDistance() (float64, error)
```

FallDistance reads `PIER_PPROP_FALL_DISTANCE`: (G) `Actor::getFallDistance`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.IsDead` {#Player.IsDead}

```go
func (p Player) IsDead() (bool, error)
```

IsDead reads `PIER_PPROP_IS_DEAD`: (G) `Actor::isDead`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.HasDiedBefore` {#Player.HasDiedBefore}

```go
func (p Player) HasDiedBefore() (bool, error)
```

HasDiedBefore reads `PIER_PPROP_HAS_DIED_BEFORE`: (G) `Player::hasDiedBefore`

- Return type: `(bool, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.Dimension` {#Player.Dimension}

```go
func (p Player) Dimension() (float64, error)
```

Dimension reads `PIER_PPROP_DIMENSION`: (G) `Actor::getDimensionId`

- Return type: `(float64, error)`
- Slots: [`player_get_num`](../cpp/player.md#player_get_num)

### `Player.RealName` {#Player.RealName}

```go
func (p Player) RealName() (string, error)
```

RealName reads `PIER_PSTR_REAL_NAME`: `Player::getRealName`

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.Uuid` {#Player.Uuid}

```go
func (p Player) Uuid() (string, error)
```

Uuid reads `PIER_PSTR_UUID`: `Player::getUuid()`.asString()

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.Xuid` {#Player.Xuid}

```go
func (p Player) Xuid() (string, error)
```

Xuid reads `PIER_PSTR_XUID`: `Player::getXuid`

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.IpAndPort` {#Player.IpAndPort}

```go
func (p Player) IpAndPort() (string, error)
```

IpAndPort reads `PIER_PSTR_IP_AND_PORT`: `Player::getIPAndPort`

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.LocaleCode` {#Player.LocaleCode}

```go
func (p Player) LocaleCode() (string, error)
```

LocaleCode reads `PIER_PSTR_LOCALE_CODE`: `Player::getLocaleCode`

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.NameTag` {#Player.NameTag}

```go
func (p Player) NameTag() (string, error)
```

NameTag reads `PIER_PSTR_NAME_TAG`: `Actor::getNameTag` (display name)

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.LastDeathPos` {#Player.LastDeathPos}

```go
func (p Player) LastDeathPos() (string, error)
```

LastDeathPos reads `PIER_PSTR_LAST_DEATH_POS`: SNBT {x,y,z} or "" if none

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.LastDeathDimension` {#Player.LastDeathDimension}

```go
func (p Player) LastDeathDimension() (string, error)
```

LastDeathDimension reads `PIER_PSTR_LAST_DEATH_DIMENSION`: dimension id as string

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.NetworkStatus` {#Player.NetworkStatus}

```go
func (p Player) NetworkStatus() (string, error)
```

NetworkStatus reads `PIER_PSTR_NETWORK_STATUS`: SNBT {ping,`avg_ping`,`packet_loss`,`max_ping`}

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.PlatformOnlineId` {#Player.PlatformOnlineId}

```go
func (p Player) PlatformOnlineId() (string, error)
```

PlatformOnlineId reads `PIER_PSTR_PLATFORM_ONLINE_ID`: `Player::getPlatformOnlineId`

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.RespawnPos` {#Player.RespawnPos}

```go
func (p Player) RespawnPos() (string, error)
```

RespawnPos reads `PIER_PSTR_RESPAWN_POS`: SNBT {x,y,z,dim}: where this player would respawn. Never empty; a player with no bed reports the world spawn, and the two are not distinguishable.

- Return type: `(string, error)`
- Slots: [`player_get_str`](../cpp/player.md#player_get_str)

### `Player.SetAbility` {#Player.SetAbility}

```go
func (p Player) SetAbility(sarg string, a, b, c float64) (string, error)
```

SetAbility runs `PIER_PACT_SET_ABILITY`: a=AbilitiesIndex, b=0/1 (bool slots) or float (FlySpeed etc.). Restores PlayerPermissionLevel to its pre-write value afterwards: the engine's `LayeredAbilities::setAbility` is the "switch to custom permissions" path and pushes the player to Custom, and that level ships to the client inside UpdateAbilitiesPacket together with the ability layer. To change the level, use `PIER_PACT_SET_PERMISSION_LEVEL`. Refused (false) until the player has finished joining (`Player::isPlayerInitialized`): an ability written while the client is still loading desynchronizes until the player rejoins. Simulated players are exempt.

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.CanUseAbilityAction` {#Player.CanUseAbilityAction}

```go
func (p Player) CanUseAbilityAction(sarg string, a, b, c float64) (string, error)
```

CanUseAbilityAction runs `PIER_PACT_CAN_USE_ABILITY`: a=AbilitiesIndex → out "0"/"1" `Player::canUseAbility`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SetSelectedSlot` {#Player.SetSelectedSlot}

```go
func (p Player) SetSelectedSlot(sarg string, a, b, c float64) (string, error)
```

SetSelectedSlot runs `PIER_PACT_SET_SELECTED_SLOT`: a=slot `Player::setSelectedSlot`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.GiveItem` {#Player.GiveItem}

```go
func (p Player) GiveItem(sarg string, a, b, c float64) (string, error)
```

GiveItem runs `PIER_PACT_GIVE_ITEM`: sarg=item SNBT `ItemStack::fromTag` + `Player::addAndRefresh`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SetSpawnPoint` {#Player.SetSpawnPoint}

```go
func (p Player) SetSpawnPoint(sarg string, a, b, c float64) (string, error)
```

SetSpawnPoint runs `PIER_PACT_SET_SPAWN_POINT`: a,b,c=pos, sarg=dim id (any registered dim); native `Player::setRespawnPosition`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.ClearTitle` {#Player.ClearTitle}

```go
func (p Player) ClearTitle(sarg string, a, b, c float64) (string, error)
```

ClearTitle runs `PIER_PACT_CLEAR_TITLE`: native SetTitlePacket(Clear)

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SetTitle` {#Player.SetTitle}

```go
func (p Player) SetTitle(sarg string, a, b, c float64) (string, error)
```

SetTitle runs `PIER_PACT_SET_TITLE`: sarg=text, a=slot(0 title,1 subtitle,2 actionbar); native SetTitlePacket, text sent verbatim

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.AddExperience` {#Player.AddExperience}

```go
func (p Player) AddExperience(sarg string, a, b, c float64) (string, error)
```

AddExperience runs `PIER_PACT_ADD_EXPERIENCE`: a=xp `Player::addExperience`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.AddLevels` {#Player.AddLevels}

```go
func (p Player) AddLevels(sarg string, a, b, c float64) (string, error)
```

AddLevels runs `PIER_PACT_ADD_LEVELS`: a=levels `Player::addLevels`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.StartCooldown` {#Player.StartCooldown}

```go
func (p Player) StartCooldown(sarg string, a, b, c float64) (string, error)
```

StartCooldown runs `PIER_PACT_START_COOLDOWN`: sarg=item name, a=ticks `Player::startItemCooldown`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.StartRiding` {#Player.StartRiding}

```go
func (p Player) StartRiding(sarg string, a, b, c float64) (string, error)
```

StartRiding runs `PIER_PACT_START_RIDING`: a=vehicle ActorUniqueID (lower 64b) `Player::startRiding`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.StopRiding` {#Player.StopRiding}

```go
func (p Player) StopRiding(sarg string, a, b, c float64) (string, error)
```

StopRiding runs `PIER_PACT_STOP_RIDING`: `Player::stopRiding`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.Attack` {#Player.Attack}

```go
func (p Player) Attack(sarg string, a, b, c float64) (string, error)
```

Attack runs `PIER_PACT_ATTACK`: a=target ActorUniqueID (lower 64b) `Player::attack`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.Drop` {#Player.Drop}

```go
func (p Player) Drop(sarg string, a, b, c float64) (string, error)
```

Drop runs `PIER_PACT_DROP`: sarg=item SNBT, a=random(0/1) `Player::drop`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.Interact` {#Player.Interact}

```go
func (p Player) Interact(sarg string, a, b, c float64) (string, error)
```

Interact runs `PIER_PACT_INTERACT`: a=target ActorUniqueID `Player::interact`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.StartUsingItem` {#Player.StartUsingItem}

```go
func (p Player) StartUsingItem(sarg string, a, b, c float64) (string, error)
```

StartUsingItem runs `PIER_PACT_START_USING_ITEM`: sarg=item SNBT, a=duration `Player::startUsingItem`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.StopUsingItem` {#Player.StopUsingItem}

```go
func (p Player) StopUsingItem(sarg string, a, b, c float64) (string, error)
```

StopUsingItem runs `PIER_PACT_STOP_USING_ITEM`: `Player::stopUsingItem`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SetChunkRadius` {#Player.SetChunkRadius}

```go
func (p Player) SetChunkRadius(sarg string, a, b, c float64) (string, error)
```

SetChunkRadius runs `PIER_PACT_SET_CHUNK_RADIUS`: a=radius `Player::setChunkRadius`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SetEnchantmentSeed` {#Player.SetEnchantmentSeed}

```go
func (p Player) SetEnchantmentSeed(sarg string, a, b, c float64) (string, error)
```

SetEnchantmentSeed runs `PIER_PACT_SET_ENCHANTMENT_SEED`: a=seed `Player::setEnchantmentSeed`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.RegisterTrackedBoss` {#Player.RegisterTrackedBoss}

```go
func (p Player) RegisterTrackedBoss(sarg string, a, b, c float64) (string, error)
```

RegisterTrackedBoss runs `PIER_PACT_REGISTER_TRACKED_BOSS`: a=boss ActorUniqueID `Player::registerTrackedBoss`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.UnregisterTrackedBoss` {#Player.UnregisterTrackedBoss}

```go
func (p Player) UnregisterTrackedBoss(sarg string, a, b, c float64) (string, error)
```

UnregisterTrackedBoss runs `PIER_PACT_UNREGISTER_TRACKED_BOSS`: a=boss ActorUniqueID `Player::unRegisterTrackedBoss`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.PlayEmote` {#Player.PlayEmote}

```go
func (p Player) PlayEmote(sarg string, a, b, c float64) (string, error)
```

PlayEmote runs `PIER_PACT_PLAY_EMOTE`: sarg=piece id `Player::playEmote`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.ResendAllChunks` {#Player.ResendAllChunks}

```go
func (p Player) ResendAllChunks(sarg string, a, b, c float64) (string, error)
```

ResendAllChunks runs `PIER_PACT_RESEND_ALL_CHUNKS`: `Player::resendAllChunks`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.OpenInventory` {#Player.OpenInventory}

```go
func (p Player) OpenInventory(sarg string, a, b, c float64) (string, error)
```

OpenInventory runs `PIER_PACT_OPEN_INVENTORY`: `Player::openInventory`

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SidebarSet` {#Player.SidebarSet}

```go
func (p Player) SidebarSet(sarg string, a, b, c float64) (string, error)
```

SidebarSet runs `PIER_PACT_SIDEBAR_SET`: sarg="obj\\ntitle\\nline…" per-player sidebar

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SidebarClear` {#Player.SidebarClear}

```go
func (p Player) SidebarClear(sarg string, a, b, c float64) (string, error)
```

SidebarClear runs `PIER_PACT_SIDEBAR_CLEAR`: sarg=objective RemoveObjectivePacket

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

### `Player.SetPermissionLevel` {#Player.SetPermissionLevel}

```go
func (p Player) SetPermissionLevel(sarg string, a, b, c float64) (string, error)
```

SetPermissionLevel runs `PIER_PACT_SET_PERMISSION_LEVEL`: a=PlayerPermissionLevel (0 Visitor, 1 Member, 2 Operator, 3 Custom). `LayeredAbilities::setPlayerPermissions` plus UpdateAbilitiesPacket. The read side is `PIER_PPROP_PERMISSION_LEVEL`. Refused until the player has finished joining, like `PIER_PACT_SET_ABILITY`.

The output, when the verb has one, is returned; abi.h names each argument.

- Parameters:
    - sarg : `string`
    - a : `float64`
    - b : `float64`
    - c : `float64`
- Return type: `(string, error)`
- Slots: [`player_action`](../cpp/player.md#player_action)

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

PlayerInfo is one online player as ListPlayers reports it. Dim and the position are read only when present, and HasDim and HasPos say whether they were.

## `SelKind` {#SelKind}

```go
type SelKind int32
```

SelKind says how a PlayerSel names its player.

| Name | Value | Description |
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

PlayerSel names one player by name, xuid or uuid.

## `PlayerPos` {#PlayerPos}

```go
type PlayerPos struct {
    X, Y, Z   float64
    Dimension int32
    Found     bool
}
```

PlayerPos is where a player is; Found is false when nobody matched.

## `PProp*` {#PProp}

| Name | Value | Description |
|---|---|---|
| <span id="PPropGameType"></span>`PPropGameType` | `0` | PPropGameType is `PIER_PPROP_GAME_TYPE`: (G) `Player::getPlayerGameType`; write via `player_set_gamemode` |
| <span id="PPropLevel"></span>`PPropLevel` | `1` | PPropLevel is `PIER_PPROP_LEVEL`: (S) attribute `Player::LEVEL()` |
| <span id="PPropExperience"></span>`PPropExperience` | `2` | PPropExperience is `PIER_PPROP_EXPERIENCE`: (S) attribute `Player::EXPERIENCE()` (progress 0..1) |
| <span id="PPropHunger"></span>`PPropHunger` | `3` | PPropHunger is `PIER_PPROP_HUNGER`: (S) attribute `Player::HUNGER()` |
| <span id="PPropSaturation"></span>`PPropSaturation` | `4` | PPropSaturation is `PIER_PPROP_SATURATION`: (S) attribute `Player::SATURATION()` |
| <span id="PPropExhaustion"></span>`PPropExhaustion` | `5` | PPropExhaustion is `PIER_PPROP_EXHAUSTION`: (S) attribute `Player::EXHAUSTION()` |
| <span id="PPropXpNeededNextLevel"></span>`PPropXpNeededNextLevel` | `6` | PPropXpNeededNextLevel is `PIER_PPROP_XP_NEEDED_NEXT_LEVEL`: (G) `Player::getXpNeededForNextLevel` |
| <span id="PPropLuck"></span>`PPropLuck` | `7` | PPropLuck is `PIER_PPROP_LUCK`: (G) `Player::getLuck` |
| <span id="PPropSelectedSlot"></span>`PPropSelectedSlot` | `8` | PPropSelectedSlot is `PIER_PPROP_SELECTED_SLOT`: (G) `Player::getSelectedItemSlot`; set via `PIER_PACT_SET_SELECTED_SLOT` |
| <span id="PPropIsOperator"></span>`PPropIsOperator` | `9` | PPropIsOperator is `PIER_PPROP_IS_OPERATOR`: (G) `Player::isOperator` |
| <span id="PPropCanUseOperatorBlocks"></span>`PPropCanUseOperatorBlocks` | `10` | PPropCanUseOperatorBlocks is `PIER_PPROP_CAN_USE_OPERATOR_BLOCKS`: (G) `Player::canUseOperatorBlocks` |
| <span id="PPropIsFlying"></span>`PPropIsFlying` | `11` | PPropIsFlying is `PIER_PPROP_IS_FLYING`: (G) `Player::isFlying` |
| <span id="PPropCanJump"></span>`PPropCanJump` | `12` | PPropCanJump is `PIER_PPROP_CAN_JUMP`: (G) `Player::canJump` |
| <span id="PPropIsEmoting"></span>`PPropIsEmoting` | `13` | PPropIsEmoting is `PIER_PPROP_IS_EMOTING`: (G) `Player::isEmoting` |
| <span id="PPropIsInRaid"></span>`PPropIsInRaid` | `14` | PPropIsInRaid is `PIER_PPROP_IS_IN_RAID`: (G) `Player::isInRaid` |
| <span id="PPropIsHurt"></span>`PPropIsHurt` | `15` | PPropIsHurt is `PIER_PPROP_IS_HURT`: (G) `Player::isHurt` |
| <span id="PPropIsScoping"></span>`PPropIsScoping` | `16` | PPropIsScoping is `PIER_PPROP_IS_SCOPING`: (G) `Player::isScoping` |
| <span id="PPropCanSleep"></span>`PPropCanSleep` | `17` | PPropCanSleep is `PIER_PPROP_CAN_SLEEP`: (G) `Player::canSleep` |
| <span id="PPropHasRespawnPosition"></span>`PPropHasRespawnPosition` | `18` | PPropHasRespawnPosition is `PIER_PPROP_HAS_RESPAWN_POSITION`: (G) `Player::hasRespawnPosition` |
| <span id="PPropClientSubId"></span>`PPropClientSubId` | `19` | PPropClientSubId is `PIER_PPROP_CLIENT_SUB_ID`: (G) `Player::getClientSubId` |
| <span id="PPropCanUseAbility"></span>`PPropCanUseAbility` | `20` | PPropCanUseAbility is `PIER_PPROP_CAN_USE_ABILITY`: (G) `Player::canUseAbility`; the ability index is passed through the `player_action` GET path, see `PIER_PACT_CAN_USE_ABILITY` |
| <span id="PPropDirection"></span>`PPropDirection` | `21` | PPropDirection is `PIER_PPROP_DIRECTION`: (G) `Player::getDirection` (0=S,1=W,2=N,3=E) |
| <span id="PPropChunkRadius"></span>`PPropChunkRadius` | `22` | PPropChunkRadius is `PIER_PPROP_CHUNK_RADIUS`: (G) `Player::getChunkRadius` |
| <span id="PPropNetworkRtt"></span>`PPropNetworkRtt` | `23` | PPropNetworkRtt is `PIER_PPROP_NETWORK_RTT`: (G) getNetworkStatus().mPing (ms) |
| <span id="PPropPlatform"></span>`PPropPlatform` | `24` | PPropPlatform is `PIER_PPROP_PLATFORM`: (G) `Player::getPlatform` |
| <span id="PPropEnchantmentSeed"></span>`PPropEnchantmentSeed` | `25` | PPropEnchantmentSeed is `PIER_PPROP_ENCHANTMENT_SEED`: (G) `Player::getEnchantmentSeed` |
| <span id="PPropIsUsingItem"></span>`PPropIsUsingItem` | `26` | PPropIsUsingItem is `PIER_PPROP_IS_USING_ITEM`: (G) `Player::isUsingItem` |
| <span id="PPropIsBlocking"></span>`PPropIsBlocking` | `27` | PPropIsBlocking is `PIER_PPROP_IS_BLOCKING`: (G) `Player::isBlocking` |
| <span id="PPropIsGliding"></span>`PPropIsGliding` | `28` | PPropIsGliding is `PIER_PPROP_IS_GLIDING`: (G) `Player::isGliding` |
| <span id="PPropIsSwimming"></span>`PPropIsSwimming` | `29` | PPropIsSwimming is `PIER_PPROP_IS_SWIMMING`: (G) `Player::isSwimming` |
| <span id="PPropPermissionLevel"></span>`PPropPermissionLevel` | `30` | PPropPermissionLevel is `PIER_PPROP_PERMISSION_LEVEL`: (G) `Player::getPlayerPermissionLevel` |
| <span id="PPropScore"></span>`PPropScore` | `31` | PPropScore is `PIER_PPROP_SCORE`: (G) `Player::getScore` |
| <span id="PPropFallDistance"></span>`PPropFallDistance` | `32` | PPropFallDistance is `PIER_PPROP_FALL_DISTANCE`: (G) `Actor::getFallDistance` |
| <span id="PPropIsDead"></span>`PPropIsDead` | `33` | PPropIsDead is `PIER_PPROP_IS_DEAD`: (G) `Actor::isDead` |
| <span id="PPropHasDiedBefore"></span>`PPropHasDiedBefore` | `34` | PPropHasDiedBefore is `PIER_PPROP_HAS_DIED_BEFORE`: (G) `Player::hasDiedBefore` |
| <span id="PPropDimension"></span>`PPropDimension` | `35` | PPropDimension is `PIER_PPROP_DIMENSION`: (G) `Actor::getDimensionId` |

## `PStr*` {#PStr}

| Name | Value | Description |
|---|---|---|
| <span id="PStrRealName"></span>`PStrRealName` | `0` | PStrRealName is `PIER_PSTR_REAL_NAME`: `Player::getRealName` |
| <span id="PStrUuid"></span>`PStrUuid` | `1` | PStrUuid is `PIER_PSTR_UUID`: `Player::getUuid()`.asString() |
| <span id="PStrXuid"></span>`PStrXuid` | `2` | PStrXuid is `PIER_PSTR_XUID`: `Player::getXuid` |
| <span id="PStrIpAndPort"></span>`PStrIpAndPort` | `3` | PStrIpAndPort is `PIER_PSTR_IP_AND_PORT`: `Player::getIPAndPort` |
| <span id="PStrLocaleCode"></span>`PStrLocaleCode` | `4` | PStrLocaleCode is `PIER_PSTR_LOCALE_CODE`: `Player::getLocaleCode` |
| <span id="PStrNameTag"></span>`PStrNameTag` | `5` | PStrNameTag is `PIER_PSTR_NAME_TAG`: `Actor::getNameTag` (display name) |
| <span id="PStrLastDeathPos"></span>`PStrLastDeathPos` | `6` | PStrLastDeathPos is `PIER_PSTR_LAST_DEATH_POS`: SNBT {x,y,z} or "" if none |
| <span id="PStrLastDeathDimension"></span>`PStrLastDeathDimension` | `7` | PStrLastDeathDimension is `PIER_PSTR_LAST_DEATH_DIMENSION`: dimension id as string |
| <span id="PStrNetworkStatus"></span>`PStrNetworkStatus` | `8` | PStrNetworkStatus is `PIER_PSTR_NETWORK_STATUS`: SNBT {ping,`avg_ping`,`packet_loss`,`max_ping`} |
| <span id="PStrPlatformOnlineId"></span>`PStrPlatformOnlineId` | `9` | PStrPlatformOnlineId is `PIER_PSTR_PLATFORM_ONLINE_ID`: `Player::getPlatformOnlineId` |
| <span id="PStrRespawnPos"></span>`PStrRespawnPos` | `10` | PStrRespawnPos is `PIER_PSTR_RESPAWN_POS`: SNBT {x,y,z,dim}: where this player would respawn. Never empty; a player with no bed reports the world spawn, and the two are not distinguishable. |

## `PAct*` {#PAct}

| Name | Value | Description |
|---|---|---|
| <span id="PActSetAbility"></span>`PActSetAbility` | `0` | PActSetAbility is `PIER_PACT_SET_ABILITY`: a=AbilitiesIndex, b=0/1 (bool slots) or float (FlySpeed etc.). Restores PlayerPermissionLevel to its pre-write value afterwards: the engine's `LayeredAbilities::setAbility` is the "switch to custom permissions" path and pushes the player to Custom, and that level ships to the client inside UpdateAbilitiesPacket together with the ability layer. To change the level, use `PIER_PACT_SET_PERMISSION_LEVEL`. Refused (false) until the player has finished joining (`Player::isPlayerInitialized`): an ability written while the client is still loading desynchronizes until the player rejoins. Simulated players are exempt. |
| <span id="PActCanUseAbility"></span>`PActCanUseAbility` | `1` | PActCanUseAbility is `PIER_PACT_CAN_USE_ABILITY`: a=AbilitiesIndex → out "0"/"1" `Player::canUseAbility` |
| <span id="PActSetSelectedSlot"></span>`PActSetSelectedSlot` | `2` | PActSetSelectedSlot is `PIER_PACT_SET_SELECTED_SLOT`: a=slot `Player::setSelectedSlot` |
| <span id="PActGiveItem"></span>`PActGiveItem` | `3` | PActGiveItem is `PIER_PACT_GIVE_ITEM`: sarg=item SNBT `ItemStack::fromTag` + `Player::addAndRefresh` |
| <span id="PActSetSpawnPoint"></span>`PActSetSpawnPoint` | `4` | PActSetSpawnPoint is `PIER_PACT_SET_SPAWN_POINT`: a,b,c=pos, sarg=dim id (any registered dim); native `Player::setRespawnPosition` |
| <span id="PActClearTitle"></span>`PActClearTitle` | `5` | PActClearTitle is `PIER_PACT_CLEAR_TITLE`: native SetTitlePacket(Clear) |
| <span id="PActSetTitle"></span>`PActSetTitle` | `6` | PActSetTitle is `PIER_PACT_SET_TITLE`: sarg=text, a=slot(0 title,1 subtitle,2 actionbar); native SetTitlePacket, text sent verbatim |
| <span id="PActAddExperience"></span>`PActAddExperience` | `7` | PActAddExperience is `PIER_PACT_ADD_EXPERIENCE`: a=xp `Player::addExperience` |
| <span id="PActAddLevels"></span>`PActAddLevels` | `8` | PActAddLevels is `PIER_PACT_ADD_LEVELS`: a=levels `Player::addLevels` |
| <span id="PActStartCooldown"></span>`PActStartCooldown` | `9` | PActStartCooldown is `PIER_PACT_START_COOLDOWN`: sarg=item name, a=ticks `Player::startItemCooldown` |
| <span id="PActStartRiding"></span>`PActStartRiding` | `10` | PActStartRiding is `PIER_PACT_START_RIDING`: a=vehicle ActorUniqueID (lower 64b) `Player::startRiding` |
| <span id="PActStopRiding"></span>`PActStopRiding` | `11` | PActStopRiding is `PIER_PACT_STOP_RIDING`: `Player::stopRiding` |
| <span id="PActAttack"></span>`PActAttack` | `12` | PActAttack is `PIER_PACT_ATTACK`: a=target ActorUniqueID (lower 64b) `Player::attack` |
| <span id="PActDrop"></span>`PActDrop` | `13` | PActDrop is `PIER_PACT_DROP`: sarg=item SNBT, a=random(0/1) `Player::drop` |
| <span id="PActInteract"></span>`PActInteract` | `14` | PActInteract is `PIER_PACT_INTERACT`: a=target ActorUniqueID `Player::interact` |
| <span id="PActStartUsingItem"></span>`PActStartUsingItem` | `15` | PActStartUsingItem is `PIER_PACT_START_USING_ITEM`: sarg=item SNBT, a=duration `Player::startUsingItem` |
| <span id="PActStopUsingItem"></span>`PActStopUsingItem` | `16` | PActStopUsingItem is `PIER_PACT_STOP_USING_ITEM`: `Player::stopUsingItem` |
| <span id="PActSetChunkRadius"></span>`PActSetChunkRadius` | `17` | PActSetChunkRadius is `PIER_PACT_SET_CHUNK_RADIUS`: a=radius `Player::setChunkRadius` |
| <span id="PActSetEnchantmentSeed"></span>`PActSetEnchantmentSeed` | `18` | PActSetEnchantmentSeed is `PIER_PACT_SET_ENCHANTMENT_SEED`: a=seed `Player::setEnchantmentSeed` |
| <span id="PActRegisterTrackedBoss"></span>`PActRegisterTrackedBoss` | `19` | PActRegisterTrackedBoss is `PIER_PACT_REGISTER_TRACKED_BOSS`: a=boss ActorUniqueID `Player::registerTrackedBoss` |
| <span id="PActUnregisterTrackedBoss"></span>`PActUnregisterTrackedBoss` | `20` | PActUnregisterTrackedBoss is `PIER_PACT_UNREGISTER_TRACKED_BOSS`: a=boss ActorUniqueID `Player::unRegisterTrackedBoss` |
| <span id="PActPlayEmote"></span>`PActPlayEmote` | `21` | PActPlayEmote is `PIER_PACT_PLAY_EMOTE`: sarg=piece id `Player::playEmote` |
| <span id="PActResendAllChunks"></span>`PActResendAllChunks` | `22` | PActResendAllChunks is `PIER_PACT_RESEND_ALL_CHUNKS`: `Player::resendAllChunks` |
| <span id="PActOpenInventory"></span>`PActOpenInventory` | `23` | PActOpenInventory is `PIER_PACT_OPEN_INVENTORY`: `Player::openInventory` |
| <span id="PActSidebarSet"></span>`PActSidebarSet` | `24` | PActSidebarSet is `PIER_PACT_SIDEBAR_SET`: sarg="obj\\ntitle\\nline…" per-player sidebar |
| <span id="PActSidebarClear"></span>`PActSidebarClear` | `25` | PActSidebarClear is `PIER_PACT_SIDEBAR_CLEAR`: sarg=objective RemoveObjectivePacket |
| <span id="PActSetPermissionLevel"></span>`PActSetPermissionLevel` | `26` | PActSetPermissionLevel is `PIER_PACT_SET_PERMISSION_LEVEL`: a=PlayerPermissionLevel (0 Visitor, 1 Member, 2 Operator, 3 Custom). `LayeredAbilities::setPlayerPermissions` plus UpdateAbilitiesPacket. The read side is `PIER_PPROP_PERMISSION_LEVEL`. Refused until the player has finished joining, like `PIER_PACT_SET_ABILITY`. |
