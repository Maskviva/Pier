# levilamina::client

Client-only capabilities.

**On a server host this whole family is empty slots**

Not a compile error but a runtime `Err`. Contract §2.1: the layout is identical on every
target and an absent capability is a NULL slot, so the same mod source compiles for both
targets and loading onto the wrong one is refused explicitly by the host during the
handshake, from `mod_flags`.

Decide with [`is_available`](client.md#fn.is_available) and not with a `cfg`.

**A callback runs on the client thread**

Not the server thread. A hotkey callback must not touch server state.

## Functions {#functions}

### `client::is_available` {#fn.is_available}

```rust
pub fn is_available() -> bool
```

Whether this host was built for the client target, meaning whether the `client_*` slots
are filled.

- Return type: `bool`
- Slots: [`client_is_in_level`](../cpp/client.md#client_is_in_level)

### `client::is_in_level` {#fn.is_in_level}

```rust
pub fn is_in_level() -> Result<bool>
```

Whether the client is currently in a world.

- Return type: `Result<bool>`
- Slots: [`client_is_in_level`](../cpp/client.md#client_is_in_level)

### `client::local_player_name` {#fn.local_player_name}

```rust
pub fn local_player_name() -> Result<String>
```

The name of the local player. Not being in a world is an `Err`.

- Return type: `Result<String>`
- Slots: [`client_get_local_player`](../cpp/client.md#client_get_local_player)

### `client::screen_name` {#fn.screen_name}

```rust
pub fn screen_name() -> Result<String>
```

The current screen name, such as `"hud_screen"` or `"pause_screen"`.

- Return type: `Result<String>`
- Slots: [`client_get_screen_name`](../cpp/client.md#client_get_screen_name)

### `client::register_key` {#fn.register_key}

```rust
pub fn register_key(
    name: &str,
    key_codes: &[i32],
    allow_remap: bool,
    handler: impl FnMut(KeyAction, i32) + Send + 'static,
) -> Result<KeyBinding>
```

Registers a hotkey.

`key_codes` is the default binding and `allow_remap` decides whether the player may
change it in the settings.
The callback receives (action, focus impact) and both run on the client thread.

- Parameters:
    - name : `&str`
    - key_codes : `&[i32]`
    - allow_remap : `bool`
    - handler : `impl FnMut(KeyAction, i32) + Send + 'static`
- Return type: `Result<KeyBinding>`
- Slots: [`client_register_key`](../cpp/client.md#client_register_key)

## `KeyBinding` {#KeyBinding}

```rust
pub struct KeyBinding {
    // private fields
}
```

One registered hotkey. Dropping it deregisters.

- Implements: `Drop`

### `KeyBinding::name` {#KeyBinding.name}

```rust
pub fn name(&self) -> &str
```

- Return type: `&str`

### `KeyBinding::key_codes` {#KeyBinding.key_codes}

```rust
pub fn key_codes(&self) -> Result<Vec<i32>>
```

The key codes currently bound. They differ from the defaults once the player has
rebound them.

- Return type: `Result<Vec<i32>>`
- Slots: [`client_get_key_codes`](../cpp/client.md#client_get_key_codes)

### `KeyBinding::forget` {#KeyBinding.forget}

```rust
pub fn forget(mut self)
```

Keeps it alive until the mod unloads, when the host clears it.

## `KeyAction` {#KeyAction}

```rust
pub enum KeyAction {
        Up,
        Down,
        /// The host reported an action code this side does not recognize.
        Unknown(i32),
}
```

A key action.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `KeyAction::from_i32` {#KeyAction.from_i32}

```rust
pub fn from_i32(v: i32) -> KeyAction
```

- Parameters:
    - v : `i32`
- Return type: `KeyAction`

### `KeyAction::is_down` {#KeyAction.is_down}

```rust
pub fn is_down(self) -> bool
```

- Return type: `bool`
