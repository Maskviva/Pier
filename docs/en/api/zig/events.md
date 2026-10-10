# Zig: Events

## Functions {#functions}

### `subscribe` {#subscribe}

```zig
pub fn subscribe(id: []const u8, priority: Priority, comptime handler: fn (*const Event) void) Error!Listener
```

Calls `handler` for every event with this id, on the server thread. Server thread only.

- Parameters:
    - id : `[]const u8`
    - priority : `Priority`
    - handler : `comptime fn (*const Event) void`
- Return type: `Error!Listener`
- Slots: [`subscribe_event`](../cpp/events.md#subscribe_event)

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

One event as a listener receives it; id and snbt are valid during the callback only.

### `Event.writeBack` {#Event.writeBack}

```zig
pub fn writeBack(self: *const Event, edit: []const u8) void
```

Writes keys back into the event, as SNBT of only the keys that change; the host merges them, so two listeners writing different keys keep both.

- Parameters:
    - edit : `[]const u8`
- Return type: `void`

### `Event.cancel` {#Event.cancel}

```zig
pub fn cancel(self: *const Event) void
```

Asks the host to cancel the event. An event that cannot be cancelled ignores it.

- Return type: `void`

## `Listener` {#Listener}

```zig
pub const Listener = struct {
    raw: c.PierListenerHandle,
    // ...
};
```

One subscription, kept to end it.

### `Listener.unsubscribe` {#Listener.unsubscribe}

```zig
pub fn unsubscribe(self: *Listener) Error!void
```

Ends the subscription; calling it twice is harmless. Server thread only.

- Return type: `Error!void`
- Slots: [`unsubscribe_event`](../cpp/events.md#unsubscribe_event)

## `Priority` {#Priority}

```zig
pub const Priority = enum(i32) {
    highest = 0, high = 1, normal = 2, low = 3, lowest = 4
};
```

Listener order, mirroring `ll::event::EventPriority`.
