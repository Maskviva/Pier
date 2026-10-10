# Events

??? note "Section notes in abi.h"

    **the core slots, present since ABI v1**

## Slots {#slots}

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

Subscribe to a LeviLamina event by id (server thread only).

```text
event_id : full id, e.g. "ll::event::PlayerChatEvent". If no exact
           match exists, the loader falls back to a unique suffix
           match ("PlayerChatEvent" works if unambiguous).
priority : 0..4 (Highest..Lowest), 2 = Normal
           (mirrors ll::event::EventPriority).
```

Returns NULL on failure (unknown/ambiguous id).

- Call: `api->subscribe_event(mod, event_id, priority, cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - event_id : `PierStr`
    - priority : `int32_t`
    - cb : `PierEventCb`
    - user : `void*`
- Return type: `PierListenerHandle`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 4, counting from 0
- Callers in each binding:
    - Rust: [`event::subscribe`](../rust/event.md#fn.subscribe), [`event::subscribe_with`](../rust/event.md#fn.subscribe_with), [`Wiring::arm`](../rust/event.md#Wiring.arm), [`Wiring::arm_lenient`](../rust/event.md#Wiring.arm_lenient)
    - Go: [`Subscribe`](../go/events.md#Subscribe)
    - Zig: [`subscribe`](../zig/events.md#subscribe)

### `unsubscribe_event` {#unsubscribe_event}

```c
bool (*unsubscribe_event)(PierModHandle mod, PierListenerHandle listener);
```

Remove a listener previously returned by `subscribe_event`. Server thread only.

- Call: `api->unsubscribe_event(mod, listener)`
- Parameters:
    - mod : `PierModHandle`
    - listener : `PierListenerHandle`
- Return type: `bool`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 5, counting from 0
- Callers in each binding:
    - Go: [`Listener.Unsubscribe`](../go/events.md#Listener.Unsubscribe), [`Raw.UnsubscribeEvent`](../go/raw.md#Raw.UnsubscribeEvent)
    - Zig: [`Listener.unsubscribe`](../zig/events.md#Listener.unsubscribe)

### `list_events` {#list_events}

```c
void (*list_events)(void* ctx, PierStrSink sink);
```

Enumerate all currently registered event ids. Server thread only.

- Call: `api->list_events(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Section of abi.h: the core slots, present since ABI v1
- Position in the table: slot 6, counting from 0
- Callers in each binding:
    - Rust: [`event::list`](../rust/event.md#fn.list), [`event::exists`](../rust/event.md#fn.exists), [`Host::list_events`](../rust/host.md#Host.list_events)
    - Go: [`ListEvents`](../go/events.md#ListEvents), [`Raw.ListEvents`](../go/raw.md#Raw.ListEvents)

## Types {#types}

### `PierListenerHandle` {#PierListenerHandle}

```c
typedef void* PierListenerHandle;
```

Opaque handle to an event listener.

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

Event callback.

```text
event_id : the full event id this listener fired for.
snbt     : event data serialized as SNBT (CompoundTag). For cancellable
           events it contains a `cancelled` byte field.
write_ctx / write_back : to mutate the event (e.g. cancel it, edit the
           chat message), call write_back(write_ctx, new_snbt) with the
           modified SNBT before returning. The loader deserializes it
           back into the event. Calling it zero times leaves the event
           untouched; the last call wins.
```
