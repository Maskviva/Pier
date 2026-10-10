# Shared types

## Types {#types}

### `PierStr` {#PierStr}

```c
typedef struct PierStr
{
    const char* ptr;
    size_t len;
} PierStr;
```

UTF-8 string view. An explicit {pointer, length} struct, not an alias for any language's string type: the layout is defined by this declaration alone and does not depend on either side's standard library. The zero-copy conversion to and from `std::string_view` lives in pier-support.

### `PierStrSink` {#PierStrSink}

```c
typedef void (*PierStrSink)(void* ctx, PierStr s);
```

Generic string sink: receives a string within the current call frame.

### `PierPlayerPos` {#PierPlayerPos}

```c
typedef struct PierPlayerPos
{
    double x;
    double y;
    double z;
    int32_t dimension;
    bool found;
} PierPlayerPos;
```

A player's feet position + dimension. `found` is false if no such player.

### `PierBlockSink` {#PierBlockSink}

```c
typedef void (*PierBlockSink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr name, PierStr snbt);
```

Block sink: invoked once per cell during `scan_region`.

```text
x, y, z : the cell's world coordinates.
name    : block type name, e.g. "minecraft:redstone_wire".
snbt    : full block serialization (name + states + version) as SNBT.
```

### `PierEntitySink` {#PierEntitySink}

```c
typedef void (*PierEntitySink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr type, PierStr snbt);
```

Entity sink: invoked once per entity whose position falls inside the region.

```text
x, y, z : the block cell that contains the entity (floor of its position).
type    : entity type name, e.g. "minecraft:creeper".
snbt    : the entity's serialized NBT (Actor::save) as SNBT.
```

### `PierSlotSink` {#PierSlotSink}

```c
typedef void (*PierSlotSink)(void* ctx, int32_t slot, PierStr item_snbt);
```

Slot sink for `container_get_items`: one call per slot, in slot order.

### `PierPlayerSel` {#PierPlayerSel}

```c
typedef struct PierPlayerSel
{
    int32_t kind;
    PierStr value;
} PierPlayerSel;
```

Player selector — the identifier half of the "handles are identifiers, not pointers" rule. Resolved against the live player list on every call.

```text
kind: 0 = name (getRealName, falling back to getNameTag),
      1 = xuid, 2 = uuid (canonical string form).
```

### `PierActorId` {#PierActorId}

```c
typedef int64_t PierActorId;
```

ActorUniqueID raw value. 0 / negative-invalid never resolves.

### `PierContainerRef` {#PierContainerRef}

```c
typedef struct PierContainerRef
{
    int32_t which;
    PierPlayerSel player;
    int32_t dim;
    int32_t x;
    int32_t y;
    int32_t z;
} PierContainerRef;
```

Container reference — "owner + which container".

```text
which: 0=inventory 1=ender_chest 2=armor 3=offhand 4=block container.
player: valid for which 0..3.   dim/x/y/z: valid for which == 4.
```

### `PierActorSink` {#PierActorSink}

```c
typedef void (*PierActorSink)(void* ctx, PierActorId id, PierStr type_name);
```

Actor sink (`list_actors`).
