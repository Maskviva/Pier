# 世界

??? note "abi.h 里的分节说明"

    **追加（`Appended`）**

    **§A 世界的读写与时钟（`§A world read/write & clock`）**

    **§C 实体（玩家也可以经 `player_resolve` 解析到这里）（`§C actors (players resolve here too, via player_resolve)`）**

    **§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）**

    **关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）**

    **同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）**

    追加了五个槽位，没有改动 `PIER_ABI_VERSION`：单纯的追加不算版本变化，`struct_size` 才是准确的关卡。

    两个方向都成立。新加载器运行旧模组：旧表是新表逐字节相同的前缀，模组够不着这五个槽位，照常工作。新模组配旧加载器：SDK 运行时初始化时比较 `struct_size`，发现加载器的表比它编译时用的短，于是拒绝加载。这是正确的结果，因为在没有 `lane_publish` 的加载器上读那个单元会越界。

    简单地说：版本号记录「语义变了」，`struct_size` 记录「表变长了」。这次改动只属于后者。

    见上面 `PierLaneDesc` 处的长注释。一句话概括：服务是跨语言的 (名字, JSON) -> JSON 通道；快速通道直接调用函数表，只在两边由同一个工具链构建时成立；指纹对不上时一个指针都不交出，使用方退回到服务通道。

    只能在服务器线程调用。

    **追加的槽位。只加在末尾，由 `struct_size` 把关（`Appended slots. Added at the tail only, guarded by struct_size.`）**

    移除实体和治疗实体已经由 `actor_action` 的 `AACT_DESPAWN` 和 `AACT_HEAL` 提供。再单独加一个槽位就是同一件事做两遍，两份实现时间一长就会出现差异。

    列出实体已经有 `list_actors`（一个维度里的全部实体，带类型名）；配合 `actor_get_num` 读取坐标，就能筛出一个长方体里的实体。再加一个 `actors_in_box` 槽位也是同一件事做两遍，两份实现时间一长就会出现差异。

    **追加：液体层（含水方块）（`Appended: the liquid layer (waterlogged blocks).`）**

    基岩版的含水用的是同一格里的第二个方块，没有对应的方块状态：主层放楼梯、栅栏或珊瑚，液体层放水。`get_block` 和 `set_block` 只看得到主层，所以复制、粘贴含水楼梯会丢掉所有的水：主层完全正确，另一层没了。

    这两个槽位用来读写液体层。空的液体层读出来是 `"minecraft:air"`。

    **追加：批量读写方块（`Appended: bulk block reads and writes`）**

## 槽位 {#slots}

### `spawn_particle` {#spawn_particle}

```c
bool (*spawn_particle)(int32_t dimension, PierStr effect_name, double x, double y, double z);
```

在一个世界坐标上生成粒子效果，可以用来逐条边地勾出选区的轮廓。只能在服务器线程调用。世界或维度还没就绪时返回 false。

- `dimension`：0 为主世界，1 为下界，2 为末地。
- `effect_name`：例如 `"minecraft:basic_flame_particle"` 或 `"minecraft:redstone_wire_dust_particle"`。

- 调用形式：`api->spawn_particle(dimension, effect_name, x, y, z)`
- 参数：
    - dimension : `int32_t`
    - effect_name : `PierStr`
    - x : `double`
    - y : `double`
    - z : `double`
