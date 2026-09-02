# The Pier code contract (v2)

This document defines the rules and does not describe the present state; where the two
conflict, the present state changes. Its reader is anyone about to add something to this
repository, including yourself six months from now. Every item states why, because a rule
that only states what gets worked around the first time it is inconvenient.

The substantive change of v2 over v1 is one thing, and it rewrote sections 1 and 2: the
dependency graph v1 claimed did not match the real include and link graph, since the host
linked symbols of api, hooks included headers of api, and an optional package could not be
dropped. A rule cannot describe a graph that does not exist, so v2 made the graph true and
gave optionality and decoupling a mechanism each rather than a claim.

---

## 0. In one sentence

> **`pier-abi` is the product and everything else is one implementation of it.**

What Pier sells is not a loader, it is that C ABI. Any change that bends the ABI toward an
implementation is going the wrong way. It follows that `sdk/abi.h` has to be parseable by a
C11 compiler: the consumers of the contract are any language, and C is the one they all
read. CI compiles it once as C11 and once as C++20.

---

## 1. Packages and dependencies

```
pier-abi          a pure C header, no source file, no dependency        <- the contract
   ^
pier-support      small cross-package utilities (PierStr and sv, SNBT escaping, the logging entry)
   ^
pier-host         the mod lifecycle, the SPI registry, and the owner of the PierApi table
   ^
   |-- pier-api           the core domain implementation (core / runtime / actors / world / net)
   |-- pier-hooks         events synthesized with native detours       (capability package)
   |-- pier-lane          the same-toolchain fast lane                 (capability package, optional)
   |-- pier-dimensions    custom dimensions                            (capability package, optional, server)
   +-- pier-client        client-only slots                            (capability package, client build only)

pier-sys-rs -> pier-rs                                                  <- the binding of one language
```

**These are two independent build lines.** The eight packages above are compiled by the root
`xmake.lua` into the host itself, `pier.dll`, and the two crates below by the root
`Cargo.toml` into the binding. There is no build dependency between the lines, only a
contract dependency: a binding reads the single header `sdk/abi.h`, through a hand-written
mirror, and the `sys-mirrors-abi` check guards the agreement.

The consequence is practical: the host failing to build and the binding failing to build
are two things that can be fixed separately, and that is how the sentence of §0 lands. Every
directory under `packages/` has to belong to one of the lines, having either an `xmake.lua`
or a `Cargo.toml`, and one that belongs to neither is reported by `pkg-layering`, since a
directory no set of criteria covers is a directory no rule governs.

**Rule 1: arrows point upward only, and this graph is the complete set of edges.**
Capability packages neither include nor link one another; they know only `pier-abi`,
`pier-support` and the SPI of `pier-host`. `pier-api` is one of the capability packages and
holds no privilege: its size is history and not a level.

A superfluous edge and a missing edge are equally a violation. A sideways edge sitting in
`add_deps` that nothing currently uses, as when hooks and dimensions both declared a
dependency on pier-api while neither package held one include or one symbol reference,
causes no trouble today while demoting "this edge does not exist" from a structural fact to
a coincidence: the next person reaching for it is stopped by neither compilation nor
linking. The `pkg-layering` check reads both graphs, the link graph of `add_deps` and the
compile graph of `#include`.

**Rule 2: sideways collaboration always goes through the host SPI in `pier/host/spi.h`, and
the direction is always a capability package registering into the host with the host calling
back at the right moment.** Four registration faces:

| SPI | Who registers | When the host uses it | What it replaced in v1 |
|---|---|---|---|
| the slot pack `SlotPack` | every capability package | filling `PierApi` at load time | ApiTable calling into each package directly |
| the teardown step `Teardown(stage)` | packages holding mod resources | at unload, in ascending stage order | a hand-written list in `onHostedModGone` |
| the unload veto `UnloadVeto` | lane and others | asked one by one before `unload` | a hardcoded forward through `laneBusyName` |
| the event provider `EventProvider` | hooks, and the command events of api | while `subscribe_event` resolves | substring matching plus a hardcoded short circuit |

Why registration and not a direct call: every direct call is a link edge running from the
center out to a capability package, which is what made an optional package impossible to
drop in v1. Registration reverses the edge: with the package absent the registration does
not happen, the slot stays NULL, and the host changes not one line.

**Rule 3: optional means that after deleting that one `includes(...)` line from the root
`xmake.lua`, configuring, compiling and running all proceed as usual with the matching slots
NULL.** That is the executable criterion of optionality, and `optional-drops` runs that
sentence once per package.

