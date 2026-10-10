# levilamina::world

The world: reads and writes at the level layer, covering time, weather, difficulty,
game rules, biomes and chunks.

The boundary with [`crate::host`](host.md) is whether something speaks about the host or the
world. The server stage, scheduling and executing a command belong to the host and hold
for another game; time, weather and chunks belong to the world.

The switches of the level itself are here, changing things inside the world is in
`edit`, and assembling commands is in `commands`.

## Functions {#functions}

### `world::commands::split_box` {#fn.split_box}

```rust
pub fn split_box(from: PositionI32, to: PositionI32) -> Vec<Box3D>
```

Cuts a cuboid along y into slices, each within [`MAX_FILL_VOLUME`](world.md#MAX_FILL_VOLUME).

Along y rather than along the longest edge: the cost of a `/fill` lies mostly in how
many chunks it spans, and the cells of one y column are necessarily in the same chunk.

- Parameters:
    - from : `PositionI32`
    - to : `PositionI32`
- Return type: `Vec<Box3D>`

### `world::commands::is_valid_ticking_area_name` {#fn.is_valid_ticking_area_name}

```rust
pub fn is_valid_ticking_area_name(name: &str) -> bool
```

Whether the name of a ticking area is valid.

The engine accepts `A-Z a-z 0-9 _` only. A name containing a space or a non-ASCII
character makes `/tickingarea add` parse the name as the next argument, and the error
it reports has nothing to do with the name.

- Parameters:
    - name : `&str`
- Return type: `bool`

## `World` {#World}

```rust
pub struct World(/* private */);
```

The level facade. Zero sized.

- Implements: `Clone`, `Copy`

### `World::get` {#World.get}

```rust
pub fn get() -> World
```

- Return type: `World`

### `World::time` {#World.time}

```rust
pub fn time(&self) -> Result<i64>
```

- Return type: `Result<i64>`
- Slots: [`get_time`](../cpp/world.md#get_time)

### `World::set_time` {#World.set_time}

```rust
pub fn set_time(&self, t: i64) -> Result<()>
```

- Parameters:
    - t : `i64`
- Return type: `Result<()>`
- Slots: [`set_time`](../cpp/world.md#set_time)

### `World::set_weather` {#World.set_weather}

```rust
pub fn set_weather(&self, weather: Weather) -> Result<()>
```

- Parameters:
    - weather : `Weather`
- Return type: `Result<()>`
- Slots: [`set_weather`](../cpp/world.md#set_weather)

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

Sets the level and the remaining duration, in ticks, of rain and of lightning
individually.

Finer than [`World::set_weather`](world.md#World.set_weather), which has three settings, this can do light rain for
three minutes.

- Parameters:
    - rain_level : `f32`
    - rain_ticks : `i32`
    - lightning_level : `f32`
    - lightning_ticks : `i32`
- Return type: `Result<()>`
- Slots: [`level_update_weather`](../cpp/world.md#level_update_weather)

### `World::difficulty` {#World.difficulty}

```rust
pub fn difficulty(&self) -> Result<Difficulty>
```

- Return type: `Result<Difficulty>`
- Slots: [`get_difficulty`](../cpp/world.md#get_difficulty)

### `World::set_difficulty` {#World.set_difficulty}

```rust
pub fn set_difficulty(&self, d: Difficulty) -> Result<()>
```

- Parameters:
    - d : `Difficulty`
- Return type: `Result<()>`
- Slots: [`set_difficulty`](../cpp/world.md#set_difficulty)

### `World::seed` {#World.seed}

```rust
pub fn seed(&self) -> Result<i64>
```

- Return type: `Result<i64>`
- Slots: [`get_seed`](../cpp/world.md#get_seed)

### `World::game_rule` {#World.game_rule}

```rust
pub fn game_rule(&self, name: &str) -> Result<GameRuleValue>
```

Reads one game rule. An unrecognized rule name is an `Err` and not some default value.

- Parameters:
    - name : `&str`
- Return type: `Result<GameRuleValue>`
- Slots: [`game_rule_get`](../cpp/world.md#game_rule_get)

### `World::set_game_rule` {#World.set_game_rule}

```rust
pub fn set_game_rule(&self, name: &str, value: &str) -> Result<()>
```

- Parameters:
    - name : `&str`
    - value : `&str`
- Return type: `Result<()>`
- Slots: [`game_rule_set`](../cpp/world.md#game_rule_set)

### `World::default_spawn` {#World.default_spawn}

```rust
pub fn default_spawn(&self) -> Result<PositionI32>
```

- Return type: `Result<PositionI32>`
- Slots: [`level_get_default_spawn`](../cpp/world.md#level_get_default_spawn)

### `World::set_default_spawn` {#World.set_default_spawn}

```rust
pub fn set_default_spawn(&self, x: i32, y: i32, z: i32) -> Result<()>
```

- Parameters:
    - x : `i32`
    - y : `i32`
    - z : `i32`
- Return type: `Result<()>`
- Slots: [`level_set_default_spawn`](../cpp/world.md#level_set_default_spawn)

### `World::save` {#World.save}

```rust
pub fn save(&self) -> Result<()>
```

Saves to disk immediately.

- Return type: `Result<()>`
- Slots: [`level_save`](../cpp/world.md#level_save)

### `World::sleep_status` {#World.sleep_status}

```rust
pub fn sleep_status(&self) -> Result<SleepStatus>
```

- Return type: `Result<SleepStatus>`
- Slots: [`level_get_sleep_status`](../cpp/world.md#level_get_sleep_status)

### `World::biome` {#World.biome}

```rust
pub fn biome(&self, dim: i32, x: i32, y: i32, z: i32) -> Result<String>
```

- Parameters:
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- Return type: `Result<String>`
- Slots: [`level_get_biome`](../cpp/world.md#level_get_biome)

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

Sets the biome of a region, column by column.

It takes no y. `setBiome3d` works per y while Bedrock stores a biome per column, and
taking a y would suggest layers can be set separately. It returns how many columns were
set, and 0 means none were, because the chunks are not loaded or the biome name is
unrecognized, rather than being set with nothing changing.

- Parameters:
    - dim : `i32`
    - from : `(i32, i32)`
    - to : `(i32, i32)`
    - biome : `&str`
- Return type: `Result<i32>`
- Slots: [`level_set_biome`](../cpp/world.md#level_set_biome)

### `World::try_villages` {#World.try_villages}

```rust
pub fn try_villages(&self, dim: i32) -> Result<Vec<VillageInfo>>
```

The villages in one dimension, and `Err` for a host without the villages slot, which
an empty list would hide.

- Parameters:
    - dim : `i32`
- Return type: `Result<Vec<VillageInfo>>`
- Slots: [`villages`](../cpp/world.md#villages)

### `World::villages` {#World.villages}

!!! warning "Deprecated since 26.51.2"

    use try_villages: this answers an empty list when the host has no villages slot

```rust
pub fn villages(&self, dim: i32) -> Vec<VillageInfo>
```

The villages in one dimension, and an empty list when the host cannot list them.

- Parameters:
    - dim : `i32`
- Return type: `Vec<VillageInfo>`
- Slots: [`villages`](../cpp/world.md#villages)

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

The hardcoded generation areas in the loaded chunks within a radius.

Only loaded chunks are examined, since a read-only query should not force chunks to
load. An empty result therefore means either that there are none nearby or that the
nearby chunks are not loaded; a host without the slot is an `Err`.

- Parameters:
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - radius : `i32`
- Return type: `Result<Vec<StructureInfo>>`
- Slots: [`structures_near`](../cpp/world.md#structures_near)

### `World::structures_near` {#World.structures_near}

!!! warning "Deprecated since 26.51.2"

    use try_structures_near: this answers an empty list when the host has no structures slot

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

As `Self::try_structures_near`, with an empty list when the host cannot answer.

- Parameters:
    - dim : `i32`
    - x : `i32`
    - y : `i32`
    - z : `i32`
    - radius : `i32`
- Return type: `Vec<StructureInfo>`
- Slots: [`structures_near`](../cpp/world.md#structures_near)

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

Whether every chunk covered by `[min..max]` is in memory.

This has to be asked before deleting a save key: a loaded chunk has a copy in memory
and writes the key just deleted straight back on unload, while the deletion itself
succeeded and reported a positive number.

- Parameters:
    - dim : `i32`
    - min_x : `i32`
    - min_z : `i32`
    - max_x : `i32`
    - max_z : `i32`
- Return type: `Result<bool>`
- Slots: [`level_chunks_loaded`](../cpp/world.md#level_chunks_loaded)

### `World::delete_chunk_keys` {#World.delete_chunk_keys}

```rust
pub fn delete_chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<i32>
```

Deletes every save key of a chunk, so the engine regenerates it from the generator on
the next load.

The chunk must be unloaded first; see [`World::chunks_loaded`](world.md#World.chunks_loaded). Getting a chunk
unloaded is the caller's business, since who is nearby and when unloading is possible
needs domain knowledge this layer should not have.

It returns how many keys were deleted. A 0 is a normal result and means that chunk was
never generated.

- Parameters:
    - dim : `i32`
    - chunk_x : `i32`
    - chunk_z : `i32`
- Return type: `Result<i32>`
- Slots: [`level_delete_chunk_keys`](../cpp/world.md#level_delete_chunk_keys)

### `World::chunk_keys` {#World.chunk_keys}

```rust
pub fn chunk_keys(&self, dim: i32, chunk_x: i32, chunk_z: i32) -> Result<Vec<Vec<u8>>>
```

Lists every save key of a chunk.

A key is binary and contains zero bytes, so it is a `Vec<u8>` and not a `String`: a
UTF-8 conversion would corrupt it into a key that cannot be deleted.

- Parameters:
    - dim : `i32`
    - chunk_x : `i32`
    - chunk_z : `i32`
- Return type: `Result<Vec<Vec<u8>>>`
- Slots: [`level_chunk_keys`](../cpp/world.md#level_chunk_keys)

### `World::delete_key` {#World.delete_key}

```rust
pub fn delete_key(&self, key: &[u8]) -> Result<()>
```

Deletes one save key byte for byte. The content is not interpreted and what is passed
is what is deleted.

- Parameters:
    - key : `&[u8]`
- Return type: `Result<()>`
- Slots: [`level_delete_key`](../cpp/world.md#level_delete_key)

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

Fills a region with `/fill`, cut automatically into slices within the volume cap.

It returns how many commands ran. A failure partway stops there and returns `Err`
rather than continuing, since continuing gives a half-filled region and the return
value does not say how far it got.

- Parameters:
    - dim : `i32`
    - from : `PositionI32`
    - to : `PositionI32`
    - block : `&str`
- Return type: `Result<usize>`
- Slots: [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

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

Creates a ticking area.

A ticking area belongs to the save, survives a restart and belongs to no mod, so it is
not removed automatically when a mod unloads and needs
[`World::remove_ticking_area`](world.md#World.remove_ticking_area).

- Parameters:
    - dim : `i32`
    - from : `(i32, i32)`
    - to : `(i32, i32)`
    - name : `&str`
- Return type: `Result<()>`
- Slots: [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `World::remove_ticking_area` {#World.remove_ticking_area}

```rust
pub fn remove_ticking_area(&self, dim: i32, name: &str) -> Result<()>
```

- Parameters:
    - dim : `i32`
    - name : `&str`
- Return type: `Result<()>`
- Slots: [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `World::list_ticking_areas` {#World.list_ticking_areas}

```rust
pub fn list_ticking_areas(&self, dim: i32) -> Result<Vec<String>>
```

Lists the ticking area names of one dimension.

The engine output is prose meant for a human, split here on commas and whitespace. The
format follows the version, so failing to split returns an empty table rather than an
error, and the raw output is in the log.

- Parameters:
    - dim : `i32`
- Return type: `Result<Vec<String>>`
- Slots: [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `World::scan` {#World.scan}

```rust
pub fn scan(&self, dim: i32, bounds: Bounds) -> Result<Scan>
```

Scans a region and collects every block and actor into memory.

A large region uses [`World::scan_with`](world.md#World.scan_with) instead, since the memory this function uses
is proportional to the cell count.

- Parameters:
    - dim : `i32`
    - bounds : `Bounds`
- Return type: `Result<Scan>`
- Slots: [`scan_region`](../cpp/world.md#scan_region)

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

A streaming scan: the callback runs once per entry the host sinks and nothing is
accumulated.

Both callbacks run synchronously during this call, the pointers the host passes become
invalid the moment it returns, and a callback therefore receives an already copied
`String` (contract §3).

- Parameters:
    - dim : `i32`
    - bounds : `Bounds`
    - on_block : `&mut dyn FnMut(BlockInfo)`
    - on_entity : `&mut dyn FnMut(EntityInfo)`
- Return type: `Result<()>`
- Slots: [`scan_region`](../cpp/world.md#scan_region)

### `World::scan_indexed` {#World.scan_indexed}

```rust
pub fn scan_indexed(&self, dim: i32, bounds: Bounds) -> Result<IndexedScan>
```

Scans the blocks of a region into a palette plus cells.

The host serializes each distinct block state once and reports every cell as an
index, so a large region costs a few strings instead of two per cell; this is the
form for copying, saving and diffing regions. Entities are not included; use
[`World::scan`](world.md#World.scan) with the entity half for those. The same 2^24-cell limit applies.

- Parameters:
    - dim : `i32`
    - bounds : `Bounds`
- Return type: `Result<IndexedScan>`
- Slots: [`scan_region_indexed`](../cpp/world.md#scan_region_indexed)

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

Fills a box with one block. `spec` is a bare name such as `minecraft:stone` or full
SNBT, resolved once for the whole box. `update_flags` is as in
[`crate::Block::set_nbt`](block.md#Block.set_nbt): `BlockUpdate::NONE` is the fastest and leaves the client to
catch up on the next chunk send. Returns the number of cells written.

One call replaces a loop over `set_block`, which cost an FFI call, a dimension lookup
and a spec parse per cell.

- Parameters:
    - dim : `i32`
    - bounds : `Bounds`
    - spec : `&str`
    - update : `BlockUpdate`
- Return type: `Result<u64>`
- Slots: [`edit_fill_region`](../cpp/edit.md#edit_fill_region)

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

Writes many cells in one call. Each entry of `palette` is a block spec resolved once,
and each cell names one by index. Returns the number of cells written; a cell whose
index is out of range or whose spec did not resolve is skipped, so a result below
`cells.len()` means some were.

Pasting an [`IndexedScan`](world.md#IndexedScan) is `set_blocks(dim, &scan.palette_specs(), &scan.cells, BlockUpdate::NONE)`.

- Parameters:
    - dim : `i32`
    - palette : `&[&str]`
    - cells : `&[BlockCell]`
    - update : `BlockUpdate`
- Return type: `Result<u64>`
- Slots: [`edit_set_blocks`](../cpp/edit.md#edit_set_blocks)

### `World::spawn_mob` {#World.spawn_mob}

```rust
pub fn spawn_mob(&self, dim: i32, type_name: &str, x: f64, y: f64, z: f64) -> Result<Entity>
```

- Parameters:
    - dim : `i32`
    - type_name : `&str`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<Entity>`
- Slots: [`spawn_mob`](../cpp/entity.md#spawn_mob)

### `World::spawn_entity_nbt` {#World.spawn_entity_nbt}

```rust
pub fn spawn_entity_nbt(
        &self,
        dim: i32,
        snbt: &str,
        pos: Option<(f64, f64, f64)>,
    ) -> Result<Entity>
```

Spawns an actor from full NBT, the inverse of [`Entity::snapshot`](entity.md#Entity.snapshot).

A given `pos` overrides the `Pos` inside the NBT. The engine allocates a new UniqueID,
so the id from the save is not reused.

- Parameters:
    - dim : `i32`
    - snbt : `&str`
    - pos : `Option<(f64, f64, f64)>`
- Return type: `Result<Entity>`
- Slots: [`edit_spawn_entity_nbt`](../cpp/edit.md#edit_spawn_entity_nbt)

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

Detonates. A `source` of `None` means there is no source actor.

- Parameters:
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
- Return type: `Result<()>`
- Slots: [`explode`](../cpp/world.md#explode)

### `World::spawn_particle` {#World.spawn_particle}

```rust
pub fn spawn_particle(&self, dim: i32, effect: &str, x: f64, y: f64, z: f64) -> Result<()>
```

A particle visible across the whole dimension. Showing it to one person only uses
[`crate::player::Player::spawn_particle`](player.md#Player.spawn_particle).

- Parameters:
    - dim : `i32`
    - effect : `&str`
    - x : `f64`
    - y : `f64`
    - z : `f64`
- Return type: `Result<()>`
- Slots: [`spawn_particle`](../cpp/world.md#spawn_particle)

### `World::find_path` {#World.find_path}

```rust
pub fn find_path(&self, who: Entity, x: i32, y: i32, z: i32) -> Result<NbtValue>
```

Computes a path for an actor to a target cell.

- Parameters:
    - who : `Entity`
    - x : `i32`
    - y : `i32`
    - z : `i32`
- Return type: `Result<NbtValue>`
- Slots: [`level_find_path`](../cpp/world.md#level_find_path)

## `IndexedScan` {#IndexedScan}

```rust
pub struct IndexedScan {
    pub palette: Vec<PaletteEntry>,
    pub cells: Vec<BlockCell>,
}
```

The result of one [`World::scan_indexed`](world.md#World.scan_indexed): every distinct block state once, and one
small cell per position. A region of a million stone cells holds one `String` pair here
where [`Scan`](world.md#Scan) holds two million.

- Implements: `Debug`, `Clone`, `Default`

### `IndexedScan::block_of` {#IndexedScan.block_of}

```rust
pub fn block_of(&self, cell: &BlockCell) -> Option<&PaletteEntry>
```

The palette entry of a cell.

- Parameters:
    - cell : `&BlockCell`
- Return type: `Option<&PaletteEntry>`

### `IndexedScan::palette_specs` {#IndexedScan.palette_specs}

```rust
pub fn palette_specs(&self) -> Vec<&str>
```

The palette as block specs, the shape [`World::set_blocks`](world.md#World.set_blocks) takes.

- Return type: `Vec<&str>`

### `IndexedScan::non_air_count` {#IndexedScan.non_air_count}

```rust
pub fn non_air_count(&self) -> usize
```

Cells that are not air, by palette name.

- Return type: `usize`

## `Scan` {#Scan}

```rust
pub struct Scan {
    pub blocks: Vec<BlockInfo>,
    pub entities: Vec<EntityInfo>,
}
```

The result of one scan.

- Implements: `Debug`, `Clone`, `Default`, `PartialEq`

### `Scan::block_map` {#Scan.block_map}

```rust
pub fn block_map(&self) -> std::collections::HashMap<PositionI32, &BlockInfo>
```

Indexes a block by coordinate.

It rebuilds a table on every call, so it does not belong in a loop, where it is O(n^2).
Repeated lookups keep the returned value. The ABI does not guarantee the traversal
order of the sink, so a position cannot be computed from an index.

- Return type: `std::collections::HashMap<PositionI32, &BlockInfo>`

### `Scan::non_air_count` {#Scan.non_air_count}

```rust
pub fn non_air_count(&self) -> usize
```

- Return type: `usize`

### `Scan::entity_count` {#Scan.entity_count}

```rust
pub fn entity_count(&self) -> usize
```

How many actors fell inside this region.

- Return type: `usize`

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

One actor that fell inside the region during a scan.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`

## `PaletteEntry` {#PaletteEntry}

```rust
pub struct PaletteEntry {
    pub name: String,
    /// The full block serialization, name + states + version, as SNBT.
    pub snbt: String,
}
```

One distinct block state met by [`World::scan_indexed`](world.md#World.scan_indexed).

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`

## `BlockCell` {#BlockCell}

```rust
pub struct BlockCell {
    pub pos: PositionI32,
    pub index: u32,
}
```

One cell of [`World::scan_indexed`](world.md#World.scan_indexed) or [`World::set_blocks`](world.md#World.set_blocks): a position and an index
into the palette that travels with it.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

## `VillageInfo` {#VillageInfo}

```rust
pub struct VillageInfo {
    pub uuid: String,
    pub center: PositionI32,
    pub bounds: Bounds,
    pub poi_count: i32,
}
```

One village.

- Implements: `Debug`, `Clone`, `PartialEq`

## `StructureInfo` {#StructureInfo}

```rust
pub struct StructureInfo {
    pub kind: String,
    pub bounds: Bounds,
}
```

One hardcoded generation area: a stronghold, a witch hut, an ocean monument or a
pillager outpost.

- Implements: `Debug`, `Clone`, `PartialEq`

## `GameRuleValue` {#GameRuleValue}

```rust
pub enum GameRuleValue {
        Bool(bool),
        Int(i64),
        Float(f64),
}
```

The value of one game rule.

- Implements: `Debug`, `Clone`, `PartialEq`

## `SleepStatus` {#SleepStatus}

```rust
pub struct SleepStatus {
    pub sleeping: bool,
    pub total_players: i32,
    pub active_sleeping: i32,
}
```

The sleep status, from `level_get_sleep_status`.

- Implements: `Debug`, `Clone`, `Copy`, `Default`, `PartialEq`, `Eq`

## `Box3D` {#Box3D}

```rust
pub type Box3D = (PositionI32, PositionI32);
```

A cuboid in whole cells, as `(min, max)`.

## Constants {#constants}

| Name | Value | Description |
|---|---|---|
| <span id="MAX_FILL_VOLUME"></span>`MAX_FILL_VOLUME` | `32_768` | The volume cap of one `/fill` command. The engine's own cap is 32768 cells and exceeding it fails the whole command: not a few cells short, but not one cell filled. |
