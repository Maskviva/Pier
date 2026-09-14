# Installation

## Requirements

| | |
|---|---|
| Server | Bedrock Dedicated Server 1.26.32 |
| Loader | LeviLamina 26.40.0 |
| Optional | [LegacyMoney](https://github.com/LiteLDev/LegacyMoney) for the economy calls |

LegacyMoney really is optional. Pier delay-loads it, so a server without it starts
normally; the economy calls return failure values and everything else is unaffected.

## With lip

[lip](https://lip.futrime.com) is the LeviLamina package manager.

```bash
lip install github.com/Maskviva/pier
```

## By hand

Download `pier-windows-x64.zip` from the
[releases page](https://github.com/Maskviva/pier/releases) and unpack it into
`plugins/pier/`.

## Checking it worked

Start the server. The log carries a line from the host once it is ready:

```
[host] ready, ABI v2, api table 1600 bytes
```

The byte count is whatever the host compiled and grows every time a capability is
appended, so read the line for the ABI version and not for that number.

Then, in the console:

```
/pier list
```

That lists the mods Pier has loaded. It is empty until you install one.

## The language of the log

Pier reads `plugins/pier/lang/<code>.lang` and never writes there. The directory comes
with the release and holds `en_US.lang` and `zh_CN.lang`, so a server in either language
needs nothing done.

Install by copying the whole mod directory. Copying the dll alone leaves the lang
directory behind, and the symptom is an English log on a translated server; English is
compiled in as the fallback, so nothing fails, and nothing says why either.

Which code is read is `language` in `config.json`, and the default `"auto"` follows the
locale the engine reports.

Set it explicitly when the operator reads a different language than the server runs in:

```json
{ "language": "zh_CN" }
```

`zh_CN` and `zh-CN` both work. The startup line says whether the code came from the
config or from the engine, and a code with no file behind it gets a warning of its own.
See [Configuring](./configuration).

To change a line, edit its value in the file for that language. To add a language, copy
`en_US.lang` to a new code and translate the values. Any key left out falls back to
English, so a half-finished file is usable while it is being written.

`{}` is a placeholder the host fills in by position. A line whose placeholder count
differs from `en_US.lang` prints `[bad translation]` instead, and their order is not
checked at all, so moving one lands the arguments in the wrong slots.


## What Pier adds to the server

One command, `/pier`, with these subcommands:

| | |
|---|---|
| `/pier list` | The mods Pier has loaded |
| `/pier events` | Every event id the host can resolve, including the synthetic ones |
| `/pier abi` | The ABI version and table length, which is what a compatibility report needs |

`/pier events` is the fastest way to find the id of an event when a subscription is not
firing.

## Installing a mod

A Pier mod is a directory under `mods/` holding a DLL and a `manifest.json`:

```
mods/
  my-mod/
    my_mod.dll
    manifest.json
```

See [The manifest](/guide/manifest) for what goes in that file. The one field worth
checking twice is `"type": "pier"`, because a wrong value means the mod is never scanned
and nothing is reported.
