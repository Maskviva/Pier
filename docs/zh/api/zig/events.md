# Zig：事件

## 函数 {#functions}

### `subscribe` {#subscribe}

```zig
pub fn subscribe(id: []const u8, priority: Priority, comptime handler: fn (*const Event) void) Error!Listener
```

对这个 id 的每一个事件，在服务器线程上调用 `handler`。只能在服务器线程调用。

- 参数：
    - id : `[]const u8`
    - priority : `Priority`
    - handler : `comptime fn (*const Event) void`
- 返回值类型：`Error!Listener`
- 对应槽位：[`subscribe_event`](../cpp/events.md#subscribe_event)

## `Event` {#Event}

```zig
pub const Event = struct {
    id: []const u8,
    snbt: []const u8,
    write_ctx: ?*anyopaque,
    write_back: c.PierStrSink,
    // ...
};
```

监听器收到的一个事件；`id` 和 `snbt` 只在回调期间有效。

### `Event.writeBack` {#Event.writeBack}

```zig
pub fn writeBack(self: *const Event, edit: []const u8) void
```

把键写回事件里，SNBT 中只放要改的键；宿主会合并它们，所以两个监听器写不同的键时，两边的改动都会保留。

- 参数：
    - edit : `[]const u8`
- 返回值类型：`void`

### `Event.cancel` {#Event.cancel}

```zig
pub fn cancel(self: *const Event) void
```

请求宿主取消这个事件。不能取消的事件会忽略这个请求。

- 返回值类型：`void`

## `Listener` {#Listener}

```zig
pub const Listener = struct {
    raw: c.PierListenerHandle,
    // ...
};
```

一个订阅，保存下来用来结束它。

### `Listener.unsubscribe` {#Listener.unsubscribe}

```zig
pub fn unsubscribe(self: *Listener) Error!void
```

结束订阅；调用两次也没有问题。只能在服务器线程调用。

- 返回值类型：`Error!void`
- 对应槽位：[`unsubscribe_event`](../cpp/events.md#unsubscribe_event)

## `Priority` {#Priority}

```zig
pub const Priority = enum(i32) {
    highest = 0, high = 1, normal = 2, low = 3, lowest = 4
};
```

监听器的顺序，对应 `ll::event::EventPriority`。
