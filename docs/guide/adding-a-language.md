# Adding a language

One file is enough: `packages/pier-abi/include/sdk/abi.h`. It parses as C11, so an FFI
tool takes it directly. Nothing else in this repository is a dependency of a binding.

The reference implementation is `bindings/rust/pier-sys-rs`, the first official binding.
Read the four steps, then read that crate; do not copy it, because the contract is the
header and not any one binding.

## The four steps

### 1. Read `sdk/abi.h`, and only that

The header is the product. Everything else in the repository is one implementation of it.

### 2. Mirror `PierApi`, unconditionally

Declare **every** field, with no target branch anywhere. The layout is identical on every
build target, and an absent capability is a NULL slot rather than a missing field.

This is the one rule whose violation is worst. A conditional field set made the tail
offsets drift per target once, every slot after the fork landed on the wrong function
pointer, and both sides compiled. Do not reintroduce it.

### 3. Export `pier_main`

```c
bool pier_main(const PierApi* api, PierModHandle self, PierModVTable* out);
```

Fill `out` with `struct_size`, `abi_version`, `mod_flags` and the three lifecycle
callbacks. The host looks for that one symbol and refuses to load with no fallback when
it is absent.

`struct_size` is the length **your** side compiled, and the host reads only fields within
it. That is what lets the vtable gain callbacks later without an ABI version bump.

### 4. Gate every non-core call

Two gates, and the order cannot be reversed:

```c
/* 1: is the table long enough to reach this field? */
if (api->struct_size < offsetof(PierApi, some_slot) + sizeof(void*)) { ... }
/* 2: is the slot non-null? */
if (api->some_slot == NULL) { ... }
```

Checking only for non-null reads out of bounds when the host is older and the table is
short, and what comes back often looks like a valid function pointer. Checking only the
length calls a null pointer when the table is long enough but the capability package was
not compiled in.

## Three rules that are not optional

**A buffer crossing the boundary is allocated and freed by whoever produced it.** The
receiver reads it during the callback and copies anything it keeps. The ABI never returns
a pointer for the other side to free, because that needs an allocator contract and the
two allocators are not the same. Every output goes through a sink.

**Every slot is server-thread only unless the header says otherwise.** The exceptions are
noted per slot.

**A panic or exception must not cross back.** Whatever your language calls unwinding,
stop it at the boundary.

## The handshake

The host checks three things in this order, and your side should check the same three so
that a failure can be explained from either log:

1. **Length.** With a table too short to cover the core slots, the other two cannot even
   be read.
2. **Version range.** `MIN_SUPPORTED <= your abi_version <= the host's`. Compatibility is
   a range, not an equality.
3. **Target flags.** Bit 0 of `mod_flags` and `host_flags` must be equal. A mismatch is
   refused explicitly rather than being allowed to crash later on the first slot that
   exists on one side only.

## Where a binding lives

`bindings/<language>/`, and not in the unconditional include list of the main repository.
What Pier maintains is the contract.

Section 10 of [`CONTRACT.md`](https://github.com/Maskviva/pier/blob/main/CONTRACT.md) is
the authoritative version of this page.
