# Registries

??? note "Section notes in abi.h"

    **Registries**

## Slots {#slots}

### `registry_list` {#registry_list}

```c
bool (*registry_list)(int32_t kind, void* ctx, PierStrSink sink);
```

Lists one of the engine's registries through `sink`, one JSON object per entry, in no particular order. `kind` is a PIER\_REGISTRY\_\* value.

A block entry carries the type name, the creative category as the engine numbers it, whether the type is solid, vanilla, a container, a signal source, a fence, a rail, a slab, a wall or a crop, whether it holds a block entity, the bare-hand destroy speed of its default state (negative for a block nothing breaks), its explosion resistance, the light it emits and its description id.

An item entry carries the full name, the base rarity (0 common to 3 epic), the maximum stack size, the creative category and whether commands hide it.

An entity entry carries the identifier, whether it has a spawn egg, whether it is summonable, whether an experiment gates it, and the engine's actor type number, whose bits say whether it is a mob, a monster, an animal or a water animal.

What a listing contains is the running game's own content, so a mod that picks "a random block" or "a random rare item" decides by rules over it rather than by a list it maintains. Returns false for an unknown kind, a null sink, or a registry that does not exist yet; nothing is sunk then.

- Call: `api->registry_list(kind, ctx, sink)`
- Parameters:
    - kind : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Registries
- Position in the table: slot 200, counting from 0
- Callers in each binding:
    - Rust: [`registry::blocks`](../rust/registry.md#fn.blocks), [`registry::items`](../rust/registry.md#fn.items), [`registry::entities`](../rust/registry.md#fn.entities)
    - Go: [`RegistryList`](../go/registry.md#RegistryList), [`Raw.RegistryList`](../go/raw.md#Raw.RegistryList)

## `PIER_REGISTRY_*` {#PIER_REGISTRY_BLOCKS-group}

`registry_list` kinds: which of the engine's registries to list.

| Name | Value | Description |
|---|---|---|
| <span id="PIER_REGISTRY_BLOCKS"></span>`PIER_REGISTRY_BLOCKS` | `0` | every block type |
| <span id="PIER_REGISTRY_ITEMS"></span>`PIER_REGISTRY_ITEMS` | `1` | every item |
| <span id="PIER_REGISTRY_ENTITIES"></span>`PIER_REGISTRY_ENTITIES` | `2` | every actor the level knows |
