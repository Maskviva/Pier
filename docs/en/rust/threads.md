# Threads

## The default

Every slot may be called on the **server thread** only, and the exceptions are noted per
slot. Every callback fires on the server thread, with one exception below.

Practically: if you are inside a lifecycle callback, an event handler, a command handler
or a scheduled task, you are on the server thread and everything is available.

## Getting back onto it

```rust
use levilamina::Host;

std::thread::spawn(|| {
    let data = expensive_lookup();
    Host::get().schedule(move || {
        // on the server thread now
        Host::get().execute_command(&format!("say {data}"));
    });
});
```

`schedule` and `schedule_after` are thread safe, and so is `Logger`.

`schedule_after` takes a `Duration` rather than a millisecond count, so a call site says
which it is:

```rust
host.schedule_after(Duration::from_secs(5), || { ... })?;
```

## What is safe from any thread

| | |
|---|---|
| `Host::schedule`, `Host::schedule_after`, `Host::cancel` | Thread safe by contract |
| `Logger` | Thread safe by contract, and `Copy` |
| `KvDb` | The host locks internally |
| `Host::gaming_status` | Thread safe by contract |

Everything else is the server thread.

## The one exception: packet interception

A packet interceptor does **not** run on the server thread. An inbound callback runs on
whichever thread pumps that connection and an outbound one on the thread that started the
send. Usually that is the server thread, but an async flush means it is not guaranteed,
and the same closure may be entered by several threads at once.

That is why the closure bound there is `Send + Sync` and not `Send`. Do not touch world
state from a packet callback; `Host::get().schedule()` back onto the server thread first.

## Holding a handle across threads

A mod struct has to be `Send`, since a lifecycle callback may be entered on a different
thread. The RAII handles are `Send`, so a struct can hold them:

```rust
struct MyMod {
    chat: Option<Listener>,
    hook: Option<PacketHook>,
}
```

They are not `Sync`, which is the honest bound: the closure inside is only `Send`.
