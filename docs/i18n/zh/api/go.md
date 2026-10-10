## 4bf9b4f733

> Package levilamina writes LeviLamina mods in Go, on Pier's C ABI.
>
> A mod is a DLL built with `go build -buildmode=c-shared`. It implements Mod, calls
> Register from an init function, and has an empty main. The host calls pier_main after the
> Go runtime has run every init, so the registered mod is there when the handshake asks.
>
> Everything that reaches the host goes through the slot gates of contract section 10: a
> call to a slot the host does not have returns a *NotProvidedError without calling anything.

包 levilamina 用来在 Pier 的 C ABI 上用 Go 写 LeviLamina 模组。

一个模组是一个用 `go build -buildmode=c-shared` 构建的 DLL。它实现 `Mod`，在 init 函数里调用 `Register`，`main` 留空。宿主在 Go 运行时执行完所有 init 之后才调用 `pier_main`，所以握手的时候，注册好的模组已经在了。

所有到达宿主的调用都要经过契约第 10 节的槽位关卡：调用宿主没有的槽位，会返回一个 `*NotProvidedError`，不会调用任何东西。

## b5e9a93d39

> Status reports the server's life stage. Safe from any goroutine.

报告服务器所处的生命阶段。可以在任何 goroutine 里调用。

## fe8d0b685a

> Schedule runs fn on the server thread as soon as it can. Safe from any goroutine: most of
> the host is server-thread only, and this is how a goroutine gets back onto that thread.
>
> The task belongs to this mod: if the mod unloads first, the host drops it and fn never
> runs, so nothing calls into a DLL that is gone.

尽快在服务器线程上运行 `fn`。可以在任何 goroutine 里调用：宿主的大部分接口只能在服务器线程调用，goroutine 就是靠它回到那个线程上的。

任务属于这个模组：模组先卸载的话，宿主会丢掉这个任务，`fn` 永远不会运行，所以不会有调用进入一个已经不在的 DLL。

## 65cdf7d018

> ScheduleAfter runs fn on the server thread once d has passed, under the same ownership as
> Schedule. Safe from any goroutine.

经过 `d` 之后在服务器线程上运行 `fn`，归属规则和 `Schedule` 一样。可以在任何 goroutine 里调用。

## fddbb6d082

> Cancel voids a task that has not run. It reports false for a task that already ran, was
> cancelled, or is not this mod's. The function itself is released when the process ends,
> so cancelling in bulk on a hot path leaks.

作废一个还没运行的任务。已经运行过、已经取消过，或者不属于这个模组的任务返回 false。函数本身要到进程结束才释放，所以在热路径上大量取消会造成泄漏。

## 6a1dac5db6

> PendingTasks counts this mod's tasks that have not run, which an Unload can check is 0.
> A host that cannot count returns an error, which that check receives in place of a 0.

这个模组还没运行的任务数，`Unload` 可以检查它是否为 0。宿主数不出来时返回错误，这时那项检查拿到的是一个错误，没有拿到 0。

## 3d6c60f84d

> Register names the mod this DLL is. Call it once, from an init function; a DLL with
> nothing registered refuses to load, and a second call panics, since a DLL is one mod.

声明这个 DLL 是哪个模组。在 init 函数里调用一次；没有注册任何模组的 DLL 会拒绝加载，调用第二次会 panic，因为一个 DLL 就是一个模组。

## a13fb90e67

> IsNotProvided reports whether err says the host lacks a slot.

判断 `err` 是否表示宿主缺少某个槽位。

## 778a5b1ff8

> Logger writes to the server log under this mod's name. The zero value is ready, and it
> is safe from any goroutine.

以这个模组的名义写服务器日志。零值就能直接用，可以在任何 goroutine 里调用。

## 6fc8bd89ea

> Error logs at error level.

以 error 级别记录日志。

## f7c2af2ded

> Warn logs at warning level.

以 warning 级别记录日志。

## e9a00893f6

> Info logs at info level.

以 info 级别记录日志。

## da3e81087d

> Debug logs at debug level.

以 debug 级别记录日志。

## 2142e2a986

> Context is what a lifecycle step receives.

生命周期的每一步收到的东西。

## f95d8dab67

> Logger returns the logger of this mod.

返回这个模组的日志器。

## d0ef1bcde3

> NotProvidedError is the error of a call whose slot this host does not have: the host is
> older than this binding, or the capability package was not built into it.

调用的槽位这个宿主没有时返回的错误：宿主比这个绑定旧，或者没有编入对应的能力包。

## 545f2a3940

> GamingStatus is the server's life stage, as LeviLamina reports it.

LeviLamina 报告的服务器生命阶段。

## efb66fa1cd

> TaskID names a scheduled task, for Cancel.

一个定时任务的名字，供 `Cancel` 使用。

## 35fa38391c

> Mod is the lifecycle a Go mod implements. Both steps run on the server thread, and a
> returned error is logged and refuses the step.

Go 模组要实现的生命周期。两步都在服务器线程上运行；返回错误时会记录日志，并拒绝这一步。

## 293e96a595

> Loader is implemented by a mod with work to do at load, before it is enabled.

加载时（启用之前）有事要做的模组实现它。

## 06f527809c

> Unloader is implemented by a mod with work to do at unload, after it is disabled.

卸载时（禁用之后）有事要做的模组实现它。

## 4dfaa7cc90

> Subscribe calls fn for every event with this id, on the server thread. The id is the full
> one, such as "ll::event::PlayerJoinEvent", or a unique suffix of it. Server thread only.

对这个 id 的每一个事件，在服务器线程上调用 `fn`。id 是完整的 id，例如 `"ll::event::PlayerJoinEvent"`，也可以是它唯一的后缀。只能在服务器线程调用。

## f4363b93ab

> ListEvents lists every event id this host can subscribe to. Server thread only.

列出这个宿主可以订阅的所有事件 id。只能在服务器线程调用。

## 92433ad2fa

> Event is one event as a listener receives it. SNBT is the payload, whose fields the
> event payload reference documents; it is a copy, valid after the callback returns.

监听器收到的一个事件。`SNBT` 是载荷，它的字段写在事件载荷参考里；它是一份副本，回调返回以后仍然有效。

## 0e801b0c87

> Payload is SNBT parsed, once per event.

解析后的 `SNBT`，每个事件只解析一次。

## dad38c2f32

> Unresolved lists the fields the host could not resolve, from _unresolved. It is empty
> too when the payload does not parse, which CheckComplete does not count as complete.

列出宿主没能解析的字段，取自 `_unresolved`。载荷本身解析不了时它也是空的，`CheckComplete` 不会把这种情况算作完整。

