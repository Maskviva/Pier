# levilamina::service

Cross-mod services: one question, one answer.

Complementary to the bus, which broadcasts and returns nothing. A name is exclusive:
one name has one provider, and taking one is refused by the host, whose log names the
holder, which is far easier to diagnose than the later registration winning.

[`call`](service.md#fn.call) gives a bare `String` while [`call_json`](service.md#fn.call_json) deserializes straight into the type
you want, where a parse failure is a definite error and not the fallback of an
`unwrap_or`.

The errors are categorized in [`CallError`](service.md#CallError): no such service and the service refusing
are two different things. The first usually means a missing dependency or a misspelled
name, and the second is a refusal by business logic.

## Functions {#functions}

### `service::call` {#fn.call}

```rust
pub fn call(name: &str, request: &str) -> CallResult<String>
```

Calls a service and returns the raw reply text.

- Parameters:
    - name : `&str`
    - request : `&str`
- Return type: `CallResult<String>`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `service::call_json` {#fn.call_json}

```rust
pub fn call_json<T: DeserializeOwned>(name: &str, request: &str) -> CallResult<T>
```

Calls a service and deserializes the reply from JSON into `T`.

This is the recommended form. It replaces the `from_str`, `as_array`, `get`,
`unwrap_or(0)` boilerplate and turns a malformed reply into a real error rather than a
perfectly ordinary looking 0.

```rust
#[derive(serde::Deserialize)]
struct World { dim: i32, #[serde(rename = "plotSize")] plot_size: i32 }

let worlds: Vec<World> = service::call_json("plot:worlds", "{}")?;
```

- Parameters:
    - name : `&str`
    - request : `&str`
- Return type: `CallResult<T>`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `service::call_with` {#fn.call_with}

```rust
pub fn call_with<Q: serde::Serialize, T: DeserializeOwned>(
    name: &str,
    request: &Q,
) -> CallResult<T>
```

As above, with the request side serialized through `serde` as well.

- Parameters:
    - name : `&str`
    - request : `&Q`
- Return type: `CallResult<T>`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `service::call_optional` {#fn.call_optional}

```rust
pub fn call_optional(name: &str, request: &str) -> CallResult<Option<String>>
```

Calls, returning `None` on `NotFound` and raising every other error as usual.

For an optional integration that uses a dependency when installed and degrades
otherwise. Such code used to be written as
`let Ok(x) = call(..) else { return default }`, which swallowed a service error too.

- Parameters:
    - name : `&str`
    - request : `&str`
- Return type: `CallResult<Option<String>>`
- Slots: [`service_call`](../cpp/crossmod.md#service_call)

### `service::exists` {#fn.exists}

```rust
pub fn exists(name: &str) -> bool
```

Whether any mod provides this name.

An earlier generation substring-matched inside the JSON text of `service_list`, so a
name that is a prefix of another matched wrongly, with `plot` hitting `plot:worlds`.
This really parses it.

- Parameters:
    - name : `&str`
- Return type: `bool`
- Slots: [`service_list`](../cpp/crossmod.md#service_list)

### `service::list_json` {#fn.list_json}

```rust
pub fn list_json() -> String
```

The raw JSON of the service listing, unparsed.

For when [`list`](dimensions.md#fn.list) cannot parse it, or the host added a field this layer does not know.

- Return type: `String`
- Slots: [`service_list`](../cpp/crossmod.md#service_list)

### `service::list` {#fn.list}

```rust
pub fn list() -> Vec<ServiceInfo>
```

Every currently registered service.

- Return type: `Vec<ServiceInfo>`
- Slots: [`service_list`](../cpp/crossmod.md#service_list)

### `service::caller` {#fn.caller}

```rust
pub fn caller() -> Option<String>
```

Who is calling the service callback that is running right now; see `service_caller`
in `abi.h`.

This is the one thing a provider can trust about a request. The request body names
whoever it likes, and a provider keying an owner or an acting player on it has only
the sender's word; this asks the host, which knows whose `service::call` is on the
stack. Nested calls report the innermost one.

`None` outside a callback, when the call came without a mod handle, or on a host too
old to have the slot. The last case matters: a provider that wants attribution must
decide what to do when there is none, and "attribute to nobody" and "refuse" are both
defensible while "attribute to the empty name" is not.

- Return type: `Option<String>`
- Slots: [`service_caller`](../cpp/crossmod.md#service_caller)

### `service::register` {#fn.register}

```rust
pub fn register(
    name: &str,
    provider: impl FnMut(&str, &str) -> std::result::Result<String, String> + Send + 'static,
) -> Result<Registration>
```

Registers a service.
```rust
let _reg = service::register("plot:worlds", |_name, _req| {
    Ok(serde_json::to_string(&worlds()).unwrap_or_default())

})?;
```

A few host-side rules, which cannot be changed here and are worth knowing:
* the callback runs synchronously on the caller's thread, so nothing slow belongs in it;
* a name already taken fails outright and does not displace the holder;
* a service stays reachable while its mod is disabled, since resolving it inside
  another mod's `on_load` would otherwise fail; refusing while disabled is left to
  the provider.

- Parameters:
    - name : `&str`
    - provider : `impl FnMut(&str, &str) -> std::result::Result<String`
    - String> + Send + 'static, : ``
- Return type: `Result<Registration>`
- Slots: [`service_register`](../cpp/crossmod.md#service_register)

### `service::register_json` {#fn.register_json}

```rust
pub fn register_json<Q, R, F>(name: &str, mut provider: F) -> Result<Registration>
where
    Q: DeserializeOwned,
    R: serde::Serialize,
    F: FnMut(Q) -> std::result::Result<R, String> + Send + 'static,
```

Registers a service whose request and reply are both JSON.

It removes the `to_string` and `from_str` boilerplate on both sides, and a failed
deserialization becomes a definite business error returned to the caller rather than
the provider writing its own `unwrap_or_default`.

- Parameters:
    - name : `&str`
    - mut provider : `F`
- Return type: `Result<Registration>`
- Slots: [`service_register`](../cpp/crossmod.md#service_register)

## `Registration` {#Registration}

```rust
pub struct Registration {
    // private fields
}
```

A service registration handle. Dropping it deregisters.

[`Registration::forget`](service.md#Registration.forget) keeps it alive until the mod unloads, and the host clears what
remains at unload.

- Implements: `Drop`

### `Registration::id` {#Registration.id}

```rust
pub fn id(&self) -> u64
```

- Return type: `u64`

### `Registration::name` {#Registration.name}

```rust
pub fn name(&self) -> &str
```

- Return type: `&str`

### `Registration::forget` {#Registration.forget}

```rust
pub fn forget(mut self)
```

## `CallError` {#CallError}

```rust
pub enum CallError {
        /// Nobody provides this name, because it is not installed, not enabled, or misspelled.
        NotFound { name: String },
        /// The provider ran and said no this time. `message` is what it wrote back.
        Provider { name: String, message: String },
        /// The host refused the call: an invalid name, a call to itself, or a call depth exceeded
        /// by a cycle.
        Refused { name: String },
        /// The reply arrived and does not parse into the requested type.
        Decode { name: String, detail: String },
        /// The host offers no service capability, being older or built without that capability
        /// package.
        Unavailable,
}
```

Why a call failed.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `Display`, `Error`

## `CallResult` {#CallResult}

```rust
pub type CallResult<T> = std::result::Result<T, CallError>;
```

The result of a call.

## `ServiceInfo` {#ServiceInfo}

```rust
pub struct ServiceInfo {
    pub name: String,
    /// The name of the providing mod.
    #[serde(default)]
    #[serde(rename = "mod")]
    pub owner: String,
}
```

The registration record of one service.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`, `serde::Deserialize`
