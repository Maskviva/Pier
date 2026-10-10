# Core: entry point, logging and tasks

??? note "Section notes in abi.h"

    **the core slots, present since ABI v1**

    **Append tail, struct\_size-gated.**

    The struct's only append point. SDK mirrors declare every field unconditionally, with no cfg or ifdef branches, because the layout is the same on every target.

    Mod-scoped scheduling. `schedule` / `schedule_after` above take a bare callback with no owner. That is a use-after-free waiting to happen: a mod that schedules a task and is then unloaded leaves the executor holding a function pointer into a freed dylib. These replacements attribute each task to a mod, so the loader can drop still-pending tasks when that mod goes away — the same `weak_ptr` + ticket discipline the form callbacks already use.

    The old slots remain (ABI is additive) and still work. The loader now attributes them by the callback's module (address to DLL) and drops pending tasks at unload; mods should still prefer the owned slots below, because attribution by address cannot see a callback that lives in a different module.

## PierApi: the table header {#PierApi-header}

### `PierApi.struct_size` {#PierApi.struct_size}

```c
uint32_t struct_size;
```

sizeof(PierApi), filled in by the host from the table it compiled. This is the whole basis of forward compatibility: the SDK compares against it at every non-core slot's call site.

### `PierApi.abi_version` {#PierApi.abi_version}

```c
uint32_t abi_version;
```

Equals the host's `PIER_ABI_VERSION`.

### `PierApi.host_flags` {#PierApi.host_flags}

```c
uint32_t host_flags;
```

Bitwise OR of PIER\_FLAG\_\*. Bit 0 means a client build.

### `PierApi._reserved0` {#PierApi._reserved0}

```c
uint32_t _reserved0;
```

Reserved, always 0. Rounds the header out to 16 bytes and leaves room for future header scalars.

## Slots {#slots}

### `log` {#log}

```c
void (*log)(PierModHandle mod, int32_t level, PierStr msg);
```

Log a message through the mod's own LeviLamina logger. level: -1=Off, 0=Fatal, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Trace (mirrors `ll::io::LogLevel`). Thread-safe.

- Call: `api->log(mod, level, msg)`
- Parameters:
    - mod : `PierModHandle`
    - level : `int32_t`
    - msg : `PierStr`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 0, counting from 0
