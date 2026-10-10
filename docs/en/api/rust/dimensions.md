# levilamina::dimensions

Custom dimensions: the facade of the optional `pier-dimensions` capability package.

When it is not built into the host the whole family of slots is NULL,
[`is_available`](client.md#fn.is_available) returns false and every other call returns an `Err` saying the host
does not provide it. That is rule 3 of contract §1 at runtime: the optional package is
absent, the layout is unchanged, and the slots are empty.

Registration is idempotent: [`add_dimension`](dimensions.md#fn.add_dimension) and [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack) return the
same persisted id for the same name on the next startup, so a mod registers at startup
rather than probing with [`dimension_id`](dimensions.md#fn.dimension_id) first, which misses on the first startup.
A pack terrain is a directory with a config and a binary built by `tools/pier-pack`;
[`pack_inspect`](dimensions.md#fn.pack_inspect) tells what it asks for and [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack) mounts it, and the
host stores the file hash and every bound value with the dimension.

## Functions {#functions}

### `dimensions::is_available` {#fn.is_available}

```rust
pub fn is_available() -> bool
```

Whether this host was built with the custom dimension capability.

- Return type: `bool`
- Slots: [`md_is_available`](../cpp/dimensions.md#md_is_available), [`md_is_available`](../cpp/dimensions.md#md_is_available)

### `dimensions::dimension_id` {#fn.dimension_id}

```rust
pub fn dimension_id(name: &str) -> Option<i32>
```

Looks up a dimension id by name.

It gives an id only for a name really registered and returns `None` otherwise, rather
than the undefined dimension whose value changes at runtime while looking like a valid
id.

It is rarely needed; see the note on registering unconditionally in the module
documentation.

- Parameters:
    - name : `&str`
- Return type: `Option<i32>`
- Slots: [`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id), [`md_get_dimension_id`](../cpp/dimensions.md#md_get_dimension_id)

### `dimensions::list` {#fn.list}

```rust
pub fn list() -> Vec<ExistingDimension>
```

Every registered custom dimension.

A world manager adopting an existing save has to ask this first: the dimensions a
previous plugin created are alive in the save and players can teleport into them while
the manager's table has no row for them. The consequence is not a few missing rows but
those dimensions being governed by no rule, and a newly created world possibly being
assigned a dimension id that collides with theirs.

- Return type: `Vec<ExistingDimension>`
- Slots: [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions), [`md_list_dimensions`](../cpp/dimensions.md#md_list_dimensions)

### `dimensions::set_rule` {#fn.set_rule}

```rust
pub fn set_rule(dimension: i32, rule: DimensionRule, allow: bool) -> Result<()>
```

Sets one per-dimension rule.

- Parameters:
    - dimension : `i32`
    - rule : `DimensionRule`
    - allow : `bool`
- Return type: `Result<()>`
- Slots: [`md_set_dimension_rule`](../cpp/dimensions.md#md_set_dimension_rule)

### `dimensions::rule` {#fn.rule}

```rust
pub fn rule(dimension: i32, rule: DimensionRule) -> Result<Option<bool>>
```

Reads one rule. A dimension with no explicit registration for that rule gives
`Ok(None)`, meaning it follows vanilla behavior, which is different from being
registered with the value false. A host older than this SDK that does not know `rule`
also answers `Ok(None)`; the ABI gives no way to tell the two apart.

- Parameters:
    - dimension : `i32`
    - rule : `DimensionRule`
- Return type: `Result<Option<bool>>`
- Slots: [`md_get_dimension_rule`](../cpp/dimensions.md#md_get_dimension_rule)

### `dimensions::clear_rules` {#fn.clear_rules}

```rust
pub fn clear_rules(dimension: i32) -> Result<()>
```

Clears every rule of a dimension, for when the world was deleted.

- Parameters:
    - dimension : `i32`
- Return type: `Result<()>`
- Slots: [`md_clear_dimension_rules`](../cpp/dimensions.md#md_clear_dimension_rules)

### `dimensions::set_plot_merges` {#fn.set_plot_merges}

```rust
pub fn set_plot_merges(dimension: i32, merges: &[PlotMerge]) -> Result<()>
```

Replaces the merge marks of a dimension as a whole.

As a whole and not incrementally: an increment requires both sides to agree at all
times on the same current state, while unlinking clears the neighbor before storing
itself, and a failure in between makes the two views diverge with no way back. A whole
push pulls both sides back into agreement every time.

The grid comes from the template pack's CONF section at [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack): a push
to a dimension without one is dropped with a warning. The geometry the mod side must
match: with `period = cell + gap`, a column `(x, z)` is inside a cell when
`mod(x, period) < cell && mod(z, period) < cell`.

- Parameters:
    - dimension : `i32`
    - merges : `&[PlotMerge]`
- Return type: `Result<()>`
- Slots: [`md_set_plot_merges`](../cpp/dimensions.md#md_set_plot_merges)

### `dimensions::add_dimension` {#fn.add_dimension}

```rust
pub fn add_dimension(name: &str, spec_snbt: &str) -> Result<i32>
```

Adds a custom dimension with a native terrain; see `md_add_dimension` in `abi.h`.

The spec is opaque to this SDK: the shape is owned by the host and the RSW world
manager (`rsw_world_spec::Generator::to_spec_snbt`) writes it. This function only
carries the string across. A pack terrain is refused here; see [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack).

- Parameters:
    - name : `&str`
    - spec_snbt : `&str`
- Return type: `Result<i32>`
- Slots: [`md_add_dimension`](../cpp/dimensions.md#md_add_dimension)

### `dimensions::add_dimension_pack` {#fn.add_dimension_pack}

```rust
pub fn add_dimension_pack(
    name: &str,
    config_path: &str,
    spec_snbt: &str,
) -> std::result::Result<i32, PackError>
```

Adds a custom dimension whose terrain is a pack; see `md_add_dimension_pack` in
`abi.h`.

`config_path` names the pack config relative to the server root with forward slashes;
`spec_snbt` is a spec whose terrain has `kind:"template"` or `"volume"` plus `params`
and `roles`. The host verifies the pack, binds the values and stores everything with
the dimension. A negative return is a `PIER_PACK_*` code and becomes a [`PackError`](dimensions.md#PackError)
with no problem lines; the reasons are in the host log, and [`pack_inspect`](dimensions.md#fn.pack_inspect) on the
same path returns them as JSON.

- Parameters:
    - name : `&str`
    - config_path : `&str`
    - spec_snbt : `&str`
- Return type: `std::result::Result<i32, PackError>`
- Slots: [`md_add_dimension_pack`](../cpp/dimensions.md#md_add_dimension_pack)

### `dimensions::pack_inspect` {#fn.pack_inspect}

```rust
pub fn pack_inspect(config_path: &str) -> std::result::Result<String, PackError>
```

What a pack asks for, as the JSON document `md_pack_inspect` describes, without
registering anything. A refusal carries the host's problem lines.

- Parameters:
    - config_path : `&str`
- Return type: `std::result::Result<String, PackError>`
- Slots: [`md_pack_inspect`](../cpp/dimensions.md#md_pack_inspect)

### `dimensions::retire_dimension` {#fn.retire_dimension}

```rust
pub fn retire_dimension(name: &str) -> Result<bool>
```

Retires a custom dimension; see `md_retire_dimension` in `abi.h`.

The host drops the name from `dimension_config.json`, from its own tables and from the
dimension factory, so it is not registered again on the next boot. Nothing in the
running engine is undone: the dimension built for this session stays and a player
inside it is not moved.

The chunks stay in the save and the id is not handed out again. Registering the same
name afterwards is a new dimension with a new id, so the old terrain is orphaned
rather than inherited.

`Ok(false)` when the host had no dimension of that name, which is also the answer to a
second call.

- Parameters:
    - name : `&str`
- Return type: `Result<bool>`
- Slots: [`md_retire_dimension`](../cpp/dimensions.md#md_retire_dimension)

### `dimensions::add_dimension_generated` {#fn.add_dimension_generated}

```rust
pub fn add_dimension_generated(name: &str, spec_snbt: &str, terrain: Terrain) -> Result<i32>
```

Registers a dimension whose terrain this mod fills; see `md_add_dimension_generated` in
`abi.h`.

`spec_snbt` carries only seed, height and sky. A terrain section is refused, since the
host does not read one.

`terrain` is leaked into a `'static`: the host uses it on chunk threads for as long as
the dimension lives, and that lifetime is the host's, so this side has no moment at
which reclaiming it is safe. A refused registration leaks it too, because a generator
built during the attempt may still hold it. Register once per dimension.

- Parameters:
    - name : `&str`
    - spec_snbt : `&str`
    - terrain : `Terrain`
- Return type: `Result<i32>`
- Slots: [`md_add_dimension_generated`](../cpp/dimensions.md#md_add_dimension_generated)

### `dimensions::set_dimension_cells` {#fn.set_dimension_cells}

```rust
pub fn set_dimension_cells(dim_id: i32, cell: i32, gap: i32) -> Result<()>
```

Gives a dimension the cell geometry its confinement rules use; see
`md_set_dimension_cells`.

`cell` is the edge of a cell and `gap` the space between cells, both in blocks. A `cell`
of 0 removes the geometry, after which the two `PIER_DIMRULE_*_CROSS_CELL` rules have
nothing to answer from.

- Parameters:
    - dim_id : `i32`
    - cell : `i32`
    - gap : `i32`
- Return type: `Result<()>`
- Slots: [`md_set_dimension_cells`](../cpp/dimensions.md#md_set_dimension_cells)

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

The vanilla generator of a `terrain:{kind:"native"}` spec.

The values are the engine's `GeneratorType`, which starts at 1 and not 0: numbering from
0 would make superflat generate a nether. The spec itself names the generator with the
lower-case string of [`GeneratorType::spec_name`](dimensions.md#GeneratorType.spec_name); the numbers survive because
`md_list_dimensions` and old saves both carry them.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `GeneratorType::from_i32` {#GeneratorType.from_i32}

```rust
pub fn from_i32(v: i32) -> Option<GeneratorType>
```

- Parameters:
    - v : `i32`
- Return type: `Option<GeneratorType>`

### `GeneratorType::as_i32` {#GeneratorType.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

### `GeneratorType::spec_name` {#GeneratorType.spec_name}

```rust
pub fn spec_name(self) -> &'static str
```

What `terrain.generator` and `sky.client` are spelled as in a spec.

- Return type: `&'static str`

### `GeneratorType::engine_name` {#GeneratorType.engine_name}

```rust
pub fn engine_name(self) -> &'static str
```

What the engine itself calls this generator.

Not the same string as [`GeneratorType::spec_name`](dimensions.md#GeneratorType.spec_name): this one appears in generation
parameters and in old saves. Use it when assembling something for the engine, not
`{:?}`.

- Return type: `&'static str`

## `PlotMerge` {#PlotMerge}

```rust
pub struct PlotMerge {
    pub x: i32,
    pub z: i32,
    /// A bit set where 1 is north, 2 east, 4 south and 8 west.
    pub mask: u32,
}
```

The merge marks of one plot.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `PlotMerge::is_empty` {#PlotMerge.is_empty}

```rust
pub fn is_empty(&self) -> bool
```

- Return type: `bool`

### `PlotMerge::from_dirs` {#PlotMerge.from_dirs}

```rust
pub fn from_dirs(x: i32, z: i32, dirs: [bool; 4]) -> PlotMerge
```

Assembles a mask from the four directions in the order north, east, south, west.

The order is the bit order, with `NORTH` as bit 0. Writing `1 | 4` by hand makes a
reader look the table up in reverse, and getting it backwards shows up as plots merging
in the wrong direction.

- Parameters:
    - x : `i32`
    - z : `i32`
    - dirs : `[bool; 4]`
- Return type: `PlotMerge`

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

Per-dimension rules. The values correspond to `PIER_DIMRULE_*`.

Why not a game rule: a Bedrock game rule applies to the whole server, so turning
`doMobSpawning` off for a creative plot world turns it off for the survival world too.
These flags are checked at the real call sites, `Spawner::spawnMob`, `Level::explode`
and others, so they really are per dimension.

A dimension that was never registered is entirely unaffected: the hook falls straight
through to the vanilla implementation and a caller need not allow vanilla dimensions
explicitly.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `DimensionRule::as_i32` {#DimensionRule.as_i32}

```rust
pub fn as_i32(self) -> i32
```

- Return type: `i32`

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

Why the host refused a pack; the values are `PIER_PACK_*`.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `PackStatus::from_code` {#PackStatus.from_code}

```rust
pub fn from_code(code: i32) -> PackStatus
```

- Parameters:
    - code : `i32`
- Return type: `PackStatus`

## `ChunkSource` {#ChunkSource}

```rust
pub trait ChunkSource: Send + Sync + 'static { /* ... */ }
```

A dimension whose terrain the mod fills itself.

An implementation is handed to the chunk worker threads, so it is `Send + Sync`, and it
must not change once registered. That is the contract of `PierGenerateChunkFn` in
`abi.h`: the host calls it concurrently, the same coordinates must always give the same
answer, and the callback must not call any other slot of the host.

A generator that reads only what it mounted satisfies all three; one that consults the
current state of the world satisfies none.

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

Fills one chunk. `materials` has `256 * height` entries, indexed
`(x * 16 + z) * height + y` with y counted from `min_y`; `biomes` has 256, one per
column. Both hold indices into the two palettes given at registration, and material
0 is air.

Returning false means the chunk could not be filled; the host writes air and logs it.

- Parameters:
    - chunk_x : `i32`
    - chunk_z : `i32`
    - min_y : `i32`
    - height : `i32`
    - materials : `&mut [u16]`
    - biomes : `&mut [u16]`
- Return type: `bool`

## `ExistingDimension` {#ExistingDimension}

```rust
pub struct ExistingDimension {
    pub name: String,
    pub dim: i32,
    /// The raw generation parameters, interpreted by the caller.
    pub snbt: String,
}
```

One custom dimension that has been registered.

- Implements: `Debug`, `Clone`, `PartialEq`

## `PackError` {#PackError}

```rust
pub struct PackError {
    pub status: PackStatus,
    pub problems: Vec<String>,
}
```

A refusal of [`add_dimension_pack`](dimensions.md#fn.add_dimension_pack) or [`pack_inspect`](dimensions.md#fn.pack_inspect), with the host's reasons when
the call produced any.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Display`

## `Terrain` {#Terrain}

```rust
pub struct Terrain {
    pub source: Box<dyn ChunkSource>,
    /// Entry 0 must be `minecraft:air`: the host reads it as "nothing was written here".
    pub materials: Vec<String>,
    pub biomes: Vec<String>,
}
```

What is handed to the host at registration, with its two palettes.
