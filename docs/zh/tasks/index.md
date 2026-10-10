# 📚 模组开发概述

## ⛳ 放在前面

Pier 是一个 LeviLamina 模组加载器：装上它以后，你可以用 **Rust**、**Go**、**Zig** 或 **C++** 给基岩版服务器写模组，
不同语言写的模组之间还能互相调用。

这一部分按「你想做什么」来分页：事件、命令、玩家、实体、世界、物品……每一页挑出做这件事最常用的接口，
配上 Rust、Go、Zig 的示例。每个绑定的全部接口在[接口参考](../api/index.md)里，按绑定分开列出。
开始看具体的任务之前，先花几分钟看完这一页，后面每一页都会用到这里说的约定。

## 💊 数据类型

### 通用数据类型约定

三种语言的类型系统不一样。同一个概念在各语言里分别对应下面这些类型：

| 概念 | Rust | Go | Zig | 说明 |
|---|---|---|---|---|
| 字符串 | `&str`（传入）、`String`（返回） | `string` | `[]const u8` | UTF-8 文本 |
| 整数 | `i32`、`i64` | `int32`、`int64` | `i32`、`i64` | |
| 小数 | `f64` | `float64` | `f64` | |
| 布尔 | `bool` | `bool` | `bool` | |
| 可能失败的结果 | `Result<T>` | `(T, error)` | `levilamina.Error!T` | 失败时带着原因 |
| 可能没有的值 | `Option<T>` | `(T, bool)` | `?T` | 「没有」不是错误 |
| 回调函数 | 闭包 | `func` | 编译期函数 | 由服务器在合适的时候调用 |
| SNBT | `NbtValue` | `*levilamina.Nbt` | `levilamina.nbt.Value` | 事件内容、物品、实体数据都是 SNBT |

### 引擎对象

除了上面这些通用类型，Pier 还为游戏里的东西准备了对象类型：

| 对象 | Rust | Go | Zig | 怎么指定 |
|---|---|---|---|---|
| 玩家 | `Player` | `levilamina.Player` | `levilamina.Player` | 名字、XUID 或 UUID，详见 [玩家对象](player.md) |
| 实体 | `Entity` | `levilamina.Entity` | `levilamina.Entity` | 唯一 id，详见 [实体对象](entity.md) |
| 方块 | `Block` | `levilamina.BlockAt` | `levilamina.BlockAt` | 维度加坐标，详见 [世界与方块](world.md) |
| 物品 | `ItemStack` | `levilamina.Item` | `levilamina.Item` | 一段 SNBT，详见 [物品与容器](item.md) |
| 容器 | `Container` | `levilamina.Container` | `c.PierContainerRef` | 玩家的背包，或者方块容器 |

## 📌 API 文档描述约定

每个接口都按同一个格式来写：

- **标题**：这个接口做什么。
- **调用形式**：三行，依次是 Rust、Go、Zig 的写法。`player`、`entity` 这类名字代表你手里的那个对象。
- **参数**：每个参数的名字、类型和含义。标着「可选参数」的可以不传。
- **返回值** 和 **返回值类型**：调用成功时你拿到什么。
- **对应槽位**：这个接口在 `abi.h` 函数表里的名字。C++ 直接用它调用；Zig 用 `levilamina.slot("名字")` 拿到它；
  Go 用 `levilamina.Raw` 下的同名方法调用。所以某个接口在你用的语言里还没有封装时，照着槽位名也能调用它。
- **示例**：三种语言各一段代码。

## 🔀 线程

**大部分接口只能在服务器主线程上调用。** 事件回调、命令回调、生命周期函数本来就在主线程上运行，在它们里面直接调用就行。

在你自己开的线程里（或者 Go 的 goroutine 里），用 [计划任务](scheduler.md) 把工作交回主线程。

## ⚠️ 出错时会怎样

调用失败时，你拿到的是一个错误，里面写着是哪一步、为什么。常见的有三种：

- **宿主没有这个功能**：Pier 版本太旧，或者没编译这个能力包。Go 里是 `*levilamina.NotProvidedError`，Zig 里是 `error.NotProvided`。
- **宿主拒绝了**：比如玩家不在线、坐标不合法。
- **宿主读不出来**：比如读一个它不认识的属性。玩家等级真是 0 时你拿到 `0`；宿主读不出等级时，你拿到的是这个错误。

## 📜 目录

| 页面 | 讲什么 |
|---|---|
| [🔄 生命周期与日志](lifecycle.md) | 模组怎么被加载、启用、禁用、卸载，怎么打日志 |
| [📣 事件监听](events.md) | 订阅事件、读取事件内容、取消事件 |
| [⌨️ 命令](commands.md) | 注册命令、带参数的命令、执行命令 |
| [🏃 玩家对象](player.md) | 找到玩家、发消息、传送、读写属性 |
| [🐷 实体对象](entity.md) | 找到实体、读属性、做动作、生成生物 |
| [🌍 世界与方块](world.md) | 时间、天气、游戏规则、读写方块 |
| [🎒 物品与容器](item.md) | 物品的 SNBT、箱子和背包 |
| [⏰ 计划任务](scheduler.md) | 延时执行、回到主线程 |
| [🔗 跨模组通信](crossmod.md) | 服务和总线，和其他语言的模组打交道 |
| [💾 数据存储与 NBT](data.md) | 键值数据库、解析 SNBT |
| [💰 经济](economy.md) | 余额、转账、交易监听 |
