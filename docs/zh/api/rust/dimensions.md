# levilamina::dimensions · 自定义维度

自定义维度：可选能力包 `pier-dimensions` 的门面。

宿主没有编入它时，整组槽位都是 NULL，[`is_available`](client.md#fn.is_available) 返回 false，其他每个调用都返回一个说明宿主没有提供的 `Err`。这就是契约 §1 第 3 条在运行时的样子：可选的包不在，布局不变，槽位为空。

注册是幂等的：[`add_dimension`](dimensions.md#fn.add_dimension) 和 [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack) 在下次启动时，对同一个名字返回同一个持久化的 id，所以模组在启动时直接注册就行，不要先用 [`dimension_id`](dimensions.md#fn.dimension_id) 探测，那样在第一次启动时会落空。地形包是一个目录，里面有一个配置文件和一个由 `tools/pier-pack` 构建的二进制文件；[`pack_inspect`](dimensions.md#fn.pack_inspect) 说明它要求什么，[`add_dimension_pack`](dimensions.md#fn.add_dimension_pack) 挂载它，宿主把文件哈希和每一个绑定的值都和维度一起保存。

## 函数 {#functions}

### `dimensions::is_available` {#fn.is_available}

```rust
pub fn is_available() -> bool
```

这个宿主是否编入了自定义维度这项能力。

- 返回值类型：`bool`
- 对应槽位：[`md_is_available`](../cpp/dimensions.md#md_is_available)、[`md_is_available`](../cpp/dimensions.md#md_is_available)

### `dimensions::dimension_id` {#fn.dimension_id}

```rust
pub fn dimension_id(name: &str) -> Option<i32>
```

按名字查维度 id。

只对真正注册过的名字给出 id，否则返回 `None`，不会给出那个数值在运行时会变、看起来却像有效 id 的未定义维度。

很少需要用到；见模块文档里关于无条件注册的说明。

- 参数：
    - name : `&str`
- 返回值类型：`Option<i32>`
- 对应槽位：[`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id)、[`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id)

### `dimensions::list` {#fn.list}

```rust
pub fn list() -> Vec<ExistingDimension>
```

所有已注册的自定义维度。

接管一个现有存档的世界管理器必须先问这个：以前的插件创建的维度活在存档里，玩家能传送进去，管理器的表里却没有它们。后果比少几行严重：这些维度不受任何规则约束，新建的世界还可能分到一个和它们冲突的维度 id。

- 返回值类型：`Vec<ExistingDimension>`
- 对应槽位：[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)、[`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `dimensions::set_rule` {#fn.set_rule}

```rust
pub fn set_rule(dimension: i32, rule: DimensionRule, allow: bool) -> Result<()>
```

设置一条按维度生效的规则。

- 参数：
    - dimension : `i32`
    - rule : `DimensionRule`
    - allow : `bool`
- 返回值类型：`Result<()>`
- 对应槽位：[`md_set_dimension_rule`](../cpp/dimensions.md#md_set_dimension_rule)

### `dimensions::rule` {#fn.rule}

```rust
pub fn rule(dimension: i32, rule: DimensionRule) -> Result<Option<bool>>
```

读取一条规则。这个维度对这条规则没有显式登记时返回 `Ok(None)`，表示它沿用原版行为，这和登记为 false 不一样。比这个 SDK 旧、不认识 `rule` 的宿主也回答 `Ok(None)`；ABI 没有办法区分这两种情况。

- 参数：
    - dimension : `i32`
    - rule : `DimensionRule`
- 返回值类型：`Result<Option<bool>>`
- 对应槽位：[`md_get_dimension_rule`](../cpp/dimensions.md#md_get_dimension_rule)

### `dimensions::clear_rules` {#fn.clear_rules}

```rust
pub fn clear_rules(dimension: i32) -> Result<()>
```

清除一个维度的所有规则，用于世界被删除的时候。

- 参数：
    - dimension : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`md_clear_dimension_rules`](../cpp/dimensions.md#md_clear_dimension_rules)

### `dimensions::set_plot_merges` {#fn.set_plot_merges}

```rust
pub fn set_plot_merges(dimension: i32, merges: &[PlotMerge]) -> Result<()>
```

整体替换一个维度的合并标记。

整体替换，不做增量：增量要求两边时刻对同一个当前状态达成一致，而解除链接会先清掉邻居再保存自己，两步之间出错，两边的视图就分开了，而且没有办法恢复。整体推送让每一次都把两边拉回一致。

网格来自 [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack) 时模板包的 CONF 段：对没有网格的维度推送会被丢弃，并记一条警告。模组一侧必须对上的几何：令 `period = cell + gap`，一列 `(x, z)` 在单元内，当且仅当 `mod(x, period) < cell && mod(z, period) < cell`。

- 参数：
    - dimension : `i32`
    - merges : `&[PlotMerge]`
- 返回值类型：`Result<()>`
- 对应槽位：[`md_set_plot_merges`](../cpp/dimensions.md#md_set_plot_merges)

### `dimensions::add_dimension` {#fn.add_dimension}

```rust
pub fn add_dimension(name: &str, spec_snbt: &str) -> Result<i32>
```

添加一个使用原生地形的自定义维度；见 `abi.h` 里的 `md_add_dimension`。

描述对这个 SDK 是不透明的：形状归宿主所有，由 RSW 世界管理器（`rsw_world_spec::Generator::to_spec_snbt`）编写。这个函数只负责把字符串传过去。地形包类的地形在这里会被拒绝；见 [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack)。

- 参数：
    - name : `&str`
    - spec_snbt : `&str`
- 返回值类型：`Result<i32>`
- 对应槽位：[`md_add_dimension`](../cpp/dimensions.md#md_add_dimension)

### `dimensions::add_dimension_pack` {#fn.add_dimension_pack}

```rust
pub fn add_dimension_pack(
    name: &str,
    config_path: &str,
    spec_snbt: &str,
) -> std::result::Result<i32, PackError>
```

添加一个地形来自地形包的自定义维度；见 `abi.h` 里的 `md_add_dimension_pack`。

`config_path` 指定地形包的配置文件，相对于服务器根目录，用正斜杠；`spec_snbt` 是一份描述，地形部分带有 `kind:"template"` 或 `"volume"`，以及 `params` 和 `roles`。宿主校验地形包、绑定参数值，并把一切和维度一起保存。返回负数时是一个 `PIER_PACK_*` 代码，会变成一个不带问题行的 [`PackError`](dimensions.md#PackError)；原因写在宿主日志里，对同一个路径调用 [`pack_inspect`](dimensions.md#fn.pack_inspect) 会以 JSON 返回它们。

- 参数：
    - name : `&str`
    - config_path : `&str`
    - spec_snbt : `&str`
- 返回值类型：`std::result::Result<i32, PackError>`
- 对应槽位：[`md_add_dimension_pack`](../cpp/dimensions.md#md_add_dimension_pack)

### `dimensions::pack_inspect` {#fn.pack_inspect}

```rust
pub fn pack_inspect(config_path: &str) -> std::result::Result<String, PackError>
```

一个地形包要求什么，以 `md_pack_inspect` 描述的 JSON 文档给出，不注册任何东西。被拒绝时带有宿主给出的问题行。

- 参数：
    - config_path : `&str`
- 返回值类型：`std::result::Result<String, PackError>`
- 对应槽位：[`md_pack_inspect`](../cpp/dimensions.md#md_pack_inspect)

### `dimensions::retire_dimension` {#fn.retire_dimension}

```rust
pub fn retire_dimension(name: &str) -> Result<bool>
```

让一个自定义维度退役；见 `abi.h` 里的 `md_retire_dimension`。

宿主把这个名字从 `dimension_config.json`、自己的表和维度工厂里去掉，下次启动时不会再注册它。正在运行的引擎里什么都不会撤销：这次会话里构造的维度仍然在，里面的玩家也不会被移走。

区块留在存档里，id 也不会再分出去。之后用同一个名字注册的是一个新维度，有新的 id，旧的地形不会被它继承，只是留在磁盘上。

宿主没有这个名字的维度时返回 `Ok(false)`，第二次调用的回答也是这个。

- 参数：
    - name : `&str`
- 返回值类型：`Result<bool>`
- 对应槽位：[`md_retire_dimension`](../cpp/dimensions.md#md_retire_dimension)

### `dimensions::add_dimension_generated` {#fn.add_dimension_generated}

```rust
pub fn add_dimension_generated(name: &str, spec_snbt: &str, terrain: Terrain) -> Result<i32>
```

注册一个地形由这个模组填充的维度；见 `abi.h` 里的 `md_add_dimension_generated`。

`spec_snbt` 只带种子、高度和天空。带 terrain 段会被拒绝，因为宿主不读它。

`terrain` 会被泄漏成 `'static`：维度存在多久，宿主就在区块线程上用它多久，而这个生命期归宿主管，这一侧找不到一个回收它也安全的时刻。注册被拒绝时也会泄漏，因为尝试期间构造的生成器可能还拿着它。每个维度只注册一次。

- 参数：
    - name : `&str`
    - spec_snbt : `&str`
    - terrain : `Terrain`
- 返回值类型：`Result<i32>`
- 对应槽位：[`md_add_dimension_generated`](../cpp/dimensions.md#md_add_dimension_generated)

### `dimensions::set_dimension_cells` {#fn.set_dimension_cells}

```rust
pub fn set_dimension_cells(dim_id: i32, cell: i32, gap: i32) -> Result<()>
```

给一个维度设置约束规则所用的单元几何；见 `md_set_dimension_cells`。

`cell` 是单元的边长，`gap` 是单元之间的间隔，单位都是方块。`cell` 为 0 会移除几何，之后两条 `PIER_DIMRULE_*_CROSS_CELL` 规则就没有依据可以回答了。

- 参数：
    - dim_id : `i32`
    - cell : `i32`
    - gap : `i32`
- 返回值类型：`Result<()>`
- 对应槽位：[`md_set_dimension_cells`](../cpp/dimensions.md#md_set_dimension_cells)

## `GeneratorType` {#GeneratorType}

```rust
pub enum GeneratorType {
        Overworld = 1,
        Flat = 2,
        Nether = 3,
        TheEnd = 4,
        Void = 5,
}
```

`terrain:{kind:"native"}` 描述里的原版生成器。

取值是引擎的 `GeneratorType`，从 1 开始，不从 0 开始：从 0 编号的话，超平坦会生成一个下界。描述本身用 [`GeneratorType::spec_name`](dimensions.md#GeneratorType.spec_name) 给出的小写字符串来指定生成器；数字之所以保留，是因为 `md_list_dimensions` 和旧存档里都带着它们。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `GeneratorType::from_i32` {#GeneratorType.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<GeneratorType>
```

- 参数：
    - v : `i32`
- 返回值类型：`Option<GeneratorType>`

### `GeneratorType::as_i32` {#GeneratorType.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

### `GeneratorType::spec_name` {#GeneratorType.spec_name}

```rust
pub fn spec_name(self) -> &'static str
```

在描述里，`terrain.generator` 和 `sky.client` 的写法。

- 返回值类型：`&'static str`

### `GeneratorType::engine_name` {#GeneratorType.engine_name}

```rust
pub fn engine_name(self) -> &'static str
```

引擎自己对这个生成器的叫法。

和 [`GeneratorType::spec_name`](dimensions.md#GeneratorType.spec_name) 是两个不同的字符串：这个出现在生成参数和旧存档里。给引擎拼装东西时用它，不要用 `{:?}`。

- 返回值类型：`&'static str`

## `PlotMerge` {#PlotMerge}

```rust
pub struct PlotMerge {
    pub x: i32,
    pub z: i32,
    /// A bit set where 1 is north, 2 east, 4 south and 8 west.
    pub mask: u32,
}
```

一块地皮的合并标记。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `PlotMerge::is_empty` {#PlotMerge.is_empty}

```rust
pub fn is_empty(&self) -> bool
```

- 返回值类型：`bool`

### `PlotMerge::from_dirs` {#PlotMerge.from_dirs}

```rust
pub fn from_dirs(x: i32, z: i32, dirs: [bool; 4]) -> PlotMerge
```

按北、东、南、西的顺序，用四个方向组装一个掩码。

这个顺序就是位的顺序，`NORTH` 是第 0 位。手写 `1 | 4` 会让读的人倒过来查表，弄反了的表现是地皮朝错误的方向合并。

- 参数：
    - x : `i32`
    - z : `i32`
    - dirs : `[bool; 4]`
- 返回值类型：`PlotMerge`

## `DimensionRule` {#DimensionRule}

```rust
pub enum DimensionRule {
        SpawnMonster = 0,
        SpawnAnimal = 1,
        SpawnSpawner = 2,
        ExplodeBlocks = 3,
        FireSpread = 4,
        MobGriefing = 5,
        Projectile = 6,
        PistonPush = 7,
        LiquidFlow = 8,
        FarmlandDecay = 9,
        Ride = 10,
        /// Blocks only a piston push crossing a cell boundary, leaving the cell interior alone. It
        /// applies together with [`DimensionRule::PistonPush`] and either one forbidding stops
        /// the push.
        PistonCrossCell = 11,
        /// Blocks only actor movement crossing a cell boundary. Players and ridden vehicles are
        /// never restricted.
        EntityCrossCell = 12,
        /// Blocks actor movement on a gap, the ground between the cells.
        ///
        /// [`DimensionRule::EntityCrossCell`] keeps an actor inside its cell and leaves the gap
        /// free, so a mob that spawns on the road walks the road; with this one forbidden the
        /// road stops being a corridor as well and everything that is not a player stays where
        /// it stands on it. Same exemptions: players and ridden vehicles pass, and vertical
        /// movement is untouched.
        EntityOnGap = 13,
}
```

按维度生效的规则。取值对应 `PIER_DIMRULE_*`。

为什么不用游戏规则：基岩版的游戏规则对整个服务器生效，所以为了创造模式的地皮世界关掉 `doMobSpawning`，生存世界也会一起关掉。这些标志在真正的调用处检查，比如 `Spawner::spawnMob`、`Level::explode`，所以真正是按维度生效的。

从来没有登记过规则的维度完全不受影响：钩子直接落到原版实现上，调用方不需要专门放行原版维度。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `DimensionRule::as_i32` {#DimensionRule.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- 返回值类型：`i32`

## `PackStatus` {#PackStatus}

```rust
pub enum PackStatus {
        BadPath,
        ConfigUnreadable,
        ConfigInvalid,
        BinaryUnreadable,
        Corrupt,
        KindMismatch,
        HashMismatch,
        Unsupported,
        Params,
        Constraint,
        Height,
        StoredMismatch,
        Spec,
        Host,
        /// A code this SDK does not know; the host is newer than the mirror.
        Other(i32),
}
```

宿主拒绝一个地形包的原因；取值是 `PIER_PACK_*`。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `PackStatus::from_code` {#PackStatus.from_code}

```rust
pub fn from_code(code: i32) -> PackStatus
```

- 参数：
    - code : `i32`
- 返回值类型：`PackStatus`

## `ChunkSource` {#ChunkSource}

```rust
pub trait ChunkSource: Send + Sync + 'static { /* ... */ }
```

地形由模组自己填充的维度。

实现会被交给区块工作线程，所以它是 `Send + Sync`，并且注册以后不能再改变。这就是 `abi.h` 里 `PierGenerateChunkFn` 的约定：宿主并发地调用它，同样的坐标必须永远给出同样的结果，回调不能调用宿主的任何其他槽位。

只读取自己挂载的数据的生成器，三条都满足；查询世界当前状态的生成器，一条都不满足。

### `ChunkSource::fill` {#ChunkSource.fill}

```rust
fn fill(
        &self,
        chunk_x: i32,
        chunk_z: i32,
        min_y: i32,
        height: i32,
        materials: &mut [u16],
        biomes: &mut [u16],
    ) -> bool
```

填充一个区块。`materials` 有 `256 * height` 项，下标是 `(x * 16 + z) * height + y`，y 从 `min_y` 开始数；`biomes` 有 256 项，每列一项。两者存的都是注册时给出的两个调色板里的索引，材料 0 是空气。

返回 false 表示这个区块填不了；宿主会写入空气，并记录日志。

- 参数：
    - chunk_x : `i32`
    - chunk_z : `i32`
    - min_y : `i32`
    - height : `i32`
    - materials : `&mut [u16]`
    - biomes : `&mut [u16]`
- 返回值类型：`bool`

## `ExistingDimension` {#ExistingDimension}

```rust
pub struct ExistingDimension {
    pub name: String,
    pub dim: i32,
    /// The raw generation parameters, interpreted by the caller.
    pub snbt: String,
}
```

一个已经注册的自定义维度。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`

## `PackError` {#PackError}

```rust
pub struct PackError {
    pub status: PackStatus,
    pub problems: Vec<String>,
}
```

[`add_dimension_pack`](dimensions.md#fn.add_dimension_pack) 或 [`pack_inspect`](dimensions.md#fn.pack_inspect) 的一次拒绝；调用产生了原因时，附带宿主给出的原因。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Display`

## `Terrain` {#Terrain}

```rust
pub struct Terrain {
    pub source: Box<dyn ChunkSource>,
    /// Entry 0 must be `minecraft:air`: the host reads it as "nothing was written here".
    pub materials: Vec<String>,
    pub biomes: Vec<String>,
}
```

注册时交给宿主的东西，连同它的两个调色板。
