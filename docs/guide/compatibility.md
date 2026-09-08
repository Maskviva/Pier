# Compatibility

## Versions

A Pier release is numbered `<BDS major>.<BDS minor>.<release>`, so the first two parts
name the BDS the release was built for and the third counts releases against it.

| Pier | BDS | LeviLamina |
|---|---|---|
| 26.40.0 | 1.26.40 | 26.40.0 |
| 26.32.2 | 1.26.32 | 26.32.1 |
| 26.32.1 | 1.26.32 | 26.32.1 |
| 26.20.2 | 1.26.20 | 26.20.4 |
| 26.20.1 | 1.26.20 | 26.20.4 |

A row is not a range. Each release is built against the one BDS its number names and does
not run on the others: Mojang's 26.32 build inlined away about a third of the functions
LeviLamina could declare in 26.20, so a 26.32 host calls things a 26.20 engine does not
have and the reverse. Match the row, do not interpolate between them.

## The ABI is separate from the release

The ABI has its own version, currently **v2**, and it moves far more slowly than the
release number. Adding a capability appends a slot and does not move it.

That is what makes a mod built against an older Pier keep working:

- A **new capability** is a slot appended at the end of the table. Your mod does not
  reach it and does not care.
- A **removed or reordered** slot advances `PIER_ABI_VERSION`, and 26.32.2 did that once:
  the four slots retired in 26.20.3 were deleted, so `PIER_ABI_VERSION` and
  `PIER_ABI_MIN_SUPPORTED` both moved to 2 together. A mod built against v1 is refused
  explicitly at load rather than being allowed to crash later, and needs a rebuild
  against the v2 SDK.

Compatibility is a range, not an equality:
`PIER_ABI_MIN_SUPPORTED <= your abi_version <= the host's PIER_ABI_VERSION`.

## What a mod sees

```rust
let (abi_version, table_bytes) = ctx.host_abi();
```

Include both in a bug report. A "your pier is too old" message is only actionable with
them.

The SDK checks every non-core call for you and turns a missing capability into one
specific error naming the slot. Two things are kept apart:

- **The slot is beyond the table.** The host predates that capability. Upgrade Pier.
- **The slot is in the table and empty.** The capability package was not compiled into
  this build. Upgrading will not help; a different build will.

## Client and server builds

One source builds for both. The client-only slots exist on a server host and are simply
empty, so calling one returns an error rather than failing to compile.

Which target a mod is for is declared in `mod_flags`, and the host compares bit 0 during
the handshake. A mismatch is refused explicitly and says so. Letting it load and then
crashing on the first slot that exists on one side only cannot be traced.

## Optional external dependencies

[LegacyMoney](https://github.com/LiteLDev/LegacyMoney) is delay-loaded. A server without
it starts normally, the `money` calls return failure values, and everything else is
unaffected. The `money` module documents which value each call returns.

## What is not promised

The **ABI** only ever gains slots. The **Rust SDK surface** may change between Pier
releases: a method can be renamed or a signature tightened when the old one made a
mistake easy. Pin the tag you build against, and read the release notes before moving.
