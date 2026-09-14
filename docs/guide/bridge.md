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

```cpp
#include "pier-bridge.h"

auto pier = pier::bridge::Client::open();
if (!pier) {
    logger.warn("Pier is not installed; falling back to operator-only");
    return;
}

auto reply = pier->call("example:economy.balance",
    R"({"player":"2535470000000000"})");

switch (reply.status) {
    case pier::bridge::Status::Ok:        useTheAnswer(reply.body); break;
    case pier::bridge::Status::NotFound:  /* nobody provides it: a normal deployment */ break;
    default:                              logger.warn(reply.body); break;
}
```

`pier->services()` lists everything registered, as `[{"name":…,"mod":…}]`. Asking once at
load reads better in a log than a `NotFound` at the moment somebody needed the answer.

A whole working mod is in `examples/hello-bridge`: a native LeviLamina mod that opens the
client in `enable()` and logs what it finds.

## What it is not

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

## Load order

`Client::open()` needs Pier to be in the process already. It binds through
`GetModuleHandleEx`, so it attaches to the copy that is loaded and never brings in a
second one; a second copy would be an empty registry rather than the running one.

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
