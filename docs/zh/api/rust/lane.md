# levilamina::lane · 快速通道

同工具链快速通道：绕开 JSON，直接调用函数表。

和 [`crate::service`](service.md) 的分工：服务是跨语言的 `(名字, JSON) -> JSON` 通道，总是成立；快速通道传递原始的数据指针和函数表指针，只在两边由同一个工具链构建时成立。指纹对不上时一个指针都不交出，退回到服务。

指纹必须由调用方计算，`0` 是无效的：这个槽位把函数表原样交出去，使用方按自己的类型布局去解读它，跳过检查就是类型混淆。`0` 会被理解成「谁都可以连」。[`list`](dimensions.md#fn.list) 显示有哪些通道，不交出任何指针。

获取之后每次调用要遵守的规矩，写在 [`Lane::with`](lane.md#Lane.with) 里。

## 函数 {#functions}

### `lane::acquire` {#fn.acquire}

```rust
pub fn acquire<C: LaneContract>() -> std::result::Result<Lane<C>, LaneError>
```

- 返回值类型：`std::result::Result<Lane<C>, LaneError>`
- 对应槽位：[`lane_acquire`](../cpp/crossmod.md#lane_acquire)、[`lane_acquire`](../cpp/crossmod.md#lane_acquire)

### `lane::guard` {#fn.guard}

```rust
pub fn guard<R>(fallback: R, f: impl FnOnce() -> R) -> R
```

截住快速通道回调里的 panic。

发布方的表函数都是 `extern "C"`，panic 跨过 `extern "C"` 边界是未定义行为。每个表函数的第一行都应该是它。里面运行的是这个模组自己的业务代码，出 panic 的可能性和别处一样，但在这里后果要严重得多：调用方是另一个模组，它的栈帧就在栈上。

发生 panic 时，返回 `fallback` 并记录日志。`fallback` 必须是这个表函数表示「答不上来」的值，不能是表示「否」的值：把 panic 当成一个明确的否定回答，等于让一个 bug 替业务逻辑做了决定。

- 参数：
    - fallback : `R`
    - f : `impl FnOnce() -> R`
- 返回值类型：`R`

### `lane::publish` {#fn.publish}

```rust
pub unsafe fn publish<C: LaneContract>(
    data: *mut c_void,
    vtable: *const C::Table,
    retain: Option<sys::PierLaneRefFn>,
    release: Option<sys::PierLaneRefFn>,
) -> Result<Publication>
```

发布一个快速通道。

**安全性**

调用方必须保证：

- `data` 和 `vtable` 在这个通道撤回之前一直有效。宿主一个字节都不解读，也不复制任何东西，只比较是否相等并标记存活；
- `vtable` 确实指向按 C 布局的表 `C::Table`，它的形状和 `C::FINGERPRINT` 一致；
- `retain` 和 `release` 不回头调用任何 `lane_*` 槽位，否则会自己锁死自己。

- 参数：
    - data : `*mut c_void`
    - vtable : `*const C::Table`
    - retain : `Option<sys::PierLaneRefFn>`
    - release : `Option<sys::PierLaneRefFn>`
- 返回值类型：`Result<Publication>`
- 对应槽位：[`lane_publish`](../cpp/crossmod.md#lane_publish)

### `lane::list` {#fn.list}

```rust
pub fn list() -> Vec<LaneInfo>
```

所有快速通道。它不交出任何指针，所以适合先看看有什么。

- 返回值类型：`Vec<LaneInfo>`
- 对应槽位：[`lane_list`](../cpp/crossmod.md#lane_list)、[`lane_list`](../cpp/crossmod.md#lane_list)

### `lane::collect_strings` {#fn.collect_strings}

```rust
pub fn collect_strings(call: impl FnOnce(*mut c_void, LaneStrSink) -> u32) -> Vec<String>
```

把一次经过 [`LaneStrSink`](lane.md#LaneStrSink) 的快速通道调用收集成 `Vec<String>`。

不是 UTF-8 的条目会被跳过，并记一条警告，不会让整批作废：另一边可能是用别的语言写的，一条坏数据不应该让整个查询答不上来。

- 参数：
    - call : `impl FnOnce(*mut c_void, LaneStrSink) -> u32`
- 返回值类型：`Vec<String>`

### `lane::list_json` {#fn.list_json}

```rust
pub fn list_json() -> String
```

快速通道列表的原始 JSON，不做解析。

用于 [`list`](dimensions.md#fn.list) 解析不了，或者宿主加了这一层不认识的字段的时候。

- 返回值类型：`String`
- 对应槽位：[`lane_list`](../cpp/crossmod.md#lane_list)、[`lane_list`](../cpp/crossmod.md#lane_list)

## `Lane` {#Lane}

```rust
pub struct Lane<C: LaneContract> {
    // private fields
}
```

一个已获取的快速通道。丢弃它就会归还租约。

- 实现的 trait：`Drop`

### `Lane::is_alive` {#Lane.is_alive}

```rust
pub fn is_alive(&self) -> bool
```

提供方是否还在。

用 `Acquire` 读取：写入方用的是 release 存储，而 relaxed 读取和它之间不建立任何 happens-before 关系。

- 返回值类型：`bool`

### `Lane::fingerprint` {#Lane.fingerprint}

```rust
pub fn fingerprint(&self) -> u64
```

- 返回值类型：`u64`

### `Lane::with` {#Lane.with}

```rust
pub fn with<R>(&self, f: impl FnOnce(&C::Table, LaneData) -> R) -> Option<R>
```

在提供方内部运行一段代码。提供方已经不在时返回 `None`。

进入时 `busy` 加一，离开时减一，它不为零时宿主拒绝卸载提供方，所以你的栈帧还在里面的时候，`FreeLibrary` 不会把代码卸掉。它要在读取存活标记**之前**加一：反过来的顺序会留下一个计数为零的窗口，退役流程在这个窗口里通过了否决、卸载了 dylib，而这次调用已经在进入的路上了。

闭包收到的是 [`LaneData`](lane.md#LaneData)，不是裸的 `*mut c_void`，因为通道表里每个函数的第一个参数都是 `LaneData`，也就是另一边的 self；交出裸指针的话，调用方在每个调用处都得自己包一层。

- 参数：
    - f : `impl FnOnce(&C::Table, LaneData) -> R`
- 返回值类型：`Option<R>`

## `Publication` {#Publication}

```rust
pub struct Publication {
    // private fields
}
```

一次发布。丢弃它就会撤回通道。

- 实现的 trait：`Drop`

### `Publication::id` {#Publication.id}

```rust
pub fn id(&self) -> u64
```

- 返回值类型：`u64`

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

通过快速通道传递的一段 UTF-8 文本。

指针有效多久由生产方决定，通常只在一次调用期间有效（契约 §3）。接收方要保留就得复制出来。

- 实现的 trait：`Clone`、`Copy`

### `LaneStr::new` {#LaneStr.new}

```rust
pub fn new(s: &str) -> LaneStr
```

借用一个 `&str`。结果的生命周期不受类型系统跟踪，这是跨 ABI 边界的代价，所以它只该用在构造完立刻传进去的地方。

- 参数：
    - s : `&str`
- 返回值类型：`LaneStr`

### `LaneStr::try_as_str` {#LaneStr.try_as_str}

```rust
pub unsafe fn try_as_str<'a>(&self) -> Option<&'a str>
```

当作 `&str` 读取。

内容不是合法的 UTF-8 时返回 `None`，不走 `from_utf8_unchecked`：另一边可能是用别的语言写的，那种语言的字符串不一定通得过这项检查。

**安全性**

`ptr` 和 `len` 描述的内存此刻必须仍然有效。

- 返回值类型：`Option<&'a str>`

## `LaneSlice` {#LaneSlice}

```rust
pub struct LaneSlice<T> {
    pub ptr: *const T,
    pub len: usize,
}
```

通过快速通道传递的一段连续的同类型元素。

- 实现的 trait：`Clone`、`Copy`

### `LaneSlice::new` {#LaneSlice.new}

```rust
pub fn new(v: &[T]) -> LaneSlice<T>
```

- 参数：
    - v : `&[T]`
- 返回值类型：`LaneSlice<T>`

### `LaneSlice::as_slice` {#LaneSlice.as_slice}

```rust
pub unsafe fn as_slice<'a>(&self) -> &'a [T]
```

**安全性**

`ptr` 和 `len` 描述的内存此刻必须仍然有效，并且里面确实是 `T`。

- 返回值类型：`&'a [T]`

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

获取快速通道失败的原因。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`Display`、`Error`

### `LaneError::advice` {#LaneError.advice}

```rust
pub fn advice(&self) -> String
```

一句话说明该怎么办。这句话写进日志，不写进枚举名（契约 §5.3）。

- 返回值类型：`String`

## `LaneContract` {#LaneContract}

```rust
pub trait LaneContract { /* ... */ }
```

一份快速通道契约：两边约定好的函数表形状。

表的形状一变，`FINGERPRINT` 就必须跟着变，包括字段顺序、参数类型和调用约定。两边引用同一份契约定义，也就是同一个 crate 的同一个版本时，指纹自动对得上。手抄一个相同的常量是错的，而它防的正是这种做法。

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

已发布通道的注册记录。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`、`serde::Deserialize`

## `LaneData` {#LaneData}

```rust
pub struct LaneData(pub *mut c_void);
```

通道函数的不透明上下文指针。

它包了一层，没有直接用 `*mut c_void`，这样契约里的表签名读起来就是：这个参数是那一边的 self。

- 实现的 trait：`Clone`、`Copy`

## `LaneStrSink` {#LaneStrSink}

```rust
pub type LaneStrSink = unsafe extern "C" fn(ctx: *mut c_void, item: LaneStr);
```

生产方每输出一项、接收方就复制出来的回调形状。
