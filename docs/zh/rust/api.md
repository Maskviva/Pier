# API 地图

Rust SDK 的完整文档由 rustdoc 生成，来源就是约束实现本身的那些注释：

```bash
cargo doc --open -p pier-rs
```

本页是地图。每一条说明这个模块是干什么的、有什么坑，细节在 rustdoc 里。

## 模组本身

| | |
|---|---|
| `LeviMod` | 你的模组实现的 trait。只有 `on_load` 必须写。 |
| `ModContext` | 生命周期回调收到的东西：`logger()`、`host()`、`world()`、`packets()`、`server()`。 |
| `register_mod!` | 生成入口符号。没有它模组在装载时被拒绝。 |
| `Logger` | 打在你的模组名下。线程安全，且 `Copy`。 |

## 世界

| | |
|---|---|
| `World` | 时间、天气、难度、游戏规则、生物群系、区块存档键、村庄、结构、区域扫描。 |
| `Block` | 一格，按维度加坐标寻址。读写、方块状态、方块实体、液体层。 |
| `Server` | tick 冻结、步进、倍速，以及分项性能采样。 |

::: warning 含水方块是两个方块
基岩版的含水不是一个方块状态，而是同一格里的第二个方块。`Block::name` 只看主层，
所以复制粘贴一个含水楼梯会把水丢干净，除非你连 `Block::extra` 一起搬。
:::

## 玩家与实体

| | |
|---|---|
| `Player` | 用 `PlayerSel` 寻址，每次调用重新解析。 |
| `PlayerSel` | 按名字、xuid 或 uuid。`is_stable()` 说明它是不是身份。 |
| `Entity` | 按 `ActorUniqueID` 寻址的任何实体，玩家也算。`Player::as_entity()` 过去。 |
| `Container` | 玩家身上的四个，加坐标上的那一个。 |
| `ItemStack` | 值对象，不是句柄。 |

::: danger 只有 xuid 是身份
`PlayerSel::Name` 会退到显示名，而显示名可以被别的模组改。一个玩家把它改成某个
离线玩家的账号名，就能让所有按名字寻址的调用落到自己身上。权限、经济、归属用
`Player::by_xuid`。
:::

::: warning ItemStack 是快照
`container.item(0)` 返回的是一份拷贝。改它不会改容器；要用
`container.set_item(0, &stack)` 写回去，然后 `container.refresh()`。
:::

## 事件与命令

| | |
|---|---|
| `event` | `subscribe`、`subscribe_with`，以及批量用的 `Wiring`。 |
| `event::names` | 事件 id 常量。**用这些，不要写字面量。** |
| `Event` | 类型化取值、`dim()`、`player()`、`pos()`、`edit()`、`cancel()`。 |
| `command` | `register` 走原始文本，`builder` 走类型化 overload。 |

## 跨模组

| | |
|---|---|
| `service` | 一对一，有应答。名字独占。 |
| `bus` | 一对多，无应答。收不到自己发的。 |
| `lane` | 一对一，原生调用，仅限同工具链。 |

## 其余

| | |
|---|---|
| `gui` | 三种表单。回调至多跑一次，也可能一次都不跑。 |
| `scoreboard` | 记分项、分数、侧边栏。 |
| `money` | 经济桥。没有后端时降级成失败值。 |
| `kvdb` | 圈在模组自己数据目录里的键值库。线程安全。 |
| `packet` | 原始数据包拦截。**不在服务器线程上。** |
| `dimensions` | 自定义维度、地皮网格、逐维度规则。可选包。 |
| `client` | 客户端专属能力。服务端宿主上是空槽。 |
| `sim` | 模拟玩家。 |
| `nbt` | `NbtValue`、SNBT 解析与写出、二进制互转。 |

## 两个值得养成的习惯

**处理那个 `Err`。** 凡是可能答不上来的都返回它，而且写着为什么。
伸手去写 `unwrap_or` 就等于把这个 SDK 存在的意义所要防的那个 bug 又放了回来。

**接住句柄。** `Listener`、`Registration`、`Subscription`、`PacketHook`
都是 drop 即释放。随手丢掉在代码上和「忘了接住」长得一模一样，
所以真想那样时请写 `.forget()` 明说。
