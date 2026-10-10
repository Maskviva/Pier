## 04be2d328f

> core.zig: the handshake, the slot gate, strings, logging, tasks, events, commands and
> the cross-mod channels of the Zig binding.
>
> Zig calls the host's function pointers directly, so there is no generated layer between:
> `slot("name")` is the function pointer of that slot, gated by both checks of contract
> section 10, and every other function here is built on it.

core.zig：Zig 绑定的握手、槽位关卡、字符串、日志、任务、事件、命令和跨模组通道。

Zig 直接调用宿主的函数指针，中间没有生成的那一层：`slot("name")` 就是那个槽位的函数指针，经过契约第 10 节的两道检查，这里的其他函数都建立在它之上。

## b4c3ed02c6

> The function pointer of slot `name`, or null when the host's table is too short to hold
> it or the slot is NULL. Every slot of abi.h is reachable this way.

名为 `name` 的槽位的函数指针；宿主的表太短、放不下这个槽位，或者槽位为 NULL 时返回 null。abi.h 的每一个槽位都可以这样取到。

## 52977a15c0

> The caller's own mod handle, for a slot that takes one.

调用方自己的模组句柄，供需要它的槽位使用。

## c18aee85e8

> A Zig slice as a PierStr; the host reads it during the call only.

把 Zig 切片当作 `PierStr`；宿主只在调用期间读取它。

## 3676bdcd83

> A PierStr as a slice, valid only during the callback that received it.

把 `PierStr` 当作切片，只在收到它的那次回调期间有效。

## 7da910a24a

> Writes one line through this mod's LeviLamina logger. Safe from any thread.

通过这个模组的 LeviLamina 日志器写一行。可以在任何线程调用。

## 8cf6b88334

> log with std.fmt formatting, into a buffer of 2048 bytes.

用 `std.fmt` 格式化的 `log`，写进一个 2048 字节的缓冲区。

## 4ef05d7b32

> Exports pier_main for mod type M, named `name` in the log. Call it from the root file:
>
>     comptime { levilamina.exportMod(MyMod, "my-mod"); }
>
> M declares any of `pub fn load`, `enable`, `disable` and `unload`, each taking a
> `*levilamina.Context` and returning `!void`; an error is logged and refuses the step.

为模组类型 `M` 导出 `pier_main`，在日志里以 `name` 命名。在根文件里调用：

```zig
comptime { levilamina.exportMod(MyMod, "my-mod"); }
```

`M` 可以声明 `pub fn load`、`enable`、`disable` 和 `unload` 中的任意几个，每个都接收 `*levilamina.Context`，返回 `!void`；返回错误时会记录日志，并拒绝这一步。

## 97aea70c59

> Runs `task` on the server thread as soon as it can. Safe from any thread. The task
> belongs to this mod: if the mod unloads first, the host drops it and it never runs.

尽快在服务器线程上运行 `task`。可以在任何线程调用。任务属于这个模组：模组先卸载的话，宿主会丢掉这个任务，它永远不会运行。

## 85a8f3a5d1

> Runs `task` on the server thread after delay_ms, under the ownership of schedule.

在 `delay_ms` 毫秒后于服务器线程上运行 `task`，归属规则和 `schedule` 一样。

## c26e70aced

> Voids a task that has not run; false for one that ran, was cancelled or is not this mod's.

作废一个还没运行的任务；已经运行过、已经取消过，或者不属于这个模组的任务返回 false。

## 735facfae9

> This mod's tasks that have not run. A host that cannot count returns an error.

这个模组还没运行的任务数。宿主数不出来时返回错误。

## 4b83dc16d0

> The ABI as translate-c read it from abi.h: every type, slot and constant.

translate-c 从 abi.h 读出来的 ABI：所有的类型、槽位和常量。

## 78456832c8

> A task id, for cancel.

任务 id，供 `cancel` 使用。

## 8cbffdfc07

> Gathers what a PierStrSink receives during one call, each piece copied with allocator.

收集一次调用期间 `PierStrSink` 收到的内容，每一段都用 `allocator` 复制一份。

## f901d6f9d5

> The PierStrSink to hand the host, with a *Collector as its ctx.

交给宿主的 `PierStrSink`，`ctx` 是一个 `*Collector`。

## cfb239ae6b

> The last piece, owned by the caller; NoAnswer when the sink received nothing.

最后一段，归调用方所有；输出回调什么都没收到时返回 `NoAnswer`。

## ad9ebb6deb

> The last piece, or an empty owned slice when the sink received nothing.

最后一段；输出回调什么都没收到时，返回一个归调用方所有的空切片。

## d55b2a51f8

> Every piece, owned by the caller with the slice holding them.

