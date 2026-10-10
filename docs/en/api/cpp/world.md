# World

??? note "Section notes in abi.h"

    **Appended**

    **§A world read/write & clock**

    **§C actors (players resolve here too, via player\_resolve)**

    **§I NBT binary, KvDb (thread-safe), system & server info**

    **Level: biome, spawn, save, weather, path, sleep (dedicated fns)**

    **Same-toolchain fast lane, appended and struct\_size-gated.**

    Five slots appended without touching `PIER_ABI_VERSION`: a pure append is not a version change, and `struct_size` is the precise gate.

    Both directions hold. A new loader running an old mod: the old table is a byte-identical prefix of the new one, the mod cannot reach these five slots, and it works unchanged. A new mod on an old loader: SDK runtime init compares `struct_size`, finds the loader's table shorter than the one it was compiled against, and refuses to load. That is the right outcome, since a mod that reads the `lane_publish` cell on a loader without it would read out of bounds.

    In short: the version number tracks "semantics changed", `struct_size` tracks "the table grew". This change is only the latter.

    See the long comment at PierLaneDesc above. In one line: service is the cross-language (name, JSON) -&gt; JSON channel, while this is a direct function-table call that holds only when both sides were built by the same toolchain; a fingerprint mismatch yields no pointer and the consumer falls back to service.

    Server thread only.

    **Appended slots. Added at the tail only, guarded by struct\_size.**

    Removing an actor and healing one already exist as `actor_action`'s `AACT_DESPAWN` and `AACT_HEAL`. A separate slot would do the same job twice, and two implementations eventually drift.

    Actor enumeration already exists as `list_actors` (everything in a dimension, with type names); combined with `actor_get_num` for positions it filters to a box. An `actors_in_box` slot would do the same job twice, and two implementations eventually drift.

    **Appended: the liquid layer (waterlogged blocks).**

    In Bedrock, waterlogging is not a block state but a second block in the same cell: stairs, fences or coral in the main layer and water in the liquid layer. `get_block` and `set_block` see the main layer only, so copying and pasting waterlogged stairs loses all the water: the main layer is exactly right and the other layer is missing.

    These two slots expose the liquid layer. An empty layer reads back as "minecraft:air".

    **Appended: bulk block reads and writes**

## Slots {#slots}

### `spawn_particle` {#spawn_particle}

```c
bool (*spawn_particle)(int32_t dimension, PierStr effect_name, double x, double y, double z);
```

Spawn a particle effect at a world coordinate. Used to outline a selection box edge-by-edge. Server thread only. Returns false if the level/dimension is not ready.

```text
dimension   : 0 = overworld, 1 = nether, 2 = the end.
effect_name : e.g. "minecraft:basic_flame_particle" or
              "minecraft:redstone_wire_dust_particle".
```

- Call: `api->spawn_particle(dimension, effect_name, x, y, z)`
- Parameters:
    - dimension : `int32_t`
    - effect_name : `PierStr`
    - x : `double`
    - y : `double`
    - z : `double`
