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

## Installing a terrain pack

A mod that creates a custom dimension from a pack ships the pack with itself: a directory
holding a config file and a binary, anywhere under the server root, named to the host by a
relative path. The host reads four keys of the config and verifies the binary against the
hash in it before the dimension is registered, so a pack that was replaced or truncated
refuses at startup with a line naming the file rather than generating something else.
See [Terrain packs](/guide/terrain-packs).
