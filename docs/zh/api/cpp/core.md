# 核心：入口、日志与任务

??? note "abi.h 里的分节说明"

    **核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）**

    **末尾追加区，受 `struct_size` 约束（`Append tail, struct_size-gated.`）**

    结构体唯一的追加点。SDK 镜像无条件地声明每一个字段，不写 cfg 或 ifdef 分支，因为所有目标上的布局都一样。

    **按模组归属的调度。** 上面的 `schedule` / `schedule_after` 接收的是没有主人的裸回调。模组排了一个任务然后被卸载，执行器手里就留着一个指向已释放 dylib 的函数指针，任务一执行就是释放后使用。下面这些替代的槽位把每个任务记在某个模组名下，这个模组离开时，加载器可以丢掉它还没执行的任务；做法和表单回调已经在用的一样，用 `weak_ptr` 加票据。

    旧槽位仍然保留（ABI 只追加），也照样能用。加载器现在按回调所在的模块（由地址找到 DLL）给它们归属，卸载时丢掉还没执行的任务。模组仍然应当优先使用下面这些带归属的槽位，因为按地址归属看不到放在另一个模块里的回调。

## PierApi：表头字段 {#PierApi-header}

### `PierApi.struct_size` {#PierApi.struct_size}

```c
uint32_t struct_size;
```

`sizeof(PierApi)`，由宿主按它编译时的表填入。向前兼容完全建立在它上面：SDK 在每个非核心槽位的调用处都拿它来比较。

### `PierApi.abi_version` {#PierApi.abi_version}

```c
uint32_t abi_version;
```

等于宿主的 `PIER_ABI_VERSION`。

### `PierApi.host_flags` {#PierApi.host_flags}

```c
uint32_t host_flags;
```

`PIER_FLAG_*` 的按位或。第 0 位表示客户端构建。

### `PierApi._reserved0` {#PierApi._reserved0}

```c
uint32_t _reserved0;
```

保留，始终为 0。它把表头凑成 16 字节，也为以后的表头标量留出位置。

## 槽位 {#slots}

### `log` {#log}

```c
void (*log)(PierModHandle mod, int32_t level, PierStr msg);
```

通过模组自己的 LeviLamina 日志器写一条日志。`level`：-1=Off，0=Fatal，1=Error，2=Warn，3=Info，4=Debug，5=Trace（对应 `ll::io::LogLevel`）。线程安全。