**Rule 4: every package is `set_kind("object")`**, except `pier-abi`, which has no source
file and is `headeronly`. A static library lets the linker discard a translation unit with
no external reference, while the SPI registration of hooks, dimensions, lane and client
rests entirely on file-level static objects registering themselves, and the symptom of one
being discarded is a capability disappearing silently. Object makes every TU reach the final
artifact. This is not a style preference, it is a precondition of the self-registration
pattern, which is why it is in the contract.

This rule was violated once, after it was already in the contract and after a delivery note
claimed every package was object, while four packages were still `static` and were exactly
the four that depend entirely on self-registration, 22 TUs. The lesson is not to be more
careful, it is that a rule with no script guarding it is a wish. The `object-kind` check was
written for this rule and guards it now.

**Rule 5: no language name appears inside `pier-abi`, and neither does a type of any one
language.** No Rust, Go or Zig, and no `std::string_view` or `enum class` either. The lesson
of v1: making `PierStr` an alias for `std::string_view` had a C ABI depend on the layout
details of the MSVC standard library and needed a runtime self-check to atone for it.
`PierStr` is now an explicit `{ptr, len}` and the self-check is deleted, because a layout
defined by a declaration does not need the declaration verified.

---

## 2. ABI evolution

The ABI starts at **v1**, and v1 made three structural decisions:

**2.1 The layout is identical on every build target, and an absent capability is a NULL
slot.** There is no conditional compilation anywhere in `PierApi`. The client group,
`client_*`, and the dimension group, `md_*`, always occupy their places and are NULL when
the capability package was not built into the host. Whether it exists is whether it is NULL,
and the SDK reports that the host does not provide X on that basis. Why: before v1 the
conditional blocks made the tail offsets drift per target, a marker in the high bits of the
version number was added to compensate, and that marker protected no mod that was not
rebuilt, which is a whole chain of patches whose root cause was only a forked layout.
Without the fork, no link of that chain needs to exist.

An optional external dependency, meaning not a capability package but a symbol another
plugin exports, such as the LegacyMoney economy backend, follows the same rule: the host has
to come up as usual with the feature degrading to failure values. That needs both halves,
the runtime availability guard and the `/DELAYLOAD` of the linker. With only the first, the
import library is still linked in statically, the loader fails while loading the host
itself, reports `0x7E, the specified module could not be found`, and not one line of the
runtime guard runs. Whether the two agree is guarded by the `delayload-matches-claims`
check.

**2.2 Append only: no reordering and no deletion.** Adding a capability means appending at
the end of the table with the version unchanged, and the SDK compares `struct_size` per slot
through `require_slot!`. Changing a signature, deleting or reordering means `PIER_ABI_VERSION`
and `PIER_ABI_MIN_SUPPORTED` both advancing to the same number. Compatibility is a range and
not an equality: `MIN_SUPPORTED <= mod_abi <= VERSION`.

**2.3 Both sides of the handshake carry `struct_size`.** From v1 `PierModVTable` carries its
own `struct_size`, `abi_version` and `mod_flags`, and the host reads only the fields within
the length the mod declared, so the vtable can also gain lifecycle callbacks without a
version bump. Target matching compares bit 0 of `mod_flags` against `host_flags`, refusing
explicitly and saying why, rather than hiding a marker in the high bits of a version number.

**2.4 There is one entry symbol, `pier_main`.** Its absence refuses the load explicitly with
no fallback of any kind.

---

## 3. Ownership across the boundary

**Rule: any buffer crossing the boundary is allocated by the producer and freed by the
producer.** The receiver reads only during the callback and copies anything it keeps. It
follows that the ABI never returns a pointer the other side has to free, which would need an
allocator contract across the boundary while the two allocators are not the same. Every
output goes through a sink.

| Shape | Who allocates | What the receiver may do |
|---|---|---|
| `PierStr`, a pointer and a length | the caller | read it before the callback returns, and copy anything kept |
| a sink callback | the host | copy it inside the sink; the pointer dies on return |
| the `data` and `vtable` of a lane | the provider | call only while the liveness flag is true |

---

## 4. Threads

**Rule: every slot of the ABI may be called on the server thread only by default, and an
exception is noted per slot.** The host therefore needs no lock internally. Work needing
another thread goes back to the server thread through `schedule_for`. Every callback, for
events, commands and scheduled tasks, fires on the server thread, and the client group fires
on the client thread.

---

## 5. Error handling

This section was traced back from an incident in a downstream consumer, and v2 keeps it as
it was and adds one item.

