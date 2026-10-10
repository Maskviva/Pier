# Commands

## Raw text

```rust
use levilamina::command::{self, CommandPermission};

command::register("hello", "Say hello", CommandPermission::Any, |inv| {
    let who = if inv.raw().is_empty() {
        inv.origin().name
    } else {
        inv.raw().to_owned()
    };
    inv.success(&format!("Hello, {who}!"));
})?;
```

`inv.raw()` is everything after `/hello`, parsed by you.

## Typed overloads

Declaring the parameters lets the engine parse and complete them, so a player sees
argument hints on the client:

```rust
use levilamina::command::{self, CommandPermission, ParamType};

command::builder("plot", "Plot management", CommandPermission::Any)
    .overload(|o| {
        o.required("action", ParamType::String)
            .optional("target", ParamType::Player)
    })
    .register(|inv| {
        match inv.arg_str("action").unwrap_or("") {
            "claim" => inv.success("claimed"),
            "info" => inv.success("..."),
            _ => inv.error("unrecognized subcommand"),
        }
    })?;
```

`arg_str`, `arg_i64`, `arg_f64` and `arg_bool` read a named argument; `arg` gives the raw
`NbtValue`. An omitted optional argument reads as `None`.

At least one overload is required. A builder with none is refused with a message saying
to use `command::register` instead, rather than leaving a bare registration failure to be
guessed at.

## Sibling words are one enum

Words that take the same arguments go into one enum parameter, not one `text` literal
each. The client lists every overload as its own line, so three literals read as three
unrelated commands where one enum reads as one command with a choice:

```rust
command::register_enum("plot_simple", &[("menu", 0), ("help", 1), ("status", 2)])?;
command::builder("plot", "Plots", CommandPermission::Any)
    .overload(|o| o.required_enum("simple", ParamType::Enum, "plot_simple"))
    // A word followed by arguments of its own stays a literal.
    .overload(|o| o.text("verb", "rate").required("score", ParamType::Int))
    .register(handler)?;
```

The handler reads the word from whichever parameter carries it. Register each word once,
and let the console through every permission gate: what it cannot do is what needs a
player, and such a command says that instead. The rule is CONTRACT.md 6.1.

## Success and error are different channels

```rust
inv.success("done");
inv.error("that plot is not yours");
```

The client colours them differently, and a command block reads the difference. A failed
command must not go out on the success channel.

## Registration is one way

Bedrock cannot remove a command. One registered by your mod lives until the server stops;
while your mod is disabled the host mutes the callback and re-enabling resumes it.

Two consequences:

- There is no `unregister` and no handle that deregisters on drop.
- Registering in `on_enable` has to survive running more than once. Re-registering the
  same name only swaps the callback, so that part is already safe.

A re-registration that *changes the shape* is refused: a Bedrock command cannot change
once built. The message says to restart the server to adopt the new declaration.

## Origin is a name, not an identity

```rust
let origin = inv.origin();
origin.name      // a player name, or the console
origin.kind      // CommandOriginType: 0 is a player, 7 the dedicated server console
origin.at        // Option, since the console has no position
```

A permission decision uses `kind` plus your own player table. A name goes through the
display-name fallback, so it is not an identity. See
[Errors and logging](errors.md#identity) for why that matters.