所有的段，连同装着它们的切片，都归调用方所有。

## cf5cffd612

> What a lifecycle step receives.

生命周期的每一步收到的东西。

## 3c4b798980

> Why a call failed. A read the host cannot answer returns NoAnswer.

调用失败的原因。宿主答不上来的读取返回 `NoAnswer`。

## fb2424e285

> The levels of the log slot, which mirror ll::io::LogLevel.

`log` 槽位的日志级别，对应 `ll::io::LogLevel`。

## 19e087631e

> Calls `handler` for every event with this id, on the server thread. Server thread only.

对这个 id 的每一个事件，在服务器线程上调用 `handler`。只能在服务器线程调用。

## a393791e97

> One event as a listener receives it; id and snbt are valid during the callback only.

监听器收到的一个事件；`id` 和 `snbt` 只在回调期间有效。

## 03201b1d42

> Writes keys back into the event, as SNBT of only the keys that change; the host
> merges them, so two listeners writing different keys keep both.

把键写回事件里，SNBT 中只放要改的键；宿主会合并它们，所以两个监听器写不同的键时，两边的改动都会保留。

## 23581b7ff1

> Asks the host to cancel the event. An event that cannot be cancelled ignores it.

请求宿主取消这个事件。不能取消的事件会忽略这个请求。

## 86464487a8

> One subscription, kept to end it.

一个订阅，保存下来用来结束它。

## ebf42359ae

> Ends the subscription; calling it twice is harmless. Server thread only.

结束订阅；调用两次也没有问题。只能在服务器线程调用。

## dbefd67589

> Listener order, mirroring ll::event::EventPriority.

监听器的顺序，对应 `ll::event::EventPriority`。

## 455569bfa5

> Registers /name, whose text after the name reaches handler as args. Call it from enable.

注册 `/name`，命令名之后的文本作为 `args` 交给 `handler`。请在 `enable` 里调用。

## ab7afc74b5

> Runs cmd as the console. Server thread only. The output lines are logged at debug level;
> a false from the host, a level that is not ready, is Refused.

以控制台的身份运行 `cmd`。只能在服务器线程调用。输出的每一行按 debug 级别记进日志；宿主返回 false（世界还没就绪）时得到 `Refused`。

## a12fa4a256

> One run of a command; valid during the callback only.

一次命令的执行；只在回调期间有效。

## 40d3b7adba

> Sends a line of output.

发送一行输出。

## cafaae6467

> Sends a line of error output.

发送一行错误输出。

## 94a705d731

> Who may run a command, mirroring CommandPermissionLevel.

谁可以运行一条命令，对应 `CommandPermissionLevel`。

## bc7dad4c9d

> One player, named by a selector.

一名玩家，由选择器指定。

## 2ba58e8287

> How a Player selector names its player. A decision about permissions, money or ownership
> keys on the xuid: a name can change hands.

`Player` 选择器用什么来指定玩家。关于权限、金钱或归属的判断，要以 xuid 为键：名字会换主人。

## 833ed6a65f

> One actor, named by its unique id, the uid of event payloads.

一个实体，由它的唯一 id 指定，也就是事件载荷里的 uid。

## 34678c9b78

> One block cell of a dimension.

一个维度里的一个方块格。

## 72886351ef

> An item stack as SNBT, a value: reading it asks the host about that SNBT.

以 SNBT 表示的物品堆，是一个值：读取它，就是拿这段 SNBT 去问宿主。

## be2abf5dd3

> Calls `handler(topic, payload)` for every message on topic, from mods of any language, on
> the publisher's thread. It returns a veto: true refuses a vetoable publish and false has
> no opinion, and a plain publish ignores it. Namespace topics, as "plot:enter".

对 `topic` 上的每一条消息调用 `handler(topic, payload)`，消息可以来自任何语言写的模组，在发布方的线程上调用。返回值是否决：true 拒绝一次可否决的发布，false 表示没有意见，普通的发布忽略它。主题请加命名空间，比如 `"plot:enter"`。

## 517aa44a45

> Sends payload to every subscriber of topic and returns how many ran; 0 means nobody is
> listening, which is not an error.

把 `payload` 发给 `topic` 的每一个订阅者，返回运行了多少个；0 表示没有人在听，这不是错误。

## 58b92587f7

> Sends payload and collects the subscribers' vetoes.

发送 `payload`，并收集订阅者的否决。

## 0f01dbd65a

> How many subscribers topic has now.

`topic` 当前有多少订阅者。

## e1784272fc

> Answers calls to name from mods of any language, and from native plugins through the
> bridge. `provider(request, reply)` runs on the caller's thread, also while this mod is
> disabled but loaded, since consumers resolve services in their own on_load.

