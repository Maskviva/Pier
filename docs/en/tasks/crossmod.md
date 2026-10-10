# 🔗 Talking to other mods API

Through Pier your mod talks to mods of any language on the server, and LeviLamina native plugins that are not Pier mods join in through the [bridge](../guide/bridge.md). Both sides pass strings in a format you agree on, JSON or SNBT; Pier does not look inside.

| | Shape | A reply? | Name |
|---|---|---|---|
| **Service** | ask and answer | yes | one provider per name |
| **Bus** | one speaks, everyone listens | no, but listeners can veto | anyone subscribes to a topic |

### Services

#### Providing a service

Rust: `service::register(name, provider)`  
Go: `levilamina.RegisterService(name, provider)`  
Zig: `levilamina.registerService(name, provider)`  

- Parameters:
    - name : string  
      the service name; namespace it, as `plot:owner`
    - provider : function  
      takes the request and returns the reply, or an error whose message reaches the caller unchanged, so it can tell "no such plot" from "the database is down"
- Return value: the registration; keep it to withdraw the service
- Return type: Rust `Result<Registration>`, Go `(*ServiceRegistration, error)`, Zig `levilamina.Error!ServiceRegistration`
    - The provider runs on the caller's thread, and is still called while your mod is disabled but loaded, since other mods may need it in their own load stage.
- Slot: `service_register`
- Example:
    - Rust

      ```rust title="Rust"
      let reg = service::register("plot:owner", |_name, request| {
          match owners.get(request) {
              Some(owner) => Ok(owner.clone()),
              None => Err(format!("no plot {request}")),
          }
      })?;
      ```

    - Go

      ```go title="Go"
      reg, err := levilamina.RegisterService("plot:owner", func(request string) (string, error) {
      	owner, ok := owners[request]
      	if !ok {
      		return "", fmt.Errorf("no plot %s", request)
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
          reply.send("no such plot");
          return false;
      }
      ```

#### Calling another mod's service

Rust: `service::call(name, request)`  
Go: `levilamina.CallService(name, request)`  
Zig: `levilamina.callService(allocator, name, request)`  

- Parameters:
    - name : string  
      the service name
    - request : string  
      the request
- Return value: the reply
- Return type: Rust `CallResult<String>`, Go `(string, error)` (the error is a `*CallError`), Zig `levilamina.Error!CallOutcome`
    - A failure is one of four: not found (nobody provides the name), provider refused (with its reason), host refused (a bad name, a call to yourself, or a cycle), unavailable (this Pier has no services).
    - For an optional integration: Rust `service::call_optional`, Go `levilamina.CallServiceOptional`.
- Slot: `service_call`
- Example:
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

#### Who is calling

Rust: `service::caller()`  
Go: `levilamina.ServiceCaller()`  
Zig: `levilamina.serviceCaller(allocator)`  

The request can claim anything. Inside the provider, ask the host which mod's call is running.

- Return value: the calling mod's name; absent for a native plugin through the bridge
- Return type: Rust `Option<String>`, Go `(string, bool)`, Zig `levilamina.Error!?[]u8`
- Slot: `service_caller`
- Example:
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

### The bus

#### Subscribing to a topic

Rust: `bus::subscribe(topic, handler)`  
Go: `levilamina.BusSubscribe(topic, handler)`  
Zig: `levilamina.busSubscribe(topic, handler)`  

- Parameters:
    - topic : string  
      the topic; namespace it, as `plot:enter`
    - handler : function  
      runs on the publisher's thread for each message. It returns a veto: `true` objects, and only a vetoable publish reads it
- Return value: the subscription; keep it to unsubscribe
- Return type: Rust `Result<Subscription>`, Go `(*Subscription, error)`, Zig `levilamina.Error!Subscription`
    - When a subscriber panics, Pier counts it as no opinion and the publish goes on.
- Slot: `bus_subscribe`
- Example:
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

#### Publishing a message

Rust: `bus::publish(topic, payload)`  
Go: `levilamina.BusPublish(topic, payload)`  
Zig: `levilamina.busPublish(topic, payload)`  

- Parameters:
    - topic : string  
      the topic
    - payload : string  
      the message
- Return value: how many subscribers received it. 0 means nobody is listening right now; the publish itself succeeded
- Return type: Rust `Result<u32>`, Go `(uint32, error)`, Zig `levilamina.Error!u32`
- Slot: `bus_publish`
- Example:
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

#### Asking everyone first

Rust: `bus::publish_vetoable(topic, payload)`  
Go: `levilamina.BusPublishVetoable(topic, payload)`  
Zig: `levilamina.busPublishVetoable(topic, payload)`  

- Parameters:
    - topic : string  
      the topic
    - payload : string  
      the message
- Return value: whether anyone objected, and how many received it
- Return type: Rust `Result<Vetoable>`, Go `(Vetoable, error)`, Zig `levilamina.Error!Vetoable`
    - One subscriber returning `true` makes the result "vetoed"; the other subscribers still receive the message.
- Slot: `bus_publish_vetoable`
- Example:
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
