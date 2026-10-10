# 自定义维度

??? note "abi.h 里的分节说明"

    **能力组：自定义维度（`md_*`），宿主没有编入 pier-dimensions 时全部为 NULL（`Capability group: custom dimensions (md_*). All NULL when pier-dimensions`）**

    宿主没有编入 pier-dimensions 时，这一组槽位全部为 NULL。

    **地皮边界的约束（`Plot-boundary confinement`）**

    为 `PIER_DIMRULE_PISTON_CROSS_CELL` 和 `PIER_DIMRULE_ENTITY_CROSS_CELL` 提供数据。这两条规则要回答「这两列在不在同一块地皮里」，答案需要网格的几何形状，再加上合并标记。这个问题是在 `PistonBlockActor::_checkAttachedBlocks` 和 `Actor::move` 里提出的，都在引擎的刻路径上，每秒几百次调用，所以数据一次推送到这里，在原生代码里读取，不跨 FFI 回头查询。

    加载器一侧实现的归属规则和插件自己的 `owning_plot` 一致：两块已合并的地皮之间的接缝算地皮；交叉口只有四周的边都合并了才算地皮。两边规则不一致时，表现出来的不会是「某一列判断错了」：地皮主人能在自己合并后的地皮上手动放方块，活塞却拒绝推到那里。只能在服务器线程调用。

    **同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）**

    追加了五个槽位，没有改动 `PIER_ABI_VERSION`：单纯的追加不算版本变化，`struct_size` 才是准确的关卡。

    两个方向都成立。新加载器运行旧模组：旧表是新表逐字节相同的前缀，模组够不着这五个槽位，照常工作。新模组配旧加载器：SDK 运行时初始化时比较 `struct_size`，发现加载器的表比它编译时用的短，于是拒绝加载。这是正确的结果，因为在没有 `lane_publish` 的加载器上读那个单元会越界。

    简单地说：版本号记录「语义变了」，`struct_size` 记录「表变长了」。这次改动只属于后者。

    见上面 `PierLaneDesc` 处的长注释。一句话概括：服务是跨语言的 (名字, JSON) -> JSON 通道；快速通道直接调用函数表，只在两边由同一个工具链构建时成立；指纹对不上时一个指针都不交出，使用方退回到服务通道。

    只能在服务器线程调用。

    **追加：批量读写方块（`Appended: bulk block reads and writes`）**

    **地形归模组所有的维度（`Dimensions whose terrain belongs to the mod`）**

    三种原版生成器和虚空是引擎自己的，宿主提供它们，是因为提供它们几乎没有代价。除此之外的一切，比如层叠的地层、地皮网格、噪声场、二进制的地形格式和读取它的代码，都属于模组，宿主不想知道它们的形状。这两个槽位就是宿主需要知道的全部。

## 槽位 {#slots}

### `md_is_available` {#md_is_available}

```c
bool (*md_is_available)(void);
```

这个宿主能不能注册自定义维度。没有编入 pier-dimensions 时这个槽位是 NULL。有这个槽位时，它的回答来自对引擎维度定义表的一次探测，所以在内存布局和这次构建对不上的引擎上可能回答 false；第一次能给出回答之后，结果会被缓存。