## d629a8eb5a

> CheckComplete reports whether the payload parsed and nothing in it is unresolved. A
> protection decision on an incomplete payload should refuse rather than guess.

判断载荷是否解析成功，并且里面没有未解析的字段。基于不完整的载荷做保护判断时，应当拒绝，不要去猜。

## 8d062b9236

> Dim is the dimension the event happened in. An unreadable or incomplete payload is an
> error and never 0: read as the overworld it would let an event in a custom dimension
> pass a rule made for the overworld.

事件发生的维度。载荷读不出来或者不完整时返回错误，永远不会返回 0：如果当成主世界，自定义维度里的事件就会通过为主世界定的规则。

## d6837e0524

> Set writes key back into the event when the callback returns. Only the keys set go back,
> and the host merges them, so two listeners setting different keys keep both.

在回调返回时把 `key` 写回事件里。只有设置过的键会写回去，宿主会合并它们，所以两个监听器设置不同的键时，两边的改动都会保留。

## befececaf7

> Cancel asks the host to cancel the event once the callback returns. An event that cannot
> be cancelled ignores it; the documentation of each event says whether it can be.

请求宿主在回调返回后取消这个事件。不能取消的事件会忽略这个请求；每个事件能不能取消，写在它的文档里。

## 724a183b24

> Uncancel undoes a Cancel of this callback. A cancel made by an earlier listener stays.

撤销这个回调里做的 `Cancel`。之前的监听器做的取消会保留。

## c72451926b

> Listener is one subscription, kept to end it.

一个订阅，保存下来用来结束它。

## cb4ac573ba

> Unsubscribe ends the subscription. Calling it twice is harmless. Server thread only.

结束订阅。调用两次也没有问题。只能在服务器线程调用。

## 59c8d7c8ef

> ListenerHandle is an event listener as the host names it.

宿主用来指代一个事件监听器的句柄。

## 647110162c

> IsZero reports whether the handle names nothing.

判断这个句柄是否什么都不指向。

## cdb914412d

> Priority orders the listeners of one event; it mirrors ll::event::EventPriority.

同一个事件的监听器之间的顺序，对应 `ll::event::EventPriority`。

## 8590861369

> ExecuteCommand runs cmd as the console and returns what it printed. Server thread only. An
> error means the level is not ready, or the host cannot run commands.

以控制台的身份运行 `cmd`，返回它输出的内容。只能在服务器线程调用。返回错误表示世界还没就绪，或者宿主不能运行命令。

## b5be827aa0

> RegisterCommand registers /name, whose text after the name reaches fn as Args. Call it
> from Enable, on the server thread. A command stays registered while the server runs,
> since Bedrock cannot unregister one; the host mutes it while the mod is disabled.

注册 `/name`，命令名之后的文本作为 `Args` 交给 `fn`。在 `Enable` 里、在服务器线程上调用。命令在服务器运行期间一直保持注册，因为基岩版不能注销命令；模组被禁用时，宿主会屏蔽它。

## 8466aaa3a9

> NewOverload begins an overload.

开始一个重载。

## e234767db4

> NewCommand begins declaring /name.

开始声明 `/name`。

## c4fe335621

> RegisterCommandEnum registers a fixed enum for RequiredEnum and OptionalEnum.

注册一个固定的枚举，供 `RequiredEnum` 和 `OptionalEnum` 使用。

## 4763b50c7c

> RegisterSoftEnum registers an enum whose values can change while the server runs.

注册一个值可以在服务器运行时改变的枚举。

## ae297128e3

> UpdateSoftEnum replaces, adds to or removes from the values of a soft enum.

替换一个软枚举的值，或者向它添加值、从它移除值。

## 64083b1c6d

> CommandCall is one run of a command declared with CommandBuilder.

用 `CommandBuilder` 声明的命令的一次执行。

## 35897631f3

> Arg is a named argument, and false for an optional one left out.

按名字取一个参数；可选参数没有给出时返回 false。

## b4ab5f42cd

> ArgString is a named argument as text.

按名字取一个参数，作为文本。

## b0e63fa875

> ArgInt is a named integer argument.

按名字取一个整数参数。

## 42034437aa

> ArgFloat is a named number argument.

按名字取一个数值参数。

## 3dd9972d22

> IsConsole reports whether the dedicated server console ran the command.

判断这条命令是不是由专用服务器的控制台运行的。

## 14d0fa31b0

> Success sends a line of output.

发送一行输出。

## 530015ab8b

> Error sends a line of error output.

发送一行错误输出。

## 30005768dd

> Overload is one way a command parses: its parameters in order.

命令的一种解析方式：按顺序排列的参数。

## b127782625

> Required adds a parameter that must be given.

添加一个必须给出的参数。

## d4e89f5d55

> Optional adds a parameter that may be left out; Arg answers false for it then.

添加一个可以省略的参数；省略时 `Arg` 对它返回 false。

## f94662d813

> RequiredEnum adds an enum parameter, which RegisterCommandEnum or RegisterSoftEnum
> registered first.

添加一个枚举参数，枚举要先用 `RegisterCommandEnum` 或 `RegisterSoftEnum` 注册。

## a32a77f78e

> OptionalEnum adds an enum parameter that may be left out.

添加一个可以省略的枚举参数。

## de026b0728

> Text adds a literal word, the shape /scoreboard objectives add is made of. The word comes
> back among the arguments under name, so a handler reads it like any other parameter. Sibling
> words taking the same arguments are one enum instead (contract section 6.1).

添加一个字面单词，`/scoreboard objectives add` 就是由这样的单词组成的。这个单词会以 `name` 为名出现在参数里，处理函数读它和读其他参数一样。接收相同参数的几个并列单词，应当合成一个枚举（契约 6.1 节）。

## 64de7a3b87

> Invocation is one run of a command. Success and Error may be called any number of times
> during the callback and not after it returns.

命令的一次执行。回调期间可以任意多次调用 `Success` 和 `Error`，回调返回以后就不能再调用了。

## 0826867f6d

> CommandBuilder declares a command with typed overloads. Each overload must parse a
> different input: two that accept the same words leave the engine to pick one.

声明一个带类型化重载的命令。每个重载解析的输入必须不同：两个重载接受同样的单词时，引擎会自己从中挑一个。

## 646059de3e

> Overload adds one way the command parses.

添加命令的一种解析方式。

## 6395883893

> Register registers the command. fn runs on the server thread for every run; a payload the
> binding cannot parse is reported to whoever ran the command and fn does not run.