**5.1 No silent fallback.** Three legitimate ways of not having a value: return a value able
to express that there is no answer; log and then fall back while saying what it fell back
to; or refuse to act. A real counterexample: an event payload could not be read for `dim`,
the consumer wrote `unwrap_or(0)`, every event inside a custom dimension was treated as
happening in the overworld, land protection refused in the overworld and allowed everywhere
else, and nothing was logged.

**5.2 Cannot-be-determined and the answer being no must stay apart.** A decision function
that may fail to answer cannot have a bare `bool` return type. Collapsed into the same
`false` a caller can only guess, and collapsed into `true` it is a security hole.

**5.3 A log line has to answer what to do about it.** A "subscribe failed" does not qualify;
an event not existing on this BDS version, with the nearby ids A, B and C from the registry,
does.  A success path logs nothing at info level.

**5.4 A comment must not lie about the code (new).** v1 had three comments whose claimed
behavior was not in the code: an exception said to be swallowed with no try, a match said to
be on a cancel flag while the implementation matched something else, and a tail said to be
at the same offset while another place in the same file proved otherwise. **A comment
claiming a safety property has to sit next to the code implementing that property**, and
where that is impossible the comment changes into the fact. A lying comment is more
dangerous than no comment, because the next person reasons from it.

---

## 6. Event routing

**Rule: a synthetic event, from hooks or the command events, matches on an exact id or on a
suffix carrying the namespace separator, and never on a substring.** There is one decision
function, `spi::idMatches`, because two matchers of differing strictness eventually give two
answers. The resolution order:

1. an event provider claims it through `idMatches`, and claiming means owning it: a failed
   subscription reports an error and does not fall through to another path;
2. an exact registry name, then a unique suffix. Several entries under one name are not an
   ambiguity, since that is only several mods each registering an emitter;
3. everything failing reports an error and lists the nearby ids.

Why a provider comes before the registry: the emitter of a command event is in the registry
while LL dispatches it only to typed listeners, so the dynamic path finds it and cannot
receive it. Such a provider declares `covers_registry = true`, where replacing the registry
path is the fix and warns about nothing. A purely synthetic event from hooks declares
`false`, and once an id with the same suffix appears in the registry it warns, because that
means upstream introduced a real event whose name collides and the shadowing has to be
visible. Why no substring: the `find(name)` of v1 means that once LeviLamina upstream adds
an event containing the same stem, a subscriber is silently hijacked onto a synthetic event
with an entirely different payload shape and cannot reach the real one.

---

## 7. Comments and naming

**The comment standard is `COMMENTS.md`, which expands this section, and where the two
conflict it governs.** In one sentence: a comment only writes what the code cannot answer,
meaning constraints, counter-intuitive facts, danger, contract, and a rejected obvious
approach. A bug fix writes the symptom, since a symptom can be searched for and a conclusion
cannot, and it does not write the sequence of events, because the history of the code is in
git. The budgets are at most 16 lines for an L1 file header, 14 for an L2 declaration and 8
for an L3 body comment, with `pier-abi` the one exception, being product documentation. The
check is `tools/checks/comment_style.py`.

Naming: an ABI type is `Pier` plus Pascal case, a macro is `PIER_` plus upper case, an entry
point is `pier_` plus snake case, the namespace is `pier`, and a C++ type carries no language
name. A Rust package is named `pier-*-rs` while the crate names stay `levilamina` and
`levilamina_sys`: the package name says which ABI it belongs to and the crate name follows
the mental model of the caller. **A user-visible string is bound by the same ban on language
names**: the command is `/pier`, the mod type is `"pier"`, and an error speaks of the pier
host. The `/llr` command, the "rust" manager and the update line naming a historical product
were all removed on this basis.

## 8. Files and directories

One concern per TU. Each `.cpp` under `pier-hooks/src/` is one event, self-registering
through the SPI, so adding or removing an event changes no table. A file name matches its
contents, and a directory name states a responsibility and not a technology.

## 9. Machine checks

Only what the compiler and clippy cannot find is worth a script. They all live in
`tools/checks/` and `python3 tools/run-checks.py` runs them together.

**Scripts come first**: a property gets no checkmark in a delivery note before a script
guards it. v2 adds one equally important item: **a passing script only earns a checkmark for
what it covers.** The output of each check states what it covers and what it cannot see, and
a delivery note copies that sentence rather than turning "the static necessary condition
passed" into "this property holds".

