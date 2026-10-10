# levilamina::world · 世界

世界：关卡层面的读写，包括时间、天气、难度、游戏规则、生物群系和区块。

和 [`crate::host`](host.md) 的分界在于说的是宿主还是世界。服务器阶段、调度、执行命令属于宿主，换一个游戏也成立；时间、天气、区块属于世界。

关卡本身的开关在这里，改变世界里的东西在 `edit`，拼装命令在 `commands`。

## 函数 {#functions}

### `world::commands::split_box` {#fn.split_box}

```rust
pub fn split_box(from: PositionI32, to: PositionI32) -> Vec<Box3D>
```

把一个长方体沿 y 切成若干片，每一片都在 [`MAX_FILL_VOLUME`](world.md#MAX_FILL_VOLUME) 以内。

沿 y 切，不沿最长的边切：`/fill` 的开销主要在于跨了多少个区块，而同一个 y 列上的格子一定在同一个区块里。

- 参数：
    - from : `PositionI32`
    - to : `PositionI32`
- 返回值类型：`Vec<Box3D>`

### `world::commands::is_valid_ticking_area_name` {#fn.is_valid_ticking_area_name}

```rust
pub fn is_valid_ticking_area_name(name: &str) -> bool
```

常加载区域的名字是否合法。

引擎只接受 `A-Z a-z 0-9 _`。名字里有空格或非 ASCII 字符时，`/tickingarea add` 会把名字解析成下一个参数，报出来的错误和名字毫无关系。

- 参数：
    - name : `&str`
- 返回值类型：`bool`

## `World` {#World}

```rust
pub struct World(/* private */);
```

关卡门面。零大小。

- 实现的 trait：`Clone`、`Copy`

### `World::get` {#World.get}

```rust
pub fn get() -> World
```

- 返回值类型：`World`

### `World::time` {#World.time}

```rust
pub fn time(&self) -> Result<i64>
```

- 返回值类型：`Result<i64>`
- 对应槽位：[`get_time`](../cpp/world.md#get_time)

### `World::set_time` {#World.set_time}

```rust
pub fn set_time(&self, t: i64) -> Result<()>
```

- 参数：
    - t : `i64`
- 返回值类型：`Result<()>`
- 对应槽位：[`set_time`](../cpp/world.md#set_time)

### `World::set_weather` {#World.set_weather}

```rust
pub fn set_weather(&self, weather: Weather) -> Result<()>
```

- 参数：
    - weather : `Weather`
- 返回值类型：`Result<()>`
- 对应槽位：[`set_weather`](../cpp/world.md#set_weather)

### `World::update_weather` {#World.update_weather}

```rust
pub fn update_weather(
        &self,
        rain_level: f32,
        rain_ticks: i32,
        lightning_level: f32,
        lightning_ticks: i32,
    ) -> Result<()>
```

分别设置降雨和闪电的强度，以及各自剩余的持续时间（单位刻）。

比只有三档的 [`World::set_weather`](world.md#World.set_weather) 更细，可以做到「下三分钟的小雨」。

- 参数：
    - rain_level : `f32`
    - rain_ticks : `i32`
    - lightning_level : `f32`
    - lightning_ticks : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`level_update_weather`](../cpp/world.md#level_update_weather)

### `World::difficulty` {#World.difficulty}

```rust
pub fn difficulty(&self) -> Result<Difficulty>
```

- 返回值类型：`Result<Difficulty>`
- 对应槽位：[`get_difficulty`](../cpp/world.md#get_difficulty)

### `World::set_difficulty` {#World.set_difficulty}

```rust
pub fn set_difficulty(&self, d: Difficulty) -> Result<()>
```

- 参数：
    - d : `Difficulty`
- 返回值类型：`Result<()>`
- 对应槽位：[`set_difficulty`](../cpp/world.md#set_difficulty)

### `World::seed` {#World.seed}

```rust
pub fn seed(&self) -> Result<i64>
```

- 返回值类型：`Result<i64>`
- 对应槽位：[`get_seed`](../cpp/world.md#get_seed)

### `World::game_rule` {#World.game_rule}

```rust
pub fn game_rule(&self, name: &str) -> Result<GameRuleValue>
```

读取一条游戏规则。规则名认不出来时返回 `Err`，不会给某个默认值。

- 参数：
    - name : `&str`
- 返回值类型：`Result<GameRuleValue>`
- 对应槽位：[`game_rule_get`](../cpp/world.md#game_rule_get)

### `World::set_game_rule` {#World.set_game_rule}

```rust
pub fn set_game_rule(&self, name: &str, value: &str) -> Result<()>
```

- 参数：
    - name : `&str`
    - value : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`game_rule_set`](../cpp/world.md#game_rule_set)

### `World::default_spawn` {#World.default_spawn}

```rust
pub fn default_spawn(&self) -> Result<PositionI32>
```

- 返回值类型：`Result<PositionI32>`
- 对应槽位：[`level_get_default_spawn`](../cpp/world.md#level_get_default_spawn)

### `World::set_default_spawn` {#World.set_default_spawn}

```rust
pub fn set_default_spawn(&self, x: i32, y: i32, z: i32) -> Result<()>
```

- 参数：
    - x : `i32`
    - y : `i32`
    - z : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`level_set_default_spawn`](../cpp/world.md#level_set_default_spawn)

### `World::save` {#World.save}

```rust
pub fn save(&self) -> Result<()>
```

立即保存到磁盘。

- 返回值类型：`Result<()>`
- 对应槽位：[`level_save`](../cpp/world.md#level_save)

### `World::sleep_status` {#World.sleep_status}

```rust
pub fn sleep_status(&self) -> Result<SleepStatus>
```

- 返回值类型：`Result<SleepStatus>`
- 对应槽位：[`level_get_sleep_status`](../cpp/world.md#level_get_sleep_status)

### `World::biome` {#World.biome}

```rust
pub fn biome(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<String>
```

- 参数：
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- 返回值类型：`Result<String>`
- 对应槽位：[`level_get_biome`](../cpp/world.md#level_get_biome)

### `World::set_biome` {#World.set_biome}

```rust
pub fn set_biome(
        &self,
        dim: i32,
        from: (i32, i32),
        to: (i32, i32),
        biome: &str,
    ) -> Result<i32>
```

按列设置一片区域的生物群系。

不接收 y。`setBiome3d` 按 y 设置，而基岩版按列存储生物群系，接收 y 会让人以为可以分层设置。返回设置了多少列；返回 0 表示一列都没设置，原因是区块没加载或者生物群系名认不出来，不会出现设置了却没有变化的情况。

- 参数：
    - dim : `i32`
    - from : `(i32, i32)`
    - to : `(i32, i32)`
    - biome : `&str`
- 返回值类型：`Result<i32>`
- 对应槽位：[`level_set_biome`](../cpp/world.md#level_set_biome)

### `World::try_villages` {#World.try_villages}

```rust
pub fn try_villages(&self, dim: i32) -> Result<Vec<VillageInfo>>
```

一个维度里的村庄；宿主没有 villages 槽位时返回 `Err`，空列表会把这一点掩盖掉。

- 参数：
    - dim : `i32`
- 返回值类型：`Result<Vec<VillageInfo>>`
- 对应槽位：[`villages`](../cpp/world.md#villages)

### `World::villages` {#World.villages}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_villages`：宿主没有 villages 槽位时，这个函数回答的是空列表

```rust
pub fn villages(&self, dim: i32) -> Vec<VillageInfo>
```

一个维度里的村庄；宿主列不出来时返回空列表。

- 参数：
    - dim : `i32`
- 返回值类型：`Vec<VillageInfo>`
- 对应槽位：[`villages`](../cpp/world.md#villages)

### `World::try_structures_near` {#World.try_structures_near}

```rust
pub fn try_structures_near(
        &self,
        dim: i32,
        x: i32,
        y: i32,
        z: i32,
        radius: i32,
    ) -> Result<Vec<StructureInfo>>
```

一个半径内已加载区块里的硬编码生成区。

只检查已加载的区块，因为只读查询不应该强制加载区块。所以结果为空，可能是附近没有，也可能是附近的区块没加载；宿主没有这个槽位时返回 `Err`。

- 参数：
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - radius : `i32`
- 返回值类型：`Result<Vec<StructureInfo>>`
- 对应槽位：[`structures_near`](../cpp/world.md#structures_near)

### `World::structures_near` {#World.structures_near}

!!! warning "已弃用（自 26.51.2 起）"

    请用 `try_structures_near`：宿主没有 structures 槽位时，这个函数回答的是空列表

```rust
pub fn structures_near(
        &self,
        dim: i32,
        x: i32,
        y: i32,
        z: i32,
        radius: i32,
    ) -> Vec<StructureInfo>
```

和 `Self::try_structures_near` 一样，但宿主答不上来时返回空列表。

- 参数：
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - radius : `i32`
- 返回值类型：`Vec<StructureInfo>`
- 对应槽位：[`structures_near`](../cpp/world.md#structures_near)

### `World::chunks_loaded` {#World.chunks_loaded}

```rust
pub fn chunks_loaded(
        &self,
        dim: i32,
        min_x: i32,
        min_z: i32,
        max_x: i32,
        max_z: i32,
    ) -> Result<bool>
```

覆盖 `[min..max]` 的每个区块是否都在内存里。

删除存档键之前必须先问这个：已加载的区块在内存里有一份副本，卸载时会把刚删掉的键直接写回去，而删除本身显示成功，还报告了一个正数。

- 参数：
    - dim : `i32`
    - min_x : `i32`
    - min_z : `i32`
    - max_x : `i32`
    - max_z : `i32`
- 返回值类型：`Result<bool>`
- 对应槽位：[`level_chunks_loaded`](../cpp/world.md#level_chunks_loaded)

### `World::delete_chunk_keys` {#World.delete_chunk_keys}

```rust
pub fn delete_chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<i32>
```

删除一个区块的所有存档键，下次加载时引擎会用生成器重新生成它。

区块必须先卸载；见 [`World::chunks_loaded`](world.md#World.chunks_loaded)。让区块卸载是调用方的事，因为谁在附近、什么时候能卸载，需要这一层不该有的领域知识。

返回删除了多少个键。返回 0 是正常结果，表示这个区块从来没有生成过。

- 参数：
    - dim : `i32`
    - chunk_x : `i32`
    - chunk_z : `i32`
- 返回值类型：`Result<i32>`
- 对应槽位：[`level_delete_chunk_keys`](../cpp/world.md#level_delete_chunk_keys)

### `World::chunk_keys` {#World.chunk_keys}

```rust
pub fn chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<Vec<Vec<u8>>>
```

列出一个区块的所有存档键。

键是二进制的，里面有零字节，所以类型是 `Vec<u8>`，没有用 `String`：转成 UTF-8 会把它损坏成一个删不掉的键。

- 参数：
    - dim : `i32`
    - chunk_x : `i32`
    - chunk_z : `i32`
- 返回值类型：`Result<Vec<Vec<u8>>>`
- 对应槽位：[`level_chunk_keys`](../cpp/world.md#level_chunk_keys)

### `World::delete_key` {#World.delete_key}

```rust
pub fn delete_key(&self, key: &[u8]) -> Result<()>
```

逐字节删除一个存档键。内容不会被解读，传进来什么就删除什么。

- 参数：
    - key : `&[u8]`
- 返回值类型：`Result<()>`
- 对应槽位：[`level_delete_key`](../cpp/world.md#level_delete_key)

### `World::fill_blocks` {#World.fill_blocks}

```rust
pub fn fill_blocks(
        &self,
        dim: i32,
        from: PositionI32,
        to: PositionI32,
        block: &str,
    ) -> Result<usize>
```

用 `/fill` 填充一片区域，自动切成不超过体积上限的若干片。

返回运行了多少条命令。中途失败就在那里停下并返回 `Err`，不会继续：继续下去会得到一片填了一半的区域，返回值也说不出填到了哪里。

- 参数：
    - dim : `i32`
    - from : `PositionI32`
    - to : `PositionI32`
    - block : `&str`
- 返回值类型：`Result<usize>`
- 对应槽位：[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `World::add_ticking_area` {#World.add_ticking_area}

```rust
pub fn add_ticking_area(
        &self,
        dim: i32,
        from: (i32, i32),
        to: (i32, i32),
        name: &str,
    ) -> Result<()>
```

创建一个常加载区域。

常加载区域属于存档，重启后仍然存在，也不属于任何模组，所以模组卸载时不会自动移除，需要调用 [`World::remove_ticking_area`](world.md#World.remove_ticking_area)。

- 参数：
    - dim : `i32`
    - from : `(i32, i32)`
    - to : `(i32, i32)`
    - name : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `World::remove_ticking_area` {#World.remove_ticking_area}

```rust
pub fn remove_ticking_area(&self, dim: i32, name: &str) -> Result<()>
```

- 参数：
    - dim : `i32`
    - name : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `World::list_ticking_areas` {#World.list_ticking_areas}

```rust
pub fn list_ticking_areas(&self, dim: i32) -> Result<Vec<String>>
```

列出一个维度里的常加载区域名。

引擎的输出是写给人看的文字，这里按逗号和空白切分。格式随版本变化，所以切分失败时返回空表、不报错，原始输出会写进日志。

- 参数：
    - dim : `i32`
- 返回值类型：`Result<Vec<String>>`
- 对应槽位：[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `World::scan` {#World.scan}

```rust
pub fn scan(&self, dim: i32, bounds: Bounds) -> Result<Scan>
```

扫描一片区域，把每个方块和实体都收集到内存里。

大区域请改用 [`World::scan_with`](world.md#World.scan_with)，因为这个函数用的内存和格子数成正比。

- 参数：
    - dim : `i32`
    - bounds : `Bounds`
- 返回值类型：`Result<Scan>`
- 对应槽位：[`scan_region`](../cpp/world.md#scan_region)

### `World::scan_with` {#World.scan_with}

```rust
pub fn scan_with(
        &self,
        dim: i32,
        bounds: Bounds,
        on_block: &mut dyn FnMut(BlockInfo),
        on_entity: &mut dyn FnMut(EntityInfo),
    ) -> Result<()>
```

流式扫描：宿主每输出一项，回调就运行一次，什么都不积累。

两个回调都在这次调用期间同步运行，宿主传来的指针在调用返回的那一刻就失效了，所以回调收到的是已经复制好的 `String`（契约 §3）。

- 参数：
    - dim : `i32`
    - bounds : `Bounds`
    - on_block : `&mut dyn FnMut(BlockInfo)`
    - on_entity : `&mut dyn FnMut(EntityInfo)`
- 返回值类型：`Result<()>`
- 对应槽位：[`scan_region`](../cpp/world.md#scan_region)

### `World::scan_indexed` {#World.scan_indexed}

```rust
pub fn scan_indexed(&self, dim: i32, bounds: Bounds) -> Result<IndexedScan>
```

把一片区域的方块扫描成调色板加格子。

宿主把每种不同的方块状态只序列化一次，每一格只报告一个索引，所以大区域只需要几个字符串，用不着每格两个；复制、保存和比较区域都用这种形式。不包括实体，实体请用 [`World::scan`](world.md#World.scan) 的实体那一半。同样有 2^24 格的上限。

- 参数：
    - dim : `i32`
    - bounds : `Bounds`
- 返回值类型：`Result<IndexedScan>`
- 对应槽位：[`scan_region_indexed`](../cpp/world.md#scan_region_indexed)

### `World::fill_region` {#World.fill_region}

```rust
pub fn fill_region(
        &self,
        dim: i32,
        bounds: Bounds,
        spec: &str,
        update: BlockUpdate,
    ) -> Result<u64>
```

用一种方块填满一个长方体。`spec` 可以是 `minecraft:stone` 这样的方块名，也可以是完整的 SNBT，整个长方体只解析一次。`update_flags` 和 [`crate::Block::set_nbt`](block.md#Block.set_nbt) 的一样：`BlockUpdate::NONE` 最快，客户端要等下一次发送区块时才更新。返回写入的格子数。

一次调用就代替了对 `set_block` 的循环，后者每一格都要一次 FFI 调用、一次维度查找和一次描述解析。

- 参数：
    - dim : `i32`
    - bounds : `Bounds`
    - spec : `&str`
    - update : `BlockUpdate`
- 返回值类型：`Result<u64>`
- 对应槽位：[`edit_fill_region`](../cpp/edit.md#edit_fill_region)

### `World::set_blocks` {#World.set_blocks}

```rust
pub fn set_blocks(
        &self,
        dim: i32,
        palette: &[&str],
        cells: &[BlockCell],
        update: BlockUpdate,
    ) -> Result<u64>
```

一次调用写入很多格。`palette` 的每一项是一个只解析一次的方块描述，每一格用索引指定其中一项。返回写入的格子数；索引越界或者描述解析不出来的格子会被跳过，所以结果小于 `cells.len()` 就表示有格子被跳过了。

粘贴一个 [`IndexedScan`](world.md#IndexedScan) 就是 `set_blocks(dim, &scan.palette_specs(), &scan.cells, BlockUpdate::NONE)`。

- 参数：
    - dim : `i32`
    - palette : `&[&str]`
    - cells : `&[BlockCell]`
    - update : `BlockUpdate`
- 返回值类型：`Result<u64>`
- 对应槽位：[`edit_set_blocks`](../cpp/edit.md#edit_set_blocks)

### `World::spawn_mob` {#World.spawn_mob}

```rust
pub fn spawn_mob(&self, dim: i32, type_name: &str, x: f64, y: f64, z: f64) -> Result<Entity>
```

- 参数：
    - dim : `i32`
    - type_name : `&str`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<Entity>`
- 对应槽位：[`spawn_mob`](../cpp/entity.md#spawn_mob)

### `World::spawn_entity_nbt` {#World.spawn_entity_nbt}

```rust
pub fn spawn_entity_nbt(
        &self,
        dim: i32,
        snbt: &str,
        pos: Option<(f64, f64, f64)>,
    ) -> Result<Entity>
```

用完整的 NBT 生成一个实体，是 [`Entity::snapshot`](entity.md#Entity.snapshot) 的反向操作。

给了 `pos` 时会覆盖 NBT 里的 `Pos`。引擎会分配新的 UniqueID，不会沿用存档里的 id。

- 参数：
    - dim : `i32`
    - snbt : `&str`
    - pos : `Option<(f64, f64, f64)>`
- 返回值类型：`Result<Entity>`
- 对应槽位：[`edit_spawn_entity_nbt`](../cpp/edit.md#edit_spawn_entity_nbt)

### `World::explode` {#World.explode}

```rust
pub fn explode(
        &self,
        dim: i32,
        x: f64,
        y: f64,
        z: f64,
        radius: f32,
        max_resistance: f32,
        source: Option<Entity>,
        fire: bool,
        breaks_blocks: bool,
        allow_underwater: bool,
    ) -> Result<()>
```

引爆。`source` 为 `None` 表示没有来源实体。

- 参数：
    - dim : `i32`
    - x : `f64`
    - y : `f64`
    - z : `f64`
    - radius : `f32`
    - max_resistance : `f32`
    - source : `Option<Entity>`
    - fire : `bool`
    - breaks_blocks : `bool`
    - allow_underwater : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`explode`](../cpp/world.md#explode)

### `World::spawn_particle` {#World.spawn_particle}

```rust
pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()>
```

整个维度都看得到的粒子。只显示给一个人看，请用 [`crate::player::Player::spawn_particle`](player.md#Player.spawn_particle)。

- 参数：
    - dim : `i32`
    - effect : `&str`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- 返回值类型：`Result<()>`
- 对应槽位：[`spawn_particle`](../cpp/world.md#spawn_particle)

### `World::find_path` {#World.find_path}

```rust
pub fn find_path(&self, who: Entity, x: i32, y: i32, z: i32) -> Result<NbtValue>
```

为一个实体计算到目标格子的路径。

- 参数：
    - who : `Entity`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- 返回值类型：`Result<NbtValue>`
- 对应槽位：[`level_find_path`](../cpp/world.md#level_find_path)

## `IndexedScan` {#IndexedScan}

```rust
pub struct IndexedScan {
    pub palette: Vec<PaletteEntry>,
    pub cells: Vec<BlockCell>,
}
```

一次 [`World::scan_indexed`](world.md#World.scan_indexed) 的结果：每种不同的方块状态一份，每个位置一个小格子。一百万格石头的区域，在这里只有一对 `String`，在 [`Scan`](world.md#Scan) 里则有两百万个。

- 实现的 trait：`Debug`、`Clone`、`Default`

### `IndexedScan::block_of` {#IndexedScan.block_of}

```rust
pub fn block_of(&self, cell: &BlockCell) -> Option<&PaletteEntry>
```

一个格子对应的调色板条目。

- 参数：
    - cell : `&BlockCell`
- 返回值类型：`Option<&PaletteEntry>`

### `IndexedScan::palette_specs` {#IndexedScan.palette_specs}

```rust
pub fn palette_specs(&self) -> Vec<&str>
```

把调色板转成方块描述，也就是 [`World::set_blocks`](world.md#World.set_blocks) 接收的形状。

- 返回值类型：`Vec<&str>`

### `IndexedScan::non_air_count` {#IndexedScan.non_air_count}

```rust
pub fn non_air_count(&self) -> usize
```

按调色板里的名字，统计不是空气的格子数。

- 返回值类型：`usize`

## `Scan` {#Scan}

```rust
pub struct Scan {
    pub blocks: Vec<BlockInfo>,
    pub entities: Vec<EntityInfo>,
}
```

一次扫描的结果。

- 实现的 trait：`Debug`、`Clone`、`Default`、`PartialEq`

### `Scan::block_map` {#Scan.block_map}

```rust
pub fn block_map(&self) -> std::collections::HashMap<PositionI32, &BlockInfo>
```

按坐标索引方块。

每次调用都会重建一张表，所以不该放在循环里，那样是 O(n^2)。要反复查询，就保存返回的值。ABI 不保证输出回调的遍历顺序，所以不能从下标推算位置。

- 返回值类型：`std::collections::HashMap<PositionI32, &BlockInfo>`

### `Scan::non_air_count` {#Scan.non_air_count}

```rust
pub fn non_air_count(&self) -> usize
```

- 返回值类型：`usize`

### `Scan::entity_count` {#Scan.entity_count}

```rust
pub fn entity_count(&self) -> usize
```

有多少实体落在这片区域里。

- 返回值类型：`usize`

## `EntityInfo` {#EntityInfo}

```rust
pub struct EntityInfo {
    /// The cell the actor is in, its position floored.
    pub cell: PositionI32,
    pub type_name: String,
    /// The full NBT of `Actor::save`.
    pub snbt: String,
}
```

扫描时落在区域里的一个实体。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`

## `PaletteEntry` {#PaletteEntry}

```rust
pub struct PaletteEntry {
    pub name: String,
    /// The full block serialization, name + states + version, as SNBT.
    pub snbt: String,
}
```

[`World::scan_indexed`](world.md#World.scan_indexed) 遇到的一种不同的方块状态。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`

## `BlockCell` {#BlockCell}

```rust
pub struct BlockCell {
    pub pos: PositionI32,
    pub index: u32,
}
```

[`World::scan_indexed`](world.md#World.scan_indexed) 或 [`World::set_blocks`](world.md#World.set_blocks) 的一格：一个位置，加上随它一起传递的调色板里的索引。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

## `VillageInfo` {#VillageInfo}

```rust
pub struct VillageInfo {
    pub uuid: String,
    pub center: PositionI32,
    pub bounds: Bounds,
    pub poi_count: i32,
}
```

一个村庄。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

## `StructureInfo` {#StructureInfo}

```rust
pub struct StructureInfo {
    pub kind: String,
    pub bounds: Bounds,
}
```

一个硬编码生成区：要塞、女巫小屋、海底神殿或掠夺者前哨站。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

## `GameRuleValue` {#GameRuleValue}

```rust
pub enum GameRuleValue {
        Bool(bool),
        Int(i64),
        Float(f64),
}
```

一条游戏规则的值。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

## `SleepStatus` {#SleepStatus}

```rust
pub struct SleepStatus {
    pub sleeping: bool,
    pub total_players: i32,
    pub active_sleeping: i32,
}
```

睡眠状态，取自 `level_get_sleep_status`。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`Default`、`PartialEq`、`Eq`

## `Box3D` {#Box3D}

```rust
pub type Box3D = (PositionI32, PositionI32);
```

以整格表示的长方体，写作 `(min, max)`。

## 常量 {#constants}

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="MAX_FILL_VOLUME"></span>`MAX_FILL_VOLUME` | `32_768` | 一条 `/fill` 命令的体积上限。 引擎自己的上限是 32768 格，超过时整条命令都会失败，一格都不会填。 |