注册这条命令。每次执行时 `fn` 都在服务器线程上运行；绑定解析不了的载荷会报告给执行命令的人，这时 `fn` 不会运行。

## 2e8ebf2380

> CommandOutput is one line a command printed.

命令输出的一行。

## 118bdadaaf

> Permission is who may run a command; it mirrors CommandPermissionLevel.

谁可以运行一条命令，对应 `CommandPermissionLevel`。

## 33ecf045b6

> ParamType is the type of one command parameter, the kind words of register_command_ex.

一个命令参数的类型，也就是 `register_command_ex` 的 kind 单词。

## 18594eb6d6

> CommandOrigin is who ran a command and where.

谁在什么地方运行了这条命令。

## 077ee6d2f0

> EnumValue is one value of a command enum.

命令枚举的一个值。

## 46fe75caf8

> SoftEnumOp is how UpdateSoftEnum changes the values.

`UpdateSoftEnum` 怎样改变枚举的值。

## 04b2646192

> CurrentTick is the server's tick counter.

服务器的刻计数器。

## 94c5592047

> TickDeltaTime is the length of the last tick, in seconds.

上一刻的长度，单位是秒。

## 62f44ae9d6

> PlayerCount is how many players are online.

在线的玩家数。

## 54da9b76d7

> SimPaused reports whether the simulation is paused.

判断模拟是否暂停。

## 1f9cb97d1b

> Tps is the ticks per second over the last windowSeconds. A negative answer, which the
> host gives before it has measured anything, is an error and not a rate.

最近 `windowSeconds` 秒里每秒的刻数。宿主还没有测量到任何东西时会给出负数，这时返回错误，不把它当作速率。

## fb12c869b4

> Mspt is the milliseconds per tick over the last windowSeconds, with Tps's rule.

最近 `windowSeconds` 秒里每刻的毫秒数，规则和 `Tps` 一样。

## 25e14d0e64

> Env reads an environment variable of the server process.

读取服务器进程的一个环境变量。

## 1400ccf72d

> SetEnv sets an environment variable of the server process.

设置服务器进程的一个环境变量。

## c391f84636

> ScanRegion reads every block and entity in a box. Server thread only.

读取一个长方体里的每个方块和实体。只能在服务器线程调用。

## 5f79e60c9f

> ScanRegionIndexed reads a box as a palette of distinct block states and the cells that
> index it, which is far smaller than ScanRegion for a large box. Server thread only.

把一个长方体读成由不同方块状态组成的调色板，加上引用调色板的格子；长方体很大时，结果比 `ScanRegion` 小得多。只能在服务器线程调用。

## 778a4bc3c4

> GetBlock reads the block at a cell. Server thread only.

读取一格的方块。只能在服务器线程调用。

## 0a26dc423f

> Seed is the level seed.

世界种子。

## 706092e863

> Difficulty is the level difficulty, 0 peaceful to 3 hard.

世界难度，从 0 和平到 3 困难。

## e25c6c79e9

> SetDifficulty sets the level difficulty.

设置世界难度。

## 6c3cd7f6e1

> GameRule reads a game rule as text.

以文本形式读取一条游戏规则。

## b50649645e

> SetGameRule writes a game rule from text.

以文本形式写入一条游戏规则。

## 3096b7604c

> Time is the time of day in ticks.

一天中的时间，单位是刻。

## 42b2156a69

> SetTime sets the time of day in ticks.

设置一天中的时间，单位是刻。

## ba3703ca2d

> SetWeather sets the weather: 0 clear, 1 rain, 2 thunder.

设置天气：0 晴，1 雨，2 雷雨。

## 9d9ec0b156

> SaveLevel saves the level now.

立刻保存世界。

## bfb9c983f5

> SpawnParticle shows a particle effect to everyone near it.

向附近的所有人显示一个粒子效果。

## b036988449

> Explode makes an explosion of the given radius.

按给定的半径制造一次爆炸。

## 6e5fb228dc

> DefaultSpawn is the level's world spawn.

世界的出生点。

## f65891502e

> SetDefaultSpawn moves the level's world spawn.

移动世界的出生点。

## 46ff951668

> SetBiome sets the biome of every column in a rectangle and returns how many changed.

设置一个矩形内每一列的生物群系，返回改变了多少列。

## cf152758a4

> EntityInfo is one entity a region scan found: the cell holding it, its type and its SNBT.

区域扫描找到的一个实体：它所在的格子、它的类型和它的 SNBT。

## ec78594a1e

> PaletteEntry is one distinct block state of an indexed scan.

带索引扫描里的一种不同的方块状态。

## bf4781aabf

> BlockCell is one cell and an index into a palette, for ScanRegionIndexed and SetBlocks.

一个格子加上调色板里的一个索引，供 `ScanRegionIndexed` 和 `SetBlocks` 使用。

## 0423b6ba37

> ExplodeOptions are the optional parts of an explosion.

爆炸的可选部分。

## 0c47c14664

> SetBlocks writes many blocks in one call: palette holds block specs and each cell names one
> by index. It returns how many cells changed. Server thread only.

一次调用写入很多方块：`palette` 里是方块描述，每一格用索引指定其中一个。返回改变了多少格。只能在服务器线程调用。

## ed71ac9415

> FillRegion fills a box with one block and returns how many cells changed.

用一种方块填满一个长方体，返回改变了多少格。

## 15cef79797

> PlayerByName is the player with this name. A decision about permissions, money or
> ownership uses PlayerByXuid: a name can change hands.

这个名字的玩家。关于权限、金钱或归属的判断要用 `PlayerByXuid`：名字会换主人。

## fa7496dd10

> PlayerByXuid is the player with this xuid.

这个 xuid 的玩家。

## 163d4c48cd

> PlayerByUuid is the player with this uuid.

这个 uuid 的玩家。

## 35af4870d1

> Broadcast sends a message to every online player.

给每个在线玩家发送一条消息。

## 72df71a531

> ListPlayers lists the online players. An entry that does not parse is skipped with a
> warning, so one bad entry does not make who is online unanswerable.

列出在线玩家。解析不了的条目会被跳过，并记一条警告，所以一条坏数据不会让「谁在线」变得答不上来。

## f74986ce4b

> ByName selects a player by name.

按名字选择一名玩家。

## ecbc05bbd4

> ByXuid selects a player by xuid.

按 xuid 选择一名玩家。

## e3a9eb2d98

> ByUuid selects a player by uuid.

按 uuid 选择一名玩家。

## 4a50cd818b

> Player is one player, named by a selector. Most of its methods are generated from the
> constant tables of abi.h, one per property and verb; the rest are below.

