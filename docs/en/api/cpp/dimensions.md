# Custom dimensions

??? note "Section notes in abi.h"

    **Capability group: custom dimensions (md\_\*). All NULL when pier-dimensions**

    was not built into the host.

    **Plot-boundary confinement**

    Backing store for `PIER_DIMRULE_PISTON_CROSS_CELL` and `PIER_DIMRULE_ENTITY_CROSS_CELL`. Those two rules ask "are these two columns in the same plot?", and the answer needs the grid geometry plus the merge markers. The question is asked from `PistonBlockActor::_checkAttachedBlocks` and `Actor::move` — engine tick paths, hundreds of calls a second — so the data is pushed here once and read natively rather than queried back across the FFI.

    The ownership rule implemented on the loader side mirrors the plugin's own `owning_plot`: a seam between two merged plots counts as plot, a junction counts as plot only when all four surrounding edges are merged. Divergence does not show up as "one column judged wrong" — it shows up as an owner who can place a block by hand on their merged plot but whose piston refuses to push there. Server thread only.

    **Same-toolchain fast lane, appended and struct\_size-gated.**

    Five slots appended without touching `PIER_ABI_VERSION`: a pure append is not a version change, and `struct_size` is the precise gate.

    Both directions hold. A new loader running an old mod: the old table is a byte-identical prefix of the new one, the mod cannot reach these five slots, and it works unchanged. A new mod on an old loader: SDK runtime init compares `struct_size`, finds the loader's table shorter than the one it was compiled against, and refuses to load. That is the right outcome, since a mod that reads the `lane_publish` cell on a loader without it would read out of bounds.

    In short: the version number tracks "semantics changed", `struct_size` tracks "the table grew". This change is only the latter.

    See the long comment at PierLaneDesc above. In one line: service is the cross-language (name, JSON) -&gt; JSON channel, while this is a direct function-table call that holds only when both sides were built by the same toolchain; a fingerprint mismatch yields no pointer and the consumer falls back to service.

    Server thread only.

    **Appended: bulk block reads and writes**

    **Dimensions whose terrain belongs to the mod**

    The three vanilla generators and the void are the engine's own, and this host serves them because they cost it nothing to serve. Everything past that -- a layer stack, a grid of plots, a noise field, a binary terrain format and the code that reads it -- is a mod's, and this host does not want to know its shape. These two slots are the whole of what it needs to know.

## Slots {#slots}

### `md_is_available` {#md_is_available}

```c
bool (*md_is_available)(void);
```

Whether this host can register custom dimensions. NULL when pier-dimensions was not built in. Filled in, it answers from a probe of the engine's dimension definition table, so it can be false on an engine whose layout this build does not match; the answer is cached after the first call that can make it.

