# Go：跨模组：总线、服务与快速通道

## 函数 {#functions}

### `BusSubscribe` {#BusSubscribe}

```go
func BusSubscribe(topic string, fn func(topic, payload string) (veto bool)) (*Subscription, error)
```

对 `topic` 上发布的每一条消息调用 `fn`，在发布方的线程上调用，不管发布方是用什么语言写的。`fn` 返回否决：true 拒绝一次可否决的发布，false 表示没有意见，普通的发布忽略它。订阅者只能拒绝，不能推翻别人的拒绝。主题请加命名空间，比如 `"plot:enter"`。

- 参数：
    - topic : `string`
    - fn : `func(topic, payload string) (veto bool)`
- 返回值类型：`(*Subscription, error)`
- 对应槽位：[`bus_subscribe`](../cpp/crossmod.md#bus_subscribe)

### `BusPublish` {#BusPublish}

```go
func BusPublish(topic, payload string) (uint32, error)
```

把 `payload` 发给 `topic` 的每一个订阅者，返回运行了多少个；0 是正常的回答，表示没有人在听。载荷对宿主是不透明的，所以两个模组要在别处约定它的格式，JSON 或者 SNBT。

- 参数：
    - topic : `string`
    - payload : `string`
- 返回值类型：`(uint32, error)`
- 对应槽位：[`bus_publish`](../cpp/crossmod.md#bus_publish)

### `BusPublishVetoable` {#BusPublishVetoable}

```go
func BusPublishVetoable(topic, payload string) (Vetoable, error)
```

发送 `payload`，并收集订阅者的否决。

- 参数：
    - topic : `string`
    - payload : `string`
- 返回值类型：`(Vetoable, error)`
- 对应槽位：[`bus_publish_vetoable`](../cpp/crossmod.md#bus_publish_vetoable)

### `BusSubscriberCount` {#BusSubscriberCount}

```go
func BusSubscriberCount(topic string) (uint32, error)
```

`topic` 当前有多少订阅者。

- 参数：
    - topic : `string`
- 返回值类型：`(uint32, error)`
- 对应槽位：[`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)

### `RegisterService` {#RegisterService}

```go
func RegisterService(name string, fn func(request string) (string, error)) (*ServiceRegistration, error)
```

回答任何语言写的模组对 `name` 的调用，也回答原生插件经 bridge 发来的调用。`fn` 收到请求，返回回答，或者返回一个错误，错误信息会原样到达调用方。它在调用方的线程上运行；这个模组被禁用、但仍然加载着的时候也会运行，因为使用方会在自己的 `on_load` 里解析服务。

- 参数：
    - name : `string`
    - fn : `func(request string) (string, error)`
- 返回值类型：`(*ServiceRegistration, error)`
- 对应槽位：[`service_register`](../cpp/crossmod.md#service_register)

### `CallService` {#CallService}

```go
func CallService(name, request string) (string, error)
```

调用另一个模组的服务，不管它是用什么语言写的；返回它的回答，或者一个 `*CallError`。

- 参数：
    - name : `string`
    - request : `string`
- 返回值类型：`(string, error)`
- 对应槽位：[`service_call`](../cpp/crossmod.md#service_call)

### `CallServiceOptional` {#CallServiceOptional}

```go
func CallServiceOptional(name, request string) (reply string, found bool, err error)
```

没有模组提供这个名字时，以 `found` 为 false 返回的 `CallService`，用于没有另一个模组也能工作的可选集成。

- 参数：
    - name : `string`
    - request : `string`
- 返回值类型：`(reply string, found bool, err error)`
- 对应槽位：[`service_call`](../cpp/crossmod.md#service_call)

### `ListServices` {#ListServices}

```go
func ListServices() ([]ServiceInfo, error)
```

列出所有已注册的服务。

- 返回值类型：`([]ServiceInfo, error)`
- 对应槽位：[`service_list`](../cpp/crossmod.md#service_list)

### `ServiceExists` {#ServiceExists}

```go
func ServiceExists(name string) bool
```

判断现在有没有模组提供 `name`。

- 参数：
    - name : `string`
- 返回值类型：`bool`
- 对应槽位：[`service_list`](../cpp/crossmod.md#service_list)

### `ServiceCaller` {#ServiceCaller}

```go
func ServiceCaller() (string, bool)
```

在提供方内部调用时，返回正在调用它的模组。在提供方之外，或者调用方没有模组（比如经 bridge 调用的原生插件）时为 false：提供方由此知道它无法确定请求来自谁。

- 返回值类型：`(string, bool)`
- 对应槽位：[`service_caller`](../cpp/crossmod.md#service_caller)

## `Subscription` {#Subscription}

```go
type Subscription struct {
    // unexported fields
}
```

一个总线订阅，保存下来用来结束它。

### `Subscription.ID` {#Subscription.ID}

```go
func (s *Subscription) ID() uint64
```

宿主给这个订阅的 id。

- 返回值类型：`uint64`

### `Subscription.Topic` {#Subscription.Topic}

```go
func (s *Subscription) Topic() string
```

订阅的主题。

- 返回值类型：`string`

### `Subscription.Unsubscribe` {#Subscription.Unsubscribe}

```go
func (s *Subscription) Unsubscribe() error
```

结束订阅。调用两次也没有问题。

- 返回值类型：`error`
- 对应槽位：[`bus_unsubscribe`](../cpp/crossmod.md#bus_unsubscribe)

## `ServiceRegistration` {#ServiceRegistration}

```go
type ServiceRegistration struct {
    // unexported fields
}
```

这个模组提供的一项服务，保存下来用来撤回它。

### `ServiceRegistration.ID` {#ServiceRegistration.ID}

```go
func (r *ServiceRegistration) ID() uint64
```

宿主给这项注册的 id。

- 返回值类型：`uint64`

### `ServiceRegistration.Name` {#ServiceRegistration.Name}

```go
func (r *ServiceRegistration) Name() string
```

服务的名字。

- 返回值类型：`string`

### `ServiceRegistration.Unregister` {#ServiceRegistration.Unregister}

```go
func (r *ServiceRegistration) Unregister() error
```

撤回这项服务。调用两次也没有问题。

- 返回值类型：`error`
- 对应槽位：[`service_unregister`](../cpp/crossmod.md#service_unregister)

## `CallError` {#CallError}

```go
type CallError struct {
    Kind    CallErrorKind
    Name    string
    Message string
}
```

服务调用失败的原因：没有模组提供这个名字；提供方拒绝了，`Message` 是它给的理由；宿主拒绝了这次调用（名字不合法、调用自己，或者形成了循环）；或者宿主没有服务这项能力。

### `CallError.Error` {#CallError.Error}

```go
func (e *CallError) Error() string
```

- 返回值类型：`string`

## `Vetoable` {#Vetoable}

```go
type Vetoable struct {
    // Vetoed is true when any subscriber refused.
    Vetoed bool
    // Delivered is how many subscribers ran. There is no short circuit: after a veto the
    // later subscribers still receive the message, so observers see a consistent stream.
    Delivered uint32
}
```

一次可否决发布的结果。

## `CallErrorKind` {#CallErrorKind}

```go
type CallErrorKind int
```

说明一次服务调用为什么失败。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="CallNotFound"></span>`CallNotFound` | `1` |  |
| <span id="CallProvider"></span>`CallProvider` | `2` |  |
| <span id="CallRefused"></span>`CallRefused` | `3` |  |
| <span id="CallUnavailable"></span>`CallUnavailable` | `4` |  |

## `ServiceInfo` {#ServiceInfo}

```go
type ServiceInfo struct {
    Name string `json:"name"`
    Mod  string `json:"mod"`
}
```

一项已注册的服务，以及提供它的模组。