一名玩家，由选择器指定。它的大部分方法由 abi.h 的常量表生成，每个属性和动作一个；其余是手写的方法。

## bcba2a5e95

> Resolve is the player's actor. An error means nobody matches the selector.

这名玩家对应的实体。返回错误表示没有人和选择器对得上。

## 51765b258d

> IsOnline reports whether the selector matches an online player. Every ABI v2 host has the
> slot it asks, so false means nobody matches.

判断选择器是否对得上一名在线玩家。每个 ABI v2 宿主都有它用到的那个槽位，所以 false 就表示没有人对得上。

## 68f34de64d

> SendMessage sends a chat line to the player.

给这名玩家发送一行聊天消息。

## 1b5c5e19dc

> SendMessageTyped sends a message of a given TextPacket type to the player.

给这名玩家发送一条指定 `TextPacket` 类型的消息。

## 36e428d9a5

> SendTitle shows a title: slot 0 title, 1 subtitle, 2 action bar; the times are in ticks.

显示一个标题：`slot` 为 0 是主标题，1 是副标题，2 是动作栏；时间的单位是刻。

## 2ef805b4ab

> Disconnect removes the player from the server with a reason.

带着原因把这名玩家从服务器上移除。

## d43b61dfb4

> SetGameType sets the player's game mode, the value GameType reads.

设置这名玩家的游戏模式，也就是 `GameType` 读到的那个值。

## 8589d272b7

> Teleport moves the player to a position of any registered dimension.

把这名玩家移到任何已注册维度里的一个位置。

## a68daee125

> CarriedItem is the item in the player's hand.

这名玩家手里拿着的物品。

## 8ec8c63fcf

> InventoryItem is the item in one inventory slot.

物品栏某一格里的物品。

## 0f2ba339ff

> SetInventoryItem puts an item into one inventory slot.

把一个物品放进物品栏的某一格。

## 5273b012d6

> ConnID is the player's network connection id.

这名玩家的网络连接 id。

## ad8267c023

> Inventory is the player's inventory as a container.

把这名玩家的物品栏当作容器。

## bf3ab7197d

> EnderChest is the player's ender chest as a container.

把这名玩家的末影箱当作容器。

## 5b17966951

> Armor is the player's armor slots as a container.

把这名玩家的盔甲栏当作容器。

## ed8127a8ea

> Offhand is the player's offhand slot as a container.

把这名玩家的副手栏当作容器。

## 90bb5463a0

> PlayerInfo is one online player as ListPlayers reports it. Dim and the position are
> read only when present, and HasDim and HasPos say whether they were.

`ListPlayers` 报告的一名在线玩家。`Dim` 和位置只在有的时候才读取，`HasDim` 和 `HasPos` 说明它们有没有。

## b0d2ae5078

> SelKind says how a PlayerSel names its player.

说明 `PlayerSel` 用什么来指定玩家。

## 7f8d6a771b

> PlayerSel names one player by name, xuid or uuid.

按名字、xuid 或 uuid 指定一名玩家。

## ad1fb60276

> PlayerPos is where a player is; Found is false when nobody matched.

一名玩家所在的位置；没有人对得上时 `Found` 为 false。

## f3f88b3e88

> ListActors lists every actor in a dimension. Server thread only.

列出一个维度里的所有实体。只能在服务器线程调用。

## bd47fa6e4f

> EntityByID is the actor with this unique id, the uid of event payloads.

这个唯一 id 的实体，id 就是事件载荷里的 uid。

## 4a093cf7f3

> SpawnMob spawns a mob and returns it.

生成一个生物，并返回它。

## ca5c3bbf65

> Entity is one actor, named by its unique id.

一个实体，由它的唯一 id 指定。

## b5d979bb67

> Snapshot is the actor's full NBT as SNBT.

这个实体完整的 NBT，以 SNBT 给出。

## 88f0642aad

> Vehicle is what the actor rides.

这个实体骑着的东西。

## 50e1111952

> Owner is the actor's owner, for a tamed or summoned one.

这个实体的主人，用于驯服的或召唤出来的实体。

## e39b26a81b

> Target is the actor's current target.

这个实体当前的目标。

## b1634b3317

> DistanceTo is the distance to another actor.

到另一个实体的距离。

## eae33d43ea

> Clone copies the actor to a position and returns the copy.

把这个实体复制到一个位置，返回复制出来的实体。

## d8cd334936

> ActorInfo is one actor of a dimension: its unique id and its type name.

一个维度里的一个实体：它的唯一 id 和类型名。

## 37ebaf28f6

> ActorID is an actor's unique id, the `uid` of event payloads. 0 never resolves.

实体的唯一 id，即事件载荷里的 `uid`。0 永远解析不到。

## f0c58acba7

> Block is the cell at x, y, z of dimension dim.

维度 `dim` 里 x、y、z 处的那一格。

## 89a99196b4

> BlockAt is one block cell of a dimension.

一个维度里的一个方块格。

## 1ccab91483

> Info reads the block: its type name and state.

读取这个方块：它的类型名和状态。

## 5e4b7c9a3d

> Set places a block from a block spec, as /setblock reads one.

按方块描述放置一个方块，描述的写法和 `/setblock` 读的一样。

## cd64ef6944

> State reads one block state by name.

按名字读取一个方块状态。

## 80da2e66c7

> SetState writes one block state by name.

按名字写入一个方块状态。

## bf34078f20

> EntitySNBT is the block entity at the cell, such as a chest's contents, as SNBT.

这一格的方块实体（比如箱子里的东西），以 SNBT 给出。

## bdd1c7f765

> Biome is the biome at the cell.

这一格的生物群系。

## 0d6d5efd62

> Container is the cell's block container, such as a chest.

这一格的方块容器，比如箱子。

## 26cabcea9f

> BlockInfo is one block: its cell, its type name and its block state as SNBT.

一个方块：它的格子、类型名，以及以 SNBT 表示的方块状态。

## 573e4ff3fa

> ContainerItems lists the occupied slots of a container. Server thread only.

列出一个容器里有东西的格子。只能在服务器线程调用。

## 127e70bd8f

> ItemOf is the item stack these SNBT describe.

这段 SNBT 描述的物品堆。

## bd148f8271

> Item is an item stack as SNBT, a value: reading it asks the host about that SNBT, and
> changing it produces a new Item.

以 SNBT 表示的物品堆，是一个值：读取它，就是拿这段 SNBT 去问宿主；修改它会得到一个新的 `Item`。

## 3b489b475e

> Payload is the item's SNBT parsed.

解析后的物品 SNBT。

