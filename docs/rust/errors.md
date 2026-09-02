# Errors and logging

## A value that cannot be read is an error

Everything in this SDK that can fail to answer returns a `Result`, and the error says
why. Nothing returns a plausible default.

```rust
let dim = ev.dim()?;                  // not `.unwrap_or(0)`
let pos = player.position()?;         // not a silent (0, 0, 0)
```

The reason is one specific incident. An event in a custom dimension could not be read for
its dimension, the consumer wrote `unwrap_or(0)`, and every such event was treated as
happening in the overworld. Land protection then refused inside the overworld and allowed
everything everywhere else. Nothing was logged, and the behaviour looked deliberate.

When you do not know, refuse. That is the whole rule.

## Cannot-be-determined is not the same as no

A decision that might fail to answer does not return a bare `bool`. Collapsed into the
same `false` a caller can only guess; collapsed into `true` it is a security hole.

So `Ok(None)` and `Err` mean different things throughout:

```rust
let rule = dimensions::rule(dim, DimensionRule::SpawnMonster)?;
// Ok(Some(false)) -> registered, and off
// Ok(None)        -> never registered, so vanilla behaviour applies
// Err             -> the question could not be answered at all
```

## Identity {#identity}

`PlayerSel::Name` is not an identity. When the host cannot match an account name it falls
back to the display name, and another mod can change a display name. A player who sets
theirs to the account name of an offline player redirects every by-name call onto
themselves.

```rust
let p = Player::by_xuid(&id.xuid);     // permissions, economy, ownership
let p = Player::by_name("Steve");      // a name someone typed in chat
```

`sel.is_stable()` reports which kind you are holding. A permission decision that receives
`false` should take care.

## Logging

```rust
let log = ctx.logger();
log.info("started");
log.warn("the config had no entry for X, using nothing");
log.error(&format!("could not write the plot ledger: {e}"));
```

The line comes out under your mod's name, not under `[pier]`.

A log line has to answer what to do about it. "subscribe failed" does not; the id not
existing on this BDS version, with the nearby ids from the registry, does. Pier's own
messages follow that rule, so read them rather than skimming for the word `error`.

The success path logs nothing at info level.

## Panics

A panic must not cross the boundary back into the server, so every callback the SDK hands
to the host is wrapped. A panic in your handler is caught, logged, and the operation
degrades in the direction that is safe:

| Where | What happens |
|---|---|
| An event handler | Caught, logged, this edit discarded |
| A packet interceptor | Caught, logged, the packet forwarded unchanged |
| An economy callback | Caught, logged, treated as no veto |
| A lane table function | Caught, logged, the fallback value returned |
| A scheduled task | Caught, logged, that one task dropped |

Note the direction. A panicking packet interceptor forwards rather than drops, because
dropping everything would present as players being unable to join while the server looks
fine, which is far harder to diagnose. A panicking economy callback does not veto,
because a veto is the stronger action and should not be triggered by a bug.

This is a fence, not a feature. Handle your own errors.
