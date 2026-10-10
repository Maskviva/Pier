# FAQ

## Which languages can I write a mod in?

Any language that can export a C function from a DLL. Rust and C++ are official; the
[Adding a language](adding-a-language.md) guide is the whole list of what a new binding
has to do.

## Do I have to rebuild my mod when Pier updates?

Only when the ABI version changes. A release that keeps `PIER_ABI_VERSION` loads every mod
built against the same version unchanged, and no binding removes anything from its public
surface in such a release; what is retired is deprecated first. The
[changelog](https://github.com/Maskviva/pier/blob/main/CHANGELOG.md) states the ABI
version of every release.

## How do I get the killer from a death event?

From the damage source of `MobDieEvent` or `PlayerDieEvent`: `attackerUid` is the actor
responsible, the shooter for a projectile, and `attacker` describes it while it exists.
[Event payloads](event-payloads.md#who-killed-whom) has the details.

## Can Pier stop a mod that hangs the server?

It can name it and restart the server, not stop the mod alone. A mod runs on the server's
own threads, and no thread can be pulled out of code that holds locks without leaving them
held. The [watchdog](configuration.md#watchdog) logs the mod and ends the process so a
supervisor restarts it.

## Does Pier work on the client?

Pier has a client build that loads client-target mods. Server-only capabilities are NULL
slots there, and every binding reports them as not provided rather than failing silently.
The [bridge](bridge.md) is server only.

## Why does a call return "not provided" when the slot exists?

A slot can be in the table and empty: its capability package was not built into the host,
or the host does not implement it on this engine version. Bindings report both as not
provided, which is different from a call that ran and failed.

## Can a LeviLamina C++ plugin call into a Pier mod?

Through the [bridge](bridge.md): a single header that lets a native plugin call the
services Pier mods register, by name, without linking against Pier.
