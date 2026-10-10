# Talking to other mods

A Go mod talks to every other mod on the server, whatever language it is written in, and
to native LeviLamina plugins through the [bridge](../guide/bridge.md). Both channels carry UTF-8
strings the two sides agree on, JSON or SNBT, and the host never looks inside them, so a
Go provider and a Rust consumer need nothing but the same format.

| | Shape | Returns | Name |
|---|---|---|---|
| **Service** | One to one | Yes | Exclusive |
| **Bus** | One to many | No, but a subscriber can veto | Shared |

## Services: ask a question, get an answer

```go
reg, err := levilamina.RegisterService("plot:owner", func(request string) (string, error) {
	owner, ok := owners[request]
	if !ok {
		return "", fmt.Errorf("no plot %s", request)
	}
	return owner, nil
})
```

The reply goes back to the caller. An error's message goes back unchanged as the reason,
which is how a caller tells "no such plot" from "the database is down". A panic in the
provider reaches the caller as an error too, and never unwinds into the host.

A provider is called on the caller's thread, and also while this mod is disabled but still
loaded: consumers resolve services in their own `on_load`, which runs before anything is
enabled. Withdraw it with `reg.Unregister()`.

Calling one:

```go
owner, err := levilamina.CallService("plot:owner", "12,7")
var ce *levilamina.CallError
if errors.As(err, &ce) && ce.Kind == levilamina.CallProvider {
	log.Warn("the plot mod said no: " + ce.Message)
}
```

`CallError.Kind` is `CallNotFound` when no mod provides the name, `CallProvider` when the
provider refused, `CallRefused` when the host refused the call, and `CallUnavailable` on a
host without services. `CallServiceOptional` returns found false instead of an error when
nobody provides the name, for an integration that works without the other mod.
`ListServices` and `ServiceExists` say what is registered.

### Who is asking

The request text can claim anything. Inside a provider, `ServiceCaller()` asks the host
which mod's call is running; it returns false when the caller has no mod, such as a native
plugin through the bridge, so the provider knows it cannot attribute the request.

## The bus: tell everyone

```go
sub, err := levilamina.BusSubscribe("plot:enter", func(topic, payload string) bool {
	log.Info("someone entered a plot: " + payload)
	return false // no veto
})

delivered, err := levilamina.BusPublish("plot:enter", `{"player":"Steve","plot":"12,7"}`)
```

`BusPublish` returns how many subscribers ran; 0 means nobody is listening, which is not an
error. A subscriber runs on the publisher's thread. Namespace topics, as `plot:enter`.

`BusPublishVetoable` asks first: a subscriber returning **true** refuses, and the result's
`Vetoed` says whether anyone did. A subscriber can only refuse, never overturn another's
refusal, and every subscriber still receives the message. A panicking subscriber casts no
veto, as in the Rust binding: a veto is the stronger act and a bug should not cast one.

## Lanes

A lane hands raw function tables between two mods built by the same toolchain, as a faster
path beside a service. Its fingerprint folds in the compiler, so a Go mod never matches a
Rust or C++ one, and the Go binding does not offer lanes. A mod that publishes a lane also
provides a service, which consumers whose fingerprint does not match fall back to, and the
Go mod calls that service.
