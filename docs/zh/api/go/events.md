# Go：事件

## 函数 {#functions}

### `Subscribe` {#Subscribe}

```go
func Subscribe(id string, priority Priority, fn func(*Event)) (*Listener, error)
```

对这个 id 的每一个事件，在服务器线程上调用 `fn`。id 是完整的 id，例如 `"ll::event::PlayerJoinEvent"`，也可以是它唯一的后缀。只能在服务器线程调用。

- 参数：
    - id : `string`
    - priority : `Priority`
    - fn : `func(*Event)`
- 返回值类型：`(*Listener, error)`
- 对应槽位：[`subscribe_event`](../cpp/events.md#subscribe_event)

### `ListEvents` {#ListEvents}

```go
func ListEvents() ([]string, error)
```

列出这个宿主可以订阅的所有事件 id。只能在服务器线程调用。

- 返回值类型：`([]string, error)`
- 对应槽位：[`list_events`](../cpp/events.md#list_events)

## `Event` {#Event}

```go
type Event struct {
    ID   string
    SNBT string
    // unexported fields
}
```

监听器收到的一个事件。`SNBT` 是载荷，它的字段写在事件载荷参考里；它是一份副本，回调返回以后仍然有效。

### `Event.Payload` {#Event.Payload}

```go
func (e *Event) Payload() (*Nbt, error)
```

解析后的 `SNBT`，每个事件只解析一次。

- 返回值类型：`(*Nbt, error)`

### `Event.Unresolved` {#Event.Unresolved}

```go
func (e *Event) Unresolved() []string
```

列出宿主没能解析的字段，取自 `_unresolved`。载荷本身解析不了时它也是空的，`CheckComplete` 不会把这种情况算作完整。

- 返回值类型：`[]string`

### `Event.CheckComplete` {#Event.CheckComplete}

```go
func (e *Event) CheckComplete() bool
```

判断载荷是否解析成功，并且里面没有未解析的字段。基于不完整的载荷做保护判断时，应当拒绝，不要去猜。

- 返回值类型：`bool`

### `Event.Dim` {#Event.Dim}

```go
func (e *Event) Dim() (int32, error)
```

事件发生的维度。载荷读不出来或者不完整时返回错误，永远不会返回 0：如果当成主世界，自定义维度里的事件就会通过为主世界定的规则。

- 返回值类型：`(int32, error)`

### `Event.Set` {#Event.Set}

```go
func (e *Event) Set(key string, value *Nbt)
```

在回调返回时把 `key` 写回事件里。只有设置过的键会写回去，宿主会合并它们，所以两个监听器设置不同的键时，两边的改动都会保留。

- 参数：
    - key : `string`
    - value : `*Nbt`

### `Event.Cancel` {#Event.Cancel}

```go
func (e *Event) Cancel()
```

请求宿主在回调返回后取消这个事件。不能取消的事件会忽略这个请求；每个事件能不能取消，写在它的文档里。

### `Event.Uncancel` {#Event.Uncancel}

```go
func (e *Event) Uncancel()
```

撤销这个回调里做的 `Cancel`。之前的监听器做的取消会保留。

## `Listener` {#Listener}

```go
type Listener struct {
    // unexported fields
}
```

一个订阅，保存下来用来结束它。

### `Listener.Unsubscribe` {#Listener.Unsubscribe}

```go
func (l *Listener) Unsubscribe() error
```

结束订阅。调用两次也没有问题。只能在服务器线程调用。

- 返回值类型：`error`
- 对应槽位：[`unsubscribe_event`](../cpp/events.md#unsubscribe_event)

## `ListenerHandle` {#ListenerHandle}

```go
type ListenerHandle struct{ p unsafe.Pointer }
type ListenerHandle struct{ p unsafe.Pointer }
```

宿主用来指代一个事件监听器的句柄。

### `ListenerHandle.IsZero` {#ListenerHandle.IsZero}

```go
func (h ListenerHandle) IsZero() bool
```

判断这个句柄是否什么都不指向。

- 返回值类型：`bool`

## `Priority` {#Priority}

```go
type Priority int32
```

同一个事件的监听器之间的顺序，对应 `ll::event::EventPriority`。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PriorityHighest"></span>`PriorityHighest` | `0` |  |
| <span id="PriorityHigh"></span>`PriorityHigh` | `1` |  |
| <span id="PriorityNormal"></span>`PriorityNormal` | `2` |  |
| <span id="PriorityLow"></span>`PriorityLow` | `3` |  |
| <span id="PriorityLowest"></span>`PriorityLowest` | `4` |  |
