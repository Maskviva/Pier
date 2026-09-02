# The mod lifecycle

A mod implements `LeviMod`. Only `on_load` is required.

```rust
impl LeviMod for MyMod {
    fn on_load(ctx: &ModContext) -> Result<Self> { ... }
    fn on_enable(&mut self, ctx: &ModContext) -> Result<()> { Ok(()) }
    fn on_disable(&mut self, ctx: &ModContext) -> Result<()> { Ok(()) }
    fn on_unload(&mut self, ctx: &ModContext) -> Result<()> { Ok(()) }
}
```

## What belongs where

| Callback | When | What belongs here |
|---|---|---|
| `on_load` | The mod is being brought up | Build your state. Returning an `Err` fails the load and the host rolls back. |
| `on_enable` | Every dependency has loaded | Subscribe to events, register commands, start work. This can run more than once. |
| `on_disable` | The mod is being stopped | Drop subscriptions, cancel scheduled tasks. |
| `on_unload` | The mod is going away | Anything left. |

## Enable runs more than once

A reload disables and re-enables without unloading, so `on_enable` has to be safe to run
again. Registering a command twice under the same name only swaps the callback, so that
part is already idempotent; anything of your own that is not needs a guard.

## Subscriptions unsubscribe when dropped

A `Listener` unsubscribes on drop, so holding one in your struct ties the subscription to
your mod:

```rust
struct MyMod {
    chat: Option<Listener>,
}

fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
    self.chat = Some(event::subscribe(names::PLAYER_CHAT, |ev| { ... })?);
    Ok(())
}

fn on_disable(&mut self, _ctx: &ModContext) -> Result<()> {
    self.chat = None;   // unsubscribes here
    Ok(())
}
```

Dropping the returned value straight away unsubscribes immediately, which looks identical
in code to forgetting to keep it. If you really want a subscription to live until unload,
say so with `.forget()` rather than letting it fall out of scope.

The same holds for `service::Registration`, `bus::Subscription` and `PacketHook`.

## Commands are one way

Bedrock has no route to remove a command, so one registered by your mod lives until the
server stops. While your mod is disabled the host mutes the callback rather than removing
it, and re-enabling resumes it.

There is therefore no `unregister` and no handle that deregisters on drop. A handle would
suggest it can be undone.

## Cancel what you scheduled

A scheduled task is accounted to your mod, and the host drops what is left at unload with
a warning. Cancel your own timers in `on_disable`:

```rust
let id = ctx.host().schedule_after(Duration::from_secs(30), || { ... })?;
// later
ctx.host().cancel(id);
```

`ctx.host().pending_tasks()` is what to assert on in `on_unload` if you want to be sure.
