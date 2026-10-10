# 事件

??? note "abi.h 里的分节说明"

    **核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）**

## 槽位 {#slots}

### `subscribe_event` {#subscribe_event}

```c
PierListenerHandle (*subscribe_event)(
    PierModHandle mod,
    PierStr event_id,
    int32_t priority,
    PierEventCb cb,
    void* user
);
```

按 id 订阅一个 LeviLamina 事件（只能在服务器线程调用）。

- `event_id`：完整的 id，例如 `"ll::event::PlayerChatEvent"`。没有完全匹配时，加载器退回到唯一的后缀匹配，不产生歧义时写 `"PlayerChatEvent"` 也可以。
- `priority`：0 到 4（Highest 到 Lowest），2 为 Normal（对应 `ll::event::EventPriority`）。

失败（id 不认识或有歧义）时返回 NULL。

- 调用形式：`api->subscribe_event(mod, event_id, priority, cb, user)`
- 参数：
    - mod : `PierModHandle`
    - event_id : `PierStr`
    - priority : `int32_t`
    - cb : `PierEventCb`
    - user : `void*`
- 返回值类型：`PierListenerHandle`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 4 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`event::subscribe`](../rust/event.md#fn.subscribe)、[`event::subscribe_with`](../rust/event.md#fn.subscribe_with)、[`Wiring::arm`](../rust/event.md#Wiring.arm)、[`Wiring::arm_lenient`](../rust/event.md#Wiring.arm_lenient)
    - Go：[`Subscribe`](../go/events.md#Subscribe)
    - Zig：[`subscribe`](../zig/events.md#subscribe)

### `unsubscribe_event` {#unsubscribe_event}

```c
bool (*unsubscribe_event)(PierModHandle mod, PierListenerHandle listener);
```

移除之前由 `subscribe_event` 返回的监听器。只能在服务器线程调用。

- 调用形式：`api->unsubscribe_event(mod, listener)`
- 参数：
    - mod : `PierModHandle`
    - listener : `PierListenerHandle`
- 返回值类型：`bool`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 5 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`Listener.Unsubscribe`](../go/events.md#Listener.Unsubscribe)、[`Raw.UnsubscribeEvent`](../go/raw.md#Raw.UnsubscribeEvent)
    - Zig：[`Listener.unsubscribe`](../zig/events.md#Listener.unsubscribe)

### `list_events` {#list_events}

```c
void (*list_events)(void* ctx, PierStrSink sink);
```

列出当前已注册的所有事件 id。只能在服务器线程调用。

- 调用形式：`api->list_events(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 所在分节：核心槽位，从 ABI v1 起就有（`the core slots, present since ABI v1`）
- 表内序号：第 6 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`event::list`](../rust/event.md#fn.list)、[`event::exists`](../rust/event.md#fn.exists)、[`Host::list_events`](../rust/host.md#Host.list_events)
    - Go：[`ListEvents`](../go/events.md#ListEvents)、[`Raw.ListEvents`](../go/raw.md#Raw.ListEvents)

## 类型 {#types}

### `PierListenerHandle` {#PierListenerHandle}

```c
typedef void* PierListenerHandle;
```

事件监听器的不透明句柄。

### `PierEventCb` {#PierEventCb}

```c
typedef void (*PierEventCb)(
    void* user,
    PierStr event_id,
    PierStr snbt,
    void* write_ctx,
    PierStrSink write_back
);
```

事件回调。

- `event_id`：这个监听器响应的事件的完整 id。
- `snbt`：序列化成 SNBT（`CompoundTag`）的事件数据。可取消的事件里有一个 `cancelled` 字节字段。
- `write_ctx` / `write_back`：要修改事件（比如取消它、改聊天内容），在返回之前调用 `write_back(write_ctx, new_snbt)`，传入改过的 SNBT，加载器会把它反序列化回事件里。一次都不调用，事件保持原样；调用多次，以最后一次为准。