- Return type: `bool`
- Section of abi.h: Appended
- Position in the table: slot 13, counting from 0
- Callers in each binding:
    - Rust: [`World::spawn_particle`](../rust/world.md#World.spawn_particle)
    - Go: [`SpawnParticle`](../go/world.md#SpawnParticle), [`Raw.SpawnParticle`](../go/raw.md#Raw.SpawnParticle)

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

Scan a cuboid region, corners inclusive (order-independent). For every cell in the box, `blocks_sink` is called with the block name + full SNBT. For every entity whose position lies within the box, `entities_sink` is called with the containing cell and the entity's SNBT. Both sinks run synchronously within this call; nothing is retained afterwards. Server thread only. Returns false if the level/dimension is not ready.

- Call: `api->scan_region(dimension, x1, y1, z1, x2, y2, z2, ctx, blocks_sink, entities_sink)`
- Parameters:
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
- Return type: `bool`
- Section of abi.h: Appended
- Position in the table: slot 15, counting from 0
- Callers in each binding:
    - Rust: [`World::scan`](../rust/world.md#World.scan), [`World::scan_with`](../rust/world.md#World.scan_with)
    - Go: [`ScanRegion`](../go/world.md#ScanRegion)

### `get_block` {#get_block}

```c
bool (*get_block)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierBlockSink sink);
```

Read one block: sink called once with (x,y,z, type name, full SNBT).

- Call: `api->get_block(dim, x, y, z, ctx, sink)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierBlockSink`
- Return type: `bool`
- Section of abi.h: §A world read/write & clock
- Position in the table: slot 16, counting from 0
- Callers in each binding:
    - Rust: [`Block::read`](../rust/block.md#Block.read)
    - Go: [`GetBlock`](../go/world.md#GetBlock), [`BlockAt.Info`](../go/block.md#BlockAt.Info)

### `set_block` {#set_block}

```c
bool (*set_block)(int32_t dim, int32_t x, int32_t y, int32_t z, PierStr block_spec);
```

Place a block natively (`BlockSource::setBlock`, DEFAULT update flags). `block_spec` = "minecraft:stone" / "stone" (default state) or a full {name,states,...} SNBT. Unknown names fail instead of placing a placeholder.

- Call: `api->set_block(dim, x, y, z, block_spec)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - block_spec : `PierStr`
- Return type: `bool`
- Section of abi.h: §A world read/write & clock
- Position in the table: slot 17, counting from 0
- Callers in each binding:
    - Rust: [`Block::set`](../rust/block.md#Block.set)
    - Go: [`BlockAt.Set`](../go/block.md#BlockAt.Set), [`Raw.SetBlock`](../go/raw.md#Raw.SetBlock)

### `get_time` {#get_time}

```c
bool (*get_time)(int64_t* out);
```

World time (`Level::getTime`).

- Call: `api->get_time(out)`
- Parameters:
    - out : `int64_t*`
- Return type: `bool`
- Section of abi.h: §A world read/write & clock
- Position in the table: slot 18, counting from 0
- Callers in each binding:
    - Rust: [`World::time`](../rust/world.md#World.time)
    - Go: [`Time`](../go/world.md#Time), [`Raw.GetTime`](../go/raw.md#Raw.GetTime)

### `set_time` {#set_time}

```c
bool (*set_time)(int64_t t);
```

Set world time natively (`Level::setTime`).

- Call: `api->set_time(t)`
- Parameters:
    - t : `int64_t`
- Return type: `bool`
- Section of abi.h: §A world read/write & clock
- Position in the table: slot 19, counting from 0
- Callers in each binding:
    - Rust: [`World::set_time`](../rust/world.md#World.set_time)
    - Go: [`SetTime`](../go/world.md#SetTime), [`Raw.SetTime`](../go/raw.md#Raw.SetTime)

### `set_weather` {#set_weather}

```c
bool (*set_weather)(int32_t weather);
```

0=clear 1=rain 2=thunder, native (`Level::updateWeather`).

- Call: `api->set_weather(weather)`
- Parameters:
    - weather : `int32_t`
- Return type: `bool`
- Section of abi.h: §A world read/write & clock
- Position in the table: slot 20, counting from 0
- Callers in each binding:
    - Rust: [`World::set_weather`](../rust/world.md#World.set_weather)
    - Go: [`SetWeather`](../go/world.md#SetWeather), [`Raw.SetWeather`](../go/raw.md#Raw.SetWeather)

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

`Level::explode`. source may be 0 (no source actor).

- Call: `api->explode(dim, x, y, z, radius, max_resistance, source, fire, breaks_blocks, allow_underwater)`
- Parameters:
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
- Return type: `bool`
- Section of abi.h: §C actors (players resolve here too, via player\_resolve)
- Position in the table: slot 38, counting from 0
- Callers in each binding:
    - Rust: [`World::explode`](../rust/world.md#World.explode)
    - Go: [`Explode`](../go/world.md#Explode), [`Raw.Explode`](../go/raw.md#Raw.Explode)

### `get_difficulty` {#get_difficulty}

```c
bool (*get_difficulty)(int32_t* out);
```

!!! note "Group note"

    Server / world-level settings.

`Level::getDifficulty`

- Call: `api->get_difficulty(out)`
- Parameters:
    - out : `int32_t*`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 72, counting from 0
- Callers in each binding:
    - Rust: [`World::difficulty`](../rust/world.md#World.difficulty)
    - Go: [`Difficulty`](../go/world.md#Difficulty), [`Raw.GetDifficulty`](../go/raw.md#Raw.GetDifficulty)

### `set_difficulty` {#set_difficulty}

```c
bool (*set_difficulty)(int32_t d);
```

!!! note "Group note"

    Server / world-level settings.

native `Level::setDifficulty`

- Call: `api->set_difficulty(d)`
- Parameters:
    - d : `int32_t`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 73, counting from 0
- Callers in each binding:
    - Rust: [`World::set_difficulty`](../rust/world.md#World.set_difficulty)
    - Go: [`SetDifficulty`](../go/world.md#SetDifficulty), [`Raw.SetDifficulty`](../go/raw.md#Raw.SetDifficulty)

### `get_seed` {#get_seed}

```c
bool (*get_seed)(int64_t* out);
```

!!! note "Group note"

    Server / world-level settings.

`Level::getLevelSeed64`

- Call: `api->get_seed(out)`
- Parameters:
    - out : `int64_t*`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 74, counting from 0
- Callers in each binding:
    - Rust: [`World::seed`](../rust/world.md#World.seed)
    - Go: [`Seed`](../go/world.md#Seed), [`Raw.GetSeed`](../go/raw.md#Raw.GetSeed)

### `game_rule_get` {#game_rule_get}

```c
bool (*game_rule_get)(PierStr name, void* ctx, PierStrSink sink);
```

!!! note "Group note"

    Server / world-level settings.

out sink receives SNBT {type:"bool"|"int"|"float", value:…}; false if unknown rule.

- Call: `api->game_rule_get(name, ctx, sink)`
- Parameters:
    - name : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 75, counting from 0
- Callers in each binding:
    - Rust: [`World::game_rule`](../rust/world.md#World.game_rule)
    - Go: [`GameRule`](../go/world.md#GameRule), [`Raw.GameRuleGet`](../go/raw.md#Raw.GameRuleGet)

### `game_rule_set` {#game_rule_set}

```c
bool (*game_rule_set)(PierStr name, PierStr value);
```

!!! note "Group note"

    Server / world-level settings.

/gamerule

- Call: `api->game_rule_set(name, value)`
- Parameters:
    - name : `PierStr`
    - value : `PierStr`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 76, counting from 0
- Callers in each binding:
    - Rust: [`World::set_game_rule`](../rust/world.md#World.set_game_rule)
    - Go: [`SetGameRule`](../go/world.md#SetGameRule), [`Raw.GameRuleSet`](../go/raw.md#Raw.GameRuleSet)

### `spawn_particle_for` {#spawn_particle_for}

```c
bool (*spawn_particle_for)(
    PierPlayerSel sel, int32_t dimension, PierStr effect_name, double x, double y, double z);
```

!!! note "Group note"

    Per-player particle packet (additive, gated by `struct_size`). Sends a SpawnParticleEffectPacket ONLY to the resolved player (`Player::sendNetworkPacket`) instead of `Level::spawnParticleEffect`'s dimension-wide broadcast — other clients never receive it. `dimension` is the vanilla dimension id carried in the packet; pass the dimension the coordinates refer to (normally the player's own — clients don't render particles for another dimension). False if the player is offline / can't be resolved.

- Call: `api->spawn_particle_for(sel, dimension, effect_name, x, y, z)`
- Parameters:
    - sel : `PierPlayerSel`
    - dimension : `int32_t`
    - effect_name : `PierStr`
    - x : `double`
    - y : `double`
    - z : `double`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 78, counting from 0
- Callers in each binding:
    - Rust: [`Player::spawn_particle`](../rust/player.md#Player.spawn_particle)
    - Go: [`Raw.SpawnParticleFor`](../go/raw.md#Raw.SpawnParticleFor)

### `villages` {#villages}

```c
void (*villages)(int32_t dimension, void* ctx, PierStrSink snbt_sink);
```

!!! note "Group note"

    Read-only world-data queries (additive, gated by `struct_size`). Both stream one SNBT object per result through the sink; observational only. Server thread only.

Enumerate villages in a dimension. Each: {uuid, center:\[x,y,z\], bounds:{min,max}, `poi_count`}.

- Call: `api->villages(dimension, ctx, snbt_sink)`
- Parameters:
    - dimension : `int32_t`
    - ctx : `void*`
    - snbt_sink : `PierStrSink`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 89, counting from 0
- Callers in each binding:
    - Rust: [`World::try_villages`](../rust/world.md#World.try_villages), [`World::villages`](../rust/world.md#World.villages)
    - Go: [`Raw.Villages`](../go/raw.md#Raw.Villages)

### `structures_near` {#structures_near}

```c
void (*structures_near)(
    int32_t dimension, int32_t x, int32_t y, int32_t z, int32_t radius, void* ctx,
    PierStrSink snbt_sink);
```

!!! note "Group note"

    Read-only world-data queries (additive, gated by `struct_size`). Both stream one SNBT object per result through the sink; observational only. Server thread only.

Hardcoded spawn areas (nether fortress / witch hut / ocean monument / pillager outpost) whose chunks intersect a radius around (x,y,z). Each: {type, bounds:{min,max}}. Only LOADED chunks are inspected — a read-only query never force-loads.

- Call: `api->structures_near(dimension, x, y, z, radius, ctx, snbt_sink)`
- Parameters:
    - dimension : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - radius : `int32_t`
    - ctx : `void*`
    - snbt_sink : `PierStrSink`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 90, counting from 0
- Callers in each binding:
    - Rust: [`World::try_structures_near`](../rust/world.md#World.try_structures_near), [`World::structures_near`](../rust/world.md#World.structures_near)
    - Go: [`Raw.StructuresNear`](../go/raw.md#Raw.StructuresNear)

### `level_get_biome` {#level_get_biome}

```c
bool (*level_get_biome)(int32_t dim, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
```

- Call: `api->level_get_biome(dim, x, y, z, ctx, sink)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Level: biome, spawn, save, weather, path, sleep (dedicated fns)
- Position in the table: slot 129, counting from 0
- Callers in each binding:
    - Rust: [`World::biome`](../rust/world.md#World.biome)
    - Go: [`BlockAt.Biome`](../go/block.md#BlockAt.Biome), [`Raw.LevelGetBiome`](../go/raw.md#Raw.LevelGetBiome)

### `level_get_default_spawn` {#level_get_default_spawn}

```c
bool (*level_get_default_spawn)(int32_t* x, int32_t* y, int32_t* z);
```

- Call: `api->level_get_default_spawn(x, y, z)`
- Parameters:
    - x : `int32_t*`
    - y : `int32_t*`
    - z : `int32_t*`
- Return type: `bool`
- Section of abi.h: Level: biome, spawn, save, weather, path, sleep (dedicated fns)
- Position in the table: slot 130, counting from 0
- Callers in each binding:
    - Rust: [`World::default_spawn`](../rust/world.md#World.default_spawn)
    - Go: [`DefaultSpawn`](../go/world.md#DefaultSpawn), [`Raw.LevelGetDefaultSpawn`](../go/raw.md#Raw.LevelGetDefaultSpawn)

### `level_set_default_spawn` {#level_set_default_spawn}

```c
bool (*level_set_default_spawn)(int32_t x, int32_t y, int32_t z);
```

- Call: `api->level_set_default_spawn(x, y, z)`
- Parameters:
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
- Return type: `bool`
- Section of abi.h: Level: biome, spawn, save, weather, path, sleep (dedicated fns)
- Position in the table: slot 131, counting from 0
- Callers in each binding:
    - Rust: [`World::set_default_spawn`](../rust/world.md#World.set_default_spawn)
    - Go: [`SetDefaultSpawn`](../go/world.md#SetDefaultSpawn), [`Raw.LevelSetDefaultSpawn`](../go/raw.md#Raw.LevelSetDefaultSpawn)

### `level_save` {#level_save}

```c
bool (*level_save)(void);
```

- Call: `api->level_save()`
- Return type: `bool`
- Section of abi.h: Level: biome, spawn, save, weather, path, sleep (dedicated fns)
- Position in the table: slot 132, counting from 0
- Callers in each binding:
    - Rust: [`World::save`](../rust/world.md#World.save)
    - Go: [`SaveLevel`](../go/world.md#SaveLevel), [`Raw.LevelSave`](../go/raw.md#Raw.LevelSave)

### `level_get_sleep_status` {#level_get_sleep_status}

```c
bool (*level_get_sleep_status)(void* ctx, PierStrSink sink);
```

SNBT {sleeping, `total_players`, `active_sleeping`}

- Call: `api->level_get_sleep_status(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Level: biome, spawn, save, weather, path, sleep (dedicated fns)
- Position in the table: slot 133, counting from 0
- Callers in each binding:
    - Rust: [`World::sleep_status`](../rust/world.md#World.sleep_status)
    - Go: [`Raw.LevelGetSleepStatus`](../go/raw.md#Raw.LevelGetSleepStatus)

### `level_update_weather` {#level_update_weather}

```c
bool (*level_update_weather)(float rain_level, int32_t rain_time, float lightning_level, int32_t lightning_time);
```

- Call: `api->level_update_weather(rain_level, rain_time, lightning_level, lightning_time)`
- Parameters:
    - rain_level : `float`
    - rain_time : `int32_t`
    - lightning_level : `float`
    - lightning_time : `int32_t`
- Return type: `bool`
- Section of abi.h: Level: biome, spawn, save, weather, path, sleep (dedicated fns)
- Position in the table: slot 134, counting from 0
- Callers in each binding:
    - Rust: [`World::update_weather`](../rust/world.md#World.update_weather)
    - Go: [`Raw.LevelUpdateWeather`](../go/raw.md#Raw.LevelUpdateWeather)

### `level_find_path` {#level_find_path}

```c
bool (*level_find_path)(PierActorId id, int32_t x, int32_t y, int32_t z, void* ctx, PierStrSink sink);
```

SNBT {nodes:\[{x,y,z},…\], reached:1b/0b}. NULL on every current host: pathfinding is not implemented.

- Call: `api->level_find_path(id, x, y, z, ctx, sink)`
- Parameters:
    - id : `PierActorId`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Level: biome, spawn, save, weather, path, sleep (dedicated fns)
- Position in the table: slot 135, counting from 0
- Callers in each binding:
    - Rust: [`World::find_path`](../rust/world.md#World.find_path)
    - Go: [`Raw.LevelFindPath`](../go/raw.md#Raw.LevelFindPath)

### `level_delete_chunk_keys` {#level_delete_chunk_keys}

```c
int32_t (*level_delete_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z);
```

Delete every save-file key belonging to one chunk, so the engine regenerates it from the generator on next load.

Restoring an area by writing every cell with `set_block` is the wrong approach: a 32x32 plot times the world height is hundreds of thousands of cells and as many FFI crossings, and it still misses things, because block entities, actors and pending ticks (redstone, crop growth) are not block data. After such a rewrite the chests are still there and the redstone is still running.

Erasing the save keys has neither problem: one forEachKeyWithPrefix yields every key of the chunk (all tags, all subchunks, actors, block entities, pending ticks), one pass deletes them, and the engine regenerates from the generator on next load.

Key shape: a BDS chunk key is prefixed with &lt;chunkX:i32 LE&gt;&lt;chunkZ:i32 LE&gt;, followed by &lt;dimension:i32 LE&gt; outside the overworld. After the prefix come a tag byte and a subchunk index, which this slot does not interpret; deleting everything with the prefix is exactly "everything in this chunk".

The chunk must be unloaded. A loaded chunk has a LevelChunk in memory that the engine writes back on unload, recreating the deleted keys verbatim, so the deletion is silently undone. The caller is responsible for getting the chunk unloaded first (move players away, wait for it to leave tick range). This slot does not do that: deciding who is nearby and when unloading is acceptable needs the caller's domain knowledge, which this layer must not have.

@return number of keys deleted; -1 if the save layer is unavailable. 0 is

```text
a normal result, meaning that chunk was never generated.
```

Pure append: `PIER_ABI_VERSION` is unchanged, `struct_size` is the gate.

- Call: `api->level_delete_chunk_keys(dim, chunk_x, chunk_z)`
- Parameters:
    - dim : `int32_t`
    - chunk_x : `int32_t`
    - chunk_z : `int32_t`
- Return type: `int32_t`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 178, counting from 0
- Callers in each binding:
    - Rust: [`World::delete_chunk_keys`](../rust/world.md#World.delete_chunk_keys)
    - Go: [`Raw.LevelDeleteChunkKeys`](../go/raw.md#Raw.LevelDeleteChunkKeys)

### `level_chunks_loaded` {#level_chunks_loaded}

```c
int32_t (*level_chunks_loaded)(int32_t dim, int32_t min_x, int32_t min_z, int32_t max_x, int32_t max_z);
```

Are the chunks covering \[min..max\] currently loaded in memory?

Companion to `level_delete_chunk_keys`: erasing save keys only works on unloaded chunks, since a loaded one lives in memory and writes the deleted keys back verbatim on unload, while the erase itself "succeeds" and reports a positive key count. Without this slot a caller can only guess from "nobody is nearby", and guessing wrong fails silently.

@return 1 if all are loaded, 0 if at least one is not, -1 if the dimension

```text
is unavailable.
```

Pure append: `PIER_ABI_VERSION` is unchanged, `struct_size` is the gate.

- Call: `api->level_chunks_loaded(dim, min_x, min_z, max_x, max_z)`
- Parameters:
    - dim : `int32_t`
    - min_x : `int32_t`
    - min_z : `int32_t`
    - max_x : `int32_t`
    - max_z : `int32_t`
- Return type: `int32_t`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 179, counting from 0
- Callers in each binding:
    - Rust: [`World::chunks_loaded`](../rust/world.md#World.chunks_loaded)
    - Go: [`Raw.LevelChunksLoaded`](../go/raw.md#Raw.LevelChunksLoaded)

### `level_chunk_keys` {#level_chunk_keys}

```c
int32_t (*level_chunk_keys)(int32_t dim, int32_t chunk_x, int32_t chunk_z, void* ctx, PierStrSink sink);
```

List every save-file key belonging to one chunk. One callback per key.

Listing and deleting are two slots: the host accumulates no container of keys across a call, so the caller keeps the keys it wants and deletes each through `level_delete_key`. A host-side string container held across the engine's virtual calls is the shape that corrupts the heap here.

Keys are binary and contain 0 bytes, hence PierStr with an explicit length rather than a C string.

@return how many keys were reported; -1 if the save layer is unavailable.

- Call: `api->level_chunk_keys(dim, chunk_x, chunk_z, ctx, sink)`
- Parameters:
    - dim : `int32_t`
    - chunk_x : `int32_t`
    - chunk_z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `int32_t`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 181, counting from 0
- Callers in each binding:
    - Rust: [`World::chunk_keys`](../rust/world.md#World.chunk_keys)
    - Go: [`Raw.LevelChunkKeys`](../go/raw.md#Raw.LevelChunkKeys)

### `level_delete_key` {#level_delete_key}

```c
bool (*level_delete_key)(PierStr key);
```

Delete one chunk-category key, verbatim.

Companion to `level_chunk_keys`. The key's content is not interpreted: whatever is passed is what gets deleted, which is exactly why it is safe, since it need not understand the subchunk format.

- Call: `api->level_delete_key(key)`
- Parameters:
    - key : `PierStr`
- Return type: `bool`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 182, counting from 0
- Callers in each binding:
    - Rust: [`World::delete_key`](../rust/world.md#World.delete_key)
    - Go: [`Raw.LevelDeleteKey`](../go/raw.md#Raw.LevelDeleteKey)

### `level_set_biome` {#level_set_biome}

```c
int32_t (*level_set_biome)(int32_t dim,
                           int32_t minX, int32_t minZ,
                           int32_t maxX, int32_t maxZ,
                           PierStr biome);
```

Set the biome over an area.

Applied per whole column, so no y is taken: setBiome3d works per y, but Bedrock stores biomes per column. biome is a biome name such as "minecraft:plains".

Returns how many columns were set. 0 means none were, either because the chunks are not loaded or because the name was not recognized.

- Call: `api->level_set_biome(dim, minX, minZ, maxX, maxZ, biome)`
- Parameters:
    - dim : `int32_t`
    - minX : `int32_t`
    - minZ : `int32_t`
    - maxX : `int32_t`
    - maxZ : `int32_t`
    - biome : `PierStr`
- Return type: `int32_t`
- Section of abi.h: Appended slots. Added at the tail only, guarded by struct\_size.
- Position in the table: slot 183, counting from 0
- Callers in each binding:
    - Rust: [`World::set_biome`](../rust/world.md#World.set_biome)
    - Go: [`SetBiome`](../go/world.md#SetBiome), [`Raw.LevelSetBiome`](../go/raw.md#Raw.LevelSetBiome)

### `get_extra_block` {#get_extra_block}

```c
bool (*get_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                        void* ctx, PierStrSink sink);
```

Read the liquid layer. The sink receives a block name such as "minecraft:water"; an empty layer gives air.

- Call: `api->get_extra_block(dim, x, y, z, ctx, sink)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Appended: the liquid layer (waterlogged blocks).
- Position in the table: slot 184, counting from 0
- Callers in each binding:
    - Rust: [`Block::extra`](../rust/block.md#Block.extra)
    - Go: [`Raw.GetExtraBlock`](../go/raw.md#Raw.GetExtraBlock)

### `set_extra_block` {#set_extra_block}

```c
bool (*set_extra_block)(int32_t dim, int32_t x, int32_t y, int32_t z,
                        PierStr block_spec, int32_t update_flags);
```

Write the liquid layer. `block_spec` takes a bare block name or full SNBT; write "minecraft:air" to clear it. `update_flags` is as in `edit_set_block_nbt`: bit 1 notifies neighbours, bit 2 syncs the client.

- Call: `api->set_extra_block(dim, x, y, z, block_spec, update_flags)`
- Parameters:
    - dim : `int32_t`
    - x : `int32_t`
    - y : `int32_t`
    - z : `int32_t`
    - block_spec : `PierStr`
    - update_flags : `int32_t`
- Return type: `bool`
- Section of abi.h: Appended: the liquid layer (waterlogged blocks).
- Position in the table: slot 185, counting from 0
- Callers in each binding:
    - Rust: [`Block::set_extra`](../rust/block.md#Block.set_extra)
    - Go: [`Raw.SetExtraBlock`](../go/raw.md#Raw.SetExtraBlock)

### `scan_region_indexed` {#scan_region_indexed}

```c
bool (*scan_region_indexed)(int32_t dimension, int32_t x1, int32_t y1, int32_t z1,
                            int32_t x2, int32_t y2, int32_t z2, void* ctx,
                            PierPaletteSink palette, PierCellSink cells);
```

As `scan_region` for blocks, but each distinct block state is serialized once through the palette sink and every cell reports only an index. A region of one million stone cells costs one SNBT serialization instead of one million. Entities are not covered; use `scan_region` with a null block sink for those. The same 2^24-cell limit applies. Server thread only. Returns false if the dimension is not ready or the region is too large.

- Call: `api->scan_region_indexed(dimension, x1, y1, z1, x2, y2, z2, ctx, palette, cells)`
- Parameters:
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
- Return type: `bool`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 189, counting from 0
- Callers in each binding:
    - Rust: [`World::scan_indexed`](../rust/world.md#World.scan_indexed)
    - Go: [`ScanRegionIndexed`](../go/world.md#ScanRegionIndexed)
