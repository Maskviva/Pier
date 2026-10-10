# Bulk world editing

??? note "Section notes in abi.h"

    **Bulk world editing, appended and struct\_size-gated.**

    Native write paths that bypass the console-command route used by `set_block` (`execute in <dim> run setblock…`). With these, block states come from structured NBT instead of command-string splicing, block entities can be written back, and entities can be respawned from saved NBT — all via existing engine entry points.

    `update_flags` is a bitmask: 1 = notify neighbours, 2 = sync client, 3 = both (equivalent to /setblock), 0 = neither (fastest for bulk fills, but the caller must resync afterwards). Server thread only.

    **Appended: bulk block reads and writes**

## Slots {#slots}

### `edit_set_block_nbt` {#edit_set_block_nbt}

```c
bool (*edit_set_block_nbt)(
    int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt, int32_t update_flags);
```

Write a block from serialized NBT ({name,states,version}, i.e. the shape `get_block` produces).

- Call: `api->edit_set_block_nbt(dim, x, y, z, snbt, update_flags)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - snbt : `PierStr`
    - update_flags : `int32_t`
- Return type: `bool`
- Section of abi.h: Bulk world editing, appended and struct\_size-gated.
- Position in the table: slot 167, counting from 0
- Callers in each binding:
    - Rust: [`Block::set_nbt`](../rust/block.md#Block.set_nbt)
    - Go: [`Raw.EditSetBlockNbt`](../go/raw.md#Raw.EditSetBlockNbt)

### `edit_set_block_states` {#edit_set_block_states}

```c
bool (*edit_set_block_states)(
    int32_t dim, int32_t x, int32_t y, int32_t z, PierStr name, PierStr states_snbt,
    int32_t update_flags);
```

Write a block from a name + optional partial states. An empty `states_snbt` means all-default states; the version is taken from the default state on the loader side — the caller must not supply one.

- Call: `api->edit_set_block_states(dim, x, y, z, name, states_snbt, update_flags)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - name : `PierStr`
    - states_snbt : `PierStr`
    - update_flags : `int32_t`
