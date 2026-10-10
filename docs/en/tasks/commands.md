# ⌨️ Commands API

Want players or the operator to type `/hello` and reach your feature? Register a command.

### Registering commands

#### Registering a command

Rust: `command::register(name, description, permission, handler)`  
Go: `levilamina.RegisterCommand(name, description, permission, handler)`  
Zig: `levilamina.registerCommand(name, description, permission, handler)`  

- Parameters:
    - name : string  
      the command name, without the slash
    - description : string  
      shown in the game's command hints
    - permission : permission  
      who may use it: anyone, game directors, admins, the host, the owner, lowest to highest
    - handler : function  
      runs whenever someone runs the command, on the server thread. The text after the command: Rust `inv.raw()`, Go `inv.Args`, Zig `inv.args`
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`, Zig `levilamina.Error!void`
    - Register while enabling. Bedrock cannot unregister a command, so it stays until the server stops; while your mod is disabled, Pier mutes its commands.
- Slot: `register_command`
- Example:
    - Rust

      ```rust title="Rust"
      use levilamina::command::{self, CommandPermission};

      command::register("hello", "Say hello", CommandPermission::Any, |inv| {
          inv.success(&format!("Hello, {}!", inv.origin().name));
      })?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.RegisterCommand("hello", "Say hello", levilamina.PermissionAny, func(inv *levilamina.Invocation) {
      	inv.Success("Hello, " + inv.Origin + "!")
      })
      ```

    - Zig

      ```zig title="Zig"
      try levilamina.registerCommand("hello", "Say hello", .any, onHello);

      fn onHello(inv: *const levilamina.Invocation) void {
          inv.success("Hello!");
      }
      ```

#### A command with parameters

Rust: `command::builder(name, description, permission).overload(..).register(handler)`  
Go: `levilamina.NewCommand(name, description, permission).Overload(..).Register(handler)`  
Zig: `levilamina.slot("register_command_ex")`  

Lets the game parse the parameters, with completion in the chat box. Each overload is one combination of parameters.

- Parameters:
    - overload : overload  
      required parameters, optional ones, literal words and enum parameters, in order
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
- Slot: `register_command_ex`
- Example:
    - Rust

      ```rust title="Rust"
      command::builder("plot", "Plot management", CommandPermission::Any)
          .overload(|o| o.required("action", ParamType::String).optional("target", ParamType::Player))
          .register(|inv| {
              match inv.arg_str("action").unwrap_or("") {
                  "claim" => inv.success("claimed"),
                  _ => inv.error("unknown action"),
              }
          })?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.NewCommand("plot", "Plot management", levilamina.PermissionAny).
      	Overload(levilamina.NewOverload().Required("action", levilamina.ParamString).Optional("target", levilamina.ParamPlayer)).
      	Register(func(call *levilamina.CommandCall) {
      		if action, _ := call.ArgString("action"); action == "claim" {
      			call.Success("claimed")
      			return
      		}
      		call.Error("unknown action")
      	})
      ```

!!! warning "Two overloads must not accept the same input"

    When two overloads both match what a player typed, the game picks one. Words that mean different things but take the same parameters, such as `menu`, `help` and `status`, belong in **one enum parameter**.

#### Registering a command enum

Rust: `command::register_enum(name, values)`  
Go: `levilamina.RegisterCommandEnum(name, values)`  
Zig: `levilamina.slot("register_command_enum")`  

- Parameters:
    - name : string  
      the enum's name, by which an overload refers to it
    - values : a list of names and values  
      every value of the enum
- Return value: nothing on success
- Return type: Rust `Result<()>`, Go `error`
- Slot: `register_command_enum`
- Example:
    - Rust

      ```rust title="Rust"
      command::register_enum("plot_simple", &[("menu", 0), ("help", 1), ("status", 2)])?;
      ```

    - Go

      ```go title="Go"
      err := levilamina.RegisterCommandEnum("plot_simple", []levilamina.EnumValue{{Name: "menu", Value: 0}, {Name: "help", Value: 1}, {Name: "status", Value: 2}})
      ```

### Running commands

#### Running a command as the console

Rust: `ctx.host().execute_command(cmd)`  
Go: `levilamina.ExecuteCommand(cmd)`  
Zig: `levilamina.executeCommand(cmd)`  

- Parameters:
    - cmd : string  
      the command, without the slash
- Return value: the command's output. Zig logs the output at debug level
- Return type: Rust `Result<String>`, Go `([]CommandOutput, error)`, Zig `levilamina.Error!void`
    - Server thread only; before the level has loaded it returns an error.
- Slot: `execute_command`
- Example:
    - Rust

      ```rust title="Rust"
      let output = ctx.host().execute_command("time set day")?;
      ```

    - Go

      ```go title="Go"
      lines, err := levilamina.ExecuteCommand("time set day")
      ```

    - Zig

      ```zig title="Zig"
      try levilamina.executeCommand("time set day");
      ```
