# Go: Cross-mod: bus, services and lanes

## Functions {#functions}

### `BusSubscribe` {#BusSubscribe}

```go
func BusSubscribe(topic string, fn func(topic, payload string) (veto bool)) (*Subscription, error)
```

BusSubscribe calls fn for every message published on topic, on the publisher's thread, whichever language the publisher is written in. fn returns a veto: true refuses a vetoable publish and false has no opinion, and a plain publish ignores it. A subscriber can only refuse, never overturn another's refusal. Namespace topics, as "plot:enter".

- Parameters:
    - topic : `string`
    - fn : `func(topic, payload string) (veto bool)`
- Return type: `(*Subscription, error)`
- Slots: [`bus_subscribe`](../cpp/crossmod.md#bus_subscribe)

### `BusPublish` {#BusPublish}

```go
func BusPublish(topic, payload string) (uint32, error)
```

BusPublish sends payload to every subscriber of topic and returns how many ran; 0 is a normal answer, meaning nobody is listening. The payload is opaque to the host, so the two mods agree on its format, JSON or SNBT, out of band.

- Parameters:
    - topic : `string`
    - payload : `string`
- Return type: `(uint32, error)`
- Slots: [`bus_publish`](../cpp/crossmod.md#bus_publish)

### `BusPublishVetoable` {#BusPublishVetoable}

```go
func BusPublishVetoable(topic, payload string) (Vetoable, error)
```

BusPublishVetoable sends payload and collects the subscribers' vetoes.

- Parameters:
    - topic : `string`
    - payload : `string`
- Return type: `(Vetoable, error)`
- Slots: [`bus_publish_vetoable`](../cpp/crossmod.md#bus_publish_vetoable)

### `BusSubscriberCount` {#BusSubscriberCount}

```go
func BusSubscriberCount(topic string) (uint32, error)
```

BusSubscriberCount is how many subscribers topic has now.

- Parameters:
    - topic : `string`
- Return type: `(uint32, error)`
- Slots: [`bus_subscriber_count`](../cpp/crossmod.md#bus_subscriber_count)

### `RegisterService` {#RegisterService}

```go
func RegisterService(name string, fn func(request string) (string, error)) (*ServiceRegistration, error)
```

RegisterService answers calls to name from mods of any language, and from native plugins through the bridge. fn receives the request and returns the reply, or an error whose message reaches the caller unchanged. It runs on the caller's thread, and also while this mod is disabled but loaded, since consumers resolve services in their own `on_load`.

- Parameters:
    - name : `string`
    - fn : `func(request string) (string, error)`
- Return type: `(*ServiceRegistration, error)`
- Slots: [`service_register`](../cpp/crossmod.md#service_register)

### `CallService` {#CallService}

```go
func CallService(name, request string) (string, error)
```

CallService calls another mod's service, whatever language it is written in, and returns its reply or a \*CallError.

- Parameters:
    - name : `string`
    - request : `string`
- Return type: `(string, error)`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `CallServiceOptional` {#CallServiceOptional}

```go
func CallServiceOptional(name, request string) (reply string, found bool, err error)
```

CallServiceOptional is CallService with nobody providing the name as found false, for an optional integration that works without the other mod.

- Parameters:
    - name : `string`
    - request : `string`
- Return type: `(reply string, found bool, err error)`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `ListServices` {#ListServices}

```go
func ListServices() ([]ServiceInfo, error)
```

ListServices lists every registered service.

- Return type: `([]ServiceInfo, error)`
- Slots: [`service_list`](../cpp/crossmod.md#service_list)

### `ServiceExists` {#ServiceExists}

```go
func ServiceExists(name string) bool
```

ServiceExists reports whether some mod provides name now.

- Parameters:
    - name : `string`
- Return type: `bool`
- Slots: [`service_list`](../cpp/crossmod.md#service_list)

### `ServiceCaller` {#ServiceCaller}

```go
func ServiceCaller() (string, bool)
```

ServiceCaller is the mod whose call is running, asked inside a provider. It is false outside a provider and for a caller with no mod, such as a native plugin through the bridge: a provider then knows it cannot attribute the request.

- Return type: `(string, bool)`
- Slots: [`service_caller`](../cpp/crossmod.md#service_caller)

## `Subscription` {#Subscription}

```go
type Subscription struct {
    // unexported fields
}
```

Subscription is one bus subscription, kept to end it.

### `Subscription.ID` {#Subscription.ID}

```go
func (s *Subscription) ID() uint64
```

ID is the host's id of the subscription.

- Return type: `uint64`

### `Subscription.Topic` {#Subscription.Topic}

```go
func (s *Subscription) Topic() string
```

Topic is the topic subscribed to.

- Return type: `string`

### `Subscription.Unsubscribe` {#Subscription.Unsubscribe}

```go
func (s *Subscription) Unsubscribe() error
```

Unsubscribe ends the subscription. Calling it twice is harmless.

- Return type: `error`
- Slots: [`bus_unsubscribe`](../cpp/crossmod.md#bus_unsubscribe)

## `ServiceRegistration` {#ServiceRegistration}

```go
type ServiceRegistration struct {
    // unexported fields
}
```

ServiceRegistration is one service this mod provides, kept to withdraw it.

### `ServiceRegistration.ID` {#ServiceRegistration.ID}

```go
func (r *ServiceRegistration) ID() uint64
```

ID is the host's id of the registration.

- Return type: `uint64`

### `ServiceRegistration.Name` {#ServiceRegistration.Name}

```go
func (r *ServiceRegistration) Name() string
```

Name is the service name.

- Return type: `string`

### `ServiceRegistration.Unregister` {#ServiceRegistration.Unregister}

```go
func (r *ServiceRegistration) Unregister() error
```

Unregister withdraws the service. Calling it twice is harmless.

- Return type: `error`
- Slots: [`service_unregister`](../cpp/crossmod.md#service_unregister)

## `CallError` {#CallError}

```go
type CallError struct {
    Kind    CallErrorKind
    Name    string
    Message string
}
```

CallError is why a service call failed: nobody provides the name, the provider said no and Message is what it said, the host refused the call (a bad name, a call to itself or a cycle), or the host has no service capability.

### `CallError.Error` {#CallError.Error}

```go
func (e *CallError) Error() string
```

- Return type: `string`

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

Vetoable is the result of one vetoable publish.

## `CallErrorKind` {#CallErrorKind}

```go
type CallErrorKind int
```

CallErrorKind says why a service call failed.

| Name | Value | Description |
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

ServiceInfo is one registered service and the mod providing it.