- Callers in each binding:
    - Go: [`ListPlayers`](../go/player.md#ListPlayers), [`Logger.Error`](../go/core.md#Logger.Error), [`Logger.Warn`](../go/core.md#Logger.Warn), [`Logger.Info`](../go/core.md#Logger.Info), [`Logger.Debug`](../go/core.md#Logger.Debug), [`Raw.Log`](../go/raw.md#Raw.Log)
    - Zig: [`log`](../zig/core.md#log), [`logf`](../zig/core.md#logf), [`Context.info`](../zig/core.md#Context.info), [`Context.warn`](../zig/core.md#Context.warn), [`Context.err`](../zig/core.md#Context.err), [`exportMod`](../zig/core.md#exportMod) and more, 7 in all

### `gaming_status` {#gaming_status}

```c
int32_t (*gaming_status)(void);
```

Current gaming status: 0=Default, 1=Starting, 2=Running, 3=Stopping (mirrors `ll::GamingStatus`). Thread-safe.

- Call: `api->gaming_status()`
- Return type: `int32_t`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 1, counting from 0
- Callers in each binding:
    - Rust: [`Host::gaming_status`](../rust/host.md#Host.gaming_status)
    - Go: [`Status`](../go/core.md#Status), [`Raw.GamingStatus`](../go/raw.md#Raw.GamingStatus)

### `schedule` {#schedule}

```c
void (*schedule)(PierTaskCb cb, void* user);
```

Queue a task onto the server thread ASAP. Thread-safe.

- Call: `api->schedule(cb, user)`
- Parameters:
    - cb : `PierTaskCb`
    - user : `void*`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 2, counting from 0

### `schedule_after` {#schedule_after}

```c
void (*schedule_after)(PierTaskCb cb, void* user, uint64_t delay_ms);
```

Queue a task onto the server thread after `delay_ms`. Thread-safe.

- Call: `api->schedule_after(cb, user, delay_ms)`
- Parameters:
    - cb : `PierTaskCb`
    - user : `void*`
    - delay_ms : `uint64_t`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 3, counting from 0

### `schedule_for` {#schedule_for}

```c
uint64_t (*schedule_for)(PierModHandle mod, PierTaskCb cb, void* user);
```

Run `cb(user)` on the server (or client) thread ASAP, owned by `mod`. Thread-safe. Returns a task id (&gt;0), or 0 if the task was rejected. If `mod` unloads before the task runs, the task is dropped and `cb` is never called — `user` is then leaked by design, because the only code that could free it lives in the dylib that just went away.

- Call: `api->schedule_for(mod, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - cb : `PierTaskCb`
    - user : `void*`
- Return type: `uint64_t`
- Section of abi.h: Append tail, struct\_size-gated.
- Position in the table: slot 151, counting from 0
- Callers in each binding:
    - Rust: [`Host::schedule`](../rust/host.md#Host.schedule)
    - Go: [`Schedule`](../go/core.md#Schedule)
    - Zig: [`schedule`](../zig/core.md#schedule)

### `schedule_after_for` {#schedule_after_for}

```c
uint64_t (*schedule_after_for)(PierModHandle mod, PierTaskCb cb, void* user, uint64_t delay_ms);
```

As above, delayed by `delay_ms`. Thread-safe. Returns a task id (&gt;0), or 0 if rejected. The timer itself is not cancelled on unload — it still expires — but the task is dropped when it does, so nothing calls into the freed dylib.

- Call: `api->schedule_after_for(mod, cb, user, delay_ms)`
- Parameters:
    - mod : `PierModHandle`
    - cb : `PierTaskCb`
    - user : `void*`
    - delay_ms : `uint64_t`
- Return type: `uint64_t`
- Section of abi.h: Append tail, struct\_size-gated.
- Position in the table: slot 152, counting from 0
- Callers in each binding:
    - Rust: [`Host::schedule_after`](../rust/host.md#Host.schedule_after)
    - Go: [`ScheduleAfter`](../go/core.md#ScheduleAfter)
    - Zig: [`scheduleAfter`](../zig/core.md#scheduleAfter)

### `schedule_cancel` {#schedule_cancel}

```c
bool (*schedule_cancel)(PierModHandle mod, uint64_t task_id);
```

Drop a task scheduled by this mod if it has not run yet. Returns true if a pending task was actually dropped. Safe to call from any thread and from inside another task. Cancelling leaks `user` for the same reason as above, so prefer letting short tasks run.

- Call: `api->schedule_cancel(mod, task_id)`
- Parameters:
    - mod : `PierModHandle`
    - task_id : `uint64_t`
- Return type: `bool`
- Section of abi.h: Append tail, struct\_size-gated.
- Position in the table: slot 153, counting from 0
- Callers in each binding:
    - Rust: [`Host::cancel`](../rust/host.md#Host.cancel)
    - Go: [`Cancel`](../go/core.md#Cancel), [`Raw.ScheduleCancel`](../go/raw.md#Raw.ScheduleCancel)
    - Zig: [`cancel`](../zig/core.md#cancel)

### `schedule_pending_count` {#schedule_pending_count}

```c
uint32_t (*schedule_pending_count)(PierModHandle mod);
```

Number of tasks this mod still has pending. Intended for a mod to assert it has drained its own work in `on_disable` / `on_unload`, which is a precondition for being marked "`reload_safe`" in its manifest.

- Call: `api->schedule_pending_count(mod)`
- Parameters:
    - mod : `PierModHandle`
- Return type: `uint32_t`
- Section of abi.h: Append tail, struct\_size-gated.
- Position in the table: slot 154, counting from 0
- Callers in each binding:
    - Rust: [`Host::try_pending_tasks`](../rust/host.md#Host.try_pending_tasks), [`Host::pending_tasks`](../rust/host.md#Host.pending_tasks)
    - Go: [`PendingTasks`](../go/core.md#PendingTasks), [`Raw.SchedulePendingCount`](../go/raw.md#Raw.SchedulePendingCount)
    - Zig: [`pendingTasks`](../zig/core.md#pendingTasks)

## Macros {#macros}

### `PIER_ABI_VERSION` {#PIER_ABI_VERSION}

```c
#define PIER_ABI_VERSION 2u
```

See "Rules for changing this file" in the file header. Appending a slot does not touch this.

### `PIER_ABI_MIN_SUPPORTED` {#PIER_ABI_MIN_SUPPORTED}

```c
#define PIER_ABI_MIN_SUPPORTED 2u
```

Oldest mod ABI the host accepts. Moves only on a non-append change, and then to the same number as `PIER_ABI_VERSION`. It is the switch for "a table older than this is no longer a prefix of mine".

### `PIER_MAIN_SYMBOL` {#PIER_MAIN_SYMBOL}

```c
#define PIER_MAIN_SYMBOL "pier_main"
```

The only entry symbol a mod must export. The host looks for this name alone and refuses to load with an explicit error if it is missing; there is no fallback and no historical alias.

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

What to write in front of a `pier_main` definition so the name is actually exported.

Naming the symbol is not the same as exporting it. A Windows DLL exports nothing unless asked, so a plainly declared `pier_main` compiles, links, and then fails to load with "does not export `pier_main`". An ELF build exports it by default, which makes this a mistake a test on one platform cannot catch for the other.

```text
PIER_MAIN_EXPORT bool pier_main(const PierApi* api, PierModHandle self,
                                PierModVTable* out_vtable) { ... }
```

The macro carries the C linkage as well, so the definition needs no separate extern block. An SDK that emits the entry point from a macro of its own does not need this one.

### `PIER_FLAG_CLIENT` {#PIER_FLAG_CLIENT}

```c
#define PIER_FLAG_CLIENT 0x1u
```

Bits for PierApi.host\_flags and PierModVTable.mod\_flags. Bit 0 must match on both sides or the host refuses to load and says why: a server host cannot load a client-built mod, and vice versa. All other bits are reserved and must currently be 0.

## Types {#types}

### `PierModHandle` {#PierModHandle}

```c
typedef void* PierModHandle;
```

Opaque handle to the HostedMod instance managed by the loader.

### `PierTaskCb` {#PierTaskCb}

```c
typedef void (*PierTaskCb)(void* user);
```

Generic "run this" callback.

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

Filled in by the mod inside `pier_main`. instance is the mod's own opaque pointer; the three callbacks may be NULL, which counts as always succeeding. This struct follows the same append rules as PierApi: with `struct_size`, new lifecycle callbacks can be added at the tail without a version bump, and the host calls one only if it can reach it.

`out_vtable` points at no fewer than 512 zeroed bytes, so a mod writes the whole struct it compiled even when the host's is shorter. The host reads only the prefix it knows and accepts any `struct_size` that covers `on_unload`.

### `PierMainFn` {#PierMainFn}

```c
typedef bool (*PierMainFn)(const PierApi* api, PierModHandle self, PierModVTable* out_vtable);
```

The single symbol every mod must export:

```text
bool pier_main(const PierApi* api, PierModHandle self,
                  PierModVTable* out_vtable);
```

Called once on the server thread while the mod is being loaded. Return false to abort loading.

Neither this nor any callback a mod hands the host may unwind back into it: an exception, a panic or any other unwinding has to be stopped at the mod's own boundary. A callback that holds the thread past the host's watchdog limit is named in the log, and past the hang limit the host ends the process.