| Check | What it watches | Coverage boundary |
|---|---|---|
| `abi-c-parse` | `sdk/abi.h` compiles under both `gcc -std=c11` and `g++ -std=c++20` | complete |
| `abi-additive` | append only against the baseline `tools/abi-v1.slots`; a non-append change advances both version numbers together | complete |
| `abi-no-lang` | `pier-abi/` comments carry no consumer-language spelling and declarations no C++ type; a user-visible string carries no historical product name | complete |
| `abi-fixed-width` | the contract carries no type whose width depends on the platform, such as `int`, `long` or `unsigned short` | complete |
| `pkg-layering` | `add_deps`, the link graph, and `#include`, the compile graph, both follow the graph of section 1; capability packages have no sideways edge | complete |
| `object-kind` | the xmake of every package is `set_kind("object")`, except `pier-abi`, which is headeronly | complete |
| `optional-drops` | no symbol of an optional package is referenced across packages | **the necessary condition only**; the sufficient criterion is really deleting that line and running `xmake f` |
| `build-config` | target names are unique; `add_packages` is contained in `add_requires`; using a header means declaring its package | packages that ship headers; a link-time-only package is invisible |
| `include-resolves` | every internal `#include` resolves character for character to a real file | complete |
| `sys-mirrors-abi` | the slot order, **each slot signature parameter by parameter**, the struct fields, the constants and the enum members all match `abi.h` cell for cell; the mirror carries no conditional compilation | complete, and stricter than `cargo check`, since a wrong width compiles on both sides |
| `comment-claims` | a comment claiming something is swallowed, caught or never thrown has a try or a catch in the same function | an exception word has to appear as well, to avoid confusion with the swallow of packet-dropping |
| `comment-style` | the mechanical part of `COMMENTS.md`: budgets, banned wording, ticket numbers, markdown layout, line width, the language of the contract header | it cannot see whether a comment is true or restates the code, and those need a human |
| `manifest-matches-host` | the type, the dependency name and the entry of an example `manifest.json` really load under the host | complete |
| `host-loadable` | the unified memory operators and the mod registration appear exactly once each, and their package necessarily reaches the artifact | complete |
| `ledger-covers-tree` | every file in the workspace has a row in the ledger, the reverse direction of the ledger | complete |
| `no-silent-fallback` | a `catch` block that neither logs, rethrows, returns, resets the target, nor puts it into an error value | **that one shape only**; a default filled in across functions is invisible |

The last four are new in v2. `build-config`, `include-resolves` and `manifest-matches-host`
were added for the same reason: the errors they watch never surface on a development
machine, since Windows is case insensitive, xmake was never run, and a mod that cannot load
reports nothing and simply does not appear, which is the literal meaning of what the compiler
cannot find.

`manifest-matches-host` was pushed out by a real incident: the example manifest of v0
depended on a mod name that no longer existed after the rename, so the example could not
load, with no error at all and merely an absence from `/pier list`.

`host-loadable` likewise: `MemoryOperators.cpp` was never written, all 98 TUs compiled,
`pier.dll` linked, the mod packaged, and only at load time did LeviLamina say the unified
memory allocation operators were not used. The common shape of this family is that it is
entirely invisible at compile time, is reported at load time, and what is reported has
nothing to do with the build.

**The surrogates are not in this table**, and `python3 tools/run-surrogates.py` runs them
together: `build-prereqs`, `typed-storage` and `ledger-count`, standing in respectively for
the build preconditions, the `TypedStorage` collapse rules and counting by hand. The
surrogates that duplicated the compiler, the linker and clippy were removed once a toolchain
existed, since what they find the compiler reports anyway, which under the criterion of §9,
that only what the compiler and clippy cannot find is worth a script, keeps them out of the
contract.

They were filled in after the first compile on a real machine. Of the four classes of error
that compile reported, a missing include, a leftover C type name, a dangling doc comment and
a bare `int` in the contract, only the last entered §9, because only it is the kind neither
compiler reports and that surfaces once a second language gets involved. The other three
became surrogates. That dividing line is worth recording: **the criterion is not how serious
the error is, it is whether something else already guards it.**

## 10. Adding a language

1. Read `packages/pier-abi/include/sdk/abi.h` and nothing else; it is C and your FFI tooling
   takes it directly.
2. Declare the `PierApi` mirror, declaring every field unconditionally, with no target branch
   anywhere.
3. Export `pier_main(const PierApi*, PierModHandle, PierModVTable*) -> bool` and fill the
   vtable with `struct_size`, `abi_version`, `mod_flags` and the three lifecycle callbacks.
4. Before calling a non-core slot, check that `struct_size` covers it and that the slot is
   not NULL.

`bindings/rust/pier-sys-rs` is the reference implementation of those four steps. A binding
for a new language goes in `bindings/<language>/` and not in the unconditional include list
of the main repository; what Pier maintains is the contract.