- 返回值类型：`bool`
- 所在分节：追加（`Appended`）
- 表内序号：第 13 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::spawn_particle`](../rust/world.md#World.spawn_particle)
    - Go：[`SpawnParticle`](../go/world.md#SpawnParticle)、[`Raw.SpawnParticle`](../go/raw.md#Raw.SpawnParticle)

### `scan_region` {#scan_region}

```c
bool (*scan_region)(
    int32_t dimension,
    int32_t x1,
    int32_t y1,
    int32_t z1,
    int32_t x2,
    int32_t y2,
    int32_t z2,
    void* ctx,
    PierBlockSink blocks_sink,
    PierEntitySink entities_sink
);
```

扫描一个长方体区域，两个角都包含在内，顺序不限。长方体里的每一格调用一次 `blocks_sink`，传入方块名和完整的 SNBT；位置落在长方体里的每个实体调用一次 `entities_sink`，传入它所在的格子和它的 SNBT。两个回调都在这次调用里同步执行，调用结束后什么都不保留。只能在服务器线程调用。世界或维度还没就绪时返回 false。

- 调用形式：`api->scan_region(dimension, x1, y1, z1, x2, y2, z2, ctx, blocks_sink, entities_sink)`
- 参数：
    - dimension : `int32_t`
    - x1 : `int32_t`
    - y1 : `int32_t`
    - z1 : `int32_t`
    - x2 : `int32_t`
    - y2 : `int32_t`
    - z2 : `int32_t`
    - ctx : `void*`
    - blocks_sink : `PierBlockSink`
    - entities_sink : `PierEntitySink`
- 返回值类型：`bool`
- 所在分节：追加（`Appended`）
- 表内序号：第 15 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::scan`](../rust/world.md#World.scan)、[`World::scan_with`](../rust/world.md#World.scan_with)
    - Go：[`ScanRegion`](../go/world.md#ScanRegion)

### `get_block` {#get_block}

```c
bool (*get_block)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierBlockSink sink);
```

读取一个方块：输出回调被调用一次，传入 (x,y,z, 类型名, 完整的 SNBT)。

- 调用形式：`api->get_block(dim, x, y, z, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierBlockSink`
- 返回值类型：`bool`
- 所在分节：§A 世界的读写与时钟（`§A world read/write & clock`）
- 表内序号：第 16 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::read`](../rust/block.md#Block.read)
    - Go：[`GetBlock`](../go/world.md#GetBlock)、[`BlockAt.Info`](../go/block.md#BlockAt.Info)

### `set_block` {#set_block}

```c
bool (*set_block)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr block_spec);
```

原生放置方块（`BlockSource::setBlock`，默认的更新标志）。`block_spec` 可以是 `"minecraft:stone"` 或 `"stone"`（使用默认状态），也可以是完整的 `{name,states,...}` SNBT。名字认不出来时调用失败，不会放一个占位方块。

- 调用形式：`api->set_block(dim, x, y, z, block_spec)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - block_spec : `PierStr`
- 返回值类型：`bool`
- 所在分节：§A 世界的读写与时钟（`§A world read/write & clock`）
- 表内序号：第 17 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::set`](../rust/block.md#Block.set)
    - Go：[`BlockAt.Set`](../go/block.md#BlockAt.Set)、[`Raw.SetBlock`](../go/raw.md#Raw.SetBlock)

### `get_time` {#get_time}

```c
bool (*get_time)(int64_t* out);
```

世界时间（`Level::getTime`）。

- 调用形式：`api->get_time(out)`
- 参数：
    - out : `int64_t*`
- 返回值类型：`bool`
- 所在分节：§A 世界的读写与时钟（`§A world read/write & clock`）
- 表内序号：第 18 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::time`](../rust/world.md#World.time)
    - Go：[`Time`](../go/world.md#Time)、[`Raw.GetTime`](../go/raw.md#Raw.GetTime)

### `set_time` {#set_time}

```c
bool (*set_time)(int64_t t);
```

原生设置世界时间（`Level::setTime`）。

- 调用形式：`api->set_time(t)`
- 参数：
    - t : `int64_t`
- 返回值类型：`bool`
- 所在分节：§A 世界的读写与时钟（`§A world read/write & clock`）
- 表内序号：第 19 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::set_time`](../rust/world.md#World.set_time)
    - Go：[`SetTime`](../go/world.md#SetTime)、[`Raw.SetTime`](../go/raw.md#Raw.SetTime)

### `set_weather` {#set_weather}

```c
bool (*set_weather)(int32_t weather);
```

0=晴，1=雨，2=雷雨，原生调用（`Level::updateWeather`）。

- 调用形式：`api->set_weather(weather)`
- 参数：
    - weather : `int32_t`
- 返回值类型：`bool`
- 所在分节：§A 世界的读写与时钟（`§A world read/write & clock`）
- 表内序号：第 20 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::set_weather`](../rust/world.md#World.set_weather)
    - Go：[`SetWeather`](../go/world.md#SetWeather)、[`Raw.SetWeather`](../go/raw.md#Raw.SetWeather)

### `explode` {#explode}

```c
bool (*explode)(
    int32_t dim,
    double x,
    double y,
    double z,
    float radius,
    float max_resistance,
    PierActorId source,
    bool fire,
    bool breaks_blocks,
    bool allow_underwater
);
```

调用 `Level::explode`。`source` 可以为 0，表示没有来源实体。

- 调用形式：`api->explode(dim, x, y, z, radius, max_resistance, source, fire, breaks_blocks, allow_underwater)`
- 参数：
    - dim : `int32_t`
    - x : `double`
    - y : `double`
    - z : `double`
    - radius : `float`
    - max_resistance : `float`
    - source : `PierActorId`
    - fire : `bool`
    - breaks_blocks : `bool`
    - allow_underwater : `bool`
- 返回值类型：`bool`
- 所在分节：§C 实体（玩家也可以经 `player_resolve` 解析到这里）（`§C actors (players resolve here too, via player_resolve)`）
- 表内序号：第 38 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::explode`](../rust/world.md#World.explode)
    - Go：[`Explode`](../go/world.md#Explode)、[`Raw.Explode`](../go/raw.md#Raw.Explode)

### `get_difficulty` {#get_difficulty}

```c
bool (*get_difficulty)(int32_t* out);
```

!!! note "分组说明"

    服务器和世界级别的设置。

取自 `Level::getDifficulty`

- 调用形式：`api->get_difficulty(out)`
- 参数：
    - out : `int32_t*`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 72 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::difficulty`](../rust/world.md#World.difficulty)
    - Go：[`Difficulty`](../go/world.md#Difficulty)、[`Raw.GetDifficulty`](../go/raw.md#Raw.GetDifficulty)

### `set_difficulty` {#set_difficulty}

```c
bool (*set_difficulty)(int32_t d);
```

!!! note "分组说明"

    服务器和世界级别的设置。

原生调用 `Level::setDifficulty`

- 调用形式：`api->set_difficulty(d)`
- 参数：
    - d : `int32_t`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 73 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::set_difficulty`](../rust/world.md#World.set_difficulty)
    - Go：[`SetDifficulty`](../go/world.md#SetDifficulty)、[`Raw.SetDifficulty`](../go/raw.md#Raw.SetDifficulty)

### `get_seed` {#get_seed}

```c
bool (*get_seed)(int64_t* out);
```

!!! note "分组说明"

    服务器和世界级别的设置。

取自 `Level::getLevelSeed64`

- 调用形式：`api->get_seed(out)`
- 参数：
    - out : `int64_t*`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 74 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::seed`](../rust/world.md#World.seed)
    - Go：[`Seed`](../go/world.md#Seed)、[`Raw.GetSeed`](../go/raw.md#Raw.GetSeed)

### `game_rule_get` {#game_rule_get}

```c
bool (*game_rule_get)(PierStr name, void* ctx, PierStrSink sink);
```

!!! note "分组说明"

    服务器和世界级别的设置。

输出回调收到 SNBT `{type:"bool"|"int"|"float", value:…}`；规则不存在时返回 false。

- 调用形式：`api->game_rule_get(name, ctx, sink)`
- 参数：
    - name : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 75 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::game_rule`](../rust/world.md#World.game_rule)
    - Go：[`GameRule`](../go/world.md#GameRule)、[`Raw.GameRuleGet`](../go/raw.md#Raw.GameRuleGet)

### `game_rule_set` {#game_rule_set}

```c
bool (*game_rule_set)(PierStr name, PierStr value);
```

!!! note "分组说明"

    服务器和世界级别的设置。

等同于 `/gamerule`

- 调用形式：`api->game_rule_set(name, value)`
- 参数：
    - name : `PierStr`
    - value : `PierStr`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 76 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::set_game_rule`](../rust/world.md#World.set_game_rule)
    - Go：[`SetGameRule`](../go/world.md#SetGameRule)、[`Raw.GameRuleSet`](../go/raw.md#Raw.GameRuleSet)

### `spawn_particle_for` {#spawn_particle_for}

```c
bool (*spawn_particle_for)(
    PierPlayerSel sel, int32_t dimension, PierStr effect_name, double x, double y, double z);
```

!!! note "分组说明"

    只给一名玩家发送粒子包（追加的槽位，受 `struct_size` 约束）。`SpawnParticleEffectPacket` **只**发给解析出来的那名玩家（`Player::sendNetworkPacket`）；`Level::spawnParticleEffect` 会向整个维度广播，这里其他客户端收不到这个包。`dimension` 是包里带的原版维度 id，传坐标所在的维度，通常就是这名玩家所在的维度，因为客户端不渲染别的维度的粒子。玩家不在线或解析不到时返回 false。

- 调用形式：`api->spawn_particle_for(sel, dimension, effect_name, x, y, z)`
- 参数：
    - sel : `PierPlayerSel`
    - dimension : `int32_t`
    - effect_name : `PierStr`
    - x : `double`
    - y : `double`
    - z : `double`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 78 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Player::spawn_particle`](../rust/player.md#Player.spawn_particle)
    - Go：[`Raw.SpawnParticleFor`](../go/raw.md#Raw.SpawnParticleFor)

### `villages` {#villages}

```c
void (*villages)(int32_t dimension, void* ctx, PierStrSink snbt_sink);
```

!!! note "分组说明"

    只读的世界数据查询（追加的槽位，受 `struct_size` 约束）。两个槽位都通过输出回调逐个给出结果，每个结果一个 SNBT 对象；只观察，不做修改。只能在服务器线程调用。

列出一个维度里的村庄。每一项是 `{uuid, center:[x,y,z], bounds:{min,max}, poi_count}`。

- 调用形式：`api->villages(dimension, ctx, snbt_sink)`
- 参数：
    - dimension : `int32_t`
    - ctx : `void*`
    - snbt_sink : `PierStrSink`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 89 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::try_villages`](../rust/world.md#World.try_villages)、[`World::villages`](../rust/world.md#World.villages)
    - Go：[`Raw.Villages`](../go/raw.md#Raw.Villages)

### `structures_near` {#structures_near}

```c
void (*structures_near)(
    int32_t dimension, int32_t x, int32_t y, int32_t z, int32_t radius, void* ctx,
    PierStrSink snbt_sink);
```

!!! note "分组说明"

    只读的世界数据查询（追加的槽位，受 `struct_size` 约束）。两个槽位都通过输出回调逐个给出结果，每个结果一个 SNBT 对象；只观察，不做修改。只能在服务器线程调用。

以 (x,y,z) 为中心的一个半径内，所在区块与之相交的硬编码生成区：下界要塞、女巫小屋、海底神殿、掠夺者前哨站。每一项是 `{type, bounds:{min,max}}`。只检查**已加载**的区块，这个只读查询从不强制加载区块。

- 调用形式：`api->structures_near(dimension, x, y, z, radius, ctx, snbt_sink)`
- 参数：
    - dimension : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - radius : `int32_t`
    - ctx : `void*`
    - snbt_sink : `PierStrSink`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 90 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::try_structures_near`](../rust/world.md#World.try_structures_near)、[`World::structures_near`](../rust/world.md#World.structures_near)
    - Go：[`Raw.StructuresNear`](../go/raw.md#Raw.StructuresNear)

### `level_get_biome` {#level_get_biome}

```c
bool (*level_get_biome)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
```

- 调用形式：`api->level_get_biome(dim, x, y, z, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）
- 表内序号：第 129 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::biome`](../rust/world.md#World.biome)
    - Go：[`BlockAt.Biome`](../go/block.md#BlockAt.Biome)、[`Raw.LevelGetBiome`](../go/raw.md#Raw.LevelGetBiome)

### `level_get_default_spawn` {#level_get_default_spawn}

```c
bool (*level_get_default_spawn)(int32_t* x, int32_t* y, int32_t* z);
```

- 调用形式：`api->level_get_default_spawn(x, y, z)`
- 参数：
    - x : `int32_t*`
    - y : `int32_t*`
    - z : `int32_t*`
- 返回值类型：`bool`
- 所在分节：关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）
- 表内序号：第 130 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::default_spawn`](../rust/world.md#World.default_spawn)
    - Go：[`DefaultSpawn`](../go/world.md#DefaultSpawn)、[`Raw.LevelGetDefaultSpawn`](../go/raw.md#Raw.LevelGetDefaultSpawn)

### `level_set_default_spawn` {#level_set_default_spawn}

```c
bool (*level_set_default_spawn)(int32_t x, int32_t y, int32_t z);
```

- 调用形式：`api->level_set_default_spawn(x, y, z)`
- 参数：
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
- 返回值类型：`bool`
- 所在分节：关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）
- 表内序号：第 131 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::set_default_spawn`](../rust/world.md#World.set_default_spawn)
    - Go：[`SetDefaultSpawn`](../go/world.md#SetDefaultSpawn)、[`Raw.LevelSetDefaultSpawn`](../go/raw.md#Raw.LevelSetDefaultSpawn)

### `level_save` {#level_save}

```c
bool (*level_save)(void);
```

- 调用形式：`api->level_save()`
- 返回值类型：`bool`
- 所在分节：关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）
- 表内序号：第 132 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::save`](../rust/world.md#World.save)
    - Go：[`SaveLevel`](../go/world.md#SaveLevel)、[`Raw.LevelSave`](../go/raw.md#Raw.LevelSave)

### `level_get_sleep_status` {#level_get_sleep_status}

```c
bool (*level_get_sleep_status)(void* ctx, PierStrSink sink);
```

SNBT `{sleeping, total_players, active_sleeping}`

- 调用形式：`api->level_get_sleep_status(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）
- 表内序号：第 133 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::sleep_status`](../rust/world.md#World.sleep_status)
    - Go：[`Raw.LevelGetSleepStatus`](../go/raw.md#Raw.LevelGetSleepStatus)

### `level_update_weather` {#level_update_weather}

```c
bool (*level_update_weather)(float rain_level, int32_t rain_time, float lightning_level, int32_t lightning_time);
```

- 调用形式：`api->level_update_weather(rain_level, rain_time, lightning_level, lightning_time)`
- 参数：
    - rain_level : `float`
    - rain_time : `int32_t`
    - lightning_level : `float`
    - lightning_time : `int32_t`
- 返回值类型：`bool`
- 所在分节：关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）
- 表内序号：第 134 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::update_weather`](../rust/world.md#World.update_weather)
    - Go：[`Raw.LevelUpdateWeather`](../go/raw.md#Raw.LevelUpdateWeather)

### `level_find_path` {#level_find_path}

```c
bool (*level_find_path)(PierActorId id, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
```

SNBT `{nodes:[{x,y,z},…], reached:1b/0b}`。当前所有宿主上这个槽位都是 NULL：寻路还没有实现。

- 调用形式：`api->level_find_path(id, x, y, z, ctx, sink)`
- 参数：
    - id : `PierActorId`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：关卡：生物群系、出生点、保存、天气、寻路、睡眠（专用函数）（`Level: biome, spawn, save, weather, path, sleep (dedicated fns)`）
- 表内序号：第 135 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::find_path`](../rust/world.md#World.find_path)
    - Go：[`Raw.LevelFindPath`](../go/raw.md#Raw.LevelFindPath)

### `level_delete_chunk_keys` {#level_delete_chunk_keys}

```c
int32_t (*level_delete_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z);
```

删除属于一个区块的所有存档键，下次加载时引擎会用生成器重新生成这个区块。

用 `set_block` 把每一格重写一遍来恢复一片区域，这条路走不通：一块 32x32 的地皮乘以世界高度就是几十万格，也就是几十万次跨 FFI 的调用；而且还会漏掉东西，因为方块实体、实体和待处理的刻（红石、作物生长）都不属于方块数据。这样重写之后，箱子还在，红石也还在运行。

删除存档键没有这两个问题：一次 `forEachKeyWithPrefix` 就能列出这个区块的所有键（所有标签、所有子区块、实体、方块实体、待处理的刻），一遍删完，下次加载时引擎用生成器重新生成。

键的形状：BDS 的区块键以 `<chunkX:i32 LE><chunkZ:i32 LE>` 开头，主世界以外再跟一个 `<dimension:i32 LE>`。前缀之后是一个标签字节和子区块编号，这个槽位不解读它们；删除带这个前缀的所有键，正好就是「这个区块里的一切」。

区块必须处于未加载的状态。已加载的区块在内存里有一个 `LevelChunk`，引擎卸载它时会写回存档，把删掉的键原样重建，删除就这样被悄悄撤销了。让区块先卸载是调用方的责任（把玩家移开，等它离开刻的范围）。这个槽位不做这件事：判断谁在附近、什么时候卸载可以接受，需要调用方的领域知识，这一层不应该掌握这些。

返回删除的键的数量；存档层不可用时返回 -1。返回 0 是正常结果，表示这个区块从来没有生成过。

纯追加：`PIER_ABI_VERSION` 不变，由 `struct_size` 把关。

- 调用形式：`api->level_delete_chunk_keys(dim, chunk_x, chunk_z)`
- 参数：
    - dim : `int32_t`
    - chunk_x : `int32_t`
    - chunk_z : `int32_t`
- 返回值类型：`int32_t`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 178 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::delete_chunk_keys`](../rust/world.md#World.delete_chunk_keys)
    - Go：[`Raw.LevelDeleteChunkKeys`](../go/raw.md#Raw.LevelDeleteChunkKeys)

### `level_chunks_loaded` {#level_chunks_loaded}

```c
int32_t (*level_chunks_loaded)(int32_t dim, int32_t min_x, int32_t min_z, int32_t max_x, int32_t max_z);
```

覆盖 [min..max] 的区块当前是否都已加载到内存里。

这是 `level_delete_chunk_keys` 的配套槽位：删除存档键只对未加载的区块有效。已加载的区块活在内存里，卸载时会把删掉的键原样写回，而删除本身显示「成功」，还报告了一个正数的键数。没有这个槽位，调用方只能凭「附近没人」去猜，猜错了也不会有任何提示。

返回 1 表示全部已加载，0 表示至少有一个没加载，维度不可用时返回 -1。

纯追加：`PIER_ABI_VERSION` 不变，由 `struct_size` 把关。

- 调用形式：`api->level_chunks_loaded(dim, min_x, min_z, max_x, max_z)`
- 参数：
    - dim : `int32_t`
    - min_x : `int32_t`
    - min_z : `int32_t`
    - max_x : `int32_t`
    - max_z : `int32_t`
- 返回值类型：`int32_t`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 179 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::chunks_loaded`](../rust/world.md#World.chunks_loaded)
    - Go：[`Raw.LevelChunksLoaded`](../go/raw.md#Raw.LevelChunksLoaded)

### `level_chunk_keys` {#level_chunk_keys}

```c
int32_t (*level_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z, void* ctx, PierStrSink sink);
```

列出属于一个区块的所有存档键，每个键调用一次回调。

列出和删除分成两个槽位：宿主在一次调用里不积累任何存放键的容器，调用方留下它要的键，再逐个通过 `level_delete_key` 删除。在这里，宿主一侧跨越引擎的虚函数调用持有一个字符串容器，正是会破坏堆的那种写法。

键是二进制的，里面有 0 字节，所以用带显式长度的 `PierStr`，不用 C 字符串。

返回报告的键的数量；存档层不可用时返回 -1。

- 调用形式：`api->level_chunk_keys(dim, chunk_x, chunk_z, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - chunk_x : `int32_t`
    - chunk_z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`int32_t`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 181 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::chunk_keys`](../rust/world.md#World.chunk_keys)
    - Go：[`Raw.LevelChunkKeys`](../go/raw.md#Raw.LevelChunkKeys)

### `level_delete_key` {#level_delete_key}

```c
bool (*level_delete_key)(PierStr key);
```

原样删除一个区块类的键。

这是 `level_chunk_keys` 的配套槽位。键的内容不会被解读：传进来什么就删除什么。它安全也正是因为这一点，它不需要理解子区块的格式。

- 调用形式：`api->level_delete_key(key)`
- 参数：
    - key : `PierStr`
- 返回值类型：`bool`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 182 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::delete_key`](../rust/world.md#World.delete_key)
    - Go：[`Raw.LevelDeleteKey`](../go/raw.md#Raw.LevelDeleteKey)

### `level_set_biome` {#level_set_biome}

```c
int32_t (*level_set_biome)(int32_t dim,
                           int32_t minX, int32_t minZ,
                           int32_t maxX, int32_t maxZ,
                           PierStr biome);
```

设置一片区域的生物群系。

按整列设置，所以不接收 y：`setBiome3d` 按 y 设置，但基岩版按列存储生物群系。`biome` 是生物群系名，例如 `"minecraft:plains"`。

返回设置了多少列。返回 0 表示一列都没设置，原因可能是区块没有加载，也可能是名字认不出来。

- 调用形式：`api->level_set_biome(dim, minX, minZ, maxX, maxZ, biome)`
- 参数：
    - dim : `int32_t`
    - minX : `int32_t`
    - minZ : `int32_t`
    - maxX : `int32_t`
    - maxZ : `int32_t`
    - biome : `PierStr`
- 返回值类型：`int32_t`
- 所在分节：追加的槽位。只加在末尾，由 `struct_size` 把关（`Appended slots. Added at the tail only, guarded by struct_size.`）
- 表内序号：第 183 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::set_biome`](../rust/world.md#World.set_biome)
    - Go：[`SetBiome`](../go/world.md#SetBiome)、[`Raw.LevelSetBiome`](../go/raw.md#Raw.LevelSetBiome)

### `get_extra_block` {#get_extra_block}

```c
bool (*get_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                        void* ctx, PierStrSink sink);
```

读取液体层。输出回调收到一个方块名，例如 `"minecraft:water"`；这一层为空时给出空气。

- 调用形式：`api->get_extra_block(dim, x, y, z, ctx, sink)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：追加：液体层（含水方块）（`Appended: the liquid layer (waterlogged blocks).`）
- 表内序号：第 184 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::extra`](../rust/block.md#Block.extra)
    - Go：[`Raw.GetExtraBlock`](../go/raw.md#Raw.GetExtraBlock)

### `set_extra_block` {#set_extra_block}

```c
bool (*set_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                        PierStr block_spec, int32_t update_flags);
```

写入液体层。`block_spec` 可以是单纯的方块名，也可以是完整的 SNBT；写入 `"minecraft:air"` 就是清空。`update_flags` 的含义和 `edit_set_block_nbt` 一样：第 1 位通知相邻方块，第 2 位同步给客户端。

- 调用形式：`api->set_extra_block(dim, x, y, z, block_spec, update_flags)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - block_spec : `PierStr`
    - update_flags : `int32_t`
- 返回值类型：`bool`
- 所在分节：追加：液体层（含水方块）（`Appended: the liquid layer (waterlogged blocks).`）
- 表内序号：第 185 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::set_extra`](../rust/block.md#Block.set_extra)
    - Go：[`Raw.SetExtraBlock`](../go/raw.md#Raw.SetExtraBlock)

### `scan_region_indexed` {#scan_region_indexed}

```c
bool (*scan_region_indexed)(int32_t dimension, int32_t x1, int32_t y1, int32_t z1,
                            int32_t x2, int32_t y2, int32_t z2, void* ctx,
                            PierPaletteSink palette, PierCellSink cells);
```

和 `scan_region` 扫描方块的部分一样，但每种不同的方块状态只通过调色板回调序列化一次，每一格只报告一个索引。一百万格石头的区域只需要序列化一次 SNBT，不用一百万次。不包括实体；要实体请用 `scan_region`，方块回调传空。同样有 2^24 格的上限。只能在服务器线程调用。维度没有就绪或者区域太大时返回 false。

- 调用形式：`api->scan_region_indexed(dimension, x1, y1, z1, x2, y2, z2, ctx, palette, cells)`
- 参数：
    - dimension : `int32_t`
    - x1 : `int32_t`
    - y1 : `int32_t`
    - z1 : `int32_t`
    - x2 : `int32_t`
    - y2 : `int32_t`
    - z2 : `int32_t`
    - ctx : `void*`
    - palette : `PierPaletteSink`
    - cells : `PierCellSink`
- 返回值类型：`bool`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 189 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::scan_indexed`](../rust/world.md#World.scan_indexed)
    - Go：[`ScanRegionIndexed`](../go/world.md#ScanRegionIndexed)
