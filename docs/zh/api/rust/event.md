# levilamina::event · 事件

事件：订阅、读取载荷、修改载荷、取消。

**缺少的键和值为 0 必须分开**

契约 §5.1 记录了一次领地保护绕过：自定义维度里的一个事件读不到 `dim`，使用方写了 `unwrap_or(0)`，把它当成主世界，结果放行了，日志里什么都没有。所以 [`Event`](event.md#Event) 提供带类型的访问方式，缺少键和类型不符是两种不同的错误。

它还识别 `_unresolved`，这是宿主解析不出事件来源时注入的标记。[`Event::dim`](event.md#Event.dim) 这类方法遇到它会返回 `Err`，保护判断因此会以拒绝收场，不会拿一个编出来的 0 继续往下走。

[`Wiring`](event.md#Wiring) 用来链式批量订阅，并把句柄放在一起保管。

## 函数 {#functions}

### `event::subscribe` {#fn.subscribe}

```rust
pub fn subscribe(
    id: &str,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener>
```

订阅一个事件。

```rust
let l = event::subscribe(names::PLAYER_CHAT, |ev| {
    let Ok(msg) = ev.str_at("message") else { return };
    if !msg.contains("badword") { return; }
    // cancel() 返回 Result：拦不住的事件会说明原因，以及该去哪里拦。
    if let Err(e) = ev.cancel() {
        Logger::get().error(&format!("the chat filter did not take effect: {e}"));
    }
})?;
```

- 参数：
    - id : `&str`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- 返回值类型：`Result<Listener>`
- 对应槽位：[`subscribe_event`](../cpp/events.md#subscribe_event)

### `event::subscribe_with` {#fn.subscribe_with}

```rust
pub fn subscribe_with(
    id: &str,
    priority: Priority,
    handler: impl FnMut(&mut Event<'_>) + Send + 'static,
) -> Result<Listener>
```

按指定的优先级订阅。

- 参数：
    - id : `&str`
    - priority : `Priority`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- 返回值类型：`Result<Listener>`
- 对应槽位：[`subscribe_event`](../cpp/events.md#subscribe_event)

### `event::list` {#fn.list}

```rust
pub fn list() -> Vec<String>
```

宿主知道的所有事件 id，来自注册表，加上所有合成事件。

事件名拼错时，读一读它比去猜强。

- 返回值类型：`Vec<String>`
- 对应槽位：[`list_events`](../cpp/events.md#list_events)

### `event::exists` {#fn.exists}

```rust
pub fn exists(id: &str) -> bool
```

这个宿主是否知道某个事件 id。

- 参数：
    - id : `&str`
- 返回值类型：`bool`
- 对应槽位：[`list_events`](../cpp/events.md#list_events)

## `Event` {#Event}

```rust
pub struct Event<'a> {
    // private fields
}
```

一次事件分发，也就是回调收到的东西。

它只在回调期间存在，不能保存下来，生命周期参数也会阻止这样做。

- 实现的 trait：`Debug`

### `Event::id` {#Event.id}

```rust
pub fn id(&self) -> &str
```

事件 id，例如 `"ll::event::player::PlayerChatEvent"`，或者合成事件 `"BlockDestroyEvent"`。

- 返回值类型：`&str`

### `Event::snbt` {#Event.snbt}

```rust
pub fn snbt(&self) -> &str
```

原始的载荷 SNBT。调试时最直接。

- 返回值类型：`&str`

### `Event::value` {#Event.value}

```rust
pub fn value(&mut self) -> Result<&NbtValue>
```

解析后的载荷。第一次调用时解析，之后复用。

- 返回值类型：`Result<&NbtValue>`

### `Event::value_opt` {#Event.value_opt}

```rust
pub fn value_opt(&mut self) -> Option<&NbtValue>
```

载荷解析不了时返回 `None`，不报错。给把解析不了的载荷当作没有事件的观察型监听器用。

- 返回值类型：`Option<&NbtValue>`

### `Event::str_at` {#Event.str_at}

```rust
pub fn str_at(&mut self, path: &str) -> Result<&str>
```

按路径读取一个字符串。缺少键和类型不符，都会在错误里写出键名。

- 参数：
    - path : `&str`
- 返回值类型：`Result<&str>`

### `Event::i64_at` {#Event.i64_at}

```rust
pub fn i64_at(&mut self, path: &str) -> Result<i64>
```

- 参数：
    - path : `&str`
- 返回值类型：`Result<i64>`

### `Event::i32_at` {#Event.i32_at}

```rust
pub fn i32_at(&mut self, path: &str) -> Result<i32>
```

- 参数：
    - path : `&str`
- 返回值类型：`Result<i32>`

### `Event::f64_at` {#Event.f64_at}

```rust
pub fn f64_at(&mut self, path: &str) -> Result<f64>
```

- 参数：
    - path : `&str`
- 返回值类型：`Result<f64>`

### `Event::bool_at` {#Event.bool_at}

```rust
pub fn bool_at(&mut self, path: &str) -> Result<bool>
```

- 参数：
    - path : `&str`
- 返回值类型：`Result<bool>`

### `Event::opt_str` {#Event.opt_str}

```rust
pub fn opt_str(&mut self, path: &str) -> Option<String>
```

宽松的写法：读不出来时返回 `None`，不说明原因。只用在读不到也无所谓的地方。

- 参数：
    - path : `&str`
- 返回值类型：`Option<String>`

### `Event::opt_i64` {#Event.opt_i64}

```rust
pub fn opt_i64(&mut self, path: &str) -> Option<i64>
```

- 参数：
    - path : `&str`
- 返回值类型：`Option<i64>`

### `Event::unresolved` {#Event.unresolved}

```rust
pub fn unresolved(&mut self) -> Vec<String>
```

宿主没能解析的字段列表，即 `_unresolved`。

事件里带着的实体既不是在线玩家、也不在运行时的实体表里时，宿主会把字段名记在这里。列表不为空表示载荷不完整，保护判断应当拒绝，不要去猜。载荷根本解析不了时，列表也是空的，`Self::check_complete` 不会把这种情况算作完整。

- 返回值类型：`Vec<String>`

### `Event::check_complete` {#Event.check_complete}

```rust
pub fn check_complete(&mut self) -> bool
```

检查载荷是否完整：解析成功，并且 `_unresolved` 为空。解析不了的载荷不算完整，因为里面什么都没能解析出来。

- 返回值类型：`bool`

### `Event::dim` {#Event.dim}

```rust
pub fn dim(&mut self) -> Result<i32>
```

事件发生在哪个维度。

读不出来时返回 `Err`。以前的设计让调用方写 `payload.i32_at("dim").unwrap_or(0)`，于是自定义维度（id 为 3 及以上）里的每个事件都被判定在主世界，在主世界拒绝、在别处放行的领地保护就这样被绕过了，日志里什么都没有。

- 返回值类型：`Result<i32>`

### `Event::player` {#Event.player}

```rust
pub fn player(&mut self) -> Option<PlayerIdentity>
```

事件里的玩家身份。

处理三种形状：合成事件的 `_player:{name,xuid,uuid}`，宿主补充的 `_player`，以及只带名字的旧事件。以前调用方得自己知道这些区别。

- 返回值类型：`Option<PlayerIdentity>`

### `Event::pos` {#Event.pos}

```rust
pub fn pos(&mut self) -> Result<(i32, i32, i32)>
```

事件里的方块或位置坐标，即平铺的三个字段 `x`、`y`、`z`，这是合成事件的形状。

- 返回值类型：`Result<(i32, i32, i32)>`

### `Event::pos_f64` {#Event.pos_f64}

```rust
pub fn pos_f64(&mut self) -> Result<(f64, f64, f64)>
```

同上，但用浮点数，用于玩家位置之类。

- 返回值类型：`Result<(f64, f64, f64)>`

### `Event::can_cancel` {#Event.can_cancel}

```rust
pub fn can_cancel(&self) -> Option<bool>
```

这个事件能不能取消。

- `Some(true)`：能；
- `Some(false)`：不能，[`Event::cancel`](event.md#Event.cancel) 会返回 `Err`；
- `None`：不在表里，可能是第三方模组自己发出的事件，也可能是表还没跟上的上游新事件。这时 `cancel()` 照常写回，但没有人能替你确认它是否生效。

- 返回值类型：`Option<bool>`

### `Event::cancel` {#Event.cancel}

```rust
pub fn cancel(&mut self) -> Result<()>
```

取消这个事件。

不能取消的事件返回 `Err`，并说明应该去拦哪个事件，比如 `PlayerStartDestroyBlockEvent` 会指向 `PlayerDestroyBlockEvent`。如果返回 `()`，保护模组会以为自己拦住了，这种误会比崩溃更危险，因为崩溃至少看得见。

不在表里的事件（`can_cancel()` 为 `None`）不受阻拦：照常写回，返回 `Ok`。SDK 不假装知道自己不知道的事。

`Ok` 只表示取消位已经写回给宿主，并不表示引擎停下了：有些钩子点处于更新到一半的状态，宿主在那里根本不接受取消。这个边界只写在事件文档里。

- 返回值类型：`Result<()>`

### `Event::cancel_lenient` {#Event.cancel_lenient}

```rust
pub fn cancel_lenient(&mut self) -> bool
```

不管能不能取消，都去取消。

只有一种正当的用法：通用的转发或代理组件，事件 id 在运行时才知道，拦不住也只能接受。业务代码请用 [`Event::cancel`](event.md#Event.cancel)，并处理 `Err`。

- 返回值类型：`bool`

### `Event::uncancel` {#Event.uncancel}

```rust
pub fn uncancel(&mut self)
```

把 `cancelled` 写回 0，撤销之前的取消。

用于「先拦下、再判断、结果发现可以放行」的两段式决策。注意它只能撤销这个回调自己写的取消：别的模组在更早的优先级上做的取消撤销不了，宿主的事件总线也不允许把否决变回同意。

### `Event::set` {#Event.set}

```rust
pub fn set(&mut self, path: &str, value: NbtValue)
```

修改载荷里的一个字段。

写回的是差异：只有真正碰过的键会交还给宿主，没碰过的保持原样，所以同一个事件上的两个模组不会抹掉彼此的修改。

- 参数：
    - path : `&str`
    - value : `NbtValue`

### `Event::edit` {#Event.edit}

```rust
pub fn edit(&mut self, f: impl FnOnce(&mut NbtValue))
```

任意改写。闭包收到载荷的一份可变副本。

这是唯一会复制整个载荷的路径；[`Event::set`](event.md#Event.set) 和 [`Event::cancel`](event.md#Event.cancel) 只写它们碰过的键。

- 参数：
    - f : `impl FnOnce(&mut NbtValue)`

## `Wiring` {#Wiring}

```rust
pub struct Wiring {
    // private fields
}
```

批量订阅，并把句柄放在一起保管。

比手动订阅多两点：订阅失败不会悄无声息，失败会记在 [`Wiring::failures`](event.md#Wiring.failures) 里，`arm()` 也可以整体失败；标签会写进日志，方便找到失败的那一项。

```rust
let wiring = Wiring::new("plots")
    .on(names::PLAYER_DESTROY_BLOCK, "protect-break", |ev| { ... })
    .at(names::PLAYER_DISCONNECT, Priority::Low, "forget", |ev| { ... })
    .arm()?;                       // 任何一项失败，整体都失败

// wiring 离开作用域时，全部订阅都会取消
```

### `Wiring::new` {#Wiring.new}

```rust
pub fn new(owner: impl Into<String>) -> Wiring
```

- 参数：
    - owner : `impl Into<String>`
- 返回值类型：`Wiring`

### `Wiring::on` {#Wiring.on}

```rust
pub fn on(
        self,
        id: &str,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring
```

以 Normal 优先级添加一个订阅。`tag` 只用于日志。

- 参数：
    - id : `&str`
    - tag : `&str`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- 返回值类型：`Wiring`

### `Wiring::at` {#Wiring.at}

```rust
pub fn at(
        mut self,
        id: &str,
        priority: Priority,
        tag: &str,
        handler: impl FnMut(&mut Event<'_>) + Send + 'static,
    ) -> Wiring
```

以指定的优先级添加一个订阅。

- 参数：
    - id : `&str`
    - priority : `Priority`
    - tag : `&str`
    - handler : `impl FnMut(&mut Event<'_>) + Send + 'static`
- 返回值类型：`Wiring`

### `Wiring::arm` {#Wiring.arm}

```rust
pub fn arm(mut self) -> Result<Wiring>
```

执行订阅。任何一项失败都让整体失败，已经成功的会在返回前取消订阅，因为挂了一半的保护比没有保护更危险：有的点拦得住，有的拦不住，没有人知道是哪些。

- 返回值类型：`Result<Wiring>`
- 对应槽位：[`subscribe_event`](../cpp/events.md#subscribe_event)

### `Wiring::arm_lenient` {#Wiring.arm_lenient}

```rust
pub fn arm_lenient(mut self) -> Wiring
```

宽松的写法：失败的记录下来，成功的照常挂上。适合有了更好、没有也行的可选功能。

- 返回值类型：`Wiring`
- 对应槽位：[`subscribe_event`](../cpp/events.md#subscribe_event)

### `Wiring::failures` {#Wiring.failures}

```rust
pub fn failures(&self) -> &[(String, String)]
```

没有挂上的那些，形如 (标签, 原因)。

- 返回值类型：`&[(String, String)]`

### `Wiring::armed` {#Wiring.armed}

```rust
pub fn armed(&self) -> usize
```

挂上了多少个。

- 返回值类型：`usize`

### `Wiring::forget` {#Wiring.forget}

```rust
pub fn forget(mut self)
```

全部改为不自动取消订阅，一直存活到模组卸载。

## `PlayerIdentity` {#PlayerIdentity}

```rust
pub struct PlayerIdentity {
    pub name: String,
    pub xuid: String,
    pub uuid: String,
}
```

事件里的玩家身份。

三个字段各有各的用处，不能混用：

- `xuid` 唯一且不可更改，是权限或经济判断唯一能用的键。离线模式下可能为空。
- `uuid` 同样稳定，适合用作存档的键。
- `name` 用于显示。玩家可以改显示名，宿主解析名字时，账号名对不上就会退回到显示名（见 `bridge::resolvePlayer`），所以不能拿它当身份。

- 实现的 trait：`Debug`、`Clone`、`Default`、`PartialEq`、`Eq`、`From`

### `PlayerIdentity::selector` {#PlayerIdentity.selector}

```rust
pub fn selector(&self) -> crate::sel::PlayerSel
```

返回一个可以用来调用接口的选择器。

优先用 xuid，因为它伪造不了。xuid 为空（离线模式）时，依次退回到 uuid 和名字。退回到 `Name` 时，[`PlayerSel::is_stable`](player.md#PlayerSel.is_stable) 为 false，权限或经济判断遇到这种情况要当心，因为名字会经过显示名的回退；见 `sel` 模块。

- 返回值类型：`crate::sel::PlayerSel`

### `PlayerIdentity::is_identified` {#PlayerIdentity.is_identified}

```rust
pub fn is_identified(&self) -> bool
```

有没有可靠的身份，即 xuid 或 uuid。拿它当权限的键之前，先问一下这个。

- 返回值类型：`bool`

## `Listener` {#Listener}

```rust
pub struct Listener {
    // private fields
}
```

订阅句柄。丢弃它就会取消订阅。

[`Listener::forget`](event.md#Listener.forget) 让它一直存活到模组卸载。卸载时宿主会移除剩下的订阅，真的调用 `removeListener`，失败了会报告，不会不声不响。

- 实现的 trait：`Drop`

### `Listener::event_id` {#Listener.event_id}

```rust
pub fn event_id(&self) -> &str
```

所订阅的事件的 id。

- 返回值类型：`&str`

### `Listener::forget` {#Listener.forget}

```rust
pub fn forget(mut self)
```

放弃自动取消订阅。闭包随之泄漏，一直存活到进程结束。

## `Priority` {#Priority}

```rust
pub enum Priority {
        Highest = 0,
        High = 1,
        #[default]
        Normal = 2,
        Low = 3,
        Lowest = 4,
}
```

分发的优先级。值越小越先运行，和 ABI 里的 0 到 4 对齐。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`、`PartialOrd`、`Ord`、`Default`

## `EventRef` {#EventRef}

```rust
pub type EventRef<'a> = Event<'a>;
```

早先的一代管它叫 `EventRef`。这个名字保留着，指向同一个类型。

## `EventPriority` {#EventPriority}

```rust
pub type EventPriority = Priority;
```

早先的一代把优先级叫作 `EventPriority`。
