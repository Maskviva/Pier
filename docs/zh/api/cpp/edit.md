# 批量编辑世界

??? note "abi.h 里的分节说明"

    **批量编辑世界，追加的槽位，受 `struct_size` 约束（`Bulk world editing, appended and struct_size-gated.`）**

    原生的写入路径，绕开 `set_block` 所用的控制台命令那条路（`execute in <dim> run setblock…`）。有了这些槽位，方块状态来自结构化的 NBT，不用拼接命令字符串；方块实体可以写回；实体可以从保存的 NBT 重新生成。全部通过引擎现有的入口完成。

    `update_flags` 是一个位掩码：1 = 通知相邻方块，2 = 同步给客户端，3 = 两者都要（等同于 `/setblock`），0 = 都不要（批量填充最快，但调用方之后必须自己重新同步）。只能在服务器线程调用。

    **追加：批量读写方块（`Appended: bulk block reads and writes`）**

## 槽位 {#slots}

### `edit_set_block_nbt` {#edit_set_block_nbt}

```c
bool (*edit_set_block_nbt)(
    int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt, int32_t update_flags);
```

用序列化的 NBT 写入一个方块，格式是 `{name,states,version}`，也就是 `get_block` 给出的形状。

- 调用形式：`api->edit_set_block_nbt(dim, x, y, z, snbt, update_flags)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - snbt : `PierStr`
    - update_flags : `int32_t`
- 返回值类型：`bool`
- 所在分节：批量编辑世界，追加的槽位，受 `struct_size` 约束（`Bulk world editing, appended and struct_size-gated.`）
- 表内序号：第 167 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::set_nbt`](../rust/block.md#Block.set_nbt)
    - Go：[`Raw.EditSetBlockNbt`](../go/raw.md#Raw.EditSetBlockNbt)

### `edit_set_block_states` {#edit_set_block_states}

```c
bool (*edit_set_block_states)(
    int32_t dim, int32_t x, int32_t y, int32_t z, PierStr name, PierStr states_snbt,
    int32_t update_flags);
```

用方块名加上可选的部分状态写入一个方块。`states_snbt` 为空表示全部用默认状态；版本号取自加载器一侧的默认状态，调用方不能自己提供。

- 调用形式：`api->edit_set_block_states(dim, x, y, z, name, states_snbt, update_flags)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - name : `PierStr`
    - states_snbt : `PierStr`
    - update_flags : `int32_t`
- 返回值类型：`bool`
- 所在分节：批量编辑世界，追加的槽位，受 `struct_size` 约束（`Bulk world editing, appended and struct_size-gated.`）
- 表内序号：第 168 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::set_states`](../rust/block.md#Block.set_states)
    - Go：[`Raw.EditSetBlockStates`](../go/raw.md#Raw.EditSetBlockStates)

### `edit_set_block_entity` {#edit_set_block_entity}

```c
bool (*edit_set_block_entity)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt);
```

把一个方块实体的 NBT 写回去（`BlockActor::load`）。这一格里必须已经是对应的方块。

- 调用形式：`api->edit_set_block_entity(dim, x, y, z, snbt)`
- 参数：
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：批量编辑世界，追加的槽位，受 `struct_size` 约束（`Bulk world editing, appended and struct_size-gated.`）
- 表内序号：第 169 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Block::set_block_entity`](../rust/block.md#Block.set_block_entity)
    - Go：[`Raw.EditSetBlockEntity`](../go/raw.md#Raw.EditSetBlockEntity)

### `edit_spawn_entity_nbt` {#edit_spawn_entity_nbt}

```c
bool (*edit_spawn_entity_nbt)(
    int32_t dim, PierStr snbt, bool use_pos, double x, double y, double z,
    PierActorId* out);
```

用完整的 NBT 生成一个实体，是 `actor_snapshot` 的反向操作。`use_pos` 为 true 时，(x,y,z) 覆盖 `Pos` 标签；引擎会重新分配 UniqueID，并通过 `out` 返回。

当前所有宿主上这个槽位都是 NULL：给读入的实体分配新 UniqueID 的那个引擎函数被内联掉了，而沿用快照里原来的 id 会让引擎把两个实体当成同一个。

- 调用形式：`api->edit_spawn_entity_nbt(dim, snbt, use_pos, x, y, z, out)`
- 参数：
    - dim : `int32_t`
    - snbt : `PierStr`
    - use_pos : `bool`
    - x : `double`
    - y : `double`
    - z : `double`
    - out : `PierActorId*`
- 返回值类型：`bool`
- 所在分节：批量编辑世界，追加的槽位，受 `struct_size` 约束（`Bulk world editing, appended and struct_size-gated.`）
- 表内序号：第 170 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::spawn_entity_nbt`](../rust/world.md#World.spawn_entity_nbt)
    - Go：[`Raw.EditSpawnEntityNbt`](../go/raw.md#Raw.EditSpawnEntityNbt)

### `edit_trace_ray` {#edit_trace_ray}

```c
bool (*edit_trace_ray)(
    PierActorId id, float max_dist, bool include_actors, bool include_blocks, void* ctx,
    PierStrSink sink);
```

射线检测，给出命中的**方块**坐标和命中的面：`{type, block:[x,y,z], facing, pos:[x,y,z], entity}`。

- 调用形式：`api->edit_trace_ray(id, max_dist, include_actors, include_blocks, ctx, sink)`
- 参数：
    - id : `PierActorId`
    - max_dist : `float`
    - include_actors : `bool`
    - include_blocks : `bool`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：批量编辑世界，追加的槽位，受 `struct_size` 约束（`Bulk world editing, appended and struct_size-gated.`）
- 表内序号：第 171 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Entity::trace_ray_blocks`](../rust/entity.md#Entity.trace_ray_blocks)
    - Go：[`Raw.EditTraceRay`](../go/raw.md#Raw.EditTraceRay)

### `edit_fill_region` {#edit_fill_region}

```c
int64_t (*edit_fill_region)(int32_t dimension, int32_t x1, int32_t y1, int32_t z1,
                            int32_t x2, int32_t y2, int32_t z2, PierStr block_spec,
                            int32_t update_flags);
```

用同一种方块填满一个长方体。`block_spec` 可以是 `"minecraft:stone"` 这样的方块名，也可以是完整的 SNBT，整个长方体只解析一次。`update_flags` 的含义和 `edit_set_block_nbt` 一样：第 1 位通知相邻方块，第 2 位同步给客户端；传 0 最快，客户端要等下一次发送区块时才看到变化。返回写入的格子数；维度没有准备好、`block_spec` 解析不出来，或者长方体超过 2^24 个格子时返回 -1。只能在服务器线程调用。

- 调用形式：`api->edit_fill_region(dimension, x1, y1, z1, x2, y2, z2, block_spec, update_flags)`
- 参数：
    - dimension : `int32_t`
    - x1 : `int32_t`
    - y1 : `int32_t`
    - z1 : `int32_t`
    - x2 : `int32_t`
    - y2 : `int32_t`
    - z2 : `int32_t`
    - block_spec : `PierStr`
    - update_flags : `int32_t`
- 返回值类型：`int64_t`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 190 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::fill_region`](../rust/world.md#World.fill_region)
    - Go：[`FillRegion`](../go/edit.md#FillRegion)、[`Raw.EditFillRegion`](../go/raw.md#Raw.EditFillRegion)

### `edit_set_blocks` {#edit_set_blocks}

```c
int64_t (*edit_set_blocks)(int32_t dimension, PierStr const* palette, uint32_t palette_count,
                           PierBlockCell const* cells, size_t cell_count, int32_t update_flags);
```

一次调用写入很多格。`palette` 里有 `palette_count` 个方块描述，每个只解析一次；每一格用索引指定其中一个。索引越界或方块解析失败的格子会被跳过，它们的数量就是总格数减去返回值。返回写入的格子数；维度没有就绪，或者数量不为零而指针为空时返回 -1。只能在服务器线程调用。

- 调用形式：`api->edit_set_blocks(dimension, palette, palette_count, cells, cell_count, update_flags)`
- 参数：
    - dimension : `int32_t`
    - palette : `PierStr const*`
    - palette_count : `uint32_t`
    - cells : `PierBlockCell const*`
    - cell_count : `size_t`
    - update_flags : `int32_t`
- 返回值类型：`int64_t`
- 所在分节：追加：批量读写方块（`Appended: bulk block reads and writes`）
- 表内序号：第 191 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`World::set_blocks`](../rust/world.md#World.set_blocks)
    - Go：[`SetBlocks`](../go/edit.md#SetBlocks)
