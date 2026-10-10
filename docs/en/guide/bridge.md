# Calling Pier mods from a mod that is not one

Every Pier mod can register a service: a name, a UTF-8 request, a UTF-8 reply. A mod
that owns the server's economy answers a balance query; one that owns its land claims
answers whether a block may be broken. Inside Pier a mod reaches them through
`service_call`.

A native LeviLamina mod cannot. `service_call` takes a `PierModHandle` the Pier loader
issues, and Pier exports one symbol, `pier_main`, which points the other way. So a mod
whose whole value is being a backend everyone shares was reachable only by the half of
the ecosystem that had already adopted Pier.

`bindings/bridge/pier-bridge.h` is the door. Header-only, no dependency, nothing to link.
Its file name names Pier, the ABI it talks to, and its namespace is `levilamina::bridge`,
the same rule as the Rust, Go and Zig bindings. Code written against `pier::bridge`, the
namespace before 26.51.2, still builds through a deprecated alias, and MSVC and clang warn
where it is used.

```cpp
#include "pier-bridge.h"

auto client = levilamina::bridge::Client::open();
if (!client) {
    logger.warn("Pier is not installed; falling back to operator-only");
    return;
}

auto reply = client->call("example:economy.balance",
    R"({"player":"2535470000000000"})");

switch (reply.status) {
    case levilamina::bridge::Status::Ok:        useTheAnswer(reply.body); break;
    case levilamina::bridge::Status::NotFound:  /* nobody provides it: a normal deployment */ break;
    default:                              logger.warn(reply.body); break;
}
```

`client->services()` lists everything registered, as `[{"name":…,"mod":…}]`. Asking once at
load reads better in a log than a `NotFound` at the moment somebody needed the answer.

A whole working mod is in `examples/hello-bridge`: a native LeviLamina mod that opens the
client in `enable()` and logs what it finds.

## What it is not

It is not on the client. A client build of Pier exports none of the bridge symbols, so
`Client::open()` returns null there and the caller takes its "not installed" path.

It is not the Pier mod ABI. No events, no hooks, no forms, no lane, no dimensions. A mod
that wants those is a Pier mod and includes `sdk/abi.h`. This is for the mod that already
has its own loader and wants to ask one question.

It also goes one way. A native mod can call into a service a Pier mod registered; a Pier
mod cannot call out into a native mod, because there is no half of this that lets an
outside mod register a service.

## The caller is anonymous

A Pier mod's calls carry a handle, so a provider can be told who is asking. A bridge caller
has none and never will: it is a different dll on a different loader, holding nothing Pier
issued.

That is a real limit, not a formality. **A provider that grants anything on the strength of
the caller alone must not be reachable this way.** Anything a provider needs to know goes
in the request instead: a provider with an endpoint that changes something should take
the identity of whoever asked for it in the body, rather than inferring authority from
the connection.

It is not a back door. It reaches the same registry, under the same lock, with the same
revalidation of the provider immediately before crossing into its dylib. What it lacks is a
name to put in a log line.

## Versioning

`pier_bridge_abi()` returns the ABI of the three symbols. `Client::open()` calls it first
and returns null on a mismatch rather than trying: a signature that changed under a caller
is the one failure that crashes instead of returning an error.

Adding a service never raises it. Services are data over this ABI, not part of it, so a mod
built against bridge ABI 1 keeps working as the managers gain endpoints.

## Threading

Synchronous on your thread, no timeout, exactly like a Pier mod's call. A provider that
blocks blocks you. Most services expect the server thread; whether one may be called from a
worker is that service's own contract.

## Long replies

A Pier from this release on exports `pier_bridge_call_sink` beside the buffer form, and
`Client::call` uses it when it is there: the reply arrives whole and the provider runs
once, whatever its length.

An older Pier has only the buffer form. A reply that does not fit the first 2 KiB buffer is
fetched by calling again with a larger one, **which runs the provider a second time**. For
a read that costs a little time; for anything that changes state it does the change twice.
`client->runsOnce()` tells the two apart, so a caller of such a service can refuse to go on
against an older host rather than risk it.

## Load order

`Client::open()` needs Pier to be in the process already. It looks the loaded copy up with
`GetModuleHandleEx` and returns null when there is none. It does not load Pier.dll itself:
a copy loaded that way would be a second Pier with an empty service registry, and the
services you call would not be in it.

Declare Pier as a dependency in the manifest, which is what orders the two:

```json
{
  "name": "your-mod",
  "entry": "your_mod.dll",
  "type": "native",
  "dependencies": [ { "name": "pier" } ]
}
```

Then open in `enable()` rather than `load()`. The `Client` holds a **reference** to
Pier.dll until it is destroyed, so the bound pointers cannot be pulled out from under it.
That does not keep Pier's mod alive: Pier unregisters its services on disable, and calls
after that answer `NotFound`, which is a degradation a caller can handle.