回答任何语言写的模组对 `name` 的调用，也回答原生插件经 bridge 发来的调用。`provider(request, reply)` 在调用方的线程上运行；这个模组被禁用、但仍然加载着的时候也会运行，因为使用方会在自己的 `on_load` 里解析服务。

## 6228ac8153

> Calls another mod's service, whatever language it is written in.

调用另一个模组的服务，不管它是用什么语言写的。

## 7a5c520991

> Every registered service as the host's JSON array of {"name","mod"}, owned by the caller.

所有已注册的服务，以宿主给出的 JSON 数组 `{"name","mod"}` 返回，归调用方所有。

## abf528149c

> Inside a provider, the mod whose call is running; null outside one and for a caller with
> no mod, such as a native plugin through the bridge.

在提供方内部，返回正在调用它的模组；在提供方之外，或者调用方没有模组（比如经 bridge 调用的原生插件）时返回 null。

## 6e306daad0

> One bus subscription, kept to end it.

一个总线订阅，保存下来用来结束它。

## 5efa88486b

> Ends the subscription; calling it twice is harmless.

结束订阅；调用两次也没有问题。

## f43d143c3a

> The way a service provider answers: send the reply and return true, or send the reason
> and return false, which the caller receives unchanged as the provider's message.

服务提供方回答的方式：发送回答并返回 true；或者发送原因并返回 false，调用方会原样收到这段文字，作为提供方的错误信息。

## 3c09df2169

> One service this mod provides, kept to withdraw it.

这个模组提供的一项服务，保存下来用来撤回它。

## 9d6528c371

> Withdraws the service; calling it twice is harmless.

撤回这项服务；调用两次也没有问题。

## 4e0e43b15c

> The result of one vetoable publish. There is no short circuit: after a veto the later
> subscribers still receive the message.

一次可否决发布的结果。不会提前结束：有订阅者否决以后，后面的订阅者仍然会收到消息。

## 6c183efdd0

> How a service call ended, matching the PIER_SERVICE_* results.

一次服务调用的结束方式，和 `PIER_SERVICE_*` 的结果一一对应。

## 7ff192dcde

> One service call's outcome. body is the reply for ok and the provider's message for
> provider_error, owned by the caller.

一次服务调用的结果。`code` 为 ok 时 `body` 是回答，为 provider_error 时 `body` 是提供方的错误信息，归调用方所有。

## 3b89db6dff

> nbt.zig: SNBT parsing and writing for Zig mods, with the rules of the Rust and Go
> bindings: a bare number is an int, then a long, then a double; a suffix b, s, l, f or d
> fixes the type; true and false are bytes; any other bare word is a string.

nbt.zig：Zig 模组的 SNBT 解析和写出，规则和 Rust、Go 绑定一样：不带后缀的数字依次尝试 int、long、double；后缀 b、s、l、f 或 d 指定类型；true 和 false 是字节；其他不带引号的单词是字符串。

## 10e3f80f5f

> Parses SNBT. Every node is allocated with `arena`, which the caller frees as a whole; a
> bare string or key may also point into `text`, which must outlive the result.

解析 SNBT。每个节点都用 `arena` 分配，由调用方整体释放；不带引号的字符串或键也可能直接指向 `text`，所以 `text` 必须比结果存活得更久。

## f6510f6a08

> The deepest nesting accepted, the cap Minecraft puts on NBT. A deeper payload is refused
> instead of exhausting the stack of a host thread.

接受的最大嵌套深度，和 Minecraft 对 NBT 的上限一样。更深的数据会被拒绝，以免耗尽宿主线程的栈。

## 528b02bb81

> One SNBT value. Typed arrays hold their numbers as i64 whatever the element width.

一个 SNBT 值。带类型的数组不论元素多宽，都用 i64 保存数字。

## d97f247bb3

> Follows a dotted path through compounds; null when a step is missing.

沿着用点分隔的路径穿过复合标签；某一步不存在时返回 null。

## 6ab7a0f1e8

> The string at path, or null when it is absent or not a string.

`path` 处的字符串；不存在或者不是字符串时返回 null。

## 6e187c4b47

> The integer at path of any width, or null when it is absent or not one.

`path` 处任意宽度的整数；不存在或者不是整数时返回 null。

## c81472d687

> The number at path, integer or floating, or null when it is absent.

`path` 处的数字，整数或浮点数都可以；不存在时返回 null。

## da0eb67675

> The byte at path read as a boolean, or null when it is absent or not 0 or 1.

把 `path` 处的字节当作布尔值读取；不存在，或者不是 0 或 1 时返回 null。

## 3b3299023d

> One key of a compound and its value.

复合标签的一个键和它的值。
