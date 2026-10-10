# 🔗 跨模组通信 API

通过 Pier，你的模组可以和服务器上任何语言写的模组对话；不是 Pier 模组的 LeviLamina 原生插件，也可以通过 [bridge](../guide/bridge.md) 加入进来。两边传的都是字符串，格式由你们约定（JSON、SNBT 都行），Pier 不看里面的内容。

| | 形状 | 有回复吗 | 名字 |
|---|---|---|---|
| **服务** | 一问一答 | 有 | 一个名字只能有一个提供者 |
| **总线** | 一个说，大家听 | 没有，但听众可以否决 | 谁都能订阅同一个主题 |

### 服务

#### 提供一个服务

Rust：`service::register(name, provider)`  
Go：`levilamina.RegisterService(name, provider)`  
Zig：`levilamina.registerService(name, provider)`  

- 参数：
    - name : 字符串  
      服务名，建议加命名空间，比如 `plot:owner`
    - provider : 函数  
      收到请求，返回回复；或者返回一个错误，错误信息会原样交给调用方，对方据此分清「没有这块地」和「数据库挂了」
- 返回值：服务的登记，留着它用来撤销
- 返回值类型：Rust `Result<Registration>`，Go `(*ServiceRegistration, error)`，Zig `levilamina.Error!ServiceRegistration`
    - 提供函数运行在调用方的线程上。你的模组被禁用但还没卸载时，它仍然会被调用，因为别的模组可能在自己的加载阶段就要用你的服务。
- 对应槽位：`service_register`
- 示例：
    - Rust

      ```rust title="Rust"
      let reg = service::register("plot:owner", |_name, request| {
          match owners.get(request) {
              Some(owner) => Ok(owner.clone()),
              None => Err(format!("没有地皮 {request}")),
          }
      })?;
      ```

    - Go

      ```go title="Go"
      reg, err := levilamina.RegisterService("plot:owner", func(request string) (string, error) {
      	owner, ok := owners[request]
      	if !ok {
      		return "", fmt.Errorf("没有地皮 %s", request)
      	}
      	return owner, nil
      })
      ```

    - Zig

      ```zig title="Zig"
      var reg = try levilamina.registerService("plot:owner", provideOwner);

      fn provideOwner(request: []const u8, reply: *const levilamina.Reply) bool {
          if (lookup(request)) |owner| {
              reply.send(owner);
              return true;
          }
          reply.send("没有这块地皮");
          return false;
      }
      ```

#### 调用别人的服务

Rust：`service::call(name, request)`  
Go：`levilamina.CallService(name, request)`  
Zig：`levilamina.callService(allocator, name, request)`  

- 参数：
    - name : 字符串  
      服务名
    - request : 字符串  
      请求内容
- 返回值：对方的回复
- 返回值类型：Rust `CallResult<String>`，Go `(string, error)`（错误是 `*CallError`），Zig `levilamina.Error!CallOutcome`
    - 失败分四种：找不到（没人提供这个名字）、提供方拒绝（附带它给的原因）、宿主拒绝（名字不合法、调用了自己，或形成了循环）、不支持（这个 Pier 没有服务功能）。
    - 对方没装也能工作的可选集成：Rust 用 `service::call_optional`，Go 用 `levilamina.CallServiceOptional`。
- 对应槽位：`service_call`
- 示例：
    - Rust

      ```rust title="Rust"
      match service::call("plot:owner", "12,7") {
          Ok(owner) => Logger::get().info(&owner),
          Err(e) => Logger::get().warn(&e.to_string()),
      }
      ```

    - Go

      ```go title="Go"
      owner, err := levilamina.CallService("plot:owner", "12,7")
      var ce *levilamina.CallError
      if errors.As(err, &ce) && ce.Kind == levilamina.CallProvider {
      	levilamina.Logger{}.Warn(ce.Message)
      }
      ```

    - Zig

      ```zig title="Zig"
      const outcome = try levilamina.callService(allocator, "plot:owner", "12,7");
      defer allocator.free(outcome.body);
      if (outcome.code == .ok) levilamina.logf(.info, "{s}", .{outcome.body});
      ```

#### 获取调用方

Rust：`service::caller()`  
Go：`levilamina.ServiceCaller()`  
Zig：`levilamina.serviceCaller(allocator)`  

请求内容可以写任何东西。在提供函数里，向宿主询问正在运行的是哪个模组的调用。

