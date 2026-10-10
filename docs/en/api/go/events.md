# Go: Events

## Functions {#functions}

### `Subscribe` {#Subscribe}

```go
func Subscribe(id string, priority Priority, fn func(*Event)) (*Listener, error)
```

Subscribe calls fn for every event with this id, on the server thread. The id is the full one, such as "`ll::event::PlayerJoinEvent`", or a unique suffix of it. Server thread only.

- Parameters:
    - id : `string`
    - priority : `Priority`
    - fn : `func(*Event)`
- Return type: `(*Listener, error)`
- Slots: [`subscribe_event`](../cpp/events.md#subscribe_event)

### `ListEvents` {#ListEvents}

```go
func ListEvents() ([]string, error)
```

ListEvents lists every event id this host can subscribe to. Server thread only.

- Return type: `([]string, error)`
- Slots: [`list_events`](../cpp/events.md#list_events)

## `Event` {#Event}

```go
type Event struct {
    ID   string
    SNBT string
    // unexported fields
}
```

Event is one event as a listener receives it. SNBT is the payload, whose fields the event payload reference documents; it is a copy, valid after the callback returns.

### `Event.Payload` {#Event.Payload}

```go
func (e *Event) Payload() (*Nbt, error)
```

Payload is SNBT parsed, once per event.

- Return type: `(*Nbt, error)`

### `Event.Unresolved` {#Event.Unresolved}

```go
func (e *Event) Unresolved() []string
```

Unresolved lists the fields the host could not resolve, from \_unresolved. It is empty too when the payload does not parse, which CheckComplete does not count as complete.

- Return type: `[]string`

### `Event.CheckComplete` {#Event.CheckComplete}

```go
func (e *Event) CheckComplete() bool
```

CheckComplete reports whether the payload parsed and nothing in it is unresolved. A protection decision on an incomplete payload should refuse rather than guess.

- Return type: `bool`

### `Event.Dim` {#Event.Dim}

```go
func (e *Event) Dim() (int32, error)
```

Dim is the dimension the event happened in. An unreadable or incomplete payload is an error and never 0: read as the overworld it would let an event in a custom dimension pass a rule made for the overworld.

- Return type: `(int32, error)`

### `Event.Set` {#Event.Set}

```go
func (e *Event) Set(key string, value *Nbt)
```

Set writes key back into the event when the callback returns. Only the keys set go back, and the host merges them, so two listeners setting different keys keep both.

- Parameters:
    - key : `string`
    - value : `*Nbt`

### `Event.Cancel` {#Event.Cancel}

```go
func (e *Event) Cancel()
```

Cancel asks the host to cancel the event once the callback returns. An event that cannot be cancelled ignores it; the documentation of each event says whether it can be.

### `Event.Uncancel` {#Event.Uncancel}

```go
func (e *Event) Uncancel()
```

Uncancel undoes a Cancel of this callback. A cancel made by an earlier listener stays.

## `Listener` {#Listener}

```go
type Listener struct {
    // unexported fields
}
```

Listener is one subscription, kept to end it.

### `Listener.Unsubscribe` {#Listener.Unsubscribe}

```go
func (l *Listener) Unsubscribe() error
```

Unsubscribe ends the subscription. Calling it twice is harmless. Server thread only.

- Return type: `error`
- Slots: [`unsubscribe_event`](../cpp/events.md#unsubscribe_event)

## `ListenerHandle` {#ListenerHandle}

```go
type ListenerHandle struct{ p unsafe.Pointer }
type ListenerHandle struct{ p unsafe.Pointer }
```

ListenerHandle is an event listener as the host names it.

### `ListenerHandle.IsZero` {#ListenerHandle.IsZero}

```go
func (h ListenerHandle) IsZero() bool
```

IsZero reports whether the handle names nothing.

- Return type: `bool`

## `Priority` {#Priority}

```go
type Priority int32
```

Priority orders the listeners of one event; it mirrors `ll::event::EventPriority`.

| Name | Value | Description |
|---|---|---|
| <span id="PriorityHighest"></span>`PriorityHighest` | `0` |  |
| <span id="PriorityHigh"></span>`PriorityHigh` | `1` |  |
| <span id="PriorityNormal"></span>`PriorityNormal` | `2` |  |
| <span id="PriorityLow"></span>`PriorityLow` | `3` |  |
| <span id="PriorityLowest"></span>`PriorityLowest` | `4` |  |
