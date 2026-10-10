# Go

Pier 的 Go 绑定把模组写成一个编译成 DLL 的 Go 包。导入 `github.com/Maskviva/pier/bindings/go/levilamina`，
代码里写 `levilamina.Subscribe(...)`；最小的可用模组是 `examples/hello-pier-go`。

路径里写的是 Pier，表明这个包属于哪套 ABI；包名写的是你在做的东西，一个 LeviLamina 模组。
Rust 绑定也是这样：包名是 `pier-rs`，crate 名是 `levilamina`。

## 它是怎么搭起来的

**cgo 直接读 `abi.h`。** 绑定不手写函数表镜像：cgo 包含这个头文件，`PierApi` 的布局由 C 编译器排定，和宿主完全一致。
包里带着一份头文件副本，因为 `go get` 拿不到模块之外的东西；有一项检查保证副本和原件逐字节相同。

**每个槽位都通过生成的 C 函数调用。** Go 不能调用 C 函数指针，所以 `slots_gen.h` 为每个槽位生成两个函数：
`piergo_call_<槽位>` 转发调用，`piergo_has_<槽位>` 执行契约里的两道检查（表够长、槽位不为空）。
它由 `tools/gen-go-slots.py` 从 `abi.h` 生成，过期时 `go-binding` 检查会失败。

**宿主调用导出的 Go 函数。** `pier_main`、生命周期回调，以及任务、事件、命令的回调都用 `//export` 导出，
再由一个小 C 文件把它们作为宿主期望的函数指针交出去。其中任何一个发生 panic 都会在边界被恢复并记日志，不会展开进宿主。

## 需要什么

- Go 1.21 或更高版本。
- `PATH` 上有 MinGW-w64 的 `gcc`，cgo 用它编译 C 部分。DLL 只通过 C 函数和 Pier 交互，在 x64 上 MinGW 和 MSVC
  的调用方式相同，所以 C++ 模组"必须是 MSVC ABI"的要求在这里不适用。
- `CGO_ENABLED=1`，用 `go build -buildmode=c-shared` 构建。

构建出来的 DLL 应当只依赖系统 DLL；依赖服务器上没有的 MinGW 运行时 DLL，会以错误 0x7E 加载失败。

## API

API 分三层，检查方式完全相同：

- **`levilamina.Raw`**：每个不带回调的槽位一个带类型的方法，共 173 个，从 `abi.h` 生成并带上它的文档。返回值的含义与 `abi.h` 的说明一致。
- **生成的门面**：常量表里的每个属性和动作在 `Player`、`Entity`、`BlockAt`、`Item` 上各有一个方法，比如 `Player.Level`、
  `Player.SetLevel`、`Entity.IsOnFire`、`Item.Count`；每个值也有对应的 Go 常量，比如 `PPropLevel`。
- **手写函数**，建在前两层之上，范围接近 Rust 绑定：

| 范围 | 函数 |
|---|---|
| 生命周期和日志 | `Register`、`Mod`、`Loader`、`Unloader`、`Context.Logger()` |
| 服务器和世界 | `Status`、`CurrentTick`、`Tps`、`Mspt`、`Time`、`SetWeather`、`GameRule`、`ExecuteCommand`、`ListPlayers`、`ListActors`、`SpawnMob`、`Explode`、`FillRegion`、`SaveLevel` |
| 任务 | `Schedule`、`ScheduleAfter`、`Cancel`、`PendingTasks` |
| 事件 | `Subscribe`、`Event.Payload`、`Event.Dim`、`Event.CheckComplete`、`Event.Set`、`Event.Cancel` |
| 命令和表单 | `RegisterCommand`、带类型重载的 `NewCommand`、`RegisterCommandEnum`、`RegisterSoftEnum`、`SendForm` |
| 玩家和实体 | `Player.SendMessage`、`Teleport`、`Inventory`、`CarriedItem`、`Entity.Snapshot`、`Owner`、`Target`，以及生成的属性方法 |
| 方块、物品、容器 | `BlockAt.Info`、`State`、`SetState`、`Item.Payload`、`Container.Items`、`AddItem`、`Clear` |
| 数据 | `OpenKvDb`、`ParseSNBT`、`Nbt`、`SnbtToBinary` |
| 经济 | `Money`、`AddMoney`、`TransferMoney`、`OnMoneyBefore`、`OnMoneyAfter` |
| 跨模组 | `BusPublish`、`BusPublishVetoable`、`BusSubscribe`、`RegisterService`、`CallService`、`ServiceCaller`；见[跨模组通信](cross-mod.md) |
| 网络 | `RegisterPacketHook`、`RegisterPacketHookIDs`、`RegisterConnHook` |
| 区域 | `ScanRegion`、`ScanRegionIndexed`、`SetBlocks` |
| 维度和模拟玩家 | `AddDimension`、`AddGeneratedDimension`、`DimensionRule`、`SimSpawn`、`SimDo` |
| 客户端 | `RegisterKey` |

宿主没有对应槽位的调用会返回 `*NotProvidedError`，可以用 `IsNotProvided` 判断；宿主答不上来的读取是 error，不会是 0 或 false。
表里的每个槽位 Go 都能调用，只有 Lane 的两个除外：Lane 按设计只连接同一工具链构建的模组，而发布 Lane 的模组同时也提供服务，Go 模组用的就是那个服务。

## 值得知道的规则

- **线程。** 宿主大部分功能只能在服务器线程上调用。生命周期、事件和命令回调本来就在那里运行；
  goroutine 要回到服务器线程，用 `Schedule`。
- **任务属于模组。** 模组卸载时还没运行的任务会被宿主丢弃，所以不会调用进已经卸载的 DLL。
- **载荷是副本。** `Event.SNBT` 和 `Invocation.Args` 都是 Go 字符串，回调返回后仍然有效。
  载荷里的字段见[事件载荷](../guide/event-payloads.md)。
- **每个 DLL 一份运行时。** 每个 Go 模组都带一整份 Go 运行时。一个服务器装多个 Go 模组，就是一个进程里跑多个运行时，
  这是 Go 没有测试过的用法；依赖它之前，先在你的服务器上确认可行。

[第一个 Go 模组](first-mod.md)一步步讲怎么构建和安装，[跨模组通信](cross-mod.md)讲 Go 模组怎样和其他语言的模组协作。