- Return type: `bool`
- Section of abi.h: Bulk world editing, appended and struct\_size-gated.
- Position in the table: slot 168, counting from 0
- Callers in each binding:
    - Rust: [`Block::set_states`](../rust/block.md#Block.set_states)
    - Go: [`Raw.EditSetBlockStates`](../go/raw.md#Raw.EditSetBlockStates)

### `edit_set_block_entity` {#edit_set_block_entity}

```c
bool (*edit_set_block_entity)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr snbt);
```

Write a block entity's NBT back (`BlockActor::load`). The cell must already hold the matching block.

- Call: `api->edit_set_block_entity(dim, x, y, z, snbt)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: Bulk world editing, appended and struct\_size-gated.
- Position in the table: slot 169, counting from 0
- Callers in each binding:
    - Rust: [`Block::set_block_entity`](../rust/block.md#Block.set_block_entity)
    - Go: [`Raw.EditSetBlockEntity`](../go/raw.md#Raw.EditSetBlockEntity)

### `edit_spawn_entity_nbt` {#edit_spawn_entity_nbt}

```c
bool (*edit_spawn_entity_nbt)(
    int32_t dim, PierStr snbt, bool use_pos, double x, double y, double z,
    PierActorId* out);
```

Spawn an entity from full NBT (the inverse of `actor_snapshot`). When `use_pos` is true, (x,y,z) overrides the Pos tag; the UniqueID is reassigned by the engine and returned via out. NULL on every current host: the engine helper that gives a loaded actor a fresh UniqueID is inlined, and reusing the snapshot's own id makes the engine treat two actors as one.

- Call: `api->edit_spawn_entity_nbt(dim, snbt, use_pos, x, y, z, out)`
- Parameters:
    - dim : `int32_t`
    - snbt : `PierStr`
    - use_pos : `bool`
    - x : `double`
    - y : `double`
    - z : `double`
    - out : `PierActorId*`
- Return type: `bool`
- Section of abi.h: Bulk world editing, appended and struct\_size-gated.
- Position in the table: slot 170, counting from 0
- Callers in each binding:
    - Rust: [`World::spawn_entity_nbt`](../rust/world.md#World.spawn_entity_nbt)
    - Go: [`Raw.EditSpawnEntityNbt`](../go/raw.md#Raw.EditSpawnEntityNbt)

### `edit_trace_ray` {#edit_trace_ray}

```c
bool (*edit_trace_ray)(
    PierActorId id, float max_dist, bool include_actors, bool include_blocks, void* ctx,
    PierStrSink sink);
```

Ray trace yielding the BLOCK coordinate and hit face: {type, block:\[x,y,z\], facing, pos:\[x,y,z\], entity}.

- Call: `api->edit_trace_ray(id, max_dist, include_actors, include_blocks, ctx, sink)`
- Parameters:
    - id : `PierActorId`
    - max_dist : `float`
    - include_actors : `bool`
    - include_blocks : `bool`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Bulk world editing, appended and struct\_size-gated.
- Position in the table: slot 171, counting from 0
- Callers in each binding:
    - Rust: [`Entity::trace_ray_blocks`](../rust/entity.md#Entity.trace_ray_blocks)
    - Go: [`Raw.EditTraceRay`](../go/raw.md#Raw.EditTraceRay)

### `edit_fill_region` {#edit_fill_region}

```c
int64_t (*edit_fill_region)(int32_t dimension, int32_t x1, int32_t y1, int32_t z1,
                            int32_t x2, int32_t y2, int32_t z2, PierStr block_spec,
                            int32_t update_flags);
```

Fills a box with one block. `block_spec` is a bare name such as "minecraft:stone" or full SNBT; it is resolved once for the whole box. `update_flags` is as in `edit_set_block_nbt`: bit 1 notifies neighbours, bit 2 syncs the client; 0 is the fastest and leaves the client to catch up on the next chunk send. Returns the number of cells written, or -1 if the dimension is not ready, the spec does not resolve, or the box exceeds 2^24 cells. Server thread only.

- Call: `api->edit_fill_region(dimension, x1, y1, z1, x2, y2, z2, block_spec, update_flags)`
- Parameters:
    - dimension : `int32_t`
    - x1 : `int32_t`
    - y1 : `int32_t`
    - z1 : `int32_t`
    - x2 : `int32_t`
    - y2 : `int32_t`
    - z2 : `int32_t`
    - block_spec : `PierStr`
    - update_flags : `int32_t`
- Return type: `int64_t`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 190, counting from 0
- Callers in each binding:
    - Rust: [`World::fill_region`](../rust/world.md#World.fill_region)
    - Go: [`FillRegion`](../go/edit.md#FillRegion), [`Raw.EditFillRegion`](../go/raw.md#Raw.EditFillRegion)

### `edit_set_blocks` {#edit_set_blocks}

```c
int64_t (*edit_set_blocks)(int32_t dimension, PierStr const* palette, uint32_t palette_count,
                           PierBlockCell const* cells, size_t cell_count, int32_t update_flags);
```

Writes many cells in one call. palette holds `palette_count` block specs, each resolved once; every cell names one by index. A cell whose index is out of range, or whose block did not resolve, is skipped and counted in the return value's complement. Returns the number of cells written, or -1 if the dimension is not ready or a pointer is null with a non-zero count. Server thread only.

- Call: `api->edit_set_blocks(dimension, palette, palette_count, cells, cell_count, update_flags)`
- Parameters:
    - dimension : `int32_t`
    - palette : `PierStr const*`
    - palette_count : `uint32_t`
    - cells : `PierBlockCell const*`
    - cell_count : `size_t`
    - update_flags : `int32_t`
- Return type: `int64_t`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 191, counting from 0
- Callers in each binding:
    - Rust: [`World::set_blocks`](../rust/world.md#World.set_blocks)
    - Go: [`SetBlocks`](../go/edit.md#SetBlocks)
