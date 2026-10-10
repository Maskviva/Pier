# Client

??? note "Section notes in abi.h"

    **Capability group: client (client\_\*). All NULL on a server host.**

    Two capability groups follow, client and dimensions. They are unconditionally present in the layout; when the capability package was not built into the host, their slots are NULL (see the file header). New slots still go at the real end of the struct, never inside a capability group.

    Every callback fires on the client thread.

## Slots {#slots}

### `client_get_local_player` {#client_get_local_player}

```c
bool (*client_get_local_player)(void* ctx, PierStrSink sink);
```

Local player's name via `ll::service::getClientInstance()`-&gt;getLocalPlayer(). sink receives the name, or the call returns false if not in a level.

- Call: `api->client_get_local_player(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Capability group: client (client\_\*). All NULL on a server host.
- Position in the table: slot 140, counting from 0
- Callers in each binding:
    - Rust: [`client::local_player_name`](../rust/client.md#fn.local_player_name)
    - Go: [`Raw.ClientGetLocalPlayer`](../go/raw.md#Raw.ClientGetLocalPlayer)

### `client_is_in_level` {#client_is_in_level}

```c
bool (*client_is_in_level)(void);
```

True when the client is inside a level (a world is loaded).

- Call: `api->client_is_in_level()`
- Return type: `bool`
- Section of abi.h: Capability group: client (client\_\*). All NULL on a server host.
- Position in the table: slot 141, counting from 0
- Callers in each binding:
    - Rust: [`client::is_available`](../rust/client.md#fn.is_available), [`client::is_in_level`](../rust/client.md#fn.is_in_level)
    - Go: [`Raw.ClientIsInLevel`](../go/raw.md#Raw.ClientIsInLevel)

### `client_get_screen_name` {#client_get_screen_name}

```c
bool (*client_get_screen_name)(void* ctx, PierStrSink sink);
```

Current screen / UI name (e.g. "`hud_screen`", "`pause_screen`"). NULL on every current host, client builds included: the engine exposes no stable accessor for it.

- Call: `api->client_get_screen_name(ctx, sink)`
- Parameters:
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Capability group: client (client\_\*). All NULL on a server host.
- Position in the table: slot 142, counting from 0
- Callers in each binding:
    - Rust: [`client::screen_name`](../rust/client.md#fn.screen_name)
    - Go: [`Raw.ClientGetScreenName`](../go/raw.md#Raw.ClientGetScreenName)

### `client_register_key` {#client_register_key}

```c
PierKeyHandle (*client_register_key)(
    PierModHandle mod,
    PierStr name,
    int32_t const* key_codes,
    int32_t key_count,
    bool allow_remap,
    PierKeyCb down_cb,
    PierKeyCb up_cb,
    void* user
);
```

Register a key binding via `ll::input::KeyRegistry::getOrCreateKey`. Returns NULL on failure. The handle is owned by the caller; drop with `client_unregister_key`. `down_cb`/`up_cb` fire on the client thread.

- Call: `api->client_register_key(mod, name, key_codes, key_count, allow_remap, down_cb, up_cb, user)`
- Parameters:
    - mod : `PierModHandle`
    - name : `PierStr`
    - key_codes : `int32_t const*`
    - key_count : `int32_t`
    - allow_remap : `bool`
    - down_cb : `PierKeyCb`
    - up_cb : `PierKeyCb`
    - user : `void*`
- Return type: `PierKeyHandle`
- Section of abi.h: Capability group: client (client\_\*). All NULL on a server host.
- Position in the table: slot 143, counting from 0
- Callers in each binding:
    - Rust: [`client::register_key`](../rust/client.md#fn.register_key)
    - Go: [`RegisterKey`](../go/client.md#RegisterKey)

### `client_unregister_key` {#client_unregister_key}

```c
bool (*client_unregister_key)(PierKeyHandle handle);
```

Unregister a key binding: this mod's handler stops firing and the handle is freed. The `ll::input::KeyHandle` itself is not destroyed, since the key registry keeps one key per name and offers no removal; registering the same name again reuses that key.

- Call: `api->client_unregister_key(handle)`
- Parameters:
    - handle : `PierKeyHandle`
- Return type: `bool`
- Section of abi.h: Capability group: client (client\_\*). All NULL on a server host.
- Position in the table: slot 144, counting from 0
- Callers in each binding:
    - Go: [`KeyBinding.Unregister`](../go/client.md#KeyBinding.Unregister), [`Raw.ClientUnregisterKey`](../go/raw.md#Raw.ClientUnregisterKey)

### `client_get_key_codes` {#client_get_key_codes}

```c
bool (*client_get_key_codes)(PierKeyHandle handle, void* ctx, PierStrSink sink);
```

Currently assigned key codes (may differ from defaults if remapped). sink receives a JSON-style array string "\[1,2,3\]".

- Call: `api->client_get_key_codes(handle, ctx, sink)`
- Parameters:
    - handle : `PierKeyHandle`
    - ctx : `void*`
    - sink : `PierStrSink`
- Return type: `bool`
- Section of abi.h: Capability group: client (client\_\*). All NULL on a server host.
- Position in the table: slot 145, counting from 0
- Callers in each binding:
    - Rust: [`KeyBinding::key_codes`](../rust/client.md#KeyBinding.key_codes)
    - Go: [`Raw.ClientGetKeyCodes`](../go/raw.md#Raw.ClientGetKeyCodes)

## Types {#types}

### `PierKeyHandle` {#PierKeyHandle}

```c
typedef void* PierKeyHandle;
```

Opaque handle to a registered key binding owned by the loader's `ll::input::KeyRegistry`. Drop via `client_unregister_key`.

### `PierKeyAction` {#PierKeyAction}

```c
typedef int32_t PierKeyAction;
```

Key action: 0 = released (up), 1 = pressed (down). Mirrors `ll::event::KeyInputEvent::Action`.

### `PierFocusImpact` {#PierFocusImpact}

```c
typedef int32_t PierFocusImpact;
```

Focus impact level: 0=Neutral 1=ActivateFocus 2=DeactivateFocus. Mirrors ::FocusImpact.

### `PierKeyCb` {#PierKeyCb}

```c
typedef void (*PierKeyCb)(void* user, PierKeyAction action, PierFocusImpact impact);
```

Callback for key press/release events. Runs on the client thread. user   — pointer passed to `client_register_key` action — 0=released 1=pressed (see PierKeyAction) impact — current focus impact (see PierFocusImpact)
