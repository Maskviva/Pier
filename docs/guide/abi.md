# The ABI

The contract is one file:
[`packages/pier-abi/include/sdk/abi.h`](https://github.com/Maskviva/pier/blob/main/packages/pier-abi/include/sdk/abi.h).

It parses as C11 and as C++20, and CI compiles it both ways on every push. It is also the
reference documentation: every slot carries what its parameters mean, what it returns on
failure, and what it requires of threads. A binding author in any language has nothing
else to read, which is why that header is exempt from the comment budgets the rest of the
repository follows.

This page covers the shape. Read the header for the slots.

## The shape

```c
typedef struct PierApi {
    uint32_t struct_size;     /* the length THIS host compiled */
    uint32_t abi_version;
    uint32_t host_flags;      /* bit 0: this is a client build */
    uint32_t reserved;

    /* ... every slot, as an optional function pointer ... */
} PierApi;
```

Three properties follow from that header:

**The layout is identical on every build target.** There is no conditional compilation
anywhere in `PierApi`. The client group and the dimension group always occupy their
places and are NULL when the matching package was not built in.

**An absent capability is a NULL slot.** Whether a capability exists is whether its slot
is null, which is a runtime question with a runtime answer.

**`struct_size` is the whole basis of forward compatibility.** A host reports the length
it compiled, and a mod reads only what fits within it.

## The two gates

Before calling any non-core slot:

```c
if (api->struct_size < offsetof(PierApi, slot) + sizeof(void*)) { /* too old */ }
if (api->slot == NULL)                                          { /* not built in */ }
```

Neither may be skipped and the order cannot be reversed. Checking only for non-null reads
out of bounds when the table is short, and what comes back often looks like a valid
function pointer. Checking only the length calls a null pointer when the capability was
not compiled in.

In Rust the `require_slot!` macro does both and produces an error naming which one
failed.

## Ownership

Any buffer crossing the boundary is allocated and freed by whoever produced it. The
receiver reads it during the callback and copies anything it keeps.

| Shape | Allocated by | The receiver may |
|---|---|---|
| `PierStr` (pointer and length) | the caller | read it before returning, and copy what it keeps |
| A sink callback | the host | copy inside the sink; the pointer dies on return |
| A lane's `data` and `vtable` | the provider | call only while the liveness flag is true |

The ABI never returns a pointer for the other side to free. That would need an allocator
contract across the boundary, and the two allocators are not the same. Every output goes
through a sink.

## Structured data is SNBT

There is no struct passed by value on this ABI. Event payloads, forms, items, block
states, actor snapshots, command arguments and service requests are all SNBT strings.

The Rust SDK parses them for you through `NbtValue`. A binding in another language either
writes an SNBT parser or goes through `nbt_snbt_to_binary` and uses an existing NBT
library.

## Threads

Every slot is server-thread only by default, and each exception is noted on the slot.
`schedule_for`, `log` and the `kvdb_*` family are the ones marked thread safe.

Packet interception callbacks are the exception in the other direction: they do not run
on the server thread at all.

## Evolution

Append only. Adding a capability appends a slot at the end and leaves the version alone.
Reordering, deleting or changing a signature advances `PIER_ABI_VERSION` and
`PIER_ABI_MIN_SUPPORTED` together to the same number.

`tools/abi-v1.slots` is the baseline, and the `abi-additive` check compares against it on
every push, so a non-append change cannot land quietly.
