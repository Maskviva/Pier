# Go 接口

模块 `github.com/Maskviva/pier/bindings/go`，包 `levilamina`。页面按主题分，文件名和 C++ 页一一对应：方法在它接收者类型所在的页，函数在它调用的第一个槽位所属的主题页。

`levilamina.Raw` 给每个不带回调的槽位各提供一个带类型的方法，全部列在 [Raw](raw.md) 页。带回调的槽位由 `Subscribe`、`RegisterCommand`、`BusSubscribe` 这类手写函数提供。

| 页面 | 函数和方法 | 类型 | 常量 |
|---|---|---|---|
| [核心](core.md) | 13 | 8 | 4 |
| [事件](events.md) | 11 | 4 | 5 |
| [命令](commands.md) | 23 | 10 | 28 |
| [服务器、刻与系统信息](server.md) | 16 | 0 | 8 |
| [世界](world.md) | 17 | 4 | 0 |
| [批量编辑世界](edit.md) | 2 | 0 | 0 |
| [玩家](player.md) | 103 | 5 | 77 |
| [实体](entity.md) | 94 | 3 | 85 |
| [方块](block.md) | 50 | 2 | 42 |
| [物品与容器](item.md) | 52 | 4 | 44 |
| [计分板](scoreboard.md) | 1 | 0 | 0 |
| [表单](form.md) | 1 | 0 | 0 |
| [NBT 与键值数据库](data.md) | 11 | 3 | 0 |
| [经济](money.md) | 7 | 2 | 4 |
| [数据包](packet.md) | 8 | 5 | 3 |
| [模拟玩家](sim.md) | 4 | 0 | 0 |
| [客户端](client.md) | 3 | 2 | 0 |
| [自定义维度](dimensions.md) | 7 | 1 | 0 |
| [跨模组](crossmod.md) | 17 | 6 | 4 |
| [注册表](registry.md) | 1 | 0 | 0 |
| [SNBT](nbt.md) | 14 | 2 | 12 |
| [Raw](raw.md) | 173 | 1 | 0 |
