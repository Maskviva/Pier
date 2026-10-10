# levilamina::client · 客户端

只在客户端可用的能力。

**在服务器宿主上，这一整组都是空槽位**

在服务器宿主上调用，得到的是运行时的 `Err`，编译不会报错。契约 §2.1：所有目标上的布局都一样，缺少的能力就是 NULL 槽位，所以同一份模组源码能为两个目标编译；加载到错误的目标上时，宿主会在握手时根据 `mod_flags` 明确拒绝。

用 [`is_available`](client.md#fn.is_available) 判断，不要用 `cfg`。

**回调在客户端线程上运行**

不在服务器线程上。热键回调不能碰服务器的状态。

## 函数 {#functions}

### `client::is_available` {#fn.is_available}

```rust
pub fn is_available() -> bool
```

这个宿主是不是为客户端目标构建的，也就是 `client_*` 槽位有没有填上。

- 返回值类型：`bool`
- 对应槽位：[`client_is_in_level`](../cpp/client.md#client_is_in_level)

### `client::is_in_level` {#fn.is_in_level}

```rust
pub fn is_in_level() -> Result<bool>
```

客户端当前是否在一个世界里。

- 返回值类型：`Result<bool>`
- 对应槽位：[`client_is_in_level`](../cpp/client.md#client_is_in_level)

### `client::local_player_name` {#fn.local_player_name}

```rust
pub fn local_player_name() -> Result<String>
```

本地玩家的名字。不在世界里时返回 `Err`。

- 返回值类型：`Result<String>`
- 对应槽位：[`client_get_local_player`](../cpp/client.md#client_get_local_player)

### `client::screen_name` {#fn.screen_name}

```rust
pub fn screen_name() -> Result<String>
```

当前界面的名字，比如 `"hud_screen"` 或 `"pause_screen"`。

- 返回值类型：`Result<String>`
- 对应槽位：[`client_get_screen_name`](../cpp/client.md#client_get_screen_name)

### `client::register_key` {#fn.register_key}

```rust
pub fn register_key(
    name: &str,
    key_codes: &[i32],
    allow_remap: bool,
    handler: impl FnMut(KeyAction, i32) + Send + 'static,
) -> Result<KeyBinding>
```

注册一个热键。

`key_codes` 是默认的绑定，`allow_remap` 决定玩家能不能在设置里改它。回调收到（动作，焦点影响），两者都在客户端线程上运行。

- 参数：
    - name : `&str`
    - key_codes : `&[i32]`
    - allow_remap : `bool`
    - handler : `impl FnMut(KeyAction, i32) + Send + 'static`
- 返回值类型：`Result<KeyBinding>`
- 对应槽位：[`client_register_key`](../cpp/client.md#client_register_key)

## `KeyBinding` {#KeyBinding}

```rust
pub struct KeyBinding {
    // private fields
}
```

一个已注册的热键。丢弃它就会注销。

- 实现的 trait：`Drop`

### `KeyBinding::name` {#KeyBinding.name}

```rust
pub fn name(&self) -> &str
```

- 返回值类型：`&str`

### `KeyBinding::key_codes` {#KeyBinding.key_codes}

```rust
pub fn key_codes(&self) -> Result<Vec<i32>>
```

当前绑定的按键码。玩家重新绑定以后，就和默认值不同了。

- 返回值类型：`Result<Vec<i32>>`
- 对应槽位：[`client_get_key_codes`](../cpp/client.md#client_get_key_codes)

### `KeyBinding::forget` {#KeyBinding.forget}

```rust
pub fn forget(mut self)
```

让它一直存活到模组卸载，那时由宿主清掉。

## `KeyAction` {#KeyAction}

```rust
pub enum KeyAction {
        Up,
        Down,
        /// The host reported an action code this side does not recognize.
        Unknown(i32),
}
```

一个按键动作。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `KeyAction::from_i32` {#KeyAction.from_i32}

```rust
pub fn from_i32(v: i32) -> KeyAction
```

- 参数：
    - v : `i32`
- 返回值类型：`KeyAction`

### `KeyAction::is_down` {#KeyAction.is_down}

```rust
pub fn is_down(self) -> bool
```

- 返回值类型：`bool`
