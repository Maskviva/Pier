# 通用类型

## 类型 {#types}

### `PierStr` {#PierStr}

```c
typedef struct PierStr
{
    const char* ptr;
    size_t len;
} PierStr;
```

UTF-8 字符串视图。它是一个显式的 {指针, 长度} 结构体，没有用任何语言的字符串类型做别名：它的布局只由这一份声明决定，不依赖任何一侧的标准库。和 `std::string_view` 之间的零拷贝转换放在 pier-support 里。

### `PierStrSink` {#PierStrSink}

```c
typedef void (*PierStrSink)(void* ctx, PierStr s);
```

通用的字符串输出回调：在当前调用帧内收到一个字符串。

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

一名玩家的脚下位置加上所在维度。没有这名玩家时 `found` 为 false。

### `PierBlockSink` {#PierBlockSink}

```c
typedef void (*PierBlockSink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr name, PierStr snbt);
```

方块回调：`scan_region` 期间每一格调用一次。

- `x, y, z`：这一格的世界坐标。
- `name`：方块类型名，例如 `"minecraft:redstone_wire"`。
- `snbt`：完整的方块序列化（名字、状态和版本），以 SNBT 表示。

### `PierEntitySink` {#PierEntitySink}

```c
typedef void (*PierEntitySink)(void* ctx, int32_t x, int32_t y, int32_t z, PierStr type, PierStr snbt);
```

实体回调：位置落在区域里的每个实体调用一次。

- `x, y, z`：包含这个实体的方块格（它的位置向下取整）。
- `type`：实体类型名，例如 `"minecraft:creeper"`。
- `snbt`：这个实体序列化后的 NBT（`Actor::save`），以 SNBT 表示。

### `PierSlotSink` {#PierSlotSink}

```c
typedef void (*PierSlotSink)(void* ctx, int32_t slot, PierStr item_snbt);
```

`container_get_items` 的槽位回调：每个槽位调用一次，按槽位的顺序。

### `PierPlayerSel` {#PierPlayerSel}

```c
typedef struct PierPlayerSel
{
    int32_t kind;
    PierStr value;
} PierPlayerSel;
```

玩家选择器，是「句柄用标识符，不用指针」这条规则里标识符的那一半。每次调用都对照当前的在线玩家列表重新解析。

- `kind`：0 = 名字（`getRealName`，对不上时退回 `getNameTag`），1 = xuid，2 = uuid（标准的字符串形式）。

### `PierActorId` {#PierActorId}

```c
typedef int64_t PierActorId;
```

`ActorUniqueID` 的原始值。0 和负数是无效值，永远解析不到。

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

容器引用，即「所有者 + 哪一个容器」。

- `which`：0=物品栏，1=末影箱，2=盔甲，3=副手，4=方块容器。
- `player`：`which` 为 0 到 3 时有效。`dim`/`x`/`y`/`z`：`which == 4` 时有效。

### `PierActorSink` {#PierActorSink}

```c
typedef void (*PierActorSink)(void* ctx, PierActorId id, PierStr type_name);
```

实体回调（`list_actors`）。
