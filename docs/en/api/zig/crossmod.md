# Zig: Cross-mod: bus, services and lanes

## Functions {#functions}

### `busSubscribe` {#busSubscribe}

```zig
pub fn busSubscribe(topic: []const u8, comptime handler: fn ([]const u8, []const u8) bool) Error!Subscription
```

Calls `handler(topic, payload)` for every message on topic, from mods of any language, on the publisher's thread. It returns a veto: true refuses a vetoable publish and false has no opinion, and a plain publish ignores it. Namespace topics, as "plot:enter".

- Parameters:
    - topic : `[]const u8`
    - handler : `comptime fn ([]const u8, []const u8) bool`
- Return type: `Error!Subscription`
- Slots: [`bus_subscribe`](../cpp/crossmod.md#bus_subscribe)

### `busPublish` {#busPublish}

```zig
pub fn busPublish(topic: []const u8, payload: []const u8) Error!u32
```

Sends payload to every subscriber of topic and returns how many ran; 0 means nobody is listening, which is not an error.

- Parameters:
    - topic : `[]const u8`
    - payload : `[]const u8`
- Return type: `Error!u32`
- Slots: [`bus_publish`](../cpp/crossmod.md#bus_publish)

### `busPublishVetoable` {#busPublishVetoable}

```zig
pub fn busPublishVetoable(topic: []const u8, payload: []const u8) Error!Vetoable
```

Sends payload and collects the subscribers' vetoes.

- Parameters:
    - topic : `[]const u8`
    - payload : `[]const u8`
- Return type: `Error!Vetoable`
- Slots: [`bus_publish_vetoable`](../cpp/crossmod.md#bus_publish_vetoable)

### `busSubscriberCount` {#busSubscriberCount}

```zig
pub fn busSubscriberCount(topic: []const u8) Error!u32
```

How many subscribers topic has now.

- Parameters:
    - topic : `[]const u8`
- Return type: `Error!u32`
- Slots: [`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)

### `registerService` {#registerService}

```zig
pub fn registerService(name: []const u8, comptime provider: fn ([]const u8, *const Reply) bool) Error!ServiceRegistration
```

Answers calls to name from mods of any language, and from native plugins through the bridge. `provider(request, reply)` runs on the caller's thread, also while this mod is disabled but loaded, since consumers resolve services in their own `on_load`.

- Parameters:
    - name : `[]const u8`
    - provider : `comptime fn ([]const u8, *const Reply) bool`
- Return type: `Error!ServiceRegistration`
- Slots: [`service_register`](../cpp/crossmod.md#service_register)

### `callService` {#callService}

```zig
pub fn callService(allocator: std.mem.Allocator, name: []const u8, request: []const u8) Error!CallOutcome
```

Calls another mod's service, whatever language it is written in.

- Parameters:
    - allocator : `std.mem.Allocator`
    - name : `[]const u8`
    - request : `[]const u8`
- Return type: `Error!CallOutcome`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `listServicesJson` {#listServicesJson}

```zig
pub fn listServicesJson(allocator: std.mem.Allocator) Error![]u8
```

Every registered service as the host's JSON array of {"name","mod"}, owned by the caller.

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `Error![]u8`
- Slots: [`service_list`](../cpp/crossmod.md#service_list)

### `serviceCaller` {#serviceCaller}

```zig
pub fn serviceCaller(allocator: std.mem.Allocator) Error!?[]u8
```

Inside a provider, the mod whose call is running; null outside one and for a caller with no mod, such as a native plugin through the bridge.

- Parameters:
    - allocator : `std.mem.Allocator`
- Return type: `Error!?[]u8`
- Slots: [`service_caller`](../cpp/crossmod.md#service_caller)

## `Subscription` {#Subscription}

```zig
pub const Subscription = struct {
    id: u64,
    // ...
};
```

One bus subscription, kept to end it.

### `Subscription.unsubscribe` {#Subscription.unsubscribe}

```zig
pub fn unsubscribe(self: *Subscription) Error!void
```

Ends the subscription; calling it twice is harmless.

- Return type: `Error!void`
- Slots: [`bus_unsubscribe`](../cpp/crossmod.md#bus_unsubscribe)

## `Reply` {#Reply}

```zig
pub const Reply = struct {
    ctx: ?*anyopaque,
    sink: c.PierStrSink,
    // ...
};
```

The way a service provider answers: send the reply and return true, or send the reason and return false, which the caller receives unchanged as the provider's message.

### `Reply.send` {#Reply.send}

```zig
pub fn send(self: *const Reply, text: []const u8) void
```

- Parameters:
    - text : `[]const u8`
- Return type: `void`

## `ServiceRegistration` {#ServiceRegistration}

```zig
pub const ServiceRegistration = struct {
    id: u64,
    // ...
};
```

One service this mod provides, kept to withdraw it.

### `ServiceRegistration.unregister` {#ServiceRegistration.unregister}

```zig
pub fn unregister(self: *ServiceRegistration) Error!void
```

Withdraws the service; calling it twice is harmless.

- Return type: `Error!void`
- Slots: [`service_unregister`](../cpp/crossmod.md#service_unregister)

## `Vetoable` {#Vetoable}

```zig
pub const Vetoable = struct {
    vetoed: bool, delivered: u32
    // ...
};
```

The result of one vetoable publish. There is no short circuit: after a veto the later subscribers still receive the message.

## `CallCode` {#CallCode}

```zig
pub const CallCode = enum {
    ok, not_found, provider_error, refused
};
```

How a service call ended, matching the PIER\_SERVICE\_\* results.

## `CallOutcome` {#CallOutcome}

```zig
pub const CallOutcome = struct {
    code: CallCode, body: []u8
    // ...
};
```

One service call's outcome. body is the reply for ok and the provider's message for `provider_error`, owned by the caller.
