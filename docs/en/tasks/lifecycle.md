# 🔄 Lifecycle and logging API

From the moment the server finds your mod to the moment it is unloaded, a mod goes through four stages: **load → enable → disable → unload**. You do each stage's work in its function, and Pier calls the functions at the right time.

### Declaring a mod

#### Declaring a mod

Rust: `levilamina::register_mod!(MyMod)`  
Go: `levilamina.Register(name, mod)`  
Zig: `levilamina.exportMod(MyMod, name)`  

Tells Pier which type your mod is; Pier calls the four stages through it. This exports `pier_main`, the entry function the server looks for; without it the mod is refused at load.

- Parameters:
    - MyMod / mod : type  
      Rust implements `LeviMod`; Go implements the `Mod` interface (`Enable`, `Disable`, optionally `Load`, `Unload`); Zig declares whichever of `load`, `enable`, `disable`, `unload` it needs
    - name : string  
      (Go, Zig) the mod's name in the log
    - In Go, call `Register` from an `init` function: when the server calls `pier_main`, the Go runtime has run every `init`.
- Example:
    - Rust

      ```rust title="Rust"
      use levilamina::prelude::*;

      struct MyMod;

      impl LeviMod for MyMod {
          fn on_load(ctx: &ModContext) -> Result<Self> {
              ctx.logger().info("the mod loaded");
              Ok(MyMod)
          }
      }

      levilamina::register_mod!(MyMod);
      ```

    - Go

      ```go title="Go"
      type myMod struct{}

      func (m *myMod) Enable(ctx *levilamina.Context) error  { return nil }
      func (m *myMod) Disable(ctx *levilamina.Context) error { return nil }

      func init() {
      	levilamina.Register("my-mod", &myMod{})
      }

      func main() {}
      ```

    - Zig

      ```zig title="Zig"
      const levilamina = @import("levilamina");

      const MyMod = struct {
          pub fn enable(ctx: *levilamina.Context) !void {
              ctx.info("the mod is enabled");
          }
      };

      comptime {
          levilamina.exportMod(MyMod, "my-mod");
      }
      ```

The four stages:

| Stage | Rust | Go | Zig | When |
|---|---|---|---|---|
| Load | `on_load` | `Load` (optional) | `load` | when the server finds your mod |
| Enable | `on_enable` | `Enable` | `enable` | after loading, to start working |
| Disable | `on_disable` | `Disable` | `disable` | at shutdown, or on `/pier disable` |
| Unload | `on_unload` | `Unload` (optional) | `unload` | before the mod is removed |

They all run on the server thread. Returning an error makes Pier log it and refuse the step: a mod that fails to enable stays disabled. Register subscriptions and commands while enabling and undo them while disabling, so `/pier disable` followed by `/pier enable` starts the mod over.

!!! warning "Don't let a panic escape your function"

    Pier catches a Rust or Go panic at the boundary and logs it. Zig does not unwind: a panic ends the whole server process, so return errors instead.

### Logging

#### Logging a line

Rust: `ctx.logger().info(msg)`  
Go: `ctx.Logger().Info(msg)`  
Zig: `ctx.info(msg)`  

A log line carries your mod's name and goes to the server console and log file. There are warn and error levels too; where there is no `ctx`, such as in a callback, Rust uses `Logger::get()`, Go `levilamina.Logger{}`, Zig `levilamina.log(.info, msg)`.

- Parameters:
    - msg : string  
      what to write
    - Callable from any thread.
- Slot: `log`
- Example:
    - Rust

      ```rust title="Rust"
      ctx.logger().info("plain information");
      Logger::get().warn("logging from a callback works too");
      ```

    - Go

      ```go title="Go"
      ctx.Logger().Info("plain information")
      levilamina.Logger{}.Warn("logging from a callback works too")
      ```

    - Zig

      ```zig title="Zig"
      ctx.info("plain information");
      levilamina.logf(.warn, "players: {d}", .{count});
      ```