- 返回值：调用方模组的名字。调用方是原生插件（通过 bridge）时为「没有」
- 返回值类型：Rust `Option<String>`，Go `(string, bool)`，Zig `levilamina.Error!?[]u8`
- 对应槽位：`service_caller`
- 示例：
    - Rust

      ```rust title="Rust"
      let caller = service::caller();
      ```

    - Go

      ```go title="Go"
      caller, ok := levilamina.ServiceCaller()
      ```

    - Zig

      ```zig title="Zig"
      const caller = try levilamina.serviceCaller(allocator);
      ```

### 总线

#### 订阅一个主题

Rust：`bus::subscribe(topic, handler)`  
Go：`levilamina.BusSubscribe(topic, handler)`  
Zig：`levilamina.busSubscribe(topic, handler)`  

- 参数：
    - topic : 字符串  
      主题名，建议加命名空间，比如 `plot:enter`
    - handler : 函数  
      收到消息时调用，运行在发布方的线程上。返回值是否决票：`true` 表示反对，只有可否决的发布会看它
- 返回值：订阅，留着它用来退订
- 返回值类型：Rust `Result<Subscription>`，Go `(*Subscription, error)`，Zig `levilamina.Error!Subscription`
    - 订阅者崩溃（panic）时，Pier 按「没有意见」处理，这次发布照常进行。
- 对应槽位：`bus_subscribe`
- 示例：
    - Rust

      ```rust title="Rust"
      let sub = bus::subscribe("plot:enter", |_topic, payload| {
          Logger::get().info(payload);
          false
      })?;
      ```

    - Go

      ```go title="Go"
      sub, err := levilamina.BusSubscribe("plot:enter", func(topic, payload string) bool {
      	levilamina.Logger{}.Info(payload)
      	return false
      })
      ```

    - Zig

      ```zig title="Zig"
      var sub = try levilamina.busSubscribe("plot:enter", onEnter);

      fn onEnter(_: []const u8, payload: []const u8) bool {
          levilamina.log(.info, payload);
          return false;
      }
      ```

#### 发布一条消息

Rust：`bus::publish(topic, payload)`  
Go：`levilamina.BusPublish(topic, payload)`  
Zig：`levilamina.busPublish(topic, payload)`  

- 参数：
    - topic : 字符串  
      主题名
    - payload : 字符串  
      消息内容
- 返回值：有多少订阅者收到了。返回 0 表示现在没人在听，发布本身是成功的
- 返回值类型：Rust `Result<u32>`，Go `(uint32, error)`，Zig `levilamina.Error!u32`
- 对应槽位：`bus_publish`
- 示例：
    - Rust

      ```rust title="Rust"
      let heard = bus::publish("plot:enter", r#"{"player":"Steve"}"#)?;
      ```

    - Go

      ```go title="Go"
      heard, err := levilamina.BusPublish("plot:enter", `{"player":"Steve"}`)
      ```

    - Zig

      ```zig title="Zig"
      const heard = try levilamina.busPublish("plot:enter", "{\"player\":\"Steve\"}");
      ```

#### 先问问大家同不同意

Rust：`bus::publish_vetoable(topic, payload)`  
Go：`levilamina.BusPublishVetoable(topic, payload)`  
Zig：`levilamina.busPublishVetoable(topic, payload)`  

- 参数：
    - topic : 字符串  
      主题名
    - payload : 字符串  
      消息内容
- 返回值：有没有人反对，以及有多少订阅者收到了
- 返回值类型：Rust `Result<Vetoable>`，Go `(Vetoable, error)`，Zig `levilamina.Error!Vetoable`
    - 只要有一个订阅者返回 `true`，结果就是「被否决」；被否决后，其余订阅者照样会收到消息。
- 对应槽位：`bus_publish_vetoable`
- 示例：
    - Rust

      ```rust title="Rust"
      let result = bus::publish_vetoable("plot:delete", "12,7")?;
      if result.vetoed {
          // ...
      }
      ```

    - Go

      ```go title="Go"
      result, err := levilamina.BusPublishVetoable("plot:delete", "12,7")
      if result.Vetoed {
      	// ...
      }
      ```

    - Zig

      ```zig title="Zig"
      const result = try levilamina.busPublishVetoable("plot:delete", "12,7");
      if (result.vetoed) {
          // ...
      }
      ```
