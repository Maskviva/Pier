# 注册表

??? note "abi.h 里的分节说明"

    **注册表（`Registries`）**

## 槽位 {#slots}

### `registry_list` {#registry_list}

```c
bool (*registry_list)(int32_t kind, void* ctx, PierStrSink sink);
```

通过 `sink` 列出引擎的某一个注册表，每一项一个 JSON 对象，顺序不定。`kind` 是某个 `PIER_REGISTRY_*` 值。

方块的每一项包含：类型名；引擎给的创造模式分类编号；这个类型是否是实心的、原版的、容器、信号源、栅栏、铁轨、台阶、墙或作物；是否带有方块实体；默认状态下徒手挖掘的速度（什么都挖不动的方块为负数）；爆炸抗性；发出的光照；以及描述 id。

物品的每一项包含：完整的名字；基础稀有度（0 普通到 3 史诗）；最大堆叠数；创造模式分类；以及命令是否隐藏它。

实体的每一项包含：标识符；是否有刷怪蛋；是否可以召唤；是否受实验性玩法控制；以及引擎的实体类型编号，编号的各个位表示它是不是生物、怪物、动物或水生动物。

列出的是正在运行的游戏自己的内容，所以要挑「一个随机方块」或者「一个随机稀有物品」的模组，可以按规则从里面挑，不必自己维护一份列表。`kind` 不认识、`sink` 为空，或者注册表还不存在时返回 false，这时什么都不会输出。

- 调用形式：`api->registry_list(kind, ctx, sink)`
- 参数：
    - kind : `int32_t`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：注册表（`Registries`）
- 表内序号：第 200 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`registry::blocks`](../rust/registry.md#fn.blocks)、[`registry::items`](../rust/registry.md#fn.items)、[`registry::entities`](../rust/registry.md#fn.entities)
    - Go：[`RegistryList`](../go/registry.md#RegistryList)、[`Raw.RegistryList`](../go/raw.md#Raw.RegistryList)

## `PIER_REGISTRY_*` {#PIER_REGISTRY_BLOCKS-group}

`registry_list` 的种类：列出引擎的哪一个注册表。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_REGISTRY_BLOCKS"></span>`PIER_REGISTRY_BLOCKS` | `0` | 所有方块类型 |
| <span id="PIER_REGISTRY_ITEMS"></span>`PIER_REGISTRY_ITEMS` | `1` | 所有物品 |
| <span id="PIER_REGISTRY_ENTITIES"></span>`PIER_REGISTRY_ENTITIES` | `2` | 关卡知道的所有实体 |