## c162008cb7

> Enchants lists the item's enchantments as SNBT.

以 SNBT 列出这个物品的附魔。

## d4b9455f32

> Matches reports whether two items stack: the same item, data and user data.

判断两个物品能否堆叠：同一种物品，数据值和用户数据都相同。

## e9a8de5e00

> Transform applies one of the PIER_IOP_* operations of abi.h and returns the new item.

执行 abi.h 里的某一个 `PIER_IOP_*` 操作，返回新的物品。

## 9e29dc9206

> Container is a player's or a block's container.

一名玩家或一个方块的容器。

## 2c046fffa6

> Size is the number of slots.

格子的数量。

## 1bef0e5337

> Item is the item in one slot.

某一格里的物品。

## 331580d32c

> SetItem puts an item into one slot.

把一个物品放进某一格。

## 84af76909f

> AddItem adds an item where it fits.

把一个物品放进放得下的地方。

## f06cf63bb3

> RemoveItem removes count items from one slot.

从某一格移除 `count` 个物品。

## 1919f381db

> Clear empties the container.

清空这个容器。

## cf0640b12e

> Items lists the occupied slots.

列出有东西的格子。

## 7f900dbcd2

> SlotItem is one occupied slot of a container and its item as SNBT.

容器里一个有东西的格子，以及以 SNBT 表示的物品。

## 92150044f2

> ContainerRef names a container: a player's inventory, ender chest, armor or offhand, or
> the container of a block.

指定一个容器：玩家的物品栏、末影箱、盔甲栏或副手，或者一个方块的容器。

## 0d817e492b

> The values of ContainerRef.Which.

`ContainerRef.Which` 的取值。

## 0d4a0ce713

> Scoreboard runs one PIER_SB_* operation and returns its output, when it has one.

执行一个 `PIER_SB_*` 操作，有输出时返回输出。

## 29f593d202

> SendForm shows a form to a player. fn runs once, on the server thread, with the result
> SNBT, including when the player closes the form; abi.h documents the result's shape.

向一名玩家显示一个表单。`fn` 在服务器线程上运行一次，收到结果的 SNBT，玩家关掉表单时也会运行；结果的形状写在 abi.h 里。

## a5add6fb14

> SnbtToBinary encodes SNBT as binary NBT in format 0, the little-endian disk layout, or
> format 1, the network layout.

把 SNBT 编码成二进制 NBT：格式 0 是小端的磁盘布局，格式 1 是网络布局。

## 9e7f3fa144

> OpenKvDb opens the store at path, creating it when createIfMissing is set.

打开 `path` 处的存储；设置了 `createIfMissing` 时，不存在就创建。

## abcbad88a9

> KvDb is an open key-value store, kept by the host under the mod's data directory.

一个已打开的键值存储，由宿主保存在模组的数据目录下。

## e774a48c23

> Path is where the store was opened.

打开存储时用的路径。

## 69eba63478

> Get reads one key; found is false for a key that does not exist. A store the host closed
> on its own, at an unload, also reads as not found: the ABI answers both alike.

读取一个键；键不存在时 `found` 为 false。宿主自己关掉的存储（卸载时）读起来也是不存在：ABI 对这两种情况给出同样的回答。

## dbcf48db17

> Set writes one key.

写入一个键。

## 1db4f7cc02

> Delete removes one key.

删除一个键。

## a6c83a2c43

> Has reports whether a key exists.

判断一个键是否存在。

## 73d31725c5

> IsEmpty reports whether the store holds no key.

判断存储里是否一个键都没有。

## 404fd02ae8

> Iter lists every entry.

列出所有条目。

## 17eafa4f3d

> Close closes the store; using it afterwards fails.

关闭存储；之后再使用会失败。

## 7572458e11

> KvDbHandle is an open key-value store; the zero value is no store.

一个已打开的键值存储；零值表示没有存储。

## aa854676d4

> KeyValue is one entry of a key-value store.

键值存储里的一个条目。

## eb3d5c53e9

> OnMoneyBefore calls fn before every economy transaction; returning false vetoes it.

在每一笔经济交易之前调用 `fn`；返回 false 否决这笔交易。

## ea8c13ad8e

> OnMoneyAfter calls fn after every economy transaction.

在每一笔经济交易之后调用 `fn`。

## 2a358e9a8a

> Money is a balance. A negative answer means it cannot be read, an empty xuid or no
> economy backend, and is an error: a real balance is never negative.

一个余额。读不出来（xuid 为空，或者没有经济后端）时宿主给出负数，这里把它作为错误返回：真实的余额不会是负数。

## ddafdb4c82

> SetMoney sets a balance.

设置余额。

## caa34c516b

> AddMoney adds to a balance.

增加余额。

## c9ea150b97

> ReduceMoney takes from a balance.

减少余额。

## dafea0e975

> TransferMoney moves value between two balances; the recipient receives it taxed.

在两个余额之间转移 `value`；收款方收到的是扣税后的金额。

## 2ea0580adb

> MoneyKind is what an economy event does, one of the Money* kinds. A value outside them is
> one a newer economy backend sent; a veto listener should refuse it rather than guess.

一次经济事件做了什么，是 `Money*` 几种之一。超出这几种的值来自更新的经济后端；否决监听器应当拒绝它，不要去猜。

## eea260c455

> MoneyEvent is one economy transaction.

一笔经济交易。

## c8c04d7bc2

> RegisterPacketHook calls fn for every packet in the directions of dirMask, the
> PIER_PKT_MASK_* bits of abi.h: 1 for inbound, 2 for outbound. fn runs where the host pumps
> the connection or sends the packet, which is usually the server thread and not always,
> since a flush can be asynchronous; fn keeps to its own state and reaches the world through
> Schedule.

对 `dirMask` 指定方向上的每个数据包调用 `fn`，`dirMask` 用的是 abi.h 的 `PIER_PKT_MASK_*` 位：1 表示收到的包，2 表示发出的包。`fn` 在宿主泵送连接或发送数据包的地方运行，通常是服务器线程，但不总是，因为刷新可能是异步的；`fn` 只碰自己的状态，要碰世界就通过 `Schedule`。

## 0eca6bddae

> RegisterPacketHookIDs is RegisterPacketHook for the listed packet ids only: a packet whose
> id no hook listed is passed on before fn runs or any lock is taken.

只对列出的数据包 id 生效的 `RegisterPacketHook`：没有任何钩子列出其 id 的数据包，在 `fn` 运行、加锁之前就直接放行。

## 4663f34b4a

> RegisterConnHook calls fn when a connection opens and when it closes, with the threading
> RegisterPacketHook describes.

