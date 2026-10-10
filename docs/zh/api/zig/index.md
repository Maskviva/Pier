# Zig 接口

包名 `.pier`，导出模块 `levilamina`。`levilamina.slot("名字")` 能取到 abi.h 的全部 201 个槽位，每个槽位的参数和返回值见 [C++ 接口](../cpp/index.md)；下面各页列的是建立在 `slot` 之上的函数和方法。

`props_gen.zig` 和 `nbt.zig` 分别以 `levilamina.props`、`levilamina.nbt` 公开；其中 `Player`、`Entity`、`BlockAt`、`Item` 和 `SelKind` 在根上也有同名的再导出。

| 页面 | 函数和方法 | 类型 | 常量 |
|---|---|---|---|
| [核心](core.md) | 20 | 4 | 2 |
| [事件](events.md) | 4 | 3 | 0 |
| [命令](commands.md) | 4 | 2 | 0 |
| [服务器、刻与系统信息](server.md) | 8 | 0 | 0 |
| [玩家](player.md) | 83 | 2 | 0 |
| [实体](entity.md) | 86 | 1 | 0 |
| [方块](block.md) | 43 | 1 | 0 |
| [物品与容器](item.md) | 40 | 1 | 0 |
| [跨模组](crossmod.md) | 11 | 6 | 0 |
| [SNBT](nbt.md) | 6 | 3 | 1 |
