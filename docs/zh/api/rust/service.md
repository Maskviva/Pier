# levilamina::service · 服务

跨模组服务：一问一答。

和总线互补，总线是广播，不返回任何东西。名字是独占的：一个名字只有一个提供方，再去占会被宿主拒绝，宿主的日志会写出谁占着这个名字；这比后注册的悄悄胜出好排查得多。

[`call`](service.md#fn.call) 返回一个裸的 `String`，[`call_json`](service.md#fn.call_json) 直接反序列化成你要的类型，解析失败是明确的错误，不会变成某个 `unwrap_or` 的兜底值。

错误在 [`CallError`](service.md#CallError) 里分了类：服务不存在和服务拒绝是两回事。前者通常意味着缺少依赖或者名字拼错，后者是业务逻辑的拒绝。

## 函数 {#functions}

### `service::call` {#fn.call}

```rust
pub fn call(name: &str, request: &str) -> CallResult<String>
```

调用一个服务，返回原始的回答文本。

- 参数：
    - name : `&str`
    - request : `&str`
- 返回值类型：`CallResult<String>`
- 对应槽位：[`service_call`](../cpp/crossmod.md#service_call)

### `service::call_json` {#fn.call_json}

```rust
pub fn call_json<T: DeserializeOwned>(name: &str, request: &str) -> CallResult<T>
```

调用一个服务，把回答从 JSON 反序列化成 `T`。

推荐用这种形式。它省掉了 `from_str`、`as_array`、`get`、`unwrap_or(0)` 这一串样板代码，并把格式不对的回答变成真正的错误，不会变成一个看上去再正常不过的 0。

```rust
#[derive(serde::Deserialize)]
struct World { dim: i32, #[serde(rename = "plotSize")] plot_size: i32 }

let worlds: Vec<World> = service::call_json("plot:worlds", "{}")?;
```

- 参数：
    - name : `&str`
    - request : `&str`
- 返回值类型：`CallResult<T>`
- 对应槽位：[`service_call`](../cpp/crossmod.md#service_call)

### `service::call_with` {#fn.call_with}

```rust
pub fn call_with<Q: serde::Serialize, T: DeserializeOwned>(
    name: &str,
    request: &Q,
) -> CallResult<T>
```

同上，请求那一侧也通过 `serde` 序列化。

- 参数：
    - name : `&str`
    - request : `&Q`
- 返回值类型：`CallResult<T>`
- 对应槽位：[`service_call`](../cpp/crossmod.md#service_call)

### `service::call_optional` {#fn.call_optional}

```rust
pub fn call_optional(name: &str, request: &str) -> CallResult<Option<String>>
```

调用，遇到 `NotFound` 返回 `None`，其他错误照常抛出。

用于可选的集成：依赖装了就用，没装就降级。这类代码以前写成 `let Ok(x) = call(..) else { return default }`，服务的错误也一起被吞掉了。

- 参数：
    - name : `&str`
    - request : `&str`
- 返回值类型：`CallResult<Option<String>>`
- 对应槽位：[`service_call`](../cpp/crossmod.md#service_call)

### `service::exists` {#fn.exists}

```rust
pub fn exists(name: &str) -> bool
```

有没有模组提供这个名字。

早先的一代在 `service_list` 的 JSON 文本里做子串匹配，一个名字是另一个名字的前缀时就会误判，比如 `plot` 会命中 `plot:worlds`。这里是真正解析以后再判断。

- 参数：
    - name : `&str`
- 返回值类型：`bool`
- 对应槽位：[`service_list`](../cpp/crossmod.md#service_list)

### `service::list_json` {#fn.list_json}

```rust
pub fn list_json() -> String
```

服务列表的原始 JSON，不做解析。

用于 [`list`](dimensions.md#fn.list) 解析不了，或者宿主加了这一层不认识的字段的时候。

- 返回值类型：`String`
- 对应槽位：[`service_list`](../cpp/crossmod.md#service_list)

### `service::list` {#fn.list}

```rust
pub fn list() -> Vec<ServiceInfo>
```

当前已注册的所有服务。

- 返回值类型：`Vec<ServiceInfo>`
- 对应槽位：[`service_list`](../cpp/crossmod.md#service_list)

### `service::caller` {#fn.caller}

```rust
pub fn caller() -> Option<String>
```

正在运行的这个服务回调，是谁调用的；见 `abi.h` 里的 `service_caller`。

这是提供方对一个请求唯一能信任的东西。请求体想写谁就写谁，按它判断所有者或执行操作的玩家，凭的只是发送方的一面之词；这个函数去问宿主，宿主知道调用栈上是谁的 `service::call`。嵌套调用时，报告最内层的那一次。

在回调之外、调用没有带模组句柄，或者宿主太旧、没有这个槽位时，返回 `None`。最后一种情况很重要：想要归属的提供方必须决定拿不到归属时怎么办，「不算到任何人头上」和「拒绝」都说得过去，「算到空名字头上」就说不过去了。

- 返回值类型：`Option<String>`
- 对应槽位：[`service_caller`](../cpp/crossmod.md#service_caller)

### `service::register` {#fn.register}

```rust
pub fn register(
    name: &str,
    provider: impl FnMut(&str, &str) -> std::result::Result<String, String> + Send + 'static,
) -> Result<Registration>
```

注册一个服务。

```rust
let _reg = service::register("plot:worlds", |_name, _req| {
    Ok(serde_json::to_string(&worlds()).unwrap_or_default())
})?;
```

几条宿主一侧的规则，这里改不了，值得知道：

- 回调在调用方的线程上同步运行，所以里面不能放任何慢的操作；
- 名字已被占用时直接失败，不会把占用者挤掉；
- 模组被禁用期间，服务仍然可以调用，否则在另一个模组的 `on_load` 里解析它就会失败；禁用期间要不要拒绝，由提供方自己决定。

- 参数：
    - name : `&str`
    - provider : `impl FnMut(&str, &str) -> std::result::Result<String`
    - String> + Send + 'static, : ``
- 返回值类型：`Result<Registration>`
- 对应槽位：[`service_register`](../cpp/crossmod.md#service_register)

### `service::register_json` {#fn.register_json}

```rust
pub fn register_json<Q, R, F>(name: &str, mut provider: F) -> Result<Registration>
where
    Q: DeserializeOwned,
    R: serde::Serialize,
    F: FnMut(Q) -> std::result::Result<R, String> + Send + 'static,
```

注册一个请求和回答都是 JSON 的服务。

它省掉了两边的 `to_string` 和 `from_str` 样板代码；反序列化失败会变成一个明确的业务错误返回给调用方，不需要提供方自己写 `unwrap_or_default`。

- 参数：
    - name : `&str`
    - mut provider : `F`
- 返回值类型：`Result<Registration>`
- 对应槽位：[`service_register`](../cpp/crossmod.md#service_register)

## `Registration` {#Registration}

```rust
pub struct Registration {
    // private fields
}
```

服务注册的句柄。丢弃它就会注销。

[`Registration::forget`](service.md#Registration.forget) 让它一直存活到模组卸载，卸载时宿主会清掉剩下的注册。

- 实现的 trait：`Drop`

### `Registration::id` {#Registration.id}

```rust
pub fn id(&self) -> u64
```

- 返回值类型：`u64`

### `Registration::name` {#Registration.name}

```rust
pub fn name(&self) -> &str
```

- 返回值类型：`&str`

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

调用失败的原因。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Display`、`Error`

## `CallResult` {#CallResult}

```rust
pub type CallResult<T> = std::result::Result<T, CallError>;
```

一次调用的结果。

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

一个服务的注册记录。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`serde::Deserialize`