- Call: `api->md_is_available()`
- Return type: `bool`
- Section of abi.h: Capability group: custom dimensions (md\_\*). All NULL when pier-dimensions
- Position in the table: slot 146, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::is_available`](../rust/dimensions.md#fn.is_available), [`dimensions::is_available`](../rust/dimensions.md#fn.is_available)
    - Go: [`DimensionsAvailable`](../go/dimensions.md#DimensionsAvailable), [`Raw.MdIsAvailable`](../go/raw.md#Raw.MdIsAvailable)

### `md_set_dimension_rule` {#md_set_dimension_rule}

```c
void (*md_set_dimension_rule)(int32_t dimension, int32_t rule, bool allow);
```

Per-dimension rules, consulted by the loader's own hooks.

Why this exists instead of gamerules: Bedrock gamerules are server-wide. Setting `doMobSpawning=false` to quiet a creative plot world also stops spawning in the survival world. These flags are checked inside hooks on the actual call sites (`Spawner::spawnMob`, `Level::explode`, ...), so they really are per-dimension.

`rule` is one of PierDimRule. Setting a rule on a dimension the loader doesn't know about is harmless — the tables are keyed by raw dimension id and consulted only when that id shows up in a hook.

Dimensions with no entry are left completely alone: the hooks fall through to origin(), so vanilla dimensions keep vanilla behavior without the caller having to opt out.

- Call: `api->md_set_dimension_rule(dimension, rule, allow)`
- Parameters:
    - dimension : `int32_t`
    - rule : `int32_t`
    - allow : `bool`
- Section of abi.h: Capability group: custom dimensions (md\_\*). All NULL when pier-dimensions
- Position in the table: slot 147, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::set_rule`](../rust/dimensions.md#fn.set_rule)
    - Go: [`SetDimensionRule`](../go/dimensions.md#SetDimensionRule), [`Raw.MdSetDimensionRule`](../go/raw.md#Raw.MdSetDimensionRule)

### `md_get_dimension_rule` {#md_get_dimension_rule}

```c
bool (*md_get_dimension_rule)(int32_t dimension, int32_t rule, bool* outAllow);
```

Read back a rule. `outAllow` is only written when the dimension has an explicit entry for that rule; returns false otherwise. A rule number this host does not know is answered false as well, and `md_set_dimension_rule` ignores one, so a binding newer than the host cannot tell "not set" from "not supported" here.

- Call: `api->md_get_dimension_rule(dimension, rule, outAllow)`
- Parameters:
    - dimension : `int32_t`
    - rule : `int32_t`
    - outAllow : `bool*`
- Return type: `bool`
- Section of abi.h: Capability group: custom dimensions (md\_\*). All NULL when pier-dimensions
- Position in the table: slot 148, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::rule`](../rust/dimensions.md#fn.rule)
    - Go: [`DimensionRule`](../go/dimensions.md#DimensionRule), [`Raw.MdGetDimensionRule`](../go/raw.md#Raw.MdGetDimensionRule)

### `md_clear_dimension_rules` {#md_clear_dimension_rules}

```c
void (*md_clear_dimension_rules)(int32_t dimension);
```

Drop every rule for a dimension (used when a world is deleted).

- Call: `api->md_clear_dimension_rules(dimension)`
- Parameters:
    - dimension : `int32_t`
- Section of abi.h: Capability group: custom dimensions (md\_\*). All NULL when pier-dimensions
- Position in the table: slot 149, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::clear_rules`](../rust/dimensions.md#fn.clear_rules)
    - Go: [`Raw.MdClearDimensionRules`](../go/raw.md#Raw.MdClearDimensionRules)

### `md_get_dimension_id` {#md_get_dimension_id}

```c
int32_t (*md_get_dimension_id)(PierStr name);
```

Resolve a dimension name to its id. Returns -1 if not found.

Only returns an id for names that are ACTUALLY registered: unknown names yield -1, never `VanillaDimensions::Undefined()` (whose numeric value is mutated at runtime and looks like a valid id).

This is rarely the right call. `md_add_dimension` and `md_add_dimension_pack` are idempotent, so re-registering the same name on a later boot returns the same persisted id, and a caller registers unconditionally at startup instead of probing first.

- Call: `api->md_get_dimension_id(name)`
- Parameters:
    - name : `PierStr`
- Return type: `int32_t`
- Section of abi.h: Capability group: custom dimensions (md\_\*). All NULL when pier-dimensions
- Position in the table: slot 150, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::dimension_id`](../rust/dimensions.md#fn.dimension_id), [`dimensions::dimension_id`](../rust/dimensions.md#fn.dimension_id)
    - Go: [`DimensionID`](../go/dimensions.md#DimensionID), [`Raw.MdGetDimensionId`](../go/raw.md#Raw.MdGetDimensionId)

### `md_set_plot_merges` {#md_set_plot_merges}

```c
void (*md_set_plot_merges)(int32_t dimension, int32_t const* entries, int32_t count);
```

Replace a dimension's merge markers wholesale. `entries` is `count` triples `(x, z, mask)`, i.e. `count * 3` int32s; `mask` is a bitset of 1=north, 2=east, 4=south, 8=west matching the plugin's `merged[]` indices. Only plots that actually carry a marker need to be sent.

Wholesale, not incremental: incremental requires both sides to agree forever on what is currently in the table, and `unlink` clears the neighbour before storing itself — a failure in between leaves the two views apart with no way back. Replacing pulls them into agreement on every push. The grid comes from the template pack's CONF section at `md_add_dimension_pack`; a push for a dimension without one is dropped with a warning. Cell geometry, which the mod side must match: with period = cell + gap, a column at world (x, z) is inside a cell when mod(x,period) &lt; cell && mod(z,period) &lt; cell.

- Call: `api->md_set_plot_merges(dimension, entries, count)`
- Parameters:
    - dimension : `int32_t`
    - entries : `int32_t const*`
    - count : `int32_t`
- Section of abi.h: Plot-boundary confinement
- Position in the table: slot 162, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::set_plot_merges`](../rust/dimensions.md#fn.set_plot_merges)
    - Go: [`Raw.MdSetPlotMerges`](../go/raw.md#Raw.MdSetPlotMerges)

### `md_list_dimensions` {#md_list_dimensions}

```c
void (*md_list_dimensions)(void* ctx, PierStrSink sink);
```

List every registered custom dimension as a JSON array: \[{"name":"`plot_world`","dim":1000,"snbt":"{…}"}\].

Without this slot the md\_\* family can only be queried by name (`md_get_dimension_id`), so a caller must already know the name. A world manager taking over an existing save would then be blind to dimensions created by a previous plugin: they sit in `dimension_config`.json, they are alive in the engine, players can teleport into them, and the manager's table has no row for them. The consequence is not a short listing but dimensions governed by no rules at all, plus the risk that a newly created world is assigned a number that collides with one of them, leaving two worlds sharing a dimension id.

name comes from the config file key, dim is the engine-assigned and persisted number, and snbt is the verbatim generation parameters (a plot world gives {layout:{…},seed:N}, a simple world {generatorType:Flat, seed:N}) for the caller to interpret.

The sink is invoked ONCE PER DIMENSION, each with one JSON object -- not once with an array. Contrast `lane_list` above, which hands over a single JSON array. Both shapes exist in this table; each slot says which.

The callback is not invoked at all when `md_is_available()` is false.

- Call: `api->md_list_dimensions(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Section of abi.h: Same-toolchain fast lane, appended and struct\_size-gated.
- Position in the table: slot 177, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::list`](../rust/dimensions.md#fn.list), [`dimensions::list`](../rust/dimensions.md#fn.list), [`World::fill_blocks`](../rust/world.md#World.fill_blocks), [`World::add_ticking_area`](../rust/world.md#World.add_ticking_area), [`World::remove_ticking_area`](../rust/world.md#World.remove_ticking_area), [`World::list_ticking_areas`](../rust/world.md#World.list_ticking_areas)
    - Go: [`ListDimensions`](../go/dimensions.md#ListDimensions), [`Raw.MdListDimensions`](../go/raw.md#Raw.MdListDimensions)

### `md_add_dimension` {#md_add_dimension}

```c
int32_t (*md_add_dimension)(PierStr name, PierStr spec_snbt);
```

Add a custom dimension with a native terrain from one declarative spec.

`spec_snbt` is a CompoundTag SNBT string:

```text
{seed:123,
 height:{min:-64,max:320},            optional, both multiples of 16
 sky:{client:"overworld"|"nether"|"end", skylight:true, weather:true,
      time:12000},                       optional, 0..23999
 sky.time holds this dimension at one tick of day and leaves every other
 dimension on the level clock. A nether or end client sky has no day cycle to
 begin with, so the field only changes what an overworld sky shows.
 terrain:{kind:"native",
          generator:"overworld"|"nether"|"end"|"flat"|"void",
          biome:"minecraft:plains"}       biome is read for void only
```

The three vanilla generators carry their structures (villages, fortresses, end cities); what differs from the vanilla dimension is the seed, the height and the sky.

`sky.client` is what the client is told in DimensionDefinition: nether and end skies have no day/night, which is how those dimensions lock time. It is independent of the server-side generator.

A terrain of kind template or volume is refused here with -1: those go through `md_add_dimension_pack`, which verifies the pack before the spec is stored. A height not on a subchunk boundary or a generator name the host does not know refuses the same way; nothing falls back to a "close enough" generator, because the spec is persisted with the dimension and terrain generated from a wrong spec cannot be regenerated.

Idempotent by name. Returns dim id (&gt;=3) or -1.

- Call: `api->md_add_dimension(name, spec_snbt)`
- Parameters:
    - name : `PierStr`
    - spec_snbt : `PierStr`
- Return type: `int32_t`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 193, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::add_dimension`](../rust/dimensions.md#fn.add_dimension)
    - Go: [`AddDimension`](../go/dimensions.md#AddDimension), [`Raw.MdAddDimension`](../go/raw.md#Raw.MdAddDimension)

### `md_add_dimension_pack` {#md_add_dimension_pack}

```c
int32_t (*md_add_dimension_pack)(PierStr name, PierStr config_path, PierStr spec_snbt);
```

Add a custom dimension whose terrain is a terrain pack: a directory with a config file and a binary built by tools/pier-pack. `config_path` names the config relative to the server root, forward slashes, no ".." and not absolute; the binary is named inside the config relative to it.

The host reads four keys of the config and nothing else:

```text
{"pier_terrain":1, "type":"template"|"volume",
 "binary":"terrain.ptpl", "sha256":"<64 hex digits>"}
```

Anything else in the file belongs to the mod (a title, a description, a preview) and the host never looks at it.

`spec_snbt` has the shape of `md_add_dimension` with a pack terrain:

```text
terrain:{kind:"template"|"volume",
         params:{plot_size:64, road_width:7},   template parameters, ints
         roles:{floor:"minecraft:stone"}}        template role overrides
```

A parameter the pack marks fixed cannot be given another value; one it marks derived cannot be given at all; a free one must lie in its range and on its step; a choice one must be one of its choices. A role not in the pack, or a block that is not registered, refuses. The pack's own constraints are checked with the bound values and a failing one refuses with its message in the log.

Three sources are compared before anything is stored: spec terrain.kind, the config's type, and the binary's magic, and the binary must hash to the config's sha256. The stored spec then holds the path, that hash, every bound parameter and the role overrides, so the terrain is regenerable from the save alone.

On a later boot the stored spec wins: the same name returns the same id, the parameters given now are ignored with a warning if they differ, and a config whose sha256 is no longer the stored one refuses with `PIER_PACK_STORED_MISMATCH`, since terrain from a different binary cannot continue a world. Editing a pack means a new world or a new name.

A template pack with a CONF section registers the cell grid for the confinement rules from the same mount that produced the terrain, so the two can never disagree; `md_set_plot_merges` then applies.

Returns dim id (&gt;=3), or one of the PIER\_PACK\_\* codes below, all negative.

- Call: `api->md_add_dimension_pack(name, config_path, spec_snbt)`
- Parameters:
    - name : `PierStr`
    - config_path : `PierStr`
    - spec_snbt : `PierStr`
- Return type: `int32_t`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 194, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::add_dimension_pack`](../rust/dimensions.md#fn.add_dimension_pack)
    - Go: [`Raw.MdAddDimensionPack`](../go/raw.md#Raw.MdAddDimensionPack)

### `md_pack_inspect` {#md_pack_inspect}

```c
int32_t (*md_pack_inspect)(PierStr config_path, void* ctx, PierStrSink sink);
```

What a pack asks for, without registering anything: the sink receives one JSON document, the same shape `pier-pack inspect` prints:

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

A mod builds its form from "params": free is a slider, choice a list, fixed a read-only value, derived is not shown. On a refusal the sink receives {"ok":false,"status":&lt;code&gt;,"problems":\["..."\]} and the code is returned. Returns 0 or a PIER\_PACK\_\* code. Server thread only.

- Call: `api->md_pack_inspect(config_path, ctx, sink)`
- Parameters:
    - config_path : `PierStr`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `int32_t`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 195, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::pack_inspect`](../rust/dimensions.md#fn.pack_inspect)
    - Go: [`Raw.MdPackInspect`](../go/raw.md#Raw.MdPackInspect)

### `md_retire_dimension` {#md_retire_dimension}

```c
bool (*md_retire_dimension)(PierStr name);
```

Retire a custom dimension: drop it from `dimension_config`.json, from the host's own tables and from the dimension factory, so nothing registers it on the next boot and it stops appearing in `md_list_dimensions`.

This is not a delete. The chunks the dimension wrote are still in the save, the engine still holds the id it was given for this session, and a player standing in it is not moved. What ends is the host's willingness to register the name again from its own config.

The id is not returned to the pool. A retired name registered again is a new dimension and gets a fresh id, which leaves the old chunks orphaned on disk and costs space. Handing the number back instead would point the new dimension at the terrain of the old one, and nothing about that is recoverable, so the cheaper mistake is the one this makes.

True while the name was known and has been retired, false when the host had no such dimension, which is also what a second call reports.

- Call: `api->md_retire_dimension(name)`
- Parameters:
    - name : `PierStr`
- Return type: `bool`
- Section of abi.h: Appended: bulk block reads and writes
- Position in the table: slot 196, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::retire_dimension`](../rust/dimensions.md#fn.retire_dimension)
    - Go: [`Raw.MdRetireDimension`](../go/raw.md#Raw.MdRetireDimension)

### `md_add_dimension_generated` {#md_add_dimension_generated}

```c
int32_t (*md_add_dimension_generated)(PierStr name, PierStr spec_snbt,
                                      PierStr material_palette, PierStr biome_palette,
                                      PierGenerateChunkFn fn, void* user);
```

Register a dimension the calling mod fills itself.

`spec_snbt` is the shape of `md_add_dimension` without its terrain section:

```text
{seed:<u32>,
 height:{min:<int>,max:<int>},        multiples of 16, inside the world range
 sky:{client:"overworld"|"nether"|"end", skylight:<bool>, weather:<bool>,
      time:<0..23999, optional>}}
```

A terrain section here is refused: this host does not read one, and accepting a field it ignores is how a spec comes to describe a world nobody generates.

`material_palette` and `biome_palette` are newline-separated names. Material index 0 must be air and is what an untouched column holds. Every name is resolved once, here: a name outside the block or biome registry refuses the registration rather than becoming a hole in the terrain later.

`fn` is called on chunk worker threads; the contract is on PierGenerateChunkFn and is not the usual one. `user` is passed back untouched and is never read.

The dimension, its id and its spec are persisted exactly as `md_add_dimension` persists them, so it comes back on the next boot -- but the terrain does not until the same mod registers it again. A dimension whose mod is gone loads as a void, keeps its chunks and says so once.

Idempotent by name. Returns dim id (&gt;=3) or -1.

- Call: `api->md_add_dimension_generated(name, spec_snbt, material_palette, biome_palette, fn, user)`
- Parameters:
    - name : `PierStr`
    - spec_snbt : `PierStr`
    - material_palette : `PierStr`
    - biome_palette : `PierStr`
    - fn : `PierGenerateChunkFn`
    - user : `void*`
- Return type: `int32_t`
- Section of abi.h: Dimensions whose terrain belongs to the mod
- Position in the table: slot 198, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::add_dimension_generated`](../rust/dimensions.md#fn.add_dimension_generated)
    - Go: [`AddGeneratedDimension`](../go/dimensions.md#AddGeneratedDimension)

### `md_set_dimension_cells` {#md_set_dimension_cells}

```c
bool (*md_set_dimension_cells)(int32_t dim_id, int32_t cell, int32_t gap);
```

Give a generated dimension the cell geometry its confinement rules use.

`PIER_DIMRULE_PISTON_CROSS_CELL` and `PIER_DIMRULE_ENTITY_CROSS_CELL` ask whether two positions are in the same cell, and that needs the grid. The geometry belongs to whatever the mod generated, so the mod states it; the hooks stay here because they are engine hooks. `cell` and `gap` are in blocks, `cell` positive; a gap of zero means the cells touch. Passing cell 0 removes the geometry and with it any answer the two rules could give.

Returns false when the id is not a dimension this host registered.

- Call: `api->md_set_dimension_cells(dim_id, cell, gap)`
- Parameters:
    - dim_id : `int32_t`
    - cell : `int32_t`
    - gap : `int32_t`
- Return type: `bool`
- Section of abi.h: Dimensions whose terrain belongs to the mod
- Position in the table: slot 199, counting from 0
- Callers in each binding:
    - Rust: [`dimensions::set_dimension_cells`](../rust/dimensions.md#fn.set_dimension_cells)
    - Go: [`Raw.MdSetDimensionCells`](../go/raw.md#Raw.MdSetDimensionCells)

## Types {#types}

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

One chunk asked of a mod that supplies its own terrain.

`out_materials` is 256 \* height entries, indexed (x \* 16 + z) \* height + y, y counted up from `min_y`. `out_biomes` is 256 entries, one per column. Both hold indices into the palettes given at registration; index 0 of the material palette is air. The host owns both buffers and reuses them, so a callback writes and keeps nothing.

The host resolves the indices to blocks and biomes once, at registration. That is why the terrain never crosses this boundary as block names: a chunk is 98k lookups, and doing them per chunk is the difference between a generator and a stall.

### `PierGenerateChunkFn` {#PierGenerateChunkFn}

```c
typedef int32_t (*PierGenerateChunkFn)(void* user, const PierChunkRequest* request);
```

Fills one chunk. Non-zero means filled; zero means the mod could not, and the host writes air and says so once per dimension.

THREADING, and this one is the opposite of the rest of this file: the host calls this on its CHUNK WORKER THREADS, several at once, for different chunks of the same dimension. The host serializes nothing.

```text
- It must be safe to run concurrently with itself.
- It must give the same answer for the same (dim_id, chunk_x, chunk_z) forever:
  chunks are generated once and saved, and a neighbour generated later from a
  different answer leaves a seam that no later edit can remove.
- It must not call any other slot on this API. Every one of them is written for the
  server thread, and a call from here reaches a state nothing is holding a lock on.
- It must not throw across the boundary.
```

A generator that reads only what registration handed it satisfies all four. One that consults live world state satisfies none of them.

### `PierPaletteSink` {#PierPaletteSink}

```c
typedef void (*PierPaletteSink)(void* ctx, uint32_t index, PierStr name, PierStr snbt);
```

Palette sink for `scan_region_indexed`: invoked once per distinct block state met in the region, before the first cell that uses it. Indices count from 0 in the order of first appearance and are valid for that one call only.

```text
name : block type name, e.g. "minecraft:stone".
snbt : full block serialization (name + states + version) as SNBT.
```

### `PierCellSink` {#PierCellSink}

```c
typedef void (*PierCellSink)(void* ctx, int32_t x, int32_t y, int32_t z, uint32_t index);
```

Cell sink for `scan_region_indexed`: one call per cell with its palette index.

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

One cell of `edit_set_blocks`: a position and an index into the palette the same call passes.

## `PierDimRule` {#PierDimRule}

Per-dimension behavior rules for `md_set_dimension_rule`.

These are deliberately NOT a mirror of any engine enum: they name things the loader intercepts itself. Values are ABI — append only, never renumber.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_DIMRULE_SPAWN_MONSTER"></span>`PIER_DIMRULE_SPAWN_MONSTER` | `0` | natural hostile spawns |
| <span id="PIER_DIMRULE_SPAWN_ANIMAL"></span>`PIER_DIMRULE_SPAWN_ANIMAL` | `1` | natural passive spawns |
| <span id="PIER_DIMRULE_SPAWN_SPAWNER"></span>`PIER_DIMRULE_SPAWN_SPAWNER` | `2` | spawns from mob spawners |
| <span id="PIER_DIMRULE_EXPLODE_BLOCKS"></span>`PIER_DIMRULE_EXPLODE_BLOCKS` | `3` | explosions damaging terrain |
| <span id="PIER_DIMRULE_FIRE_SPREAD"></span>`PIER_DIMRULE_FIRE_SPREAD` | `4` | fire spreading to neighbours |
| <span id="PIER_DIMRULE_MOB_GRIEFING"></span>`PIER_DIMRULE_MOB_GRIEFING` | `5` | mobs changing blocks |
| <span id="PIER_DIMRULE_PROJECTILE"></span>`PIER_DIMRULE_PROJECTILE` | `6` | projectile spawns |
| <span id="PIER_DIMRULE_PISTON_PUSH"></span>`PIER_DIMRULE_PISTON_PUSH` | `7` | pistons moving blocks |
| <span id="PIER_DIMRULE_LIQUID_FLOW"></span>`PIER_DIMRULE_LIQUID_FLOW` | `8` | water/lava spreading |
| <span id="PIER_DIMRULE_FARMLAND_DECAY"></span>`PIER_DIMRULE_FARMLAND_DECAY` | `9` | farmland trampled back to dirt |
| <span id="PIER_DIMRULE_RIDE"></span>`PIER_DIMRULE_RIDE` | `10` | mounting boats/minecarts/animals |
| <span id="PIER_DIMRULE_PISTON_CROSS_CELL"></span>`PIER_DIMRULE_PISTON_CROSS_CELL` | `11` | The same value under the spelling a plot world uses. Both names are permanent: removing one is a deletion, which §2.2 makes advance both version numbers. |
| <span id="PIER_DIMRULE_PISTON_CROSS_PLOT"></span>`PIER_DIMRULE_PISTON_CROSS_PLOT` | `11` | RETIRED since 26.20.3, use `PIER_DIMRULE_PISTON_CROSS_CELL` |
| <span id="PIER_DIMRULE_ENTITY_CROSS_CELL"></span>`PIER_DIMRULE_ENTITY_CROSS_CELL` | `12` |  |
| <span id="PIER_DIMRULE_ENTITY_CROSS_PLOT"></span>`PIER_DIMRULE_ENTITY_CROSS_PLOT` | `12` | RETIRED since 26.20.3, use `PIER_DIMRULE_ENTITY_CROSS_CELL` |

## `PierPackStatus` {#PierPackStatus}

Return codes of `md_add_dimension_pack` and `md_pack_inspect`. A dimension id is never negative, so a caller tells the two apart by sign.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_PACK_OK"></span>`PIER_PACK_OK` | `0` |  |
| <span id="PIER_PACK_BAD_PATH"></span>`PIER_PACK_BAD_PATH` | `-1` | absolute, contains "..", or leaves the server root |
| <span id="PIER_PACK_CONFIG_UNREADABLE"></span>`PIER_PACK_CONFIG_UNREADABLE` | `-2` | the config file cannot be opened |
| <span id="PIER_PACK_CONFIG_INVALID"></span>`PIER_PACK_CONFIG_INVALID` | `-3` | not JSON, or a required key missing or malformed |
| <span id="PIER_PACK_BINARY_UNREADABLE"></span>`PIER_PACK_BINARY_UNREADABLE` | `-4` | the binary named by the config cannot be opened |
| <span id="PIER_PACK_CORRUPT"></span>`PIER_PACK_CORRUPT` | `-5` | a section fails its hash or an index is out of range |
| <span id="PIER_PACK_KIND_MISMATCH"></span>`PIER_PACK_KIND_MISMATCH` | `-6` | spec kind, config type and binary magic disagree |
| <span id="PIER_PACK_HASH_MISMATCH"></span>`PIER_PACK_HASH_MISMATCH` | `-7` | the binary does not hash to the config's sha256 |
| <span id="PIER_PACK_UNSUPPORTED"></span>`PIER_PACK_UNSUPPORTED` | `-8` | a pack kind or format version this host does not serve |
| <span id="PIER_PACK_PARAMS"></span>`PIER_PACK_PARAMS` | `-9` | a parameter or role outside what the pack allows |
| <span id="PIER_PACK_CONSTRAINT"></span>`PIER_PACK_CONSTRAINT` | `-10` | a constraint of the pack fails with these values |
| <span id="PIER_PACK_HEIGHT"></span>`PIER_PACK_HEIGHT` | `-11` | the dimension height does not fit the pack |
| <span id="PIER_PACK_STORED_MISMATCH"></span>`PIER_PACK_STORED_MISMATCH` | `-12` | the name exists with another binary or terrain kind |
| <span id="PIER_PACK_SPEC"></span>`PIER_PACK_SPEC` | `-13` | the spec could not be read or has no pack terrain |
| <span id="PIER_PACK_HOST"></span>`PIER_PACK_HOST` | `-14` | another host refusal; the log has the reason |