- 调用形式：`api->log(mod, level, msg)`
- 参数：
    - mod : `PierModHandle`
    - level : `int32_t`
    - msg : `PierStr`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 0 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`ListPlayers`](../go/player.md#ListPlayers)、[`Logger.Error`](../go/core.md#Logger.Error)、[`Logger.Warn`](../go/core.md#Logger.Warn)、[`Logger.Info`](../go/core.md#Logger.Info)、[`Logger.Debug`](../go/core.md#Logger.Debug)、[`Raw.Log`](../go/raw.md#Raw.Log)
    - Zig：[`log`](../zig/core.md#log)、[`logf`](../zig/core.md#logf)、[`Context.info`](../zig/core.md#Context.info)、[`Context.warn`](../zig/core.md#Context.warn)、[`Context.err`](../zig/core.md#Context.err)、[`exportMod`](../zig/core.md#exportMod) 等，共 7 个

### `gaming_status` {#gaming_status}

```c
int32_t (*gaming_status)(void);
```

当前的运行状态：0=Default，1=Starting，2=Running，3=Stopping（对应 `ll::GamingStatus`）。线程安全。

- 调用形式：`api->gaming_status()`
- 返回值类型：`int32_t`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 1 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::gaming_status`](../rust/host.md#Host.gaming_status)
    - Go：[`Status`](../go/core.md#Status)、[`Raw.GamingStatus`](../go/raw.md#Raw.GamingStatus)

### `schedule` {#schedule}

```c
void (*schedule)(PierTaskCb cb, void* user);
```

把一个任务排到服务器线程上，尽快执行。线程安全。

- 调用形式：`api->schedule(cb, user)`
- 参数：
    - cb : `PierTaskCb`
    - user : `void*`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 2 个槽位（从 0 数起）

### `schedule_after` {#schedule_after}

```c
void (*schedule_after)(PierTaskCb cb, void* user, uint64_t delay_ms);
```

把一个任务排到服务器线程上，`delay_ms` 毫秒后执行。线程安全。

- 调用形式：`api->schedule_after(cb, user, delay_ms)`
- 参数：
    - cb : `PierTaskCb`
    - user : `void*`
    - delay_ms : `uint64_t`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 3 个槽位（从 0 数起）

### `schedule_for` {#schedule_for}

```c
uint64_t (*schedule_for)(PierModHandle mod, PierTaskCb cb, void* user);
```

在服务器（或客户端）线程上尽快运行 `cb(user)`，任务归 `mod` 所有。线程安全。返回任务 id（大于 0），任务被拒绝时返回 0。如果 `mod` 在任务运行之前卸载，任务会被丢弃，`cb` 永远不会被调用；这时 `user` 会泄漏，这是有意的，因为唯一能释放它的代码就在刚刚离开的那个 dylib 里。

- 调用形式：`api->schedule_for(mod, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - cb : `PierTaskCb`
    - user : `void*`
- 返回值类型：`uint64_t`
- 所在分节：末尾追加区，受 `struct_size` 约束（`Append tail, struct_size-gated.`）
- 表内序号：第 151 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::schedule`](../rust/host.md#Host.schedule)
    - Go：[`Schedule`](../go/core.md#Schedule)
    - Zig：[`schedule`](../zig/core.md#schedule)

### `schedule_after_for` {#schedule_after_for}

```c
uint64_t (*schedule_after_for)(PierModHandle mod, PierTaskCb cb, void* user, uint64_t delay_ms);
```

同上，延迟 `delay_ms` 毫秒。线程安全。返回任务 id（大于 0），被拒绝时返回 0。卸载时计时器本身不会取消，它照样会到期，但到期时任务被丢弃，所以不会有调用进入已释放的 dylib。

- 调用形式：`api->schedule_after_for(mod, cb, user, delay_ms)`
- 参数：
    - mod : `PierModHandle`
    - cb : `PierTaskCb`
    - user : `void*`
    - delay_ms : `uint64_t`
- 返回值类型：`uint64_t`
- 所在分节：末尾追加区，受 `struct_size` 约束（`Append tail, struct_size-gated.`）
- 表内序号：第 152 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::schedule_after`](../rust/host.md#Host.schedule_after)
    - Go：[`ScheduleAfter`](../go/core.md#ScheduleAfter)
    - Zig：[`scheduleAfter`](../zig/core.md#scheduleAfter)

### `schedule_cancel` {#schedule_cancel}

```c
bool (*schedule_cancel)(PierModHandle mod, uint64_t task_id);
```

这个模组排的某个任务如果还没运行，就把它丢掉。真的丢掉了一个待执行的任务时返回 true。可以从任何线程调用，也可以在另一个任务里调用。取消同样会泄漏 `user`，原因和上面一样，所以短任务最好让它跑完。

- 调用形式：`api->schedule_cancel(mod, task_id)`
- 参数：
    - mod : `PierModHandle`
    - task_id : `uint64_t`
- 返回值类型：`bool`
- 所在分节：末尾追加区，受 `struct_size` 约束（`Append tail, struct_size-gated.`）
- 表内序号：第 153 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::cancel`](../rust/host.md#Host.cancel)
    - Go：[`Cancel`](../go/core.md#Cancel)、[`Raw.ScheduleCancel`](../go/raw.md#Raw.ScheduleCancel)
    - Zig：[`cancel`](../zig/core.md#cancel)

### `schedule_pending_count` {#schedule_pending_count}

```c
uint32_t (*schedule_pending_count)(PierModHandle mod);
```

这个模组还有多少个待执行的任务。供模组在 `on_disable` / `on_unload` 里确认自己的工作已经清空，这是在清单里标记 `"reload_safe"` 的前提。

- 调用形式：`api->schedule_pending_count(mod)`
- 参数：
    - mod : `PierModHandle`
- 返回值类型：`uint32_t`
- 所在分节：末尾追加区，受 `struct_size` 约束（`Append tail, struct_size-gated.`）
- 表内序号：第 154 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Host::try_pending_tasks`](../rust/host.md#Host.try_pending_tasks)、[`Host::pending_tasks`](../rust/host.md#Host.pending_tasks)
    - Go：[`PendingTasks`](../go/core.md#PendingTasks)、[`Raw.SchedulePendingCount`](../go/raw.md#Raw.SchedulePendingCount)
    - Zig：[`pendingTasks`](../zig/core.md#pendingTasks)

## 宏 {#macros}

### `PIER_ABI_VERSION` {#PIER_ABI_VERSION}

```c
#define PIER_ABI_VERSION 2u
```

见文件头的「修改这个文件的规则」。追加槽位不改动它。

### `PIER_ABI_MIN_SUPPORTED` {#PIER_ABI_MIN_SUPPORTED}

```c
#define PIER_ABI_MIN_SUPPORTED 2u
```

宿主接受的最旧的模组 ABI。只在非追加的改动时才变，而且变到和 `PIER_ABI_VERSION` 相同的数。它是「比这更旧的表已经不是我的前缀」的开关。

### `PIER_MAIN_SYMBOL` {#PIER_MAIN_SYMBOL}

```c
#define PIER_MAIN_SYMBOL "pier_main"
```

模组唯一必须导出的入口符号。宿主只找这个名字，找不到就拒绝加载，并给出明确的错误；没有退路，也没有历史别名。

### `PIER_MAIN_LINKAGE / PIER_MAIN_EXPORT` {#PIER_MAIN_EXPORT}

```c
#ifdef __cplusplus
#define PIER_MAIN_LINKAGE extern "C"
#else
#define PIER_MAIN_LINKAGE
#endif
#if defined(_WIN32)
#define PIER_MAIN_EXPORT PIER_MAIN_LINKAGE __declspec(dllexport)
#else
#define PIER_MAIN_EXPORT PIER_MAIN_LINKAGE __attribute__((visibility("default")))
#endif
```

要让 `pier_main` 这个名字真的被导出，定义前面要写的东西。

给符号起名和导出它是两件事。Windows 的 DLL 不要求就什么都不导出，所以直接声明的 `pier_main` 能编译、能链接，加载时却报「does not export pier_main」。ELF 构建默认会导出，所以在一个平台上的测试发现不了另一个平台上的这个错误。

```c
PIER_MAIN_EXPORT bool pier_main(const PierApi* api, PierModHandle self,
                                PierModVTable* out_vtable) { ... }
```

这个宏同时带上了 C 链接方式，所以定义不需要再单独写 extern 块。用自己的宏生成入口点的 SDK 不需要这个宏。

### `PIER_FLAG_CLIENT` {#PIER_FLAG_CLIENT}

```c
#define PIER_FLAG_CLIENT 0x1u
```

`PierApi.host_flags` 和 `PierModVTable.mod_flags` 的位。第 0 位两边必须一致，否则宿主拒绝加载并说明原因：服务器宿主不能加载为客户端构建的模组，反过来也一样。其他位都是保留位，目前必须为 0。

## 类型 {#types}

### `PierModHandle` {#PierModHandle}

```c
typedef void* PierModHandle;
```

指向加载器管理的 `HostedMod` 实例的不透明句柄。

### `PierTaskCb` {#PierTaskCb}

```c
typedef void (*PierTaskCb)(void* user);
```

通用的「执行这个」回调。

### `PierModVTable` {#PierModVTable}

```c
typedef struct PierModVTable
{
    /** sizeof(PierModVTable), filled in by the mod from the definition it compiled. */
    uint32_t struct_size;
    /** Equals the PIER_ABI_VERSION the mod was compiled against. */
    uint32_t abi_version;
    /** Bitwise OR of PIER_FLAG_*. Bit 0 means built for a client target. */
    uint32_t mod_flags;
    /** Reserved, always 0. */
    uint32_t _reserved0;
    void* instance;
    bool (*on_enable)(void* instance);
    bool (*on_disable)(void* instance);
    bool (*on_unload)(void* instance);
} PierModVTable;
```

由模组在 `pier_main` 里填写。`instance` 是模组自己的不透明指针；三个回调都可以是 NULL，NULL 视为总是成功。这个结构体和 `PierApi` 遵守同样的追加规则：有了 `struct_size`，新的生命周期回调可以追加在末尾而不提升版本，宿主只调用它够得着的回调。

`out_vtable` 指向至少 512 个清零的字节，所以即使宿主的结构体更短，模组也可以写入它编译时的整个结构体。宿主只读取它认识的前缀，接受任何覆盖到 `on_unload` 的 `struct_size`。

### `PierMainFn` {#PierMainFn}

```c
typedef bool (*PierMainFn)(const PierApi* api, PierModHandle self, PierModVTable* out_vtable);
```

每个模组都必须导出的唯一符号：

```c
bool pier_main(const PierApi* api, PierModHandle self,
               PierModVTable* out_vtable);
```

模组加载时，在服务器线程上调用一次。返回 false 会中止加载。

无论是它，还是模组交给宿主的任何回调，都不能把栈展开带回宿主：异常、panic 或其他任何形式的展开，都必须在模组自己的边界上截住。一个回调占住线程超过宿主看门狗的告警时限时，日志里会点名；超过卡死的时限，宿主会结束进程。
