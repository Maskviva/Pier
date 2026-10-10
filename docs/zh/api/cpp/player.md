# 玩家

??? note "abi.h 里的分节说明"

    **追加（`Appended`）**

    **§B 玩家管理（`§B player management`）**

    **§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）**

    **玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）**

    **标题（`Titles`）**

    `PACT_SET_TITLE`（`player_action` 的第 6 号操作）是通过执行控制台命令 `title "<name>" title <text>` 送到客户端的。这样做有三个问题，三个都会真的发生：

    - 文本不加引号就拼进命令行，名叫 `He said "hi"` 的地皮会把命令截断；
    - `title` 的文本参数类型是 `message`，会展开选择器，名叫 `@e` 的地皮就成了一次命令注入；
    - `/title` 没法在同一次调用里设置淡入和停留时间，计时用的是客户端上一次存下的值。

    这个槽位改为构造一个真正的 `SetTitlePacket`。没有任何线上格式跨过 FFI（数据包在这一侧逐个字段构造），所以它和 `spawn_particle_for` 一样，协议升级以后照样能用。

    `type` 是 `SetTitlePacketPayload::TitleType`：0 Clear · 1 Reset · 2 Title · 3 Subtitle · 4 Actionbar · 5 Times。TextObject 的几种变体（6 到 8）需要 `ResolvedTextObject`，会被拒绝。Clear、Reset、Times 忽略 `text`。

    时长的单位是**刻**。对 2、3、4，三个时长都 >= 0 时会先发一个 Times 包，计时是确定的，不沿用客户端上一次存下的值；三个都传 -1 则保留客户端当前的计时。混着传（有的是 -1，有的 >= 0）会被拒绝，不去猜：只指定一部分的时长没有合理的含义。只能在服务器线程调用。

    **同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）**

    追加了五个槽位，没有改动 `PIER_ABI_VERSION`：单纯的追加不算版本变化，`struct_size` 才是准确的关卡。

    两个方向都成立。新加载器运行旧模组：旧表是新表逐字节相同的前缀，模组够不着这五个槽位，照常工作。新模组配旧加载器：SDK 运行时初始化时比较 `struct_size`，发现加载器的表比它编译时用的短，于是拒绝加载。这是正确的结果，因为在没有 `lane_publish` 的加载器上读那个单元会越界。

    简单地说：版本号记录「语义变了」，`struct_size` 记录「表变长了」。这次改动只属于后者。

    见上面 `PierLaneDesc` 处的长注释。一句话概括：服务是跨语言的 (名字, JSON) -> JSON 通道；快速通道直接调用函数表，只在两边由同一个工具链构建时成立；指纹对不上时一个指针都不交出，使用方退回到服务通道。

    只能在服务器线程调用。

## 槽位 {#slots}

### `get_player_position` {#get_player_position}

```c
PierPlayerPos (*get_player_position)(PierStr name);
```

按名字查找一名已连接玩家的脚下位置和所在维度。用于根据玩家站的位置选取选区的角。只能在服务器线程调用。

- 调用形式：`api->get_player_position(name)`
- 参数：
    - name : `PierStr`