- 调用形式：`api->md_is_available()`
- 返回值类型：`bool`
- 所在分节：能力组：自定义维度（`md_*`），宿主没有编入 pier-dimensions 时全部为 NULL（`Capability group: custom dimensions (md_*). All NULL when pier-dimensions`）
- 表内序号：第 146 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::is_available`](../rust/dimensions.md#fn.is_available)、[`dimensions::is_available`](../rust/dimensions.md#fn.is_available)
    - Go：[`DimensionsAvailable`](../go/dimensions.md#DimensionsAvailable)、[`Raw.MdIsAvailable`](../go/raw.md#Raw.MdIsAvailable)

### `md_set_dimension_rule` {#md_set_dimension_rule}

```c
void (*md_set_dimension_rule)(int32_t dimension, int32_t rule, bool allow);
```

按维度设置的规则，由加载器自己的钩子查询。

为什么不用游戏规则：基岩版的游戏规则对整个服务器生效。为了让创造模式的地皮世界安静一点而设置 `doMobSpawning=false`，生存世界也会停止生成生物。这些标志在实际调用处的钩子里检查（`Spawner::spawnMob`、`Level::explode` 等），所以真正是按维度生效的。

`rule` 是 `PierDimRule` 中的一个。给加载器不知道的维度设置规则没有坏处：规则表按原始的维度 id 索引，只在钩子里出现那个 id 时才查。

没有设置任何规则的维度完全不受影响：钩子直接调用 `origin()`，原版维度保持原版行为，调用方不需要专门排除它们。

- 调用形式：`api->md_set_dimension_rule(dimension, rule, allow)`
- 参数：
    - dimension : `int32_t`
    - rule : `int32_t`
    - allow : `bool`
- 所在分节：能力组：自定义维度（`md_*`），宿主没有编入 pier-dimensions 时全部为 NULL（`Capability group: custom dimensions (md_*). All NULL when pier-dimensions`）
- 表内序号：第 147 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::set_rule`](../rust/dimensions.md#fn.set_rule)
    - Go：[`SetDimensionRule`](../go/dimensions.md#SetDimensionRule)、[`Raw.MdSetDimensionRule`](../go/raw.md#Raw.MdSetDimensionRule)

### `md_get_dimension_rule` {#md_get_dimension_rule}

```c
bool (*md_get_dimension_rule)(int32_t dimension, int32_t rule, bool* outAllow);
```

读回一条规则。只有这个维度对这条规则有显式设置时才写入 `outAllow`，否则返回 false。宿主不认识的规则编号也回答 false，而 `md_set_dimension_rule` 会忽略这样的编号，所以比宿主新的绑定在这里分不清「没有设置」和「不支持」。

- 调用形式：`api->md_get_dimension_rule(dimension, rule, outAllow)`
- 参数：
    - dimension : `int32_t`
    - rule : `int32_t`
    - outAllow : `bool*`
- 返回值类型：`bool`
- 所在分节：能力组：自定义维度（`md_*`），宿主没有编入 pier-dimensions 时全部为 NULL（`Capability group: custom dimensions (md_*). All NULL when pier-dimensions`）
- 表内序号：第 148 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::rule`](../rust/dimensions.md#fn.rule)
    - Go：[`DimensionRule`](../go/dimensions.md#DimensionRule)、[`Raw.MdGetDimensionRule`](../go/raw.md#Raw.MdGetDimensionRule)

### `md_clear_dimension_rules` {#md_clear_dimension_rules}

```c
void (*md_clear_dimension_rules)(int32_t dimension);
```

删除一个维度的所有规则（删除世界时使用）。

- 调用形式：`api->md_clear_dimension_rules(dimension)`
- 参数：
    - dimension : `int32_t`
- 所在分节：能力组：自定义维度（`md_*`），宿主没有编入 pier-dimensions 时全部为 NULL（`Capability group: custom dimensions (md_*). All NULL when pier-dimensions`）
- 表内序号：第 149 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::clear_rules`](../rust/dimensions.md#fn.clear_rules)
    - Go：[`Raw.MdClearDimensionRules`](../go/raw.md#Raw.MdClearDimensionRules)

### `md_get_dimension_id` {#md_get_dimension_id}

```c
int32_t (*md_get_dimension_id)(PierStr name);
```

把维度名解析成维度 id。找不到时返回 -1。

只对**真正**注册过的名字返回 id：不认识的名字得到 -1，永远不会得到 `VanillaDimensions::Undefined()`（它的数值会在运行时被修改，看起来像一个有效的 id）。

很少需要用到这个调用。`md_add_dimension` 和 `md_add_dimension_pack` 是幂等的，以后启动时用同一个名字重新注册，会返回同一个持久化的 id，所以调用方在启动时无条件注册就行，不必先探测。

- 调用形式：`api->md_get_dimension_id(name)`
- 参数：
    - name : `PierStr`
- 返回值类型：`int32_t`
- 所在分节：能力组：自定义维度（`md_*`），宿主没有编入 pier-dimensions 时全部为 NULL（`Capability group: custom dimensions (md_*). All NULL when pier-dimensions`）
- 表内序号：第 150 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::dimension_id`](../rust/dimensions.md#fn.dimension_id)、[`dimensions::dimension_id`](../rust/dimensions.md#fn.dimension_id)
    - Go：[`DimensionID`](../go/dimensions.md#DimensionID)、[`Raw.MdGetDimensionId`](../go/raw.md#Raw.MdGetDimensionId)

### `md_set_plot_merges` {#md_set_plot_merges}

```c
void (*md_set_plot_merges)(int32_t dimension, int32_t const* entries, int32_t count);
```

整体替换一个维度的合并标记。`entries` 是 `count` 个三元组 `(x, z, mask)`，也就是 `count * 3` 个 int32；`mask` 是位集合，1=北，2=东，4=南，8=西，和插件的 `merged[]` 下标对应。只需要发送确实带有标记的地皮。

整体替换，不做增量：增量要求两边永远对表里现有的内容达成一致，而 `unlink` 会先清掉邻居再保存自己，两步之间出错，两边的视图就分开了，而且没有办法恢复。整体替换让每一次推送都把两边拉回一致。网格来自 `md_add_dimension_pack` 时模板包的 CONF 段；对没有网格的维度推送会被丢弃，并记一条警告。模组一侧必须对上的单元几何是：令 `period = cell + gap`，世界坐标 (x, z) 处的一列在单元内，当且仅当 `mod(x,period) < cell && mod(z,period) < cell`。

- 调用形式：`api->md_set_plot_merges(dimension, entries, count)`
- 参数：
    - dimension : `int32_t`
    - entries : `int32_t const*`
    - count : `int32_t`
- 所在分节：地皮边界的约束（`Plot-boundary confinement`）
- 表内序号：第 162 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::set_plot_merges`](../rust/dimensions.md#fn.set_plot_merges)
    - Go：[`Raw.MdSetPlotMerges`](../go/raw.md#Raw.MdSetPlotMerges)

### `md_list_dimensions` {#md_list_dimensions}

```c
void (*md_list_dimensions)(void* ctx, PierStrSink sink);
```

以 JSON 数组列出所有已注册的自定义维度：`[{"name":"plot_world","dim":1000,"snbt":"{…}"}]`。

没有这个槽位，`md_*` 这一组只能按名字查询（`md_get_dimension_id`），调用方必须事先知道名字。接管一个现有存档的世界管理器，就看不到以前的插件创建的维度：它们写在 dimension_config.json 里，在引擎里活着，玩家能传送进去，管理器的表里却没有它们。后果比列表少几行严重：这些维度不受任何规则约束，新建的世界还可能分到一个和它们冲突的编号，两个世界共用一个维度 id。

`name` 来自配置文件的键，`dim` 是引擎分配并持久化的编号，`snbt` 是原样的生成参数（地皮世界给出 `{layout:{…},seed:N}`，简单世界给出 `{generatorType:Flat,seed:N}`），由调用方解读。

输出回调**每个维度调用一次**，每次一个 JSON 对象，并不是调用一次给一个数组。对比上面的 `lane_list`，它交出的是一个 JSON 数组。这张表里两种形状都有，每个槽位都会写明是哪一种。

`md_is_available()` 为 false 时，回调一次都不会被调用。

- 调用形式：`api->md_list_dimensions(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 所在分节：同工具链快速通道，追加的槽位，受 `struct_size` 约束（`Same-toolchain fast lane, appended and struct_size-gated.`）
- 表内序号：第 177 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::list`](../rust/dimensions.md#fn.list)、[`dimensions::list`](../rust/dimensions.md#fn.list)、[`World::fill_blocks`](../rust/world.md#World.fill_blocks)、[`World::add_ticking_area`](../rust/world.md#World.add_ticking_area)、[`World::remove_ticking_area`](../rust/world.md#World.remove_ticking_area)、[`World::list_ticking_areas`](../rust/world.md#World.list_ticking_areas)
    - Go：[`ListDimensions`](../go/dimensions.md#ListDimensions)、[`Raw.MdListDimensions`](../go/raw.md#Raw.MdListDimensions)

### `md_add_dimension` {#md_add_dimension}

```c
int32_t (*md_add_dimension)(PierStr name, PierStr spec_snbt);
```

用一份声明式的描述，添加一个使用原生地形的自定义维度。

`spec_snbt` 是一个 `CompoundTag` 的 SNBT 字符串：

```text
{seed:123,
 height:{min:-64,max:320},            可选，上下限都是 16 的倍数
 sky:{client:"overworld"|"nether"|"end", skylight:true, weather:true,
      time:12000},                       可选，0 到 23999
 terrain:{kind:"native",
          generator:"overworld"|"nether"|"end"|"flat"|"void",
          biome:"minecraft:plains"}       biome 只在 void 时读取
```

`sky.time` 把这个维度固定在一天中的某一刻，其他维度仍然跟着关卡时钟走。下界和末地的客户端天空本来就没有昼夜循环，所以这个字段只影响主世界天空显示的内容。

三种原版生成器带有它们的结构（村庄、要塞、末地城）；和原版维度不同的是种子、高度和天空。

`sky.client` 是在 `DimensionDefinition` 里告诉客户端的天空：下界和末地的天空没有昼夜，那两个维度就是这样锁定时间的。它和服务器一侧的生成器互相独立。

`kind` 为 template 或 volume 的地形在这里会被拒绝，返回 -1：它们要走 `md_add_dimension_pack`，那里会先校验地形包，再保存描述。高度不在子区块边界上，或者生成器的名字宿主不认识，也同样拒绝；不会退回到一个「差不多」的生成器，因为描述会和维度一起持久化，按错误描述生成的地形没法重新生成。

按名字幂等。返回维度 id（>=3）或 -1。

- 调用形式：`api->md_add_dimension(name, spec_snbt)`
- 参数：
    - name : `PierStr`
    - spec_snbt : `PierStr`
- 返回值类型：`int32_t`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 193 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::add_dimension`](../rust/dimensions.md#fn.add_dimension)
    - Go：[`AddDimension`](../go/dimensions.md#AddDimension)、[`Raw.MdAddDimension`](../go/raw.md#Raw.MdAddDimension)

### `md_add_dimension_pack` {#md_add_dimension_pack}

```c
int32_t (*md_add_dimension_pack)(PierStr name, PierStr config_path, PierStr spec_snbt);
```

添加一个地形来自地形包的自定义维度。地形包是一个目录，里面有一个配置文件和一个由 tools/pier-pack 构建的二进制文件。`config_path` 指定配置文件，相对于服务器根目录，用正斜杠，不能有 `..`，也不能是绝对路径；二进制文件的名字写在配置里，相对于配置文件。

宿主只读配置里的四个键，别的都不看：

```text
{"pier_terrain":1, "type":"template"|"volume",
 "binary":"terrain.ptpl", "sha256":"<64 位十六进制数>"}
```

文件里的其他内容都属于模组（标题、描述、预览图），宿主从不读取。

`spec_snbt` 和 `md_add_dimension` 的形状一样，地形部分换成地形包：

```text
terrain:{kind:"template"|"volume",
         params:{plot_size:64, road_width:7},   模板参数，整数
         roles:{floor:"minecraft:stone"}}        模板角色的替换
```

地形包标记为 fixed 的参数不能给别的值；标记为 derived 的参数根本不能给；free 的参数必须在范围内并落在步长上；choice 的参数必须是选项之一。地形包里没有的角色，或者没有注册的方块，都会被拒绝。地形包自己的约束会用绑定后的值检查，不满足的约束会拒绝注册，并把它的消息写进日志。

保存任何东西之前，要比较三个来源：描述里的 `terrain.kind`、配置里的 `type`、二进制文件的魔数，而且二进制文件的哈希必须等于配置里的 `sha256`。保存下来的描述包含路径、这个哈希、每一个绑定的参数和角色替换，所以只凭存档就能重新生成地形。

以后启动时，保存下来的描述优先：同一个名字返回同一个 id；这次给的参数如果不同，会被忽略，并记一条警告；配置的 `sha256` 不再是保存的那个时，以 `PIER_PACK_STORED_MISMATCH` 拒绝，因为另一个二进制生成的地形不能接着一个已有的世界用。修改地形包，就意味着新建世界，或者换一个名字。

带 CONF 段的模板包会用生成地形的同一次挂载，为约束规则注册单元网格，所以两者永远一致；之后可以用 `md_set_plot_merges`。

返回维度 id（>=3），或者下面某个 `PIER_PACK_*` 代码，这些代码都是负数。

- 调用形式：`api->md_add_dimension_pack(name, config_path, spec_snbt)`
- 参数：
    - name : `PierStr`
    - config_path : `PierStr`
    - spec_snbt : `PierStr`
- 返回值类型：`int32_t`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 194 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::add_dimension_pack`](../rust/dimensions.md#fn.add_dimension_pack)
    - Go：[`Raw.MdAddDimensionPack`](../go/raw.md#Raw.MdAddDimensionPack)

### `md_pack_inspect` {#md_pack_inspect}

```c
int32_t (*md_pack_inspect)(PierStr config_path, void* ctx, PierStrSink sink);
```

查看一个包要求什么，不注册任何东西。输出回调收到一个 JSON 文档，形状和 `pier-pack inspect` 打印的一样：

```text
{"ok":true,"kind":"template","pack":"...","sha256":"...","name":"...",
"biome":"...","height":{"min":-512,"max":320,"fixed":false},
"params":[{"name":"plot_size","kind":"free","default":64,"min":4,
"max":512,"step":1},
{"name":"wall_style","kind":"choice","default":0,"choices":[0,1]},
{"name":"plot_depth","kind":"derived"},
{"name":"size","kind":"fixed","value":256}],
"roles":[{"name":"floor","default":"minecraft:grass_block"}],
"zones":["interior","border","road"],
"constraints":["..."], "shapes":23, "picks":1,
"voxels":[{"size":[5,4,3],"palette":5}], "confine":true}
```

模组按 `"params"` 构造表单：`free` 用滑块，`choice` 用列表，`fixed` 是只读的值，`derived` 不显示。被拒绝时，输出回调收到 `{"ok":false,"status":<code>,"problems":["..."]}`，并返回这个代码。返回 0 或某个 `PIER_PACK_*` 代码。只能在服务器线程调用。

- 调用形式：`api->md_pack_inspect(config_path, ctx, sink)`
- 参数：
    - config_path : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`int32_t`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 195 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::pack_inspect`](../rust/dimensions.md#fn.pack_inspect)
    - Go：[`Raw.MdPackInspect`](../go/raw.md#Raw.MdPackInspect)

### `md_retire_dimension` {#md_retire_dimension}

```c
bool (*md_retire_dimension)(PierStr name);
```

让一个自定义维度退役：把它从 dimension_config.json、宿主自己的表和维度工厂里去掉，下次启动时没有任何东西会注册它，它也不再出现在 `md_list_dimensions` 里。

这不是删除。这个维度写下的区块还在存档里，引擎在这次会话里仍然持有分给它的 id，站在里面的玩家也不会被移走。结束的只是宿主从自己的配置里再次注册这个名字的做法。

id 不会退回到池里。退役的名字再次注册时是一个新维度，会分到新的 id，旧区块留在磁盘上，没有任何维度引用，白占空间。如果把编号交还出去，新维度就会指向旧维度的地形，那样什么都恢复不了，所以这里选的是代价更小的那个错误。

名字已知并且已经退役时返回 true；宿主没有这个维度时返回 false，第二次调用报告的也是 false。

- 调用形式：`api->md_retire_dimension(name)`
- 参数：
    - name : `PierStr`
- 返回值类型：`bool`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 196 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::retire_dimension`](../rust/dimensions.md#fn.retire_dimension)
    - Go：[`Raw.MdRetireDimension`](../go/raw.md#Raw.MdRetireDimension)

### `md_add_dimension_generated` {#md_add_dimension_generated}

```c
int32_t (*md_add_dimension_generated)(PierStr name, PierStr spec_snbt,
                                      PierStr material_palette, PierStr biome_palette,
                                      PierGenerateChunkFn fn, void* user);
```

注册一个地形由调用方模组自己填充的维度。

`spec_snbt` 是去掉 terrain 段的 `md_add_dimension` 的形状：

```text
{seed:<u32>,
 height:{min:<int>,max:<int>},        16 的倍数，在世界范围之内
 sky:{client:"overworld"|"nether"|"end", skylight:<bool>, weather:<bool>,
      time:<0..23999，可选>}}
```

这里带 terrain 段会被拒绝：宿主不读它，而接受一个会被忽略的字段，描述就会写着一个没有人生成的世界。

`material_palette` 和 `biome_palette` 是用换行分隔的名字。材料索引 0 必须是空气，没有动过的列里就是它。每个名字都在这里解析一次：方块或生物群系注册表里没有的名字会让注册失败，不会留到以后变成地形里的一个空洞。

`fn` 在区块工作线程上被调用；它的约定写在 `PierGenerateChunkFn` 上，和通常的约定不一样。`user` 原样传回，宿主从不读取。

维度、它的 id 和它的描述都像 `md_add_dimension` 那样持久化，所以下次启动时它会回来，但地形要等同一个模组再次注册它才会回来。模组已经不在的维度会作为虚空加载，保留它的区块，并提示一次。

按名字幂等。返回维度 id（>=3）或 -1。

- 调用形式：`api->md_add_dimension_generated(name, spec_snbt, material_palette, biome_palette, fn, user)`
- 参数：
    - name : `PierStr`
    - spec_snbt : `PierStr`
    - material_palette : `PierStr`
    - biome_palette : `PierStr`
    - fn : `PierGenerateChunkFn`
    - user : `void*`
- 返回值类型：`int32_t`
- 所在分节：地形归模组所有的维度（`Dimensions whose terrain belongs to the mod`）
- 表内序号：第 198 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::add_dimension_generated`](../rust/dimensions.md#fn.add_dimension_generated)
    - Go：[`AddGeneratedDimension`](../go/dimensions.md#AddGeneratedDimension)

### `md_set_dimension_cells` {#md_set_dimension_cells}

```c
bool (*md_set_dimension_cells)(int32_t dim_id, int32_t cell, int32_t gap);
```

给一个生成式维度设置约束规则所用的单元几何。

`PIER_DIMRULE_PISTON_CROSS_CELL` 和 `PIER_DIMRULE_ENTITY_CROSS_CELL` 要判断两个位置是否在同一个单元里，这需要网格。几何属于模组生成的东西，所以由模组来声明；钩子留在宿主这边，因为它们是引擎钩子。`cell` 和 `gap` 以方块为单位，`cell` 为正数；`gap` 为 0 表示单元彼此相接。`cell` 传 0 会移除几何，这两条规则也就不再给出任何回答。

id 不是这个宿主注册的维度时返回 false。

- 调用形式：`api->md_set_dimension_cells(dim_id, cell, gap)`
- 参数：
    - dim_id : `int32_t`
    - cell : `int32_t`
    - gap : `int32_t`
- 返回值类型：`bool`
- 所在分节：地形归模组所有的维度（`Dimensions whose terrain belongs to the mod`）
- 表内序号：第 199 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`dimensions::set_dimension_cells`](../rust/dimensions.md#fn.set_dimension_cells)
    - Go：[`Raw.MdSetDimensionCells`](../go/raw.md#Raw.MdSetDimensionCells)

## 类型 {#types}

### `PierChunkRequest` {#PierChunkRequest}

```c
typedef struct PierChunkRequest
{
    int32_t dim_id;
    int32_t chunk_x;
    int32_t chunk_z;
    int32_t min_y;
    int32_t height;
    uint16_t* out_materials;
    uint16_t* out_biomes;
} PierChunkRequest;
```

向自己提供地形的模组请求的一个区块。

`out_materials` 有 `256 * height` 项，下标是 `(x * 16 + z) * height + y`，y 从 `min_y` 往上数。`out_biomes` 有 256 项，每列一项。两者存的都是注册时给出的调色板里的索引；材料调色板的索引 0 是空气。两个缓冲区归宿主所有，会被重复使用，所以回调只写入，什么都不保留。

宿主在注册时把索引一次性解析成方块和生物群系。地形从不以方块名的形式跨过这条边界，原因就在这里：一个区块要查 9.8 万次，每个区块都这样查一遍，生成器就成了卡顿的来源。

### `PierGenerateChunkFn` {#PierGenerateChunkFn}

```c
typedef int32_t (*PierGenerateChunkFn)(void* user, const PierChunkRequest* request);
```

填充一个区块。返回非零表示已经填好；返回零表示模组填不了，宿主会写入空气，并且每个维度只提示一次。

**线程**，这一条和这个文件里的其他内容正好相反：宿主在它的**区块工作线程**上调用这个函数，同时可能有好几个，分别处理同一个维度的不同区块。宿主不做任何串行化。

- 它必须能和自己并发运行。
- 对同一个 (dim_id, chunk_x, chunk_z)，它必须永远给出同样的结果：区块生成一次就保存下来，以后生成的邻居如果用的是另一个结果，就会留下一条以后怎么编辑都去不掉的接缝。
- 它不能调用这个 API 上的任何其他槽位。那些槽位都是为服务器线程写的，从这里调用，会碰到一份没有任何人加锁保护的状态。
- 它不能让异常跨过边界。

只读取注册时交给它的数据的生成器，这四条都满足。查询实时世界状态的生成器，一条都不满足。

### `PierPaletteSink` {#PierPaletteSink}

```c
typedef void (*PierPaletteSink)(void* ctx, uint32_t index, PierStr name, PierStr snbt);
```

`scan_region_indexed` 的调色板回调：区域里遇到的每一种不同的方块状态调用一次，在第一个用到它的格子之前调用。索引按第一次出现的顺序从 0 开始计数，只在那一次调用内有效。

- `name`：方块类型名，例如 `"minecraft:stone"`。
- `snbt`：完整的方块序列化（名字、状态和版本），以 SNBT 表示。

### `PierCellSink` {#PierCellSink}

```c
typedef void (*PierCellSink)(void* ctx, int32_t x, int32_t y, int32_t z, uint32_t index);
```

`scan_region_indexed` 的格子回调：每一格调用一次，传入它的调色板索引。

### `PierBlockCell` {#PierBlockCell}

```c
typedef struct PierBlockCell
{
    int32_t x;
    int32_t y;
    int32_t z;
    uint32_t index;
} PierBlockCell;
```

`edit_set_blocks` 的一格：一个位置，加上同一次调用传入的调色板里的索引。

## `PierDimRule` {#PierDimRule}

`md_set_dimension_rule` 用的按维度行为规则。

它们有意**不**对应任何引擎枚举：它们命名的是加载器自己拦截的东西。取值属于 ABI，只能追加，不能重新编号。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_DIMRULE_SPAWN_MONSTER"></span>`PIER_DIMRULE_SPAWN_MONSTER` | `0` | 敌对生物的自然生成 |
| <span id="PIER_DIMRULE_SPAWN_ANIMAL"></span>`PIER_DIMRULE_SPAWN_ANIMAL` | `1` | 友好生物的自然生成 |
| <span id="PIER_DIMRULE_SPAWN_SPAWNER"></span>`PIER_DIMRULE_SPAWN_SPAWNER` | `2` | 刷怪笼的生成 |
| <span id="PIER_DIMRULE_EXPLODE_BLOCKS"></span>`PIER_DIMRULE_EXPLODE_BLOCKS` | `3` | 爆炸破坏地形 |
| <span id="PIER_DIMRULE_FIRE_SPREAD"></span>`PIER_DIMRULE_FIRE_SPREAD` | `4` | 火焰向相邻方块蔓延 |
| <span id="PIER_DIMRULE_MOB_GRIEFING"></span>`PIER_DIMRULE_MOB_GRIEFING` | `5` | 生物改变方块 |
| <span id="PIER_DIMRULE_PROJECTILE"></span>`PIER_DIMRULE_PROJECTILE` | `6` | 生成弹射物 |
| <span id="PIER_DIMRULE_PISTON_PUSH"></span>`PIER_DIMRULE_PISTON_PUSH` | `7` | 活塞推动方块 |
| <span id="PIER_DIMRULE_LIQUID_FLOW"></span>`PIER_DIMRULE_LIQUID_FLOW` | `8` | 水和岩浆的流动扩散 |
| <span id="PIER_DIMRULE_FARMLAND_DECAY"></span>`PIER_DIMRULE_FARMLAND_DECAY` | `9` | 耕地被踩回泥土 |
| <span id="PIER_DIMRULE_RIDE"></span>`PIER_DIMRULE_RIDE` | `10` | 骑上船、矿车或动物 |
| <span id="PIER_DIMRULE_PISTON_CROSS_CELL"></span>`PIER_DIMRULE_PISTON_CROSS_CELL` | `11` | 同一个值，用的是地皮世界的叫法。两个名字都是永久的：删掉其中一个就是一次删除，按 §2.2 要同时提升两个版本号。 |
| <span id="PIER_DIMRULE_PISTON_CROSS_PLOT"></span>`PIER_DIMRULE_PISTON_CROSS_PLOT` | `11` | 从 26.20.3 起退役，请用 `PIER_DIMRULE_PISTON_CROSS_CELL` |
| <span id="PIER_DIMRULE_ENTITY_CROSS_CELL"></span>`PIER_DIMRULE_ENTITY_CROSS_CELL` | `12` |  |
| <span id="PIER_DIMRULE_ENTITY_CROSS_PLOT"></span>`PIER_DIMRULE_ENTITY_CROSS_PLOT` | `12` | 从 26.20.3 起退役，请用 `PIER_DIMRULE_ENTITY_CROSS_CELL` |

## `PierPackStatus` {#PierPackStatus}

`md_add_dimension_pack` 和 `md_pack_inspect` 的返回码。维度 id 从不为负，所以调用方靠正负号区分两者。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_PACK_OK"></span>`PIER_PACK_OK` | `0` |  |
| <span id="PIER_PACK_BAD_PATH"></span>`PIER_PACK_BAD_PATH` | `-1` | 是绝对路径、含有 `..`，或者跑出了服务器根目录 |
| <span id="PIER_PACK_CONFIG_UNREADABLE"></span>`PIER_PACK_CONFIG_UNREADABLE` | `-2` | 配置文件打不开 |
| <span id="PIER_PACK_CONFIG_INVALID"></span>`PIER_PACK_CONFIG_INVALID` | `-3` | 不是 JSON，或者缺少某个必需的键，或者键的格式不对 |
| <span id="PIER_PACK_BINARY_UNREADABLE"></span>`PIER_PACK_BINARY_UNREADABLE` | `-4` | 配置里指定的二进制文件打不开 |
| <span id="PIER_PACK_CORRUPT"></span>`PIER_PACK_CORRUPT` | `-5` | 某一段的哈希对不上，或者某个索引越界 |
| <span id="PIER_PACK_KIND_MISMATCH"></span>`PIER_PACK_KIND_MISMATCH` | `-6` | 描述里的 kind、配置里的 type 和二进制文件的魔数三者对不上 |
| <span id="PIER_PACK_HASH_MISMATCH"></span>`PIER_PACK_HASH_MISMATCH` | `-7` | 二进制文件的哈希和配置里的 sha256 不一致 |
| <span id="PIER_PACK_UNSUPPORTED"></span>`PIER_PACK_UNSUPPORTED` | `-8` | 这个宿主不支持的包类型或格式版本 |
| <span id="PIER_PACK_PARAMS"></span>`PIER_PACK_PARAMS` | `-9` | 某个参数或角色超出了地形包允许的范围 |
| <span id="PIER_PACK_CONSTRAINT"></span>`PIER_PACK_CONSTRAINT` | `-10` | 用这些值检查时，地形包的某条约束不满足 |
| <span id="PIER_PACK_HEIGHT"></span>`PIER_PACK_HEIGHT` | `-11` | 维度的高度和地形包不匹配 |
| <span id="PIER_PACK_STORED_MISMATCH"></span>`PIER_PACK_STORED_MISMATCH` | `-12` | 这个名字已经存在，用的是另一个二进制文件或另一种地形 |
| <span id="PIER_PACK_SPEC"></span>`PIER_PACK_SPEC` | `-13` | 描述读不出来，或者里面没有地形包类的地形 |
| <span id="PIER_PACK_HOST"></span>`PIER_PACK_HOST` | `-14` | 宿主的其他拒绝；原因写在日志里 |
