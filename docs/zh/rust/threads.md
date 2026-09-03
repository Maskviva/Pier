# 线程

## 默认

每个槽位**只能在服务器线程上调**，例外逐槽注明。每个回调都在服务器线程上触发，
下面那一个例外除外。

实际上就是：只要你在生命周期回调、事件处理函数、命令处理函数或排期任务里，
你就在服务器线程上，什么都能用。

## 怎么回到服务器线程

```rust
use levilamina::Host;

std::thread::spawn(|| {
    let data = expensive_lookup();
    Host::get().schedule(move || {
        // 现在在服务器线程上
        Host::get().execute_command(&format!("say {data}"));
    });
});
```

`schedule` 和 `schedule_after` 是线程安全的，`Logger` 也是。

`schedule_after` 收的是 `Duration` 而不是一个毫秒数，这样调用点上一眼看得出单位：

```rust
host.schedule_after(Duration::from_secs(5), || { ... })?;
```

## 哪些可以从任意线程调

| | |
|---|---|
| `Host::schedule`、`Host::schedule_after`、`Host::cancel` | 契约标了线程安全 |
| `Logger` | 契约标了线程安全，而且是 `Copy` |
| `KvDb` | 宿主内部有锁 |
| `Host::gaming_status` | 契约标了线程安全 |

其余一律服务器线程。

## 唯一的例外：数据包拦截

数据包拦截器**不在服务器线程上跑**。入站回调跑在抽水那条连接的线程上，
出站回调跑在发起发送的那条线程上。通常那就是服务器线程，但一次异步 flush
就不是了，而且**同一个闭包可能被多条线程同时进入**。

这就是那里的闭包约束是 `Send + Sync` 而不是 `Send` 的原因。
**不要在数据包回调里碰世界状态**，先 `Host::get().schedule()` 回到服务器线程。

## 跨线程持有句柄

模组结构体必须是 `Send`，因为生命周期回调可能在不同线程上进入。
那些 RAII 句柄都是 `Send`，所以结构体可以持有它们：

```rust
struct MyMod {
    chat: Option<Listener>,
    hook: Option<PacketHook>,
}
```

它们不是 `Sync`，这是个诚实的约束：里面装的闭包只保证 `Send`。
