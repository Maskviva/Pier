# 模拟玩家

??? note "abi.h 里的分节说明"

    **§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）**

## 槽位 {#slots}

### `sim_spawn` {#sim_spawn}

```c
bool (*sim_spawn)(PierStr name, int32_t dimension, double x, double y, double z);
```

!!! note "分组说明"

    模拟（「假」）玩家（追加的槽位，受 `struct_size` 约束）。`sim_spawn` 用那个名字创建一个真正的 `ServerPlayer`，现有的每一个按玩家操作的接口（传送、生命值、物品栏、踢出……）都能通过普通的名字选择器作用在它身上。`sim_do` 复用 `simulate*` 这一族动作：动作的种类在 bridge 一侧增加，不需要新的表槽位（动作有 `despawn stop jump attack interact use_item drop respawn move_to navigate_to look_at destroy_block destroy_look stop_destroy interact_block sneak fly chat`，参数用 SNBT，见文档）。以 `isSimulatedPlayer()` 把关：真实玩家永远不会被操纵。动作不认识、参数格式不对、目标不在线或者不是模拟玩家时返回 false。

- 调用形式：`api->sim_spawn(name, dimension, x, y, z)`
- 参数：
    - name : `PierStr`
    - dimension : `int32_t`
    - x : `double`
    - y : `double`
    - z : `double`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 85 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`sim::spawn`](../rust/sim.md#fn.spawn)
    - Go：[`SimSpawn`](../go/sim.md#SimSpawn)、[`Raw.SimSpawn`](../go/raw.md#Raw.SimSpawn)

### `sim_do` {#sim_do}

```c
bool (*sim_do)(PierPlayerSel sel, PierStr action, PierStr args_snbt);
```

!!! note "分组说明"

    模拟（「假」）玩家（追加的槽位，受 `struct_size` 约束）。`sim_spawn` 用那个名字创建一个真正的 `ServerPlayer`，现有的每一个按玩家操作的接口（传送、生命值、物品栏、踢出……）都能通过普通的名字选择器作用在它身上。`sim_do` 复用 `simulate*` 这一族动作：动作的种类在 bridge 一侧增加，不需要新的表槽位（动作有 `despawn stop jump attack interact use_item drop respawn move_to navigate_to look_at destroy_block destroy_look stop_destroy interact_block sneak fly chat`，参数用 SNBT，见文档）。以 `isSimulatedPlayer()` 把关：真实玩家永远不会被操纵。动作不认识、参数格式不对、目标不在线或者不是模拟玩家时返回 false。

- 调用形式：`api->sim_do(sel, action, args_snbt)`
- 参数：
    - sel : `PierPlayerSel`
    - action : `PierStr`
    - args_snbt : `PierStr`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 86 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`SimPlayer::act`](../rust/sim.md#SimPlayer.act)、[`SimPlayer::despawn`](../rust/sim.md#SimPlayer.despawn)、[`SimPlayer::stop`](../rust/sim.md#SimPlayer.stop)、[`SimPlayer::jump`](../rust/sim.md#SimPlayer.jump)、[`SimPlayer::attack`](../rust/sim.md#SimPlayer.attack)、[`SimPlayer::interact`](../rust/sim.md#SimPlayer.interact) 等，共 19 个
    - Go：[`SimDo`](../go/sim.md#SimDo)、[`Raw.SimDo`](../go/raw.md#Raw.SimDo)

### `sim_is` {#sim_is}

```c
bool (*sim_is)(PierPlayerSel sel);
```

!!! note "分组说明"

    模拟（「假」）玩家（追加的槽位，受 `struct_size` 约束）。`sim_spawn` 用那个名字创建一个真正的 `ServerPlayer`，现有的每一个按玩家操作的接口（传送、生命值、物品栏、踢出……）都能通过普通的名字选择器作用在它身上。`sim_do` 复用 `simulate*` 这一族动作：动作的种类在 bridge 一侧增加，不需要新的表槽位（动作有 `despawn stop jump attack interact use_item drop respawn move_to navigate_to look_at destroy_block destroy_look stop_destroy interact_block sneak fly chat`，参数用 SNBT，见文档）。以 `isSimulatedPlayer()` 把关：真实玩家永远不会被操纵。动作不认识、参数格式不对、目标不在线或者不是模拟玩家时返回 false。

选择器能解析到一个活着的模拟玩家时返回 true。模组可以借此在重启后重新确认一个机器人：`SimulatedPlayer` 会保存在世界里，内存里的句柄不会。

- 调用形式：`api->sim_is(sel)`
- 参数：
    - sel : `PierPlayerSel`
- 返回值类型：`bool`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 87 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`SimPlayer::try_is_simulated`](../rust/sim.md#SimPlayer.try_is_simulated)、[`SimPlayer::is_simulated`](../rust/sim.md#SimPlayer.is_simulated)
    - Go：[`IsSimulated`](../go/sim.md#IsSimulated)、[`Raw.SimIs`](../go/raw.md#Raw.SimIs)

### `sim_list` {#sim_list}

```c
void (*sim_list)(void* ctx, PierStrSink name_sink);
```

!!! note "分组说明"

    模拟（「假」）玩家（追加的槽位，受 `struct_size` 约束）。`sim_spawn` 用那个名字创建一个真正的 `ServerPlayer`，现有的每一个按玩家操作的接口（传送、生命值、物品栏、踢出……）都能通过普通的名字选择器作用在它身上。`sim_do` 复用 `simulate*` 这一族动作：动作的种类在 bridge 一侧增加，不需要新的表槽位（动作有 `despawn stop jump attack interact use_item drop respawn move_to navigate_to look_at destroy_block destroy_look stop_destroy interact_block sneak fly chat`，参数用 SNBT，见文档）。以 `isSimulatedPlayer()` 把关：真实玩家永远不会被操纵。动作不认识、参数格式不对、目标不在线或者不是模拟玩家时返回 false。

列出所有活着的模拟玩家的名字（输出回调逐个收到名字）。用名字重建句柄，就能操纵一个比生成它的那次会话活得更久的机器人。

- 调用形式：`api->sim_list(ctx, name_sink)`
- 参数：
    - ctx : `void*`
    - name_sink : `PierStrSink`
- 所在分节：§I 二进制 NBT、KvDb（线程安全）、系统与服务器信息（`§I NBT binary, KvDb (thread-safe), system & server info`）
- 表内序号：第 88 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`sim::try_list`](../rust/sim.md#fn.try_list)、[`sim::list`](../rust/sim.md#fn.list)
    - Go：[`SimList`](../go/sim.md#SimList)、[`Raw.SimList`](../go/raw.md#Raw.SimList)
