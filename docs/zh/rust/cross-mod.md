# 跨模组通信

三条通道，对应三种形状的问题。

| | 形状 | 有返回 | 名字 |
|---|---|---|---|
| **服务** | 一对一 | 有 | 独占 |
| **总线** | 一对多 | 无 | 共享 |
| **车道** | 一对一，同工具链 | 有，原生调用 | 独占 |

## 服务：问一个问题，拿一个答案

```rust
use levilamina::service;

// 提供方
let _reg = service::register("plot:owner", |_name, req| {
    let plot: PlotQuery = serde_json::from_str(req).map_err(|e| e.to_string())?;
    Ok(lookup(plot))
})?;

// 调用方
let owner: Owner = service::call_with("plot:owner", &query)?;
```

`call_with` 替你序列化请求、反序列化应答；请求已经是字符串时用 `call_json` 只做后半段。
两者都会把一个形状不对的应答变成真的错误，而不是一个看起来没问题的值。

错误是**分类**的，因为要做的事不一样：

| | 含义 |
|---|---|
| `NotFound` | 对方没装、没启用，或者名字写错了 |
| `Provider` | 它跑了并且拒绝了，消息是提供方给的 |
| `Refused` | 名字不合法、自己调自己，或者成环了 |
| `Decode` | 应答解析不成你要的类型 |
| `Unavailable` | 这个宿主没有服务能力 |

`call_optional` 在 `NotFound` 时返回 `None`、其余照常报错，这正是「可选集成」想要的形状。

服务名是独占的：第二个提供方会被拒绝，宿主日志里写着当前持有者是谁。
两个提供方会让答案取决于模组装载顺序。

::: tip
模组被停用期间服务仍然可达，因为 LeviLamina 是在所有 `on_load` 跑完之后才启用模组的。
在那个窗口里不可达，会让每一个在自己 `on_load` 里解析依赖的消费方全部失败。
:::

## 总线：告诉所有人

```rust
use levilamina::bus;

let _sub = bus::subscribe("plot:claimed", |topic, payload| {
    // 返回 true 是否决，只有 publish_vetoable 会去看它
    false
})?;

let n = bus::publish("plot:claimed", &payload)?;
```

**你收不到自己发的。** 想通知自己就是一次直接函数调用，而自发自收是唯一一种
任何深度限制都分辨不出来的环。跨模组的环由深度上限接住，撞上限时最内层那次 publish
被丢弃并打日志。

回调跑在**发布方**的线程上，所以要碰世界状态先 schedule 回去。

## 车道：跳过序列化

车道把裸的 `data` 和 `vtable` 指针交出去，让同一套工具链编出来的两个模组直接互相调用。
只有指纹对得上才成立，对不上就退回服务通道：

```rust
match lane::acquire::<MyContract>() {
    Ok(lane) => lane.with(|table, data| unsafe { (table.count)(data) }),
    Err(_) => service::call("plot:count", "")?.parse().ok(),
}
```

车道名和指纹都来自 `LaneContract`，所以 `acquire` 除了类型不收别的参数。
`with` 把表和提供方自己的 `data` 指针一起交给闭包，提供方没了就返回 `None`。

表的形状一变，`LaneContract::FINGERPRINT` 就必须跟着变。两侧引用同一份契约定义时
它自然一致——而靠双方各抄一份相同的常量，正是它要防的那种情况。

**只有在服务通道实测太慢时才用车道。** 它拿安全换速度。
