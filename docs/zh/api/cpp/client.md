# 客户端

??? note "abi.h 里的分节说明"

    **能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL（`Capability group: client (client_*). All NULL on a server host.`）**

    后面是两个能力组：客户端和维度。它们在布局里无条件存在；宿主没有编入对应的能力包时，它们的槽位为 NULL（见文件头）。新槽位仍然加在结构体真正的末尾，永远不加在能力组中间。

    所有回调都在客户端线程上触发。

## 槽位 {#slots}

### `client_get_local_player` {#client_get_local_player}

```c
bool (*client_get_local_player)(void* ctx, PierStrSink sink);
```

通过 `ll::service::getClientInstance()->getLocalPlayer()` 取本地玩家的名字。输出回调收到名字；不在世界里时调用返回 false。

- 调用形式：`api->client_get_local_player(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL（`Capability group: client (client_*). All NULL on a server host.`）
- 表内序号：第 140 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`client::local_player_name`](../rust/client.md#fn.local_player_name)
    - Go：[`Raw.ClientGetLocalPlayer`](../go/raw.md#Raw.ClientGetLocalPlayer)

### `client_is_in_level` {#client_is_in_level}

```c
bool (*client_is_in_level)(void);
```

客户端在世界里（已经加载了一个世界）时为 true。

- 调用形式：`api->client_is_in_level()`
- 返回值类型：`bool`
- 所在分节：能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL（`Capability group: client (client_*). All NULL on a server host.`）
- 表内序号：第 141 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`client::is_available`](../rust/client.md#fn.is_available)、[`client::is_in_level`](../rust/client.md#fn.is_in_level)
    - Go：[`Raw.ClientIsInLevel`](../go/raw.md#Raw.ClientIsInLevel)

### `client_get_screen_name` {#client_get_screen_name}

```c
bool (*client_get_screen_name)(void* ctx, PierStrSink sink);
```

当前界面的名字（例如 `"hud_screen"`、`"pause_screen"`）。当前所有宿主上这个槽位都是 NULL，客户端构建也一样：引擎没有提供稳定的访问方式。

- 调用形式：`api->client_get_screen_name(ctx, sink)`
- 参数：
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL（`Capability group: client (client_*). All NULL on a server host.`）
- 表内序号：第 142 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`client::screen_name`](../rust/client.md#fn.screen_name)
    - Go：[`Raw.ClientGetScreenName`](../go/raw.md#Raw.ClientGetScreenName)

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

通过 `ll::input::KeyRegistry::getOrCreateKey` 注册一个按键绑定。失败时返回 NULL。句柄归调用方所有，用 `client_unregister_key` 释放。`down_cb`/`up_cb` 在客户端线程上触发。

- 调用形式：`api->client_register_key(mod, name, key_codes, key_count, allow_remap, down_cb, up_cb, user)`
- 参数：
    - mod : `PierModHandle`
    - name : `PierStr`
    - key_codes : `int32_t const*`
    - key_count : `int32_t`
    - allow_remap : `bool`
    - down_cb : `PierKeyCb`
    - up_cb : `PierKeyCb`
    - user : `void*`
- 返回值类型：`PierKeyHandle`
- 所在分节：能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL（`Capability group: client (client_*). All NULL on a server host.`）
- 表内序号：第 143 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`client::register_key`](../rust/client.md#fn.register_key)
    - Go：[`RegisterKey`](../go/client.md#RegisterKey)

### `client_unregister_key` {#client_unregister_key}

```c
bool (*client_unregister_key)(PierKeyHandle handle);
```

取消一个按键绑定：这个模组的处理函数不再触发，句柄被释放。`ll::input::KeyHandle` 本身不会被销毁，因为按键注册表每个名字只保留一个按键，也没有提供删除的办法；用同一个名字再注册时会复用那个按键。

- 调用形式：`api->client_unregister_key(handle)`
- 参数：
    - handle : `PierKeyHandle`
- 返回值类型：`bool`
- 所在分节：能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL（`Capability group: client (client_*). All NULL on a server host.`）
- 表内序号：第 144 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Go：[`KeyBinding.Unregister`](../go/client.md#KeyBinding.Unregister)、[`Raw.ClientUnregisterKey`](../go/raw.md#Raw.ClientUnregisterKey)

### `client_get_key_codes` {#client_get_key_codes}

```c
bool (*client_get_key_codes)(PierKeyHandle handle, void* ctx, PierStrSink sink);
```

当前分配的按键码（被玩家重新映射过时可能和默认值不同）。输出回调收到一个 JSON 风格的数组字符串 `"[1,2,3]"`。

- 调用形式：`api->client_get_key_codes(handle, ctx, sink)`
- 参数：
    - handle : `PierKeyHandle`
    - ctx : `void*`
    - sink : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：能力组：客户端（`client_*`）。在服务器宿主上全部为 NULL（`Capability group: client (client_*). All NULL on a server host.`）
- 表内序号：第 145 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`KeyBinding::key_codes`](../rust/client.md#KeyBinding.key_codes)
    - Go：[`Raw.ClientGetKeyCodes`](../go/raw.md#Raw.ClientGetKeyCodes)

## 类型 {#types}

### `PierKeyHandle` {#PierKeyHandle}

```c
typedef void* PierKeyHandle;
```

指向一个已注册按键绑定的不透明句柄，绑定归加载器的 `ll::input::KeyRegistry` 所有。用 `client_unregister_key` 释放。

### `PierKeyAction` {#PierKeyAction}

```c
typedef int32_t PierKeyAction;
```

按键动作：0 = 松开（up），1 = 按下（down）。对应 `ll::event::KeyInputEvent::Action`。

### `PierFocusImpact` {#PierFocusImpact}

```c
typedef int32_t PierFocusImpact;
```

焦点影响级别：0=Neutral，1=ActivateFocus，2=DeactivateFocus。对应 `::FocusImpact`。

### `PierKeyCb` {#PierKeyCb}

```c
typedef void (*PierKeyCb)(void* user, PierKeyAction action, PierFocusImpact impact);
```

按键按下和松开事件的回调，在客户端线程上运行。

- `user`：传给 `client_register_key` 的指针
- `action`：0=松开，1=按下（见 `PierKeyAction`）
- `impact`：当前的焦点影响（见 `PierFocusImpact`）
