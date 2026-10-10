# Troubleshooting

Every refusal Pier makes is logged with its reason, so the first step is always the
server console or `logs/latest.log`. This page lists the lines you will meet and what to
do about each.

## A mod does not load

| The log says | Meaning | What to do |
|---|---|---|
| `'x' does not export pier_main` | the DLL is not a Pier mod, or its entry symbol was not exported | build it with your binding's entry macro, or `PIER_MAIN_EXPORT` in C++ |
| `'x': pier_main returned false` | the mod refused to start; its own log lines say why | read the lines just above |
| `'x': an exception left pier_main` | the mod let an exception out of its entry point | fix the mod: nothing may unwind into the host |
| `'x' was built against Pier ABI vN, below the minimum vM` | the mod predates the current ABI | rebuild the mod against the current SDK |
| a line about the client or the server target | a client build on a server, or the reverse | install the build made for this side |
| `'x' filled in a vtable of N bytes` | the mod's SDK does not set `struct_size` | update the SDK the mod was built with |
| the mod is named in `mods.disabled` | the operator switched it off | remove it from `mods.disabled` in `config.json` |

## The server does not start, with error 0x7E

`0x7E, the specified module could not be found` means Windows could not load a DLL that
Pier or a mod depends on. Pier itself loads without its optional dependencies, such as
LegacyMoney; a mod built with MinGW needs the MinGW runtime DLLs next to it, and a mod built
in Debug needs the debug C++ runtime. Rebuild the mod in Release with MSVC or clang-cl, or
ship the DLLs it depends on.

## The server froze or exited with code 70

Exit code 70 is Pier's watchdog. A mod held a server thread past `watchdog.hang_ms`, or
past `watchdog.shutdown_ms` while stopping, and the process was ended so that it can be
restarted. The line before it names the mod, the entry it was stuck in and for how long:

```
[watchdog] 'land-claims' has held thread 4812 inside event for 30000 ms, past the runtime
limit of 30000 ms; ending the process with exit code 70 so it can be restarted
```

Report it to that mod's author. If the mod legitimately works that long at startup, raise
`watchdog.hang_ms` rather than turning the watchdog off; see
[Configuring](configuration.md#watchdog).

## An event never fires

- The subscription was refused: a synthetic event named in `hooks.disabled` says so in
  the log when a mod subscribes.
- The payload says why it is incomplete: `_unresolved`, `partial` and `targetKnown` are
  described in [Event payloads](event-payloads.md).
- An event that fires on one engine version and not another usually means the engine
  function it hooks changed. Report it with the Pier and BDS versions from the startup line.

## A setting has no effect

A key Pier does not read is dropped silently, and a value of the wrong type is ignored with
a line naming the key and the type it wants, such as `'watchdog.hang_ms' wants number, and
the value written there is ignored`. `config.json` is
written once and never rewritten, so a key added in a later release is absent from an old
file and takes its default. Compare the file with the one in
[Configuring](configuration.md).

## Asking for help

Include the Pier and BDS versions, which the first lines of the log carry, the full log
from startup to the problem, and the list of mods. A refused load, a watchdog exit and an
ignored setting each leave a line in the log with the reason and the name; without the log,
that line has to be asked for again.
