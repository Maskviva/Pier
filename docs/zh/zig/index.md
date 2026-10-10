# Zig

Pier 的 Zig 绑定把模组写成 Zig 动态库，适用于 Zig 0.15.2。包名是 `pier`，放在 `bindings/zig`；它导出的模块叫 `levilamina`，
代码里写 `@import("levilamina")`。最小的可用模组是 `examples/hello-pier-zig`。

包名写的是 Pier，表明它属于哪套 ABI；模块名写的是你在做的东西，一个 LeviLamina 模组。
Rust 绑定也是这样：包 `pier-rs` 导出的 crate 叫 `levilamina`。

## 它是怎么搭起来的

**translate-c 直接读 `abi.h`。** 构建时 translate-c 处理这个头文件，类型、槽位和常量都是 C 编译器自己的解读，没有任何手写镜像。
包里带着一份头文件副本，因为依赖方只看得到这个包自己的目录；有一项检查保证副本与原件完全相同。

**Zig 直接调用槽位。** `levilamina.slot("名字")` 返回槽位的函数指针；表不够长、或槽位为空时返回 null。
契约要求的两道检查在这一个调用里全部完成，适用于表里的每个槽位。其余功能都建在它之上。

**`zig build test` 用头文件检查绑定。** 测试让编译器对照 translate-c 生成的表，分析绑定里的每一个函数。
`abi.h` 里改了名的槽位或类型不对的参数，都会让构建失败；CI 会运行它。

## 需要什么

- Zig 0.15.2。
- 别的都不需要：Zig 能在任何系统上交叉编译出 Windows DLL。示例的默认目标 `x86_64-windows-gnu`，除系统自带的以外不需要任何 C 运行时。

## API

| 范围 | 函数 |
|---|---|
| 生命周期 | `exportMod`，以及模组类型上的 `load`、`enable`、`disable`、`unload` |
| 日志 | `log`、`logf`、`Context.info`、`warn`、`err` |
| 任务 | `schedule`、`scheduleAfter`、`cancel`、`pendingTasks` |
| 事件 | `subscribe`、`Event.cancel`、`Event.writeBack`、`Listener.unsubscribe` |
| 命令 | `registerCommand`、`executeCommand` |
| 跨模组 | `registerService`、`callService`、`serviceCaller`、`listServicesJson`、`busSubscribe`、`busPublish`、`busPublishVetoable` |
| 属性和动作 | `Player`、`Entity`、`BlockAt`、`Item` 上每个常量一个方法，比如 `Player.level`、`Player.setLevel`、`Entity.isOnFire` |
| 数据 | `nbt.parse`，以及 `get`、`getString`、`getInt`、`getFloat`、`getBool` |
| 其余一切 | `slot("名字")`，配合 `levilamina.c` 里的类型和常量 |

错误是 `levilamina.Error`：`NotProvided` 表示宿主没有这个槽位，`Refused` 表示宿主拒绝了调用，
`NoAnswer` 表示宿主答不上来的读取：读不出来的值会以这个错误返回，你拿不到一个 0 或 false。返回文本的函数都接收一个 allocator，内存交给调用方。

## 跨模组通信

Zig 模组可以和任何语言的模组通信，也能通过 [bridge](../guide/bridge.md) 和原生插件通信，规则与 Rust、Go 绑定完全相同：

- **服务**提供方发送回复并返回 true，或者发送原因并返回 false；调用方会原样收到这个原因，也就是 `callService` 的 `provider_error` 及其内容。
  `serviceCaller` 给出调用方模组的名字，原生插件调用时为 null。
- **总线**订阅者返回的是否决：true 拒绝一次可否决的发布。普通发布会忽略它；`busPublish` 返回 0 表示没人在听，这不是错误。
- **Lane** 不提供：它只连接同一工具链构建的模组，而发布 Lane 的模组同时提供服务，Zig 模组用的就是这个服务。

## 值得知道的规则

- **线程。** 宿主大部分功能只能在服务器线程上调用。生命周期、事件和命令回调在那里运行；服务提供方在调用方的线程上运行，总线订阅者在发布方的线程上运行。
- **借来的视图。** `Event` 的 `snbt`、`Invocation` 的 `args` 和提供方收到的 `request` 只在回调期间有效；要保留就复制。
- **panic。** Zig 不做栈展开：panic（比如 ReleaseSafe 构建里的安全检查失败）会结束服务器进程。请返回错误；
  生命周期步骤返回的错误会被绑定记日志，并拒绝该步骤。

[第一个 Zig 模组](first-mod.md)一步步讲怎么构建和安装。