- 返回值类型：`PierPlayerPos`
- 所在分节：追加（`Appended`）
- 表内序号：第 14 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::position`](../rust/player.md#Player.position)
    - Go：[`Raw.GetPlayerPosition`](../go/raw.md#Raw.GetPlayerPosition)

### `list_players` {#list_players}

```c
void (*list_players)(void* ctx, PierStrSink snbt_sink);
```

每个在线玩家一段 SNBT：`{name,xuid,uuid,dim,x,y,z}`。

- 调用形式：`api->list_players(ctx, snbt_sink)`
- 参数：
    - ctx : `void*`
    - snbt_sink : `PierStrSink`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 21 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::list`](../rust/player.md#Player.list)、[`Player::list`](../rust/player.md#Player.list)
    - Go：[`ListPlayers`](../go/player.md#ListPlayers)、[`Raw.ListPlayers`](../go/raw.md#Raw.ListPlayers)

### `player_resolve` {#player_resolve}

```c
bool (*player_resolve)(PierPlayerSel sel, PierActorId* out);
```

把玩家选择器解析成这名玩家的 `ActorUniqueID`，由此接到 `actor_*` 这组接口上。

- 调用形式：`api->player_resolve(sel, out)`
- 参数：
    - sel : `PierPlayerSel`
    - out : `PierActorId*`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 22 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::is_online`](../rust/player.md#Player.is_online)、[`Player::as_entity`](../rust/player.md#Player.as_entity)
    - Go：[`Player.Resolve`](../go/player.md#Player.Resolve)、[`Player.IsOnline`](../go/player.md#Player.IsOnline)、[`Raw.PlayerResolve`](../go/raw.md#Raw.PlayerResolve)

### `player_send_message` {#player_send_message}

```c
bool (*player_send_message)(PierPlayerSel sel, PierStr msg);
```

- 调用形式：`api->player_send_message(sel, msg)`
- 参数：
    - sel : `PierPlayerSel`
    - msg : `PierStr`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 23 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::send_message`](../rust/player.md#Player.send_message)
    - Go：[`Player.SendMessage`](../go/player.md#Player.SendMessage)、[`Raw.PlayerSendMessage`](../go/raw.md#Raw.PlayerSendMessage)

### `player_disconnect` {#player_disconnect}

```c
bool (*player_disconnect)(PierPlayerSel sel, PierStr reason);
```

- 调用形式：`api->player_disconnect(sel, reason)`
- 参数：
    - sel : `PierPlayerSel`
    - reason : `PierStr`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 24 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::disconnect`](../rust/player.md#Player.disconnect)
    - Go：[`Player.Disconnect`](../go/player.md#Player.Disconnect)、[`Raw.PlayerDisconnect`](../go/raw.md#Raw.PlayerDisconnect)

### `broadcast_message` {#broadcast_message}

```c
void (*broadcast_message)(PierStr msg);
```

对每个在线玩家调用 `sendMessage`。

- 调用形式：`api->broadcast_message(msg)`
- 参数：
    - msg : `PierStr`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 25 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::broadcast`](../rust/player.md#Player.broadcast)
    - Go：[`Broadcast`](../go/player.md#Broadcast)、[`Raw.BroadcastMessage`](../go/raw.md#Raw.BroadcastMessage)

### `player_set_gamemode` {#player_set_gamemode}

```c
bool (*player_set_gamemode)(PierPlayerSel sel, int32_t mode);
```

0=生存，1=创造，2=冒险，6=旁观，原生调用（`Player::setPlayerGameType`）。

- 调用形式：`api->player_set_gamemode(sel, mode)`
- 参数：
    - sel : `PierPlayerSel`
    - mode : `int32_t`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 26 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::set_gamemode`](../rust/player.md#Player.set_gamemode)
    - Go：[`Player.SetGameType`](../go/player.md#Player.SetGameType)、[`Raw.PlayerSetGamemode`](../go/raw.md#Raw.PlayerSetGamemode)

### `player_teleport` {#player_teleport}

```c
bool (*player_teleport)(PierPlayerSel sel, int32_t dim, double x, double y, double z);
```

原生传送（`Actor::teleport`）。可以传送到自定义维度（id >= 3），但维度桥接必须产出 id 一致的引擎实例；对不上时调用失败，不会把玩家送进一个对不上的维度。

- 调用形式：`api->player_teleport(sel, dim, x, y, z)`
- 参数：
    - sel : `PierPlayerSel`
    - dim : `int32_t`
    - x : `double`
    - y : `double`
    - z : `double`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 27 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::teleport`](../rust/player.md#Player.teleport)
    - Go：[`Player.Teleport`](../go/player.md#Player.Teleport)、[`Raw.PlayerTeleport`](../go/raw.md#Raw.PlayerTeleport)

### `player_get_num` {#player_get_num}

```c
bool (*player_get_num)(PierPlayerSel sel, int32_t prop, double* out);
```

四个 `*_get_num` 槽位遵守同一个约定。返回值表示宿主有没有答案，`*out` 是答案；返回 false 时 `*out` 不会被改动。

false 和「值为零」是两种结果，调用方不能把它们合成一种（契约 §5.2）。false 有两个原因，宿主不加区分：对象没找到，或者这个属性在当前引擎版本上取不到。这两种情况宿主都不记日志，因为调用方轮询一个读不到的属性会把日志写满；在某个版本上总是回答 false 的属性，列在 CHANGELOG.md 里那个版本的条目下。

- 调用形式：`api->player_get_num(sel, prop, out)`
- 参数：
    - sel : `PierPlayerSel`
    - prop : `int32_t`
    - out : `double*`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 28 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::num`](../rust/player.md#Player.num)、[`Player::game_type`](../rust/player.md#Player.game_type)、[`Player::permission_level`](../rust/player.md#Player.permission_level)、[`Player::dimension`](../rust/player.md#Player.dimension)、[`Player::level`](../rust/player.md#Player.level)、[`Player::experience`](../rust/player.md#Player.experience) 等，共 36 个
    - Go：[`Player.GameType`](../go/player.md#Player.GameType)、[`Player.Level`](../go/player.md#Player.Level)、[`Player.Experience`](../go/player.md#Player.Experience)、[`Player.Hunger`](../go/player.md#Player.Hunger)、[`Player.Saturation`](../go/player.md#Player.Saturation)、[`Player.Exhaustion`](../go/player.md#Player.Exhaustion) 等，共 37 个
    - Zig：[`Player.gameType`](../zig/player.md#Player.gameType)、[`Player.level`](../zig/player.md#Player.level)、[`Player.experience`](../zig/player.md#Player.experience)、[`Player.hunger`](../zig/player.md#Player.hunger)、[`Player.saturation`](../zig/player.md#Player.saturation)、[`Player.exhaustion`](../zig/player.md#Player.exhaustion) 等，共 36 个

### `player_get_str` {#player_get_str}

```c
bool (*player_get_str)(PierPlayerSel sel, int32_t prop, void* ctx, PierStrSink sink);
```

- 调用形式：`api->player_get_str(sel, prop, ctx, sink)`
- 参数：
    - sel : `PierPlayerSel`
    - prop : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 29 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::text`](../rust/player.md#Player.text)、[`Player::last_death_pos`](../rust/player.md#Player.last_death_pos)、[`Player::position`](../rust/player.md#Player.position)、[`Player::real_name`](../rust/player.md#Player.real_name)、[`Player::uuid`](../rust/player.md#Player.uuid)、[`Player::xuid`](../rust/player.md#Player.xuid) 等，共 11 个
    - Go：[`Player.RealName`](../go/player.md#Player.RealName)、[`Player.Uuid`](../go/player.md#Player.Uuid)、[`Player.Xuid`](../go/player.md#Player.Xuid)、[`Player.IpAndPort`](../go/player.md#Player.IpAndPort)、[`Player.LocaleCode`](../go/player.md#Player.LocaleCode)、[`Player.NameTag`](../go/player.md#Player.NameTag) 等，共 12 个
    - Zig：[`Player.realName`](../zig/player.md#Player.realName)、[`Player.uuid`](../zig/player.md#Player.uuid)、[`Player.xuid`](../zig/player.md#Player.xuid)、[`Player.ipAndPort`](../zig/player.md#Player.ipAndPort)、[`Player.localeCode`](../zig/player.md#Player.localeCode)、[`Player.nameTag`](../zig/player.md#Player.nameTag) 等，共 11 个

### `player_set_num` {#player_set_num}

```c
bool (*player_set_num)(PierPlayerSel sel, int32_t prop, double v);
```

- 调用形式：`api->player_set_num(sel, prop, v)`
- 参数：
    - sel : `PierPlayerSel`
    - prop : `int32_t`
    - v : `double`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 30 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::set_num`](../rust/player.md#Player.set_num)、[`Player::set_level`](../rust/player.md#Player.set_level)、[`Player::set_experience`](../rust/player.md#Player.set_experience)、[`Player::set_hunger`](../rust/player.md#Player.set_hunger)、[`Player::set_saturation`](../rust/player.md#Player.set_saturation)、[`Player::set_exhaustion`](../rust/player.md#Player.set_exhaustion)
    - Go：[`Player.SetLevel`](../go/player.md#Player.SetLevel)、[`Player.SetExperience`](../go/player.md#Player.SetExperience)、[`Player.SetHunger`](../go/player.md#Player.SetHunger)、[`Player.SetSaturation`](../go/player.md#Player.SetSaturation)、[`Player.SetExhaustion`](../go/player.md#Player.SetExhaustion)、[`Raw.PlayerSetNum`](../go/raw.md#Raw.PlayerSetNum)
    - Zig：[`Player.setLevel`](../zig/player.md#Player.setLevel)、[`Player.setExperience`](../zig/player.md#Player.setExperience)、[`Player.setHunger`](../zig/player.md#Player.setHunger)、[`Player.setSaturation`](../zig/player.md#Player.setSaturation)、[`Player.setExhaustion`](../zig/player.md#Player.setExhaustion)

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

- 调用形式：`api->player_action(sel, action, sarg, a, b, c, ctx, out)`
- 参数：
    - sel : `PierPlayerSel`
    - action : `int32_t`
    - sarg : `PierStr`
    - a : `double`
    - b : `double`
    - c : `double`
    - ctx : `void*`
    - out : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§B 玩家管理（`§B player management`）
- 表内序号：第 31 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::act`](../rust/player.md#Player.act)、[`Player::set_ability`](../rust/player.md#Player.set_ability)、[`Player::set_ability_raw`](../rust/player.md#Player.set_ability_raw)、[`Player::can_use_ability`](../rust/player.md#Player.can_use_ability)、[`Player::set_permission_level`](../rust/player.md#Player.set_permission_level)、[`Player::set_selected_slot`](../rust/player.md#Player.set_selected_slot) 等，共 22 个
    - Go：[`Player.SetAbility`](../go/player.md#Player.SetAbility)、[`Player.CanUseAbilityAction`](../go/player.md#Player.CanUseAbilityAction)、[`Player.SetSelectedSlot`](../go/player.md#Player.SetSelectedSlot)、[`Player.GiveItem`](../go/player.md#Player.GiveItem)、[`Player.SetSpawnPoint`](../go/player.md#Player.SetSpawnPoint)、[`Player.ClearTitle`](../go/player.md#Player.ClearTitle) 等，共 28 个
    - Zig：[`Player.setAbility`](../zig/player.md#Player.setAbility)、[`Player.canUseAbilityAction`](../zig/player.md#Player.canUseAbilityAction)、[`Player.setSelectedSlot`](../zig/player.md#Player.setSelectedSlot)、[`Player.giveItem`](../zig/player.md#Player.giveItem)、[`Player.setSpawnPoint`](../zig/player.md#Player.setSpawnPoint)、[`Player.clearTitle`](../zig/player.md#Player.clearTitle) 等，共 27 个

### `player_send_message_typed` {#player_send_message_typed}

```c
bool (*player_send_message_typed)(PierPlayerSel sel, PierStr msg, int32_t type);
```

!!! note "分组说明"

    给一名玩家发送指定 `TextPacketType` 的消息（追加的槽位，受 `struct_size` 约束）。`type` 是 `TextPacketType` 的值：0 Raw · 1 Chat · 2 Translate · 3 Popup · 4 JukeboxPopup · 5 Tip · 6 SystemMessage · 7 Whisper · 8 Announcement · 9 TextObjectWhisper · 10 TextObject · 11 TextObjectAnnouncement。超出范围时按 Raw 处理。消息体只有一个字符串（和 LSE 的 tell 一样）：需要作者或参数的类型（Chat、Whisper、Translate）收到的是纯文本。普通的 `player_send_message` 仍然是发 Raw 或 Chat 消息的便捷路径。

- 调用形式：`api->player_send_message_typed(sel, msg, type)`
- 参数：
    - sel : `PierPlayerSel`
    - msg : `PierStr`
    - type : `int32_t`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 91 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::tell`](../rust/player.md#Player.tell)
    - Go：[`Player.SendMessageTyped`](../go/player.md#Player.SendMessageTyped)、[`Raw.PlayerSendMessageTyped`](../go/raw.md#Raw.PlayerSendMessageTyped)

### `player_get_carried_item` {#player_get_carried_item}

```c
bool (*player_get_carried_item)(PierPlayerSel sel, void* ctx, PierStrSink sink);
```

- 调用形式：`api->player_get_carried_item(sel, ctx, sink)`
- 参数：
    - sel : `PierPlayerSel`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）
- 表内序号：第 102 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::carried_item`](../rust/player.md#Player.carried_item)
    - Go：[`Player.CarriedItem`](../go/player.md#Player.CarriedItem)、[`Raw.PlayerGetCarriedItem`](../go/raw.md#Raw.PlayerGetCarriedItem)

### `player_get_item` {#player_get_item}

```c
bool (*player_get_item)(PierPlayerSel sel, int32_t slot, void* ctx, PierStrSink sink);
```

- 调用形式：`api->player_get_item(sel, slot, ctx, sink)`
- 参数：
    - sel : `PierPlayerSel`
    - slot : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）
- 表内序号：第 103 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::item`](../rust/player.md#Player.item)
    - Go：[`Player.InventoryItem`](../go/player.md#Player.InventoryItem)、[`Raw.PlayerGetItem`](../go/raw.md#Raw.PlayerGetItem)

### `player_set_item` {#player_set_item}

```c
bool (*player_set_item)(PierPlayerSel sel, int32_t slot, PierStr item_snbt);
```

- 调用形式：`api->player_set_item(sel, slot, item_snbt)`
- 参数：
    - sel : `PierPlayerSel`
    - slot : `int32_t`
    - item_snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）
- 表内序号：第 104 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::set_item`](../rust/player.md#Player.set_item)
    - Go：[`Player.SetInventoryItem`](../go/player.md#Player.SetInventoryItem)、[`Raw.PlayerSetItem`](../go/raw.md#Raw.PlayerSetItem)

### `player_get_equipment` {#player_get_equipment}

```c
bool (*player_get_equipment)(PierPlayerSel sel, void* ctx, PierStrSink sink);
```

全部装备，以 SNBT `[{slot, item_snbt},…]` 给出。`slot`：0 为主手，1 为副手，2 到 5 为盔甲。

- 调用形式：`api->player_get_equipment(sel, ctx, sink)`
- 参数：
    - sel : `PierPlayerSel`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）
- 表内序号：第 105 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::equipment`](../rust/player.md#Player.equipment)
    - Go：[`Raw.PlayerGetEquipment`](../go/raw.md#Raw.PlayerGetEquipment)

### `player_get_cooldown` {#player_get_cooldown}

```c
int32_t (*player_get_cooldown)(PierPlayerSel sel, PierStr item_name);
```

一个物品的冷却还剩多少刻（不在冷却中或者玩家不在线时为 -1）。

- 调用形式：`api->player_get_cooldown(sel, item_name)`
- 参数：
    - sel : `PierPlayerSel`
    - item_name : `PierStr`
- 返回值类型：`int32_t`
- 所在分节：玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）
- 表内序号：第 106 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::cooldown`](../rust/player.md#Player.cooldown)
    - Go：[`Raw.PlayerGetCooldown`](../go/raw.md#Raw.PlayerGetCooldown)

### `player_start_cooldown` {#player_start_cooldown}

```c
bool (*player_start_cooldown)(PierPlayerSel sel, PierStr item_name, int32_t ticks);
```

- 调用形式：`api->player_start_cooldown(sel, item_name, ticks)`
- 参数：
    - sel : `PierPlayerSel`
    - item_name : `PierStr`
    - ticks : `int32_t`
- 返回值类型：`bool`
- 所在分节：玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）
- 表内序号：第 107 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::start_cooldown`](../rust/player.md#Player.start_cooldown)
    - Go：[`Raw.PlayerStartCooldown`](../go/raw.md#Raw.PlayerStartCooldown)

### `player_get_network_status` {#player_get_network_status}

```c
bool (*player_get_network_status)(PierPlayerSel sel, void* ctx, PierStrSink sink);
```

- 调用形式：`api->player_get_network_status(sel, ctx, sink)`
- 参数：
    - sel : `PierPlayerSel`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：玩家：装备、冷却、网络（专用函数）（`Player: equipment, cooldown, network (dedicated fns)`）
- 表内序号：第 108 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::network_status`](../rust/player.md#Player.network_status)
    - Go：[`Raw.PlayerGetNetworkStatus`](../go/raw.md#Raw.PlayerGetNetworkStatus)

### `player_send_title` {#player_send_title}

```c
bool (*player_send_title)(
    PierPlayerSel sel, int32_t type, PierStr text, int32_t fade_in_ticks,
    int32_t stay_ticks, int32_t fade_out_ticks);
```

- 调用形式：`api->player_send_title(sel, type, text, fade_in_ticks, stay_ticks, fade_out_ticks)`
- 参数：
    - sel : `PierPlayerSel`
    - type : `int32_t`
    - text : `PierStr`
    - fade_in_ticks : `int32_t`
    - stay_ticks : `int32_t`
    - fade_out_ticks : `int32_t`
- 返回值类型：`bool`
- 所在分节：标题（`Titles`）
- 表内序号：第 156 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::send_title`](../rust/player.md#Player.send_title)、[`Player::set_title`](../rust/player.md#Player.set_title)、[`Player::set_subtitle`](../rust/player.md#Player.set_subtitle)、[`Player::set_actionbar`](../rust/player.md#Player.set_actionbar)、[`Player::clear_title`](../rust/player.md#Player.clear_title)
    - Go：[`Player.SendTitle`](../go/player.md#Player.SendTitle)、[`Raw.PlayerSendTitle`](../go/raw.md#Raw.PlayerSendTitle)

### `player_conn_id` {#player_conn_id}

```c
uint64_t (*player_conn_id)(PierPlayerSel who);
```

这名玩家的连接 id，和数据包拦截器在数据包上下文里看到的是同一个数。

数据包回调手里只有 `conn_id`，没有玩家，所以没有这个槽位就没法按玩家改写发出的数据包。比如锁定天空的颜色，需要知道这个连接上的人在哪个维度，也就需要知道这个人是谁。

另一种做法是定时发包覆盖服务器发出的包，这样做是错的：服务器发的是真实时间，模组发的是锁定的时间，两种包交错到达，客户端的天空就在两者之间闪烁。正确的做法是改写，而改写需要这个槽位。

返回连接 id；玩家不在线，或者拿不到它的网络标识时返回 0。

纯追加：`PIER_ABI_VERSION` 不变，由 `struct_size` 把关。

- 调用形式：`api->player_conn_id(who)`
- 参数：
    - who : `PierPlayerSel`
- 返回值类型：`uint64_t`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 180 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::conn_id`](../rust/player.md#Player.conn_id)
    - Go：[`Player.ConnID`](../go/player.md#Player.ConnID)、[`Raw.PlayerConnId`](../go/raw.md#Raw.PlayerConnId)

## `PierPlayerNumProp` {#PierPlayerNumProp}

`player_get_num` / `player_set_num` 的键。(G) 表示只能读，(S) 表示可以写。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_PPROP_GAME_TYPE"></span>`PIER_PPROP_GAME_TYPE` | `0` | (G) `Player::getPlayerGameType`；写入用 `player_set_gamemode` |
| <span id="PIER_PPROP_LEVEL"></span>`PIER_PPROP_LEVEL` | `1` | (S) 属性 `Player::LEVEL()` |
| <span id="PIER_PPROP_EXPERIENCE"></span>`PIER_PPROP_EXPERIENCE` | `2` | (S) 属性 `Player::EXPERIENCE()`（经验条进度，0 到 1） |
| <span id="PIER_PPROP_HUNGER"></span>`PIER_PPROP_HUNGER` | `3` | (S) 属性 `Player::HUNGER()` |
| <span id="PIER_PPROP_SATURATION"></span>`PIER_PPROP_SATURATION` | `4` | (S) 属性 `Player::SATURATION()` |
| <span id="PIER_PPROP_EXHAUSTION"></span>`PIER_PPROP_EXHAUSTION` | `5` | (S) 属性 `Player::EXHAUSTION()` |
| <span id="PIER_PPROP_XP_NEEDED_NEXT_LEVEL"></span>`PIER_PPROP_XP_NEEDED_NEXT_LEVEL` | `6` | (G) `Player::getXpNeededForNextLevel` |
| <span id="PIER_PPROP_LUCK"></span>`PIER_PPROP_LUCK` | `7` | (G) `Player::getLuck` |
| <span id="PIER_PPROP_SELECTED_SLOT"></span>`PIER_PPROP_SELECTED_SLOT` | `8` | (G) `Player::getSelectedItemSlot`；设置用 `PIER_PACT_SET_SELECTED_SLOT` |
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
| <span id="PIER_PPROP_CAN_USE_ABILITY"></span>`PIER_PPROP_CAN_USE_ABILITY` | `20` | (G) `Player::canUseAbility`；能力编号要经 `player_action` 的读取路径传入，见 `PIER_PACT_CAN_USE_ABILITY` |
| <span id="PIER_PPROP_DIRECTION"></span>`PIER_PPROP_DIRECTION` | `21` | (G) `Player::getDirection`（0=南，1=西，2=北，3=东） |
| <span id="PIER_PPROP_CHUNK_RADIUS"></span>`PIER_PPROP_CHUNK_RADIUS` | `22` | (G) `Player::getChunkRadius` |
| <span id="PIER_PPROP_NETWORK_RTT"></span>`PIER_PPROP_NETWORK_RTT` | `23` | (G) `getNetworkStatus().mPing`（毫秒） |
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

`player_get_str` 的键。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_PSTR_REAL_NAME"></span>`PIER_PSTR_REAL_NAME` | `0` | 取自 `Player::getRealName` |
| <span id="PIER_PSTR_UUID"></span>`PIER_PSTR_UUID` | `1` | 取自 `Player::getUuid().asString()` |
| <span id="PIER_PSTR_XUID"></span>`PIER_PSTR_XUID` | `2` | 取自 `Player::getXuid` |
| <span id="PIER_PSTR_IP_AND_PORT"></span>`PIER_PSTR_IP_AND_PORT` | `3` | 取自 `Player::getIPAndPort` |
| <span id="PIER_PSTR_LOCALE_CODE"></span>`PIER_PSTR_LOCALE_CODE` | `4` | 取自 `Player::getLocaleCode` |
| <span id="PIER_PSTR_NAME_TAG"></span>`PIER_PSTR_NAME_TAG` | `5` | 取自 `Actor::getNameTag`（显示名） |
| <span id="PIER_PSTR_LAST_DEATH_POS"></span>`PIER_PSTR_LAST_DEATH_POS` | `6` | SNBT `{x,y,z}`；没有时为 `""` |
| <span id="PIER_PSTR_LAST_DEATH_DIMENSION"></span>`PIER_PSTR_LAST_DEATH_DIMENSION` | `7` | 维度 id，以字符串表示 |
| <span id="PIER_PSTR_NETWORK_STATUS"></span>`PIER_PSTR_NETWORK_STATUS` | `8` | SNBT `{ping,avg_ping,packet_loss,max_ping}` |
| <span id="PIER_PSTR_PLATFORM_ONLINE_ID"></span>`PIER_PSTR_PLATFORM_ONLINE_ID` | `9` | 取自 `Player::getPlatformOnlineId` |
| <span id="PIER_PSTR_RESPAWN_POS"></span>`PIER_PSTR_RESPAWN_POS` | `10` | SNBT `{x,y,z,dim}`：这名玩家会在哪里重生。不会为空：没有床的玩家报告的是世界出生点，这两种情况区分不开。 |

## `PierPlayerAction` {#PierPlayerAction}

`player_action` 的动作。参数是 (sarg, a, b, c)，用不到的参数会被忽略。注明了有结果的动作，`out`（不为 NULL 时）会收到一个结果字符串。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_PACT_SET_ABILITY"></span>`PIER_PACT_SET_ABILITY` | `0` | `a` 为 `AbilitiesIndex`，`b` 为 0 或 1（布尔类的能力）或者浮点数（`FlySpeed` 等）。写完以后会把 `PlayerPermissionLevel` 恢复成写之前的值：引擎的 `LayeredAbilities::setAbility` 走的是「切换到自定义权限」那条路，会把玩家推到 Custom，而这个等级会和能力层一起放进 `UpdateAbilitiesPacket` 发给客户端。要改权限等级，用 `PIER_PACT_SET_PERMISSION_LEVEL`。玩家加入完成之前（`Player::isPlayerInitialized`）调用会被拒绝，返回 false：客户端还在加载时写入的能力会一直不同步，直到玩家重新进入。模拟玩家不受这个限制。 |
| <span id="PIER_PACT_CAN_USE_ABILITY"></span>`PIER_PACT_CAN_USE_ABILITY` | `1` | `a` 为 `AbilitiesIndex`，输出 `"0"` 或 `"1"`，调用 `Player::canUseAbility` |
| <span id="PIER_PACT_SET_SELECTED_SLOT"></span>`PIER_PACT_SET_SELECTED_SLOT` | `2` | `a` 为槽位，调用 `Player::setSelectedSlot` |
| <span id="PIER_PACT_GIVE_ITEM"></span>`PIER_PACT_GIVE_ITEM` | `3` | `sarg` 为物品 SNBT，经 `ItemStack::fromTag` 和 `Player::addAndRefresh` 给出 |
| <span id="PIER_PACT_SET_SPAWN_POINT"></span>`PIER_PACT_SET_SPAWN_POINT` | `4` | `a`、`b`、`c` 为坐标，`sarg` 为维度 id（任何已注册的维度都可以）；原生调用 `Player::setRespawnPosition` |
| <span id="PIER_PACT_CLEAR_TITLE"></span>`PIER_PACT_CLEAR_TITLE` | `5` | 原生发送 `SetTitlePacket(Clear)` |
| <span id="PIER_PACT_SET_TITLE"></span>`PIER_PACT_SET_TITLE` | `6` | `sarg` 为文本，`a` 为位置（0 主标题，1 副标题，2 动作栏）；原生发送 `SetTitlePacket`，文本原样发送 |
| <span id="PIER_PACT_ADD_EXPERIENCE"></span>`PIER_PACT_ADD_EXPERIENCE` | `7` | `a` 为经验值，调用 `Player::addExperience` |
| <span id="PIER_PACT_ADD_LEVELS"></span>`PIER_PACT_ADD_LEVELS` | `8` | `a` 为等级数，调用 `Player::addLevels` |
| <span id="PIER_PACT_START_COOLDOWN"></span>`PIER_PACT_START_COOLDOWN` | `9` | `sarg` 为物品名，`a` 为刻数，调用 `Player::startItemCooldown` |
| <span id="PIER_PACT_START_RIDING"></span>`PIER_PACT_START_RIDING` | `10` | `a` 为坐骑的 `ActorUniqueID`（低 64 位），调用 `Player::startRiding` |
| <span id="PIER_PACT_STOP_RIDING"></span>`PIER_PACT_STOP_RIDING` | `11` | 调用 `Player::stopRiding` |
| <span id="PIER_PACT_ATTACK"></span>`PIER_PACT_ATTACK` | `12` | `a` 为目标的 `ActorUniqueID`（低 64 位），调用 `Player::attack` |
| <span id="PIER_PACT_DROP"></span>`PIER_PACT_DROP` | `13` | `sarg` 为物品 SNBT，`a` 为是否随机抛出（0 或 1），调用 `Player::drop` |
| <span id="PIER_PACT_INTERACT"></span>`PIER_PACT_INTERACT` | `14` | `a` 为目标的 `ActorUniqueID`，调用 `Player::interact` |
| <span id="PIER_PACT_START_USING_ITEM"></span>`PIER_PACT_START_USING_ITEM` | `15` | `sarg` 为物品 SNBT，`a` 为持续时间，调用 `Player::startUsingItem` |
| <span id="PIER_PACT_STOP_USING_ITEM"></span>`PIER_PACT_STOP_USING_ITEM` | `16` | 调用 `Player::stopUsingItem` |
| <span id="PIER_PACT_SET_CHUNK_RADIUS"></span>`PIER_PACT_SET_CHUNK_RADIUS` | `17` | `a` 为半径，调用 `Player::setChunkRadius` |
| <span id="PIER_PACT_SET_ENCHANTMENT_SEED"></span>`PIER_PACT_SET_ENCHANTMENT_SEED` | `18` | `a` 为种子，调用 `Player::setEnchantmentSeed` |
| <span id="PIER_PACT_REGISTER_TRACKED_BOSS"></span>`PIER_PACT_REGISTER_TRACKED_BOSS` | `19` | `a` 为 Boss 的 `ActorUniqueID`，调用 `Player::registerTrackedBoss` |
| <span id="PIER_PACT_UNREGISTER_TRACKED_BOSS"></span>`PIER_PACT_UNREGISTER_TRACKED_BOSS` | `20` | `a` 为 Boss 的 `ActorUniqueID`，调用 `Player::unRegisterTrackedBoss` |
| <span id="PIER_PACT_PLAY_EMOTE"></span>`PIER_PACT_PLAY_EMOTE` | `21` | `sarg` 为表情的 id，调用 `Player::playEmote` |
| <span id="PIER_PACT_RESEND_ALL_CHUNKS"></span>`PIER_PACT_RESEND_ALL_CHUNKS` | `22` | 调用 `Player::resendAllChunks` |
| <span id="PIER_PACT_OPEN_INVENTORY"></span>`PIER_PACT_OPEN_INVENTORY` | `23` | 调用 `Player::openInventory` |
| <span id="PIER_PACT_SIDEBAR_SET"></span>`PIER_PACT_SIDEBAR_SET` | `24` | `sarg` 为 `"obj\ntitle\nline…"`，只对这名玩家显示的侧边栏 |
| <span id="PIER_PACT_SIDEBAR_CLEAR"></span>`PIER_PACT_SIDEBAR_CLEAR` | `25` | `sarg` 为计分项，发送 `RemoveObjectivePacket` |
| <span id="PIER_PACT_SET_PERMISSION_LEVEL"></span>`PIER_PACT_SET_PERMISSION_LEVEL` | `26` | `a` 为 `PlayerPermissionLevel`（0 Visitor，1 Member，2 Operator，3 Custom）。调用 `LayeredAbilities::setPlayerPermissions` 并发送 `UpdateAbilitiesPacket`。读取一侧是 `PIER_PPROP_PERMISSION_LEVEL`。和 `PIER_PACT_SET_ABILITY` 一样，玩家加入完成之前调用会被拒绝。 |
