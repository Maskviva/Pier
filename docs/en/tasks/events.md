# 📣 Events API

Whatever happens on the server, a player chatting, a block breaking, a mob dying, Pier tells you about as an "event". Subscribe to one, and your function runs when it happens.

### Subscribing

#### Subscribing to an event

Rust: `event::subscribe(id, handler)`  
Go: `levilamina.Subscribe(id, priority, handler)`  
Zig: `levilamina.subscribe(id, priority, handler)`  

- Parameters:
    - id : string  
      The event name. A LeviLamina event by its full name, such as `ll::event::PlayerChatEvent`; one of Pier's own by its name, such as `PlayerAttackTargetEvent`. In Rust, the constants of `names::` turn a typo into a compile error.
    - priority : priority  
      (Go, Zig) who receives the event first: highest, high, normal, low, lowest. Rust sets it with `event::subscribe_with`
    - handler : function  
      runs when the event happens, on the server thread
- Return value: a listener; keep it to unsubscribe later
- Return type: Rust `Result<Listener>`, Go `(*Listener, error)`, Zig `levilamina.Error!Listener`
- Slot: `subscribe_event`
- Example:
    - Rust

      ```rust title="Rust"
      use levilamina::event::{self, names};

      let listener = event::subscribe(names::PLAYER_CHAT, |ev| {
          if let Ok(msg) = ev.str_at("message") {
              Logger::get().info(&format!("someone said: {msg}"));
          }
      })?;
      ```

    - Go

      ```go title="Go"
      listener, err := levilamina.Subscribe("ll::event::PlayerChatEvent", levilamina.PriorityNormal, func(ev *levilamina.Event) {
      	payload, err := ev.Payload()
      	if err != nil {
      		return
      	}
      	if msg, ok := payload.OptString("message"); ok {
      		levilamina.Logger{}.Info("someone said: " + msg)
      	}
      })
      ```

    - Zig

      ```zig title="Zig"
      const listener = try levilamina.subscribe("ll::event::PlayerChatEvent", .normal, onChat);

      fn onChat(ev: *const levilamina.Event) void {
          levilamina.logf(.info, "chat event: {s}", .{ev.snbt});
      }
      ```

!!! tip "Subscribed and nothing happens?"

    Run `/pier events` in the server console and look for your name among the events this server can subscribe to.

#### Unsubscribing

Rust: `drop(listener)`  
Go: `listener.Unsubscribe()`  
Zig: `listener.unsubscribe()`  

In Rust, dropping the listener unsubscribes; Go and Zig call the function. Unsubscribing twice is harmless.

- Return value: nothing on success
- Return type: Go `error`, Zig `levilamina.Error!void`
- Slot: `unsubscribe_event`
- Example:
    - Rust

      ```rust title="Rust"
      drop(listener);
      ```

    - Go

      ```go title="Go"
      err := listener.Unsubscribe()
      ```

    - Zig

      ```zig title="Zig"
      try listener.unsubscribe();
      ```

### Reading the payload

The payload is SNBT text; which fields each event has is in the [event payload reference](../guide/event-payloads.md).

#### Reading a field

Rust: `ev.str_at(path)`  
Go: `ev.Payload()`  
Zig: `levilamina.nbt.parse(arena, ev.snbt)`  

- Parameters:
    - path : string  
      the field's path, nested fields joined with dots, such as `_player.name`
- Return value: the field's value. Go and Zig take the whole parsed payload, then read by path
- Return type: Rust `Result<&str>` (also `bool_at`, `opt_str` and so on), Go `(*Nbt, error)`, Zig `nbt.ParseError!nbt.Value`
- Example:
    - Rust

      ```rust title="Rust"
      let name = ev.str_at("_player.name")?;
      ```

    - Go

      ```go title="Go"
      payload, err := ev.Payload()
      name, ok := payload.OptString("_player.name")
      ```

    - Zig

      ```zig title="Zig"
      const payload = try levilamina.nbt.parse(arena, ev.snbt);
      const name = payload.getString("_player.name");
      ```

#### The dimension it happened in

Rust: `ev.dim()`  
Go: `ev.Dim()`  
Zig: `payload.getInt("dim")`  

- Return value: the dimension id
- Return type: Rust `Result<i32>`, Go `(int32, error)`, Zig `?i64`
    - When the host could not resolve an actor of the event, the payload has an `_unresolved` field and `dim()` / `Dim()` return an error. A protection decision on such a payload should refuse.
- Example:
    - Rust

      ```rust title="Rust"
      let dim = ev.dim()?;
      ```

    - Go

      ```go title="Go"
      dim, err := ev.Dim()
      ```

    - Zig

      ```zig title="Zig"
      const dim = payload.getInt("dim") orelse return;
      ```

!!! info "Who killed whom"

    In death and damage events, `attackerUid` in the damage source is the actor responsible; for an arrow kill, **the shooter**. `attacker` describes it while it still exists. See the [event payload reference](../guide/event-payloads.md#who-killed-whom).

### Changing an event

#### Cancelling an event

Rust: `ev.cancel()`  
Go: `ev.Cancel()`  
Zig: `ev.cancel()`  

Cancel from the handler to stop the thing from happening, such as a block being broken.

- Return value: Rust returns an error with the reason for an event that cannot be cancelled
- Return type: Rust `Result<()>`, Go and Zig none
    - When a mod ahead of you already cancelled the event, it stays cancelled; you cannot change it back.
- Example:
    - Rust

      ```rust title="Rust"
      event::subscribe(names::PLAYER_DESTROY_BLOCK, |ev| {
          if forbidden(ev) {
              let _ = ev.cancel();
          }
      })?;
      ```

    - Go

      ```go title="Go"
      levilamina.Subscribe("ll::event::PlayerDestroyBlockEvent", levilamina.PriorityNormal, func(ev *levilamina.Event) {
      	if forbidden(ev) {
      		ev.Cancel()
      	}
      })
      ```

    - Zig

      ```zig title="Zig"
      fn onDestroy(ev: *const levilamina.Event) void {
          if (forbidden(ev)) ev.cancel();
      }
      ```

#### Writing fields back

Go: `ev.Set(key, value)`  
Zig: `ev.writeBack(snbt)`  

Writes changed fields back into the event. Only the changed keys go back and the host merges them, so two mods changing different keys keep both changes.

- Parameters:
    - key, value : a key and an NBT value (Go)  
      the field and its new value
    - snbt : string (Zig)  
      SNBT of the changed fields only, such as `{cancelled:1b}`
- Example:
    - Go

      ```go title="Go"
      ev.Set("message", levilamina.NbtStringOf("(filtered)"))
      ```

    - Zig

      ```zig title="Zig"
      ev.writeBack("{message:\"(filtered)\"}");
      ```
