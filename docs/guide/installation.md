# Installation

## Requirements

| | |
|---|---|
| Server | Bedrock Dedicated Server 1.26.20 |
| Loader | LeviLamina 26.20.4 |
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
[host] ready, ABI v1, api table 1560 bytes
```

Then, in the console:

```
/pier list
```

That lists the mods Pier has loaded. It is empty until you install one.

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
