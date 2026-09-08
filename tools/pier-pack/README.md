# pier-pack

The terrain pack tool: builds, inspects and hashes PIERTPL and PIERVOL packs, converts
old specs and Java datapacks into sources, and carries the reference generators the C++
pack layer is tested against. Python 3.10+, no dependencies.

```
python3 -m pierpack.cli build fixtures/plot.json -o terrain.ptpl
python3 -m pierpack.cli inspect terrain.ptpl
python3 -m pierpack.cli hash terrain.ptpl
python3 -m pierpack.cli from-layers old_spec.snbt -o source.json
python3 -m pierpack.cli from-datapack ./datapack minecraft:overworld -o overworld.json
```

Tests, run from this directory: `python3 tests/test_layout.py` and the other four files
under `tests/`. The two equivalence tests compile `packages/pier-dimensions/src/pack`
under g++ and skip when none is installed.