在连接打开和关闭时调用 `fn`，线程规则和 `RegisterPacketHook` 写的一样。

## 60c1fe7b0b

> PacketCall is one packet inside a hook, with the ways to change it.

钩子里的一个数据包，以及修改它的方法。

## 795cb76206

> SetPacketID makes the forwarded packet carry another id.

让转发出去的数据包带上另一个 id。

## 70bed3ff4c

> SetSubIDs changes the sender and target sub-client ids of the forwarded packet.

修改转发出去的数据包的发送方和目标子客户端 id。

## 05ea81dc58

> Replace hands over the body to forward; it takes effect when the hook returns
> PacketReplace.

交出要转发的包体；钩子返回 `PacketReplace` 时生效。

## a1bbb118ff

> PacketHook is a registered packet or connection hook, kept to remove it.

一个已注册的数据包钩子或连接钩子，保存下来用来移除它。

## 54e7cb252c

> Unregister removes the hook. Calling it twice is harmless.

移除这个钩子。调用两次也没有问题。

## 26c8d92d1b

> PacketHookHandle is a registered packet hook.

一个已注册的数据包钩子。

## 8372df972f

> PacketVerdict is what a packet hook decides about one packet.

数据包钩子对一个数据包做出的决定。

## f65d0f2187

> PacketEvent is one packet a hook sees. Body is a copy.

钩子看到的一个数据包。`Body` 是一份副本。

## ce412f97a4

> SimSpawn spawns a simulated player.

生成一个模拟玩家。

## 57e2a90481

> SimDo runs one verb on a simulated player; args is SNBT, "{}" when there is none.

让模拟玩家执行一个动作；`args` 是 SNBT，没有参数时为 `"{}"`。

## 1d9217d899

> IsSimulated reports whether name is a live simulated player.

判断 `name` 是不是一个活着的模拟玩家。

## 32a1461cb4

> SimList lists the live simulated players.

列出活着的模拟玩家。

## a3bb9c5255

> RegisterKey binds keys on the client: fn runs with pressed true on press and false on
> release, and the current focus impact. Client hosts only; a server answers not provided.

在客户端上绑定按键：按下时以 `pressed` 为 true 调用 `fn`，松开时为 false，同时传入当前的焦点影响。只有客户端宿主支持；服务器会回答「没有提供」。

## 4c022e700c

> KeyBinding is a registered client key binding.

一个已注册的客户端按键绑定。

## 5da1c312a1

> Unregister stops the binding's callbacks.

停止这个绑定的回调。

## 0d7672ef42

> KeyHandle is a registered client key binding.

一个已注册的客户端按键绑定。

## e57950d965

> AddGeneratedDimension registers a dimension whose terrain fill writes, and returns its id.
>
> fill runs on the host's chunk worker threads, several at once: it must be safe to run
> concurrently, give the same answer for the same chunk forever, and call nothing on this
> package but the request itself. Returning false leaves the chunk to air. materials[0]
> must be "minecraft:air". The function is kept for the dimension's whole life.

注册一个由 `fill` 写入地形的维度，返回它的 id。

`fill` 在宿主的区块工作线程上运行，同时可能有好几个：它必须能并发运行，对同一个区块永远给出同样的结果，并且除了请求本身，不调用这个包里的任何东西。返回 false 时，这个区块留给空气。`materials[0]` 必须是 `"minecraft:air"`。这个函数在维度的整个生命期里都会被保留。

## cbfbe7edee

> DimensionsAvailable reports whether this host can register custom dimensions.

判断这个宿主能不能注册自定义维度。

## 6032a185c2

> AddDimension registers a dimension from spec SNBT and returns its id.

用描述 SNBT 注册一个维度，返回它的 id。

## 7058e46ee7

> DimensionID is the id of a registered dimension.

一个已注册维度的 id。

## 83602b10a4

> ListDimensions lists the registered custom dimensions.

列出已注册的自定义维度。

## 460dfc672d

> SetDimensionRule sets one rule, a PIER_DIMRULE_* value, for one dimension.

为一个维度设置一条规则，规则是某个 `PIER_DIMRULE_*` 值。

## 72d4172a81

> DimensionRule reads one rule; set is false when the dimension follows vanilla for it,
> which a host older than the rule also answers.

读取一条规则；这个维度在这条规则上沿用原版行为时 `set` 为 false，比这条规则更旧的宿主也这样回答。

## c7c52eb82c

> ChunkRequest is one chunk a supplied-terrain generator fills. Materials has 256*Height
> entries indexed (x*16+z)*Height + (y-MinY), and Biomes 256, one per column; both hold
> indices into the palettes given at registration, and material 0 is air.

自供地形的生成器要填充的一个区块。`Materials` 有 `256*Height` 项，下标是 `(x*16+z)*Height + (y-MinY)`；`Biomes` 有 256 项，每列一项；两者存的都是注册时给出的调色板里的索引，材料 0 是空气。

## 78faf67c55

> BusSubscribe calls fn for every message published on topic, on the publisher's thread,
> whichever language the publisher is written in. fn returns a veto: true refuses a vetoable
> publish and false has no opinion, and a plain publish ignores it. A subscriber can only
> refuse, never overturn another's refusal. Namespace topics, as "plot:enter".

对 `topic` 上发布的每一条消息调用 `fn`，在发布方的线程上调用，不管发布方是用什么语言写的。`fn` 返回否决：true 拒绝一次可否决的发布，false 表示没有意见，普通的发布忽略它。订阅者只能拒绝，不能推翻别人的拒绝。主题请加命名空间，比如 `"plot:enter"`。

## 63c81efe4c

> BusPublish sends payload to every subscriber of topic and returns how many ran; 0 is a
> normal answer, meaning nobody is listening. The payload is opaque to the host, so the two
> mods agree on its format, JSON or SNBT, out of band.

把 `payload` 发给 `topic` 的每一个订阅者，返回运行了多少个；0 是正常的回答，表示没有人在听。载荷对宿主是不透明的，所以两个模组要在别处约定它的格式，JSON 或者 SNBT。

## f78ed5ec4c

> BusPublishVetoable sends payload and collects the subscribers' vetoes.

发送 `payload`，并收集订阅者的否决。

## 4910d68fbe

> BusSubscriberCount is how many subscribers topic has now.

`topic` 当前有多少订阅者。

## 2b28c6ac65

> RegisterService answers calls to name from mods of any language, and from native plugins
> through the bridge. fn receives the request and returns the reply, or an error whose
> message reaches the caller unchanged. It runs on the caller's thread, and also while this
> mod is disabled but loaded, since consumers resolve services in their own on_load.

