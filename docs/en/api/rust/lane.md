# levilamina::lane

The same-toolchain fast lane: a direct function-table call that bypasses JSON.

The division with [`crate::service`](service.md): service is a cross-language `(name, JSON) -> JSON`
channel that always holds, while a lane passes raw data and vtable pointers and holds
only when both sides were built by the same toolchain. A fingerprint mismatch yields no
pointer and falls back to service.

The fingerprint has to be computed by the caller and `0` is invalid: this slot hands
the vtable over as is, a consumer interprets it through its own type layout, and
skipping the check is type confusion. A `0` would read as anyone may connect.
[`list`](dimensions.md#fn.list) shows which lanes exist and passes no pointer at all.

The discipline for each call after acquiring one is in [`Lane::with`](lane.md#Lane.with).

## Functions {#functions}

### `lane::acquire` {#fn.acquire}

```rust
pub fn acquire<C: LaneContract>() -> std::result::Result<Lane<C>, LaneError>
```

- Return type: `std::result::Result<Lane<C>, LaneError>`
- Slots: [`lane_acquire`](../cpp/crossmod.md#lane_acquire), [`lane_acquire`](../cpp/crossmod.md#lane_acquire)

### `lane::guard` {#fn.guard}

```rust
pub fn guard<R>(fallback: R, f: impl FnOnce() -> R) -> R
```

Catches a panic inside a lane callback.

The table functions of a publisher are all `extern "C"`, and a panic crossing an
`extern "C"` boundary is undefined behavior.
This belongs on the first line of every table function. What runs inside is this mod's
own business code, no less likely to panic than anywhere else, while the consequence
here is far worse: the caller is another mod and its frames are on the stack.

On a panic it returns `fallback` and logs. `fallback` must be the value this table
function uses for cannot-answer and not for no: treating a panic as a definite negative
answer lets a bug make a decision that belongs to business logic.

- Parameters:
    - fallback : `R`
    - f : `impl FnOnce() -> R`
- Return type: `R`

### `lane::publish` {#fn.publish}

```rust
pub unsafe fn publish<C: LaneContract>(
    data: *mut c_void,
    vtable: *const C::Table,
    retain: Option<sys::PierLaneRefFn>,
    release: Option<sys::PierLaneRefFn>,
) -> Result<Publication>
```

Publishes a lane.

**Safety**

The caller must guarantee that:

* `data` and `vtable` stay valid until this lane is withdrawn. The host interprets not
  one byte of them and copies nothing, and only compares for equality and marks
  liveness;
* `vtable` really points at the C-layout table `C::Table` and its shape agrees with
  `C::FINGERPRINT`;
* `retain` and `release` call back into no `lane_*` slot, which would self-deadlock.

- Parameters:
    - data : `*mut c_void`
    - vtable : `*const C::Table`
    - retain : `Option<sys::PierLaneRefFn>`
    - release : `Option<sys::PierLaneRefFn>`
- Return type: `Result<Publication>`
- Slots: [`lane_publish`](../cpp/crossmod.md#lane_publish)

### `lane::list` {#fn.list}

```rust
pub fn list() -> Vec<LaneInfo>
```

Every lane. It passes no pointer at all, so it suits looking at what exists first.

- Return type: `Vec<LaneInfo>`
- Slots: [`lane_list`](../cpp/crossmod.md#lane_list), [`lane_list`](../cpp/crossmod.md#lane_list)

### `lane::collect_strings` {#fn.collect_strings}

```rust
pub fn collect_strings(call: impl FnOnce(*mut c_void, LaneStrSink) -> u32) -> Vec<String>
```

Collects a lane call going through [`LaneStrSink`](lane.md#LaneStrSink) into a `Vec<String>`.

An entry that is not UTF-8 is skipped with a warning rather than voiding the whole
batch: the other side may be written in another language, and one bad entry should not
make the whole query unanswerable.

- Parameters:
    - call : `impl FnOnce(*mut c_void, LaneStrSink) -> u32`
- Return type: `Vec<String>`

### `lane::list_json` {#fn.list_json}

```rust
pub fn list_json() -> String
```

The raw JSON of the lane listing, unparsed.

For when [`list`](dimensions.md#fn.list) cannot parse it, or the host added a field this layer does not know.

- Return type: `String`
- Slots: [`lane_list`](../cpp/crossmod.md#lane_list), [`lane_list`](../cpp/crossmod.md#lane_list)

## `Lane` {#Lane}

```rust
pub struct Lane<C: LaneContract> {
    // private fields
}
```

An acquired lane. Dropping it returns the lease.

- Implements: `Drop`

### `Lane::is_alive` {#Lane.is_alive}

```rust
pub fn is_alive(&self) -> bool
```

Whether the provider is still there.

Read with `Acquire`: the writer uses a release store and a relaxed read establishes no
happens-before relationship with it.

- Return type: `bool`

### `Lane::fingerprint` {#Lane.fingerprint}

```rust
pub fn fingerprint(&self) -> u64
```

- Return type: `u64`

### `Lane::with` {#Lane.with}

```rust
pub fn with<R>(&self, f: impl FnOnce(&C::Table, LaneData) -> R) -> Option<R>
```

Runs a piece of code inside the provider. Returns `None` when the provider is gone.

`busy` goes up on the way in and down on the way out, and the host refuses to unload
the provider while it is up, so `FreeLibrary` cannot pull the stack frame out from
under you. It goes up **before** the liveness flag is read: the other order leaves a
window with the count at zero, where a retire passes its veto and unmaps the dylib
while this call is already on its way in.

The closure receives a [`LaneData`](lane.md#LaneData) rather than a raw `*mut c_void`, because the first
parameter of every function in a lane table is a `LaneData`, which is the other side's
self, and handing over a raw pointer would make the caller wrap it by hand at every
call site.

- Parameters:
    - f : `impl FnOnce(&C::Table, LaneData) -> R`
- Return type: `Option<R>`

## `Publication` {#Publication}

```rust
pub struct Publication {
    // private fields
}
```

One publication. Dropping it withdraws the lane.

- Implements: `Drop`

### `Publication::id` {#Publication.id}

```rust
pub fn id(&self) -> u64
```

- Return type: `u64`

### `Publication::forget` {#Publication.forget}

```rust
pub fn forget(mut self)
```

## `LaneStr` {#LaneStr}

```rust
pub struct LaneStr {
    pub ptr: *const u8,
    pub len: usize,
}
```

A span of UTF-8 text passed over a lane.

The producer decides how long the pointer stays valid, usually only for the duration of
one call (contract §3). A receiver keeping it has to copy it out.

- Implements: `Clone`, `Copy`

### `LaneStr::new` {#LaneStr.new}

```rust
pub fn new(s: &str) -> LaneStr
```

Borrows a `&str`. The lifetime of the result is not tracked by the type system, which
is the cost of crossing an ABI boundary, so it belongs only where it is constructed and
passed in immediately.

- Parameters:
    - s : `&str`
- Return type: `LaneStr`

### `LaneStr::try_as_str` {#LaneStr.try_as_str}

```rust
pub unsafe fn try_as_str<'a>(&self) -> Option<&'a str>
```

Reads it as a `&str`.

Content that is not valid UTF-8 returns `None` rather than going through
`from_utf8_unchecked`: the other side may be written in another language whose strings
do not necessarily pass this test.

**Safety**
`ptr` and `len` must describe memory that is still valid now.

- Return type: `Option<&'a str>`

## `LaneSlice` {#LaneSlice}

```rust
pub struct LaneSlice<T> {
    pub ptr: *const T,
    pub len: usize,
}
```

A contiguous span of same-typed elements passed over a lane.

- Implements: `Clone`, `Copy`

### `LaneSlice::new` {#LaneSlice.new}

```rust
pub fn new(v: &[T]) -> LaneSlice<T>
```

- Parameters:
    - v : `&[T]`
- Return type: `LaneSlice<T>`

### `LaneSlice::as_slice` {#LaneSlice.as_slice}

```rust
pub unsafe fn as_slice<'a>(&self) -> &'a [T]
```

**Safety**
`ptr` and `len` must describe memory that is still valid now and really holds `T`.

- Return type: `&'a [T]`

## `LaneError` {#LaneError}

```rust
pub enum LaneError {
        /// Nobody publishes this name, because the mod is not installed.
        NotFound { name: String },
        /// It is published with a different fingerprint. Fall back to service and pass no pointer.
        Fingerprint {
            name: String,
            theirs: u64,
            ours: u64,
        },
        /// The name is invalid, the lane is its own, the provider is disabled, or the protocol
        /// version does not match.
        Refused { name: String },
        /// The host offers no fast-lane capability.
        Unavailable,
}
```

Why acquiring a lane failed.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Display`, `Error`

### `LaneError::advice` {#LaneError.advice}

```rust
pub fn advice(&self) -> String
```

One sentence on what to do about it. This goes in the log rather than the enum name
(contract §5.3).

- Return type: `String`

## `LaneContract` {#LaneContract}

```rust
pub trait LaneContract { /* ... */ }
```

One lane contract: the function-table shape both sides agreed on.

`FINGERPRINT` has to change whenever the shape of the table changes, including field
order, parameter types and calling convention. It matches automatically when both sides
reference the same contract definition, the same version of the same crate. Copying an
identical constant by hand is wrong, and is exactly what it guards against.

## `LaneInfo` {#LaneInfo}

```rust
pub struct LaneInfo {
    pub name: String,
    #[serde(rename = "mod")]
    #[serde(default)]
    pub owner: String,
    /// The host gives it as a `"0x..."` string.
    #[serde(default)]
    pub fingerprint: String,
    #[serde(default)]
    pub protocol: u32,
    #[serde(default)]
    pub leases: u32,
    #[serde(default)]
    pub alive: bool,
}
```

The registration record of a published lane.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `serde::Deserialize`

## `LaneData` {#LaneData}

```rust
pub struct LaneData(pub *mut c_void);
```

The opaque context pointer of a lane function.

It wraps rather than using `*mut c_void` directly so that a table signature in the
contract reads as this parameter being that side's self.

- Implements: `Clone`, `Copy`

## `LaneStrSink` {#LaneStrSink}

```rust
pub type LaneStrSink = unsafe extern "C" fn(ctx: *mut c_void, item: LaneStr);
```

The callback shape where a producer sinks one entry and a receiver copies it out.
