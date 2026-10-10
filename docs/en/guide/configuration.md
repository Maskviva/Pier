# ⚙️ Configuring

Pier's settings live in `plugins/Pier/config.json`. On the first start, when the file does
not exist, Pier writes a default one; after that it **only reads it**, so what you change
stays.

The default file:

```json
{
  "language": "auto",
  "mods": {
    "disabled": []
  },
  "hooks": {
    "disabled": [],
    "decision_ttl_ms": 250
  },
  "watchdog": {
    "enabled": true,
    "warn_ms": 2000,
    "hang_ms": 30000,
    "shutdown_ms": 10000
  }
}
```

A setting that a newer Pier added is absent from an older file you already have, and takes
its default. To change it, add it to the file yourself.

!!! warning "A misspelled key is ignored"

    Pier drops the keys it does not know, and the log does not mention them. When a change has
    no effect, check the spelling against the file above. A value of the wrong type, text where a
    number belongs, is ignored with a line naming the key and the type it wants.

## Log language `language` {#language}

Which language Pier's log is in. The translations are in `plugins/Pier/lang/`, shipped with
the release.

- `"auto"`: follow the game's language. For a server with one language, this is the one.
- A language code fixes one, such as `"zh_CN"`. Both `zh_CN` and `zh-CN` work.

When a line has no translation in the language chosen, Pier uses the English one, and the
line's key when English has none either. A half-translated file works.

One line at startup says which language is in use and what chose it:

```
[host] language zh_CN (named in config.json): 29 key(s) from plugins/Pier/lang
```

When the code you set has no file, a WARN line follows. The key count above still looks
healthy then, since the built-in English answers every line, and the WARN is there to tell
you so.

Adding a language needs no rebuild: copy `en_US.lang` to the new code, translate it and put
it in `lang/`. See [Adding a language](adding-a-language.md).

## Disabling a mod `mods.disabled` {#mods-disabled}

To keep a Pier mod from loading without deleting its files, name it here:

```json
"mods": { "disabled": ["some-mod"] }
```

- The name must match the `name` of that mod's `manifest.json` exactly.
- Pier refuses it **before loading the DLL**, so none of the mod's code runs, its static
  constructors and `DllMain` included.

## Turning off a Pier event `hooks.disabled` {#hooks-disabled}

To keep every mod from intercepting something, dropping items say, name the event:

```json
"hooks": { "disabled": ["PlayerDropItemEvent"] }
```

- A mod subscribing to it afterwards gets a failed subscription, and the log has a line
  naming the mod.
- The mod receives the failure, not a listener that never fires, so a protection mod does not
  go on logging that it protects something while nothing is intercepted.
- Use it to turn off one interception without removing the mod that wants it. That mod then
  behaves as if the event did not exist.

## The pressure plate cache `hooks.decision_ttl_ms` {#hooks-decision-ttl-ms}

How many milliseconds the result of a pressure plate or piston decision is cached. The
default is 250, five game ticks; from 0 to 5000, and a value outside is brought into range
with a line saying so.

Why there is a cache: every entity standing on a pressure plate triggers a decision every
tick. Each decision builds the event payload, hands it to the mod, which parses it, looks up
its land data and hands the answer back. A player standing still runs this 20 times a second.

- **0**: ask the mod on every tick. It is there for whoever is developing a land mod and wants
  to see every call, and is not meant for a running server.
- **Larger**: an "allow" is remembered for longer. The position is part of what is cached, so
  moving one block clears it; what can go wrong is a player standing still while their
  permissions change.

## The watchdog `watchdog` {#watchdog}

A Pier mod is native code running on the server's own threads. A callback that never returns,
an endless loop, a deadlock, a stuck network call, stops the server; one stuck during shutdown
(`on_disable`, `on_unload`, the DLL's destructors) keeps the server from ever stopping.

The watchdog watches for this. When a mod holds a thread too long it logs who; longer still,
it ends the process so a panel or supervisor brings the server back.

```json
"watchdog": {
  "enabled": true,
  "warn_ms": 2000,
  "hang_ms": 30000,
  "shutdown_ms": 10000
}
```

| Key | Default | Range | What it does |
|---|---|---|---|
| `enabled` | `true` | | `false` turns the whole watchdog off |
| `warn_ms` | 2000 | 0–3600000 | Log a warning naming the mod once it has held a thread this long; 0 never warns |
| `hang_ms` | 30000 | 0–3600000 | End the process once a mod has held a thread this long while running; 0 never ends it |
| `shutdown_ms` | 10000 | 0–3600000 | The same limit once shutdown has begun; 0 waits forever |

Every place Pier runs a mod's code is watched: loading and unloading the DLL, `pier_main`,
the three lifecycle functions, and every event, command, task, form, bus, service, packet
and hook callback.

The log names the mod, the entry it was in and the thread. When a slow call finally returns,
how long it took is logged too:

```
[watchdog] 'land-claims' has held thread 4812 inside event for 2000 ms
[watchdog] 'land-claims' returned from event after about 2600 ms
```

### What happens at the limit

Pier writes the reason to the console and the log, then ends **the whole server process** with
exit code **70**.

Why not stop only that mod: its code is running on the server's own thread, maybe holding
locks and halfway through writing data. Pulled out of the middle, the thread leaves those
locks held and the data half-written, and what the server does after that cannot be told.

Changes since the last autosave are lost, as when an operator kills a hung server by hand, only
sooner and with the log naming who. A panel or supervisor set to restart on a non-zero exit
brings the server back.

### How time is counted

- The watchdog keeps its own clock and counts at most 200 milliseconds per step, so time the
  process spends paused, or the machine asleep, is not charged to the mod.
- With a debugger attached the process is not ended: going past the limit logs once and the
  process goes on, so a breakpoint in a mod does not kill the session.

!!! tip "A mod really is slow at startup?"

    Raise `hang_ms` rather than turning the watchdog off. Set `enabled` to `false` only while
    tracking down a problem where the watchdog has to be ruled out.

## When the file cannot be read

A missing comma in the JSON, say. Pier then:

1. uses the default of every setting for this run;
2. logs an error saying what that means, such as: `mods.disabled` is empty this run, and the
   mods named in it load as usual;
3. starts as usual.

Pier is the loader, and when it does not start, no mod on the server starts either, so it
starts on the defaults. Where a default is looser than your setting is what that error line
spells out.
