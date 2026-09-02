# How Pier is designed

Every mechanism here exists because of something in [Why Pier exists](./why). This page is
the C++ side: what the host does and why it is arranged that way. A mod author does not
need it, but a binding author does, and anyone judging whether to build on Pier should
read it.

The rules are in [`CONTRACT.md`](https://github.com/Maskviva/pier/blob/main/CONTRACT.md),
with the reasoning for each.

## The header is the product

`packages/pier-abi/include/sdk/abi.h` is the deliverable. Everything else in the
repository is one implementation of it, and any change that bends the ABI to suit the
implementation is going the wrong way.

That is why it has to parse as **C11**: the consumers are any language, and C is the one
they all read. CI compiles it as C11 and as C++20 on every push, and a check rejects a C++
spelling or a platform-dependent width appearing in it.

## Eight packages, edges pointing one way

```
pier-abi        pure C header, no sources, no dependencies      <- the contract
   ^
pier-support    small shared utilities
   ^
pier-host       the mod lifecycle, the SPI registry, the owner of PierApi
   ^
   |-- pier-api           the core domains
   |-- pier-hooks         events synthesized with native detours
   |-- pier-lane          the same-toolchain fast lane          (optional)
   |-- pier-dimensions    custom dimensions                     (optional)
   +-- pier-client        client-only slots                     (client builds)
```

Arrows point upward only, and that graph is the complete set of edges. Capability packages
neither include nor link one another.

A superfluous edge counts as a violation too. An unused sideways entry in `add_deps`
causes no trouble today while demoting "this edge does not exist" from a structural fact
to a coincidence, and the next person reaching for it is stopped by neither compilation
nor linking. The `pkg-layering` check reads both graphs, the link graph and the include
graph.

## Capabilities register inward

The loader's dispatch table called each package's functions directly, which is what made
an optional package impossible to drop. Pier reverses the edge.

A capability package registers itself with the host through a service provider interface,
at four points:

| What it registers | When the host uses it |
|---|---|
| A slot pack | Filling `PierApi` at load time |
| Teardown steps, with a stage number | At unload, in ascending stage order |
| An unload veto | Asked before an unload is allowed |
| An event provider | While a subscription is being resolved |

The package is absent, the registration does not happen, the slots stay NULL, and
`pier-host` changes by not one line.

**Optional is therefore executable**, and it is defined that way: delete the
`includes(...)` line from the root build file and configuring, compiling and running all
proceed with the matching slots NULL. A check runs that sentence once per package.

Self-registration has one precondition worth naming: every package is compiled as an
object library rather than a static one. A static library lets the linker discard a
translation unit with no external reference, and those registrations are file-level static
objects with exactly no external reference. The symptom of one being discarded is a
capability silently disappearing. That is in the contract, and a check guards it, because
it was violated once after being written down.

## One layout, on every target

`PierApi` contains no conditional compilation anywhere. The client group and the dimension
group always occupy their places and are NULL when the matching package was not compiled
in.

Whether a capability exists is whether its slot is null: a runtime question with a runtime
answer, which a binding reports by name. The version-number marker the loader needed, and
the chain of patches defending it, are all gone, because the layout no longer forks.

The Rust mirror of that header is hand written, and a check compares it to `abi.h`
**parameter by parameter** rather than slot name by slot name. `cargo check` catches `int`
not being a Rust type; it cannot catch the mirror writing `i32` where the header has
`int64_t`, which compiles on both sides and reads half a number at runtime.

## Two gates before every call

A non-core slot is reached only after both:

```c
if (api->struct_size < offsetof(PierApi, slot) + sizeof(void*)) { /* host too old */ }
if (api->slot == NULL)                                          { /* not compiled in */ }
```

Neither may be skipped and the order cannot be reversed. Checking only for non-null reads
out of bounds when the host is older and the table is short, and out-of-bounds memory
often looks like a valid function pointer. Checking only the length calls a null pointer
when the table is long enough but the capability was not built in.

The two failures are also reported differently, because they call for different actions:
beyond the table means upgrade Pier, in the table and empty means this build lacks the
package and upgrading will not help.

## Handles are identities

Nothing crosses the ABI as a pointer into the server. A player is a selector, an actor is
an `ActorUniqueID`, a block is a dimension and a coordinate. Every call resolves again.

A handle can therefore be kept across ticks: once the actor is gone a call returns an
error instead of jumping into freed memory. The cost is a lookup per call, which is the
right trade for a boundary whose other side is code Pier does not control.

One identity rule is worth stating because it is a security property. A player selector by
**name** falls back to the display name when no account name matches, and another mod can
change a display name. A player who sets theirs to the account name of an offline player
would redirect every by-name call onto themselves. Permissions, economy and ownership take
an xuid, and the Rust binding writes that distinction into the type so it is visible at
the call site.

## Buffers belong to whoever made them

Any buffer crossing the boundary is allocated and freed by its producer, and the receiver
copies anything it keeps.

| Shape | Allocated by | The receiver may |
|---|---|---|
| `PierStr`, a pointer and a length | the caller | read it before returning, and copy what it keeps |
| A sink callback | the host | copy inside the sink; the pointer dies on return |
| A lane's `data` and `vtable` | the provider | call while the liveness flag holds |

The ABI never returns a pointer for the other side to free. That needs an allocator
contract across the boundary and the two allocators are not the same. Every output goes
through a sink.

Structured data is SNBT rather than a struct passed by value, so a language that cannot
express the engine's memory layout does not have to.

## Errors are errors

No slot answers a question it could not determine with a plausible value. Three legitimate
ways of not having one: return a value that can express having no answer, log and fall
back while saying what it fell back to, or refuse.

Cannot-be-determined and the answer being no stay apart all the way down. A decision that
may fail to answer does not return a bare boolean, because collapsed into `false` a caller
can only guess and collapsed into `true` it is a security hole.

A log line has to answer what to do about it. "subscribe failed" does not qualify; the
event not existing on this BDS version, with the nearby ids from the registry, does.

## Events resolve exactly

One matcher, `idMatches`, on an exact id or a suffix carrying the namespace separator.
Never a substring. Two matchers of differing strictness eventually give two answers.

Resolution order, and claiming means owning:

1. An event provider claims it. A failed subscription then reports an error rather than
   falling through to another path.
2. An exact registry name, then a unique suffix.
3. Everything failing reports an error and lists the nearby ids.

A provider comes before the registry because the emitter of a command event is *in* the
registry while LeviLamina dispatches it only to typed listeners: the dynamic path finds it
and cannot receive it. Such a provider declares that it covers the registry path. A purely
synthetic event declares that it does not, and warns the day an id with the same suffix
appears upstream, because that shadowing has to be visible.

## The contract only grows

Adding a capability appends a slot at the end and leaves the version alone. Reordering,
removing or changing a signature advances `PIER_ABI_VERSION` and `PIER_ABI_MIN_SUPPORTED`
together to the same number, and every mod built before it is refused at load with a
message saying so.

Compatibility is a range: `MIN_SUPPORTED <= a mod's version <= the host's`.

`tools/abi-v1.slots` is the baseline and a check compares against it on every push, so a
non-append change cannot land quietly.

## Every rule has a script

A rule with no script guarding it is a wish, and this is not a figure of speech: the
object-library rule was violated after it was written into the contract and after a
delivery note claimed it held, in exactly the four packages that depend on
self-registration.

`python3 tools/run-checks.py` runs the lot. They find what neither the compiler nor clippy
can:

| | |
|---|---|
| `abi-c-parse` | the header compiles as C11 and as C++20 |
| `abi-additive` | the slot order only ever grew, against the baseline |
| `abi-fixed-width` | no platform-dependent width in the contract |
| `abi-no-lang` | no consumer language's spelling or types in the header |
| `pkg-layering` | both graphs follow the package diagram |
| `object-kind` | every package is an object library |
| `optional-drops` | no optional package's symbols are referenced across packages |
| `sys-mirrors-abi` | the Rust mirror matches parameter by parameter |
| `manifest-matches-host` | an example manifest really loads |
| `host-loadable` | the memory operators and mod registration reach the artifact |
| `comment-claims` | a comment claiming an exception is caught has a catch beside it |
| `no-silent-fallback` | no catch block that drops an exception without a trace |

The three at the end of that list share a shape worth naming: what they watch is
**invisible at compile time and reported at load time, in words that have nothing to do
with the build**. A mod with a wrong manifest type is never scanned and reports nothing.
A build with no memory operators links fine and is refused when the server starts.

Each check states what it covers and what it cannot see, and a delivery note may copy that
sentence and nothing stronger. A passing script means the mechanical rule holds, not that
the property does.
