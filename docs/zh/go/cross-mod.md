# 跨模组通信

Go 模组可以和服务器上的任何其他模组通信，不管对方用什么语言写；也能通过 [bridge](../guide/bridge.md) 和 LeviLamina 原生插件通信。
两种通道传的都是双方约定格式的 UTF-8 字符串（JSON 或 SNBT），宿主从不看里面的内容，所以 Go 的提供方和 Rust 的调用方只需要约定同一种格式。

| | 形状 | 有返回 | 名字 |
|---|---|---|---|
| **服务** | 一对一 | 有 | 独占 |
| **总线** | 一对多 | 没有，但订阅者可以否决 | 共享 |

## 服务：问一个问题，得到一个回答

```go
reg, err := levilamina.RegisterService("plot:owner", func(request string) (string, error) {
	owner, ok := owners[request]
	if !ok {
		return "", fmt.Errorf("no plot %s", request)
	}
	return owner, nil
})
```

回答会交给调用方。返回的 error，其消息会原样作为原因交给调用方，调用方就是靠它分清"没有这块地"和"数据库挂了"。
提供方里发生 panic 时，Go 绑定在边界上接住它，把 panic 的内容当作错误信息交给调用方。

提供方在调用方的线程上运行；本模组已禁用但还没卸载时也会被调用，因为调用方会在自己的 `on_load` 里解析服务，
而那时还没有任何模组被启用。用 `reg.Unregister()` 撤销。

调用服务：

```go
owner, err := levilamina.CallService("plot:owner", "12,7")
var ce *levilamina.CallError
if errors.As(err, &ce) && ce.Kind == levilamina.CallProvider {
	log.Warn("the plot mod said no: " + ce.Message)
}
```

`CallError.Kind` 有四种：`CallNotFound` 表示没有模组提供这个名字，`CallProvider` 表示提供方拒绝了，
`CallRefused` 表示宿主拒绝了这次调用，`CallUnavailable` 表示宿主不支持服务。
`CallServiceOptional` 在没人提供时返回 found 为 false 而不是 error，适合"没有对方模组也能工作"的集成。
`ListServices` 和 `ServiceExists` 能查到当前注册了哪些服务。

### 谁在问

请求文本可以声称任何东西。在提供方内部，`ServiceCaller()` 向宿主询问正在运行的是哪个模组的调用；
调用方没有模组时（比如通过 bridge 调用的原生插件）它返回 false，提供方由此知道无法确认请求来自谁。

## 总线：告诉所有人

```go
sub, err := levilamina.BusSubscribe("plot:enter", func(topic, payload string) bool {
	log.Info("someone entered a plot: " + payload)
	return false // 不否决
})

delivered, err := levilamina.BusPublish("plot:enter", `{"player":"Steve","plot":"12,7"}`)
```

`BusPublish` 返回有多少订阅者运行了；0 表示没人在听，这不是错误。订阅者在发布方的线程上运行。主题要加命名空间，比如 `plot:enter`。

`BusPublishVetoable` 会先征求意见：订阅者返回 **true** 表示拒绝，结果里的 `Vetoed` 说明有没有人拒绝。
订阅者只能拒绝，不能推翻别人的拒绝，而且每个订阅者都会收到消息。订阅者 panic 时不算否决，这和 Rust 绑定一致：
否决是更强的动作，不应由 bug 触发。

## Lane

Lane 在同一工具链构建的两个模组之间直接传递函数表，是服务之外的一条快速通道。它的指纹包含编译器信息，
所以 Go 模组永远匹配不上 Rust 或 C++ 模组，Go 绑定也就不提供 Lane。
发布 Lane 的模组同时也提供服务，指纹对不上的调用方会退回去调这个服务，Go 模组走的就是这条路。