回答任何语言写的模组对 `name` 的调用，也回答原生插件经 bridge 发来的调用。`fn` 收到请求，返回回答，或者返回一个错误，错误信息会原样到达调用方。它在调用方的线程上运行；这个模组被禁用、但仍然加载着的时候也会运行，因为使用方会在自己的 `on_load` 里解析服务。

## 02eef29e81

> CallService calls another mod's service, whatever language it is written in, and returns
> its reply or a *CallError.

调用另一个模组的服务，不管它是用什么语言写的；返回它的回答，或者一个 `*CallError`。

## fea8e6affc

> CallServiceOptional is CallService with nobody providing the name as found false, for an
> optional integration that works without the other mod.

没有模组提供这个名字时，以 `found` 为 false 返回的 `CallService`，用于没有另一个模组也能工作的可选集成。

## 3aab946b89

> ListServices lists every registered service.

列出所有已注册的服务。

## e50d73448d

> ServiceExists reports whether some mod provides name now.

判断现在有没有模组提供 `name`。

## 6535c40cf8

> ServiceCaller is the mod whose call is running, asked inside a provider. It is false
> outside a provider and for a caller with no mod, such as a native plugin through the
> bridge: a provider then knows it cannot attribute the request.

在提供方内部调用时，返回正在调用它的模组。在提供方之外，或者调用方没有模组（比如经 bridge 调用的原生插件）时为 false：提供方由此知道它无法确定请求来自谁。

## ba2a908ea4

> Subscription is one bus subscription, kept to end it.

一个总线订阅，保存下来用来结束它。

## 36380c3a1e

> ID is the host's id of the subscription.

宿主给这个订阅的 id。

## 5e1a11d72a

> Topic is the topic subscribed to.

订阅的主题。

## b0319c6e02

> Unsubscribe ends the subscription. Calling it twice is harmless.

结束订阅。调用两次也没有问题。

## 5ffc35b265

> ServiceRegistration is one service this mod provides, kept to withdraw it.

这个模组提供的一项服务，保存下来用来撤回它。

## 3272cb40b3

> ID is the host's id of the registration.

宿主给这项注册的 id。

## 4815032e27

> Name is the service name.

服务的名字。

## 747c736ae6

> Unregister withdraws the service. Calling it twice is harmless.

撤回这项服务。调用两次也没有问题。

## 990a7904d7

> CallError is why a service call failed: nobody provides the name, the provider said no
> and Message is what it said, the host refused the call (a bad name, a call to itself or a
> cycle), or the host has no service capability.

服务调用失败的原因：没有模组提供这个名字；提供方拒绝了，`Message` 是它给的理由；宿主拒绝了这次调用（名字不合法、调用自己，或者形成了循环）；或者宿主没有服务这项能力。

## f8e6f9ff3c

> Vetoable is the result of one vetoable publish.

一次可否决发布的结果。

## 9287135b8a

> CallErrorKind says why a service call failed.

说明一次服务调用为什么失败。

## da3d79bca7

> ServiceInfo is one registered service and the mod providing it.

一项已注册的服务，以及提供它的模组。

## 99bd7578fd

> RegistryList lists one of the engine's registries, one of the PIER_REGISTRY_* kinds.

列出引擎的某一个注册表，种类是某个 `PIER_REGISTRY_*`。

## f0f02ffe72

> ParseSNBT parses SNBT text, with the same rules as the Rust binding: a bare number is an
> int, then a long, then a double; a suffix b, s, l, f or d fixes the type; true and false
> are bytes; any other bare word is a string.

解析 SNBT 文本，规则和 Rust 绑定一样：不带后缀的数字依次尝试 int、long、double；后缀 b、s、l、f 或 d 指定类型；true 和 false 是字节；其他不带引号的单词是字符串。

## 8ccb4aa28c

> NewCompound is an empty compound.

一个空的复合标签。

## 0a6051af40

> NbtByteOf is a byte value.

一个 byte 值。

## f183fdcc3c

> NbtIntOf is an int value.

一个 int 值。

## 19956aefde

> NbtLongOf is a long value.

一个 long 值。

## fe1ae3d783

> NbtDoubleOf is a double value.

一个 double 值。

## 044125828f

> NbtStringOf is a string value.

一个字符串值。

## 6f7119718d

> Nbt is one SNBT value. Integers of every width are in Int and both floating kinds in
> Float; a compound keeps its keys in order in Keys and its values in Map.

一个 SNBT 值。各种宽度的整数都放在 `Int` 里，两种浮点数都放在 `Float` 里；复合标签把键按顺序放在 `Keys` 里，值放在 `Map` 里。

## 63b2da3cdd

> Set puts value under key of a compound, keeping the key's first position.

在复合标签里把 `value` 放到 `key` 下，键保持它第一次出现的位置。

## 5054066377

> Get follows a dotted path through compounds, and is nil when a step is missing.

沿着用点分隔的路径穿过复合标签；某一步不存在时为 nil。

## 10773d1b55

> OptString is the string at path, and false when it is absent or not a string.

`path` 处的字符串；不存在或者不是字符串时为 false。

## 652b4ccbea

> OptInt is the integer at path of any width, and false when it is absent or not one.

`path` 处任意宽度的整数；不存在或者不是整数时为 false。

## f03daaa3b0

> OptFloat is the number at path, integer or floating, and false when it is absent.

`path` 处的数字，整数或浮点数都可以；不存在时为 false。

## bbbf983b48

> OptBool is the byte at path read as a boolean, and false in the second result when it is
> absent or not a 0 or 1 byte.

把 `path` 处的字节当作布尔值读取；不存在，或者不是 0 或 1 的字节时，第二个返回值为 false。

## 883f56af45

> SNBT writes the value as SNBT. A floating value that is not finite is an error: SNBT has
> no spelling for it, and the host's parser refuses every field after one.

把这个值写成 SNBT。不是有限数的浮点值会返回错误：SNBT 没有写法能表示它，而宿主的解析器遇到一个这样的值，会拒绝它后面的所有字段。

## e2733e11c5

> NbtKind is the type of one NBT value.

一个 NBT 值的类型。

## edcd170d11

> Raw is the typed slot layer. Prefer the facades of this package where one exists.

带类型的槽位层。这个包里有对应的门面函数时，优先用门面函数。

## 1f15269fd3

> RawAPI reaches every slot without a callback, typed in Go and gated: a slot the host lacks
> is a *NotProvidedError. A result means what abi.h says it means; the rest of the package
> builds the interpreted API on top of it. Use it through Raw.

