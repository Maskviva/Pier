# Talking to other mods

Three channels, for three different shapes of question.

| | Shape | Returns | Name |
|---|---|---|---|
| **Service** | One to one | Yes | Exclusive |
| **Bus** | One to many | No | Shared |
| **Lane** | One to one, same toolchain | Yes, natively | Exclusive |

## Services: ask a question, get an answer

```rust
use levilamina::service;

// Providing
let _reg = service::register("plot:owner", |_name, req| {
    let plot: PlotQuery = serde_json::from_str(req).map_err(|e| e.to_string())?;
    Ok(lookup(plot))
})?;

// Calling
let owner: Owner = service::call_with("plot:owner", &query)?;
```

`call_with` serializes the request and deserializes the reply; `call_json` does the reply
half when the request is already a string. Either turns a malformed reply into a real
error rather than a value that looks fine.

The errors are categorized, because they call for different fixes:

| | Means |
|---|---|
| `NotFound` | The mod is not installed, not enabled, or the name is misspelled |
| `Provider` | It ran and refused; the message is the provider's |
| `Refused` | An invalid name, a call to itself, or a cycle |
| `Decode` | The reply did not parse into your type |
| `Unavailable` | The host has no service capability |

`call_optional` returns `None` on `NotFound` and raises everything else, which is what an
optional integration wants.

A service name is exclusive: a second provider is refused and the host log names the
holder. Two providers would make the answer depend on mod load order.

::: tip
A service stays reachable while its mod is disabled, because LeviLamina enables mods only
after every `on_load` has run. Being unreachable in that window would fail every consumer
that resolves its dependency during its own `on_load`.
:::

## The bus: tell everyone

```rust
use levilamina::bus;

let _sub = bus::subscribe("plot:claimed", |topic, payload| {
    // returning true is a veto, which only publish_vetoable reads
    false
})?;

let n = bus::publish("plot:claimed", &payload)?;
```

You never receive your own publish. Notifying yourself is a direct function call, and
publishing to yourself is the one cycle no depth limit can distinguish. A cross-mod cycle
is caught by the depth cap, which drops the innermost publish and logs it.

Callbacks run on the **publisher's** thread, so schedule back before touching the world.

## Lanes: skip the serialization

A lane hands over raw `data` and `vtable` pointers, so two mods built by the same
toolchain can call each other directly. It only works when the fingerprints match, and
falls back to the service channel otherwise:

```rust
match lane::acquire::<MyContract>() {
    Ok(lane) => lane.with(|table, data| unsafe { (table.count)(data) }),
    Err(_) => service::call("plot:count", "")?.parse().ok(),
}
```

The lane name and the fingerprint both come from the `LaneContract`, so `acquire` takes
no arguments beyond the type. `with` hands the closure the table and the provider's own
`data` pointer, and returns `None` once the provider is gone.

`LaneContract::FINGERPRINT` has to change whenever the table shape does. Both sides
referencing the same contract definition makes it match automatically; copying an
identical constant by hand is exactly what it guards against.

Use a lane only where a service is measurably too slow. It trades safety for speed.
