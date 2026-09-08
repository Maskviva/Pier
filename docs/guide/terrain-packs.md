# Terrain packs

A custom dimension has a terrain of one of three kinds.

| Kind | What it is | Registered with |
|---|---|---|
| `native` | A vanilla generator: `overworld`, `nether`, `end` with their structures, or `flat` and `void` | `md_add_dimension` |
| `template` | A fixed range with parameters; every cell has the same data, filled from a PIERTPL pack | `md_add_dimension_pack` |
| `volume` | A Java-style density function graph, filled from a PIERVOL pack | `md_add_dimension_pack` |

A pack is a directory with a config file and a binary. The mod ships it; the host reads
four keys of the config and nothing else:

```json
{
  "pier_terrain": 1,
  "type": "template",
  "binary": "terrain.ptpl",
  "sha256": "<64 hex digits of terrain.ptpl>",
  "title": "Plot world",
  "anything_else": "belongs to the mod"
}
```

`binary` is relative to the config; the config path a mod passes to the host is relative
to the server root, with forward slashes, no `..`, not absolute. `pier-pack hash` prints
the value for `sha256`.

## Three sources, one answer

Before anything is stored, the host compares the spec's `terrain.kind`, the config's
`type` and the binary's magic, and hashes the binary against the config. Any disagreement
refuses with a `PIER_PACK_*` code and nothing is written. The stored spec then carries the
path, the hash, every bound parameter and the role overrides, so the terrain is
regenerable from the save alone. On a later boot the stored spec wins: the same name
returns the same id, parameters given now are ignored with a warning if they differ, and a
config whose hash is no longer the stored one refuses with `PIER_PACK_STORED_MISMATCH`.
Editing a pack means a new world or a new name.

## Template packs

A template pack fixes the range and leaves the content polymorphic: the author decides the
shape of a cell and which knobs exist; the mod decides the values.

- **Parameters** have a kind. `free` takes any value in `[min, max]` on its `step`;
  `fixed` cannot be changed; `choice` takes one of a list; `derived` is an expression over
  earlier parameters and is never supplied. `md_pack_inspect` lists them so a mod can build
  a form: a slider, a read-only value, a list, and nothing for derived.
- **Roles** are the blocks the pack names symbolically, `floor`, `road`, `wall`; each has a
  default and the mod may override it in `terrain.roles`.
- **Zones** along each axis, with expression lengths, combine into a 2D zone per column;
  a **stack** per zone paints the column in painter's order.
- **Constraints** are checked with the bound values and refuse with their message.
- **Shapes** are a CSG graph over primitives and voxel blobs, **picked** per cell with
  weights and turns; `choose` selects a subtree by a choice parameter or by the cell hash.
- **Confine** registers the cell grid for `PIER_DIMRULE_PISTON_CROSS_CELL` and
  `PIER_DIMRULE_ENTITY_CROSS_CELL` from the same mount, so the grid the rules see is the
  grid the terrain drew.

The source is JSON; see `tools/pier-pack/fixtures/plot.json` for the plot world and
`town.json` for shapes and blobs. Build it:

```
python3 -m pierpack.cli build plot.json -o terrain.ptpl
python3 -m pierpack.cli inspect terrain.ptpl
python3 -m pierpack.cli hash terrain.ptpl
```

A pre-pack `{kind:"layers"}` spec converts with `from-layers`.

## Volume packs

A volume pack is a Java `noise_settings` file with its references gathered in: density
functions by id, noises by id, the biome parameter list, the surface rule. The pack stores
noise parameters, never tables; the host builds the tables from the world seed the way
Java does, with Xoroshiro128++ or, under `legacy_random_source`, the legacy source, so one
pack serves any seed.

```
python3 -m pierpack.cli from-datapack <datapack dir> minecraft:overworld -o overworld.json
python3 -m pierpack.cli build overworld.json -o terrain.pvol
```

A `biome_source` that names a preset must be written out as its biome list first; presets
live in the game, not in the datapack. Supported: the 31 density function operators,
splines, the multi-noise biome list, and the surface rule subset of `block`, `sequence`,
`condition`, `biome`, `noise_threshold`, `vertical_gradient`, `y_above`, `water`,
`temperature`, `steep`, `not`, `hole`, `above_preliminary_surface` and `stone_depth`. Not
supported and refused at build time: `bandlands`; not modelled: aquifers and ore veins.
The biome of a column is the one at its surface.

## From the mod side

```rust
let json = pier::dimensions::pack_inspect("packs/plot/config.json")?;
let spec = r#"{seed:12345,terrain:{kind:"template",params:{plot_size:64,road_width:7},roles:{floor:"minecraft:stone"}}}"#;
let id = pier::dimensions::add_dimension_pack("plots", "packs/plot/config.json", spec)?;
```

A refusal is a `PackError` with a `PackStatus`; the reasons are in the host log, and
`pack_inspect` on the same path returns them as JSON.

## Verifying the format

`tools/pier-pack/tests` holds the layout test (the Python mirror against
`pack_format.h`), the plot equivalence test (against a line-for-line port of the 26.20.2
`PlotGenerator`), and two equivalence tests that compile the engine-free pack layer under
g++ and compare its chunks cell by cell with the Python reference generators. Run them
with `python3 tests/<name>.py` from `tools/pier-pack`.