`RawAPI` 用 Go 的类型、经过关卡，取到每一个不带回调的槽位：宿主缺少的槽位返回 `*NotProvidedError`。结果的含义就是 abi.h 写的含义；这个包的其余部分在它之上构建解释过的接口。通过 `Raw` 使用它。

## 329ca234f6

> Poll for the finished report. False while sampling / nothing armed;
> true exactly once per window, sinking one SNBT report:
> {ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…},
> chunk_blocks:{…}, block_entities:{…}}}. Bucket times are INCLUSIVE
> (nested subsystems), report side by side, don't sum.

取回已经完成的报告。还在采样或者没有开启窗口时返回 false；每个窗口正好返回一次 true，并通过输出回调给出一份 SNBT 报告：`{ticks:N, buckets:{level_tick:{us,calls}, dimension_tick:{…}, redstone:{…}, chunk_blocks:{…}, block_entities:{…}}}`。各个桶的时间是**包含**关系（子系统之间有嵌套），请并排对照着看，不要相加。

## d6e9f49890

> Delete every save-file key belonging to one chunk, so the engine
> regenerates it from the generator on next load.

删除属于一个区块的所有存档键，下次加载时引擎会用生成器重新生成这个区块。

## a46afb9d91

> Are the chunks covering [min..max] currently loaded in memory?

覆盖 [min..max] 的区块当前是否都已加载到内存里。

## 9e797ceae9

> List every save-file key belonging to one chunk. One callback per key.

列出属于一个区块的所有存档键，每个键调用一次回调。

## c059055562

> Delete one chunk-category key, verbatim.

原样删除一个区块类的键。

## efdab14ff1

> Set the biome over an area.

设置一片区域的生物群系。

## c629ff0520

> The four *_get_num slots share one convention. The return is whether the host has
> an answer, and *out is the answer; a false leaves *out untouched.

四个 `*_get_num` 槽位遵守同一个约定。返回值表示宿主有没有答案，`*out` 是答案；返回 false 时 `*out` 不会被改动。

## f000c5e15e

> `PACT_SET_TITLE` (player_action opcode 6) reaches the client by running
> the console command `title "<name>" title <text>`. Three things are
> wrong with that and none of them are theoretical:
> - the text is pasted into a command line unquoted, so a plot named
> `He said "hi"` truncates the command;
> - `title`'s text parameter is a `message`, which expands selectors —
> a plot named `@e` is a command injection, not a name;
> - `/title` has no way to set fade/stay for the same call, so timing is
> whatever the client last stored.
> This slot builds a real SetTitlePacket instead. No wire format crosses
> the FFI (the packet is constructed field-by-field on this side), so it
> survives protocol bumps the way `spawn_particle_for` does.

`PACT_SET_TITLE`（`player_action` 的第 6 号操作）是通过执行控制台命令 `title "<name>" title <text>` 送到客户端的。这样做有三个问题，三个都会真的发生：

- 文本不加引号就拼进命令行，名叫 `He said "hi"` 的地皮会把命令截断；
- `title` 的文本参数类型是 `message`，会展开选择器，名叫 `@e` 的地皮就成了一次命令注入；
- `/title` 没法在同一次调用里设置淡入和停留时间，计时用的是客户端上一次存下的值。

这个槽位改为构造一个真正的 `SetTitlePacket`。没有任何线上格式跨过 FFI（数据包在这一侧逐个字段构造），所以它和 `spawn_particle_for` 一样，协议升级以后照样能用。

## 3b8468c237

> This player's connection id — the same number packet interceptors see
> in the packet context.

这名玩家的连接 id，和数据包拦截器在数据包上下文里看到的是同一个数。

## b43d952fd4

> Per-dimension rules, consulted by the loader's own hooks.

按维度设置的规则，由加载器自己的钩子查询。

## 8e2df8d5b5

> Resolve a dimension name to its id. Returns -1 if not found.

把维度名解析成维度 id。找不到时返回 -1。

## 22124dcf24

> Replace a dimension's merge markers wholesale. `entries` is `count`
> triples `(x, z, mask)`, i.e. `count * 3` int32s; `mask` is a bitset of
> 1=north, 2=east, 4=south, 8=west matching the plugin's `merged[]`
> indices. Only plots that actually carry a marker need to be sent.

整体替换一个维度的合并标记。`entries` 是 `count` 个三元组 `(x, z, mask)`，也就是 `count * 3` 个 int32；`mask` 是位集合，1=北，2=东，4=南，8=西，和插件的 `merged[]` 下标对应。只需要发送确实带有标记的地皮。

## be7ae16230

> List every registered custom dimension as a JSON array:
> [{"name":"plot_world","dim":1000,"snbt":"{…}"}].

以 JSON 数组列出所有已注册的自定义维度：`[{"name":"plot_world","dim":1000,"snbt":"{…}"}]`。

## 458b258b42

> Add a custom dimension with a native terrain from one declarative spec.

用一份声明式的描述，添加一个使用原生地形的自定义维度。

## 5a11287ba1

> Add a custom dimension whose terrain is a terrain pack: a directory with a
> config file and a binary built by tools/pier-pack. `config_path` names the
> config relative to the server root, forward slashes, no ".." and not
> absolute; the binary is named inside the config relative to it.

添加一个地形来自地形包的自定义维度。地形包是一个目录，里面有一个配置文件和一个由 tools/pier-pack 构建的二进制文件。`config_path` 指定配置文件，相对于服务器根目录，用正斜杠，不能有 `..`，也不能是绝对路径；二进制文件的名字写在配置里，相对于配置文件。

## e225cfb19e

> Retire a custom dimension: drop it from dimension_config.json, from the host's
> own tables and from the dimension factory, so nothing registers it on the next
> boot and it stops appearing in md_list_dimensions.

让一个自定义维度退役：把它从 dimension_config.json、宿主自己的表和维度工厂里去掉，下次启动时没有任何东西会注册它，它也不再出现在 `md_list_dimensions` 里。

## f4b75e79a7

> Give a generated dimension the cell geometry its confinement rules use.

给一个生成式维度设置约束规则所用的单元几何。

## 0c9b829393

> Who is calling the service callback that is running right now.

正在运行的这个服务回调，是谁调用的。

## 44b0bcba20

> Lists one of the engine's registries through `sink`, one JSON object per entry, in
> no particular order. `kind` is a PIER_REGISTRY_* value.

通过 `sink` 列出引擎的某一个注册表，每一项一个 JSON 对象，顺序不定。`kind` 是某个 `PIER_REGISTRY_*` 值。
