# 📦 Installation

This page takes you through putting Pier on a server and installing your first mod. It takes
a few minutes.

## What you need

| | Version |
|---|---|
| Server | Bedrock Dedicated Server (BDS) 1.26.51 |
| Loader | LeviLamina 26.51.5 |
| Optional | [LegacyMoney](https://github.com/LiteLDev/LegacyMoney), only for the economy functions |

!!! tip "LegacyMoney really is optional"

    Without LegacyMoney the server starts as usual. The economy APIs then return an "unavailable"
    error, and everything else is unaffected.

## Step one: install Pier

### With lip (recommended)

[lip](https://lip.futrime.com) is LeviLamina's package manager. One command does it:

```bash
lip install github.com/Maskviva/Pier
```

### By hand

1. Download `pier-server-windows-x64.zip` from the
   [releases page](https://github.com/Maskviva/Pier/releases);
2. Unpack it into the server's `plugins/` folder.

The archive holds one `Pier` folder, so you end up with `plugins/Pier/`, holding
`manifest.json`, `Pier.dll` and `lang/`.

!!! warning "The folder must be named Pier, with a capital P"

    LeviLamina compares the folder name with the manifest's name letter by letter. A folder named
    `pier` gets `Mod name Pier do not match folder pier` in the log, and Pier never loads.

    Windows does not rename an existing `pier` to `Pier` for you: delete the old folder first.

## Step two: check that it worked

Start the server. Once Pier is ready, the log has this line:

```
[host] ready, ABI v2, api table 1600 bytes
```

`ABI v2` is what to look for. The byte count grows as Pier gains functions; no need to
check it.

Then type in the server console:

```
/pier list
```

It lists the mods Pier loaded. With no mod installed yet, it is empty.

## Step three: install a mod

A Pier mod is, like Pier itself, a folder under `plugins/`, holding the mod's DLL and its
`manifest.json`:

```
plugins/
  Pier/
  my-mod/
    my_mod.dll
    manifest.json
```

Restart the server and `/pier list` shows it.

!!! warning "The manifest's type must be “pier”"

    Pier takes a mod over only when its `manifest.json` says `"type": "pier"`. Get it wrong and
    the mod does not load, **with no message at all**. Every field of `manifest.json` is
    explained in [The manifest](manifest.md).

## The commands Pier adds

Pier gives the server a `/pier` command:

| Command | What it does |
|---|---|
| `/pier list` | Lists the mods Pier loaded |
| `/pier events` | Lists every event name you can subscribe to, Pier's own included |
| `/pier abi` | Shows the ABI version and table size, useful in a compatibility report |

!!! tip "Subscribed to an event and nothing happens?"

    Check the event name with `/pier events` first; it is the quickest way to find the problem.

## The language of the log

Pier's log speaks several languages; the translations are in `plugins/Pier/lang/`. The release
ships `en_US.lang` and `zh_CN.lang`, so English and Chinese servers need nothing.

!!! warning "Copy the whole folder when installing"

    Copy only `Pier.dll` and leave `lang/` behind, and a Chinese server logs in English. English
    is the built-in fallback, so nothing breaks, and nothing tells you why either.

Which language is used comes from `language` in `config.json`. The default, `"auto"`,
follows the game's language. To fix one, write it:

```json
{ "language": "zh_CN" }
```

Both `zh_CN` and `zh-CN` work. More settings are in [Configuring](configuration.md).

### Editing a translation

- **To change one line**: change that line's value in the language's file;
- **To add a language**: copy `en_US.lang` to the new language code and translate it. Lines
  left untranslated fall back to English, so a half-done file works;
- `{}` in a translation is a placeholder that Pier fills in order. **The count must match the
  line in `en_US.lang`**, or the line shows as `[bad translation]`. The order is not checked:
  swapping two puts the values in the wrong places.
