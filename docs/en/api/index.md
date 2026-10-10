# API reference

Pier's API is listed binding by binding.

| Binding | Brought in with | Functions and methods | Types | Constants |
|---|---|---|---|---|
| [Rust](rust/index.md) | `use levilamina::...` | 792 | 117 | 76 |
| [Go](go/index.md) | `import "github.com/Maskviva/pier/bindings/go/levilamina"` | 628 | 62 | 316 |
| [Zig](zig/index.md) | `@import("levilamina")` | 305 | 23 | 3 |
| [C++](cpp/index.md) | `#include "sdk/abi.h"` | 207 | 42 | 323 |

The C++ count is the 201 slots of abi.h plus 6 macros.

## Reading an entry

- **Heading**: the interface's name. A method is written `Type::method` (Rust) or `Type.method` (Go, Zig).
- **Declaration**: the signature as the source writes it.
- **Description**: the doc comment above it in the source.
- **Parameters** and **return type**: taken apart from the signature.
- **Slots**: the slots of abi.h the interface ends up calling, each linking to the slot's entry on the C++ pages, which says what the host does with the call.
- **Deprecated**: a deprecated interface has a box under its heading saying since which version and what to use instead.

The Slots column lists the abi.h slots this interface reaches in the source: the ones the body names, and the ones reached through another method of the same type, a call qualified by a type name or a function of the same file, up to four calls deep. A method called on a variable of unknown type is not listed, so the slots behind it are missing from the column. Each slot that is listed has a call path in the source.
## By task

To see how something is done first, such as subscribing to an event, registering a command or reading a player property, see [Common tasks](../tasks/index.md) under Tutorials, where each task has an example in three languages.
