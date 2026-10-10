---
hide:
  - navigation
  - toc
---

<div class="pier-hero" markdown>
<div markdown>

# <span class="pier-accent">LeviLamina</span> mods in the language you like

<p class="pier-tagline">Rust, Go, Zig or C++. Install Pier, write a dynamic library, drop it into plugins/, and the server loads it; mods of different languages call each other.</p>

[:material-rocket-launch: Get started](guide/installation.md){ .md-button .md-button--primary }
[:material-book-open-variant: API](api/index.md){ .md-button }
[:material-pencil: Your first mod](rust/first-mod.md){ .md-button }

</div>
<div markdown>

```rust title="src/lib.rs"
use levilamina::prelude::*;
use levilamina::event::{self, names};

struct Hello {
    join: Option<Listener>,
}

impl LeviMod for Hello {
    fn on_load(_ctx: &ModContext) -> Result<Self> {
        Ok(Hello { join: None })
    }

    fn on_enable(&mut self, _ctx: &ModContext) -> Result<()> {
        self.join = Some(event::subscribe(names::PLAYER_JOIN, |ev| {
            if let Ok(name) = ev.str_at("_player.name") {
                Logger::get().info(&format!("{name} joined"));
            }
        })?);
        Ok(())
    }
}

levilamina::register_mod!(Hello);
```

</div>
</div>

<div class="grid cards" markdown>

-   :material-translate:{ .lg .middle } **The language you know**

    ---

    Rust, C++, Go and Zig have official bindings. A language not on the list can plug in too, as long as it can call a C function.

    [:octicons-arrow-right-24: What is Pier](guide/what-is-pier.md)

-   :material-transit-connection-variant:{ .lg .middle } **Mods talk to each other**

    ---

    When a Go mod calls a service a Rust mod provides, Pier hands the request over and the reply back; neither side needs to know the other's language.

    [:octicons-arrow-right-24: Talking to other mods](tasks/crossmod.md)

-   :material-shield-check:{ .lg .middle } **An unreadable value is an error**

    ---

    When the host cannot read a value, the call returns an error naming what it tried to read, and your code can refuse the action.

    [:octicons-arrow-right-24: API overview](api/index.md)

-   :material-update:{ .lg .middle } **Upgrades without rebuilding**

    ---

    While the ABI version stays the same, a mod built against an older Pier loads unchanged; a real incompatibility is refused at load with the reason.

    [:octicons-arrow-right-24: Compatibility](guide/compatibility.md)

-   :material-dog-side:{ .lg .middle } **A watchdog for hangs**

    ---

    When a mod holds a server thread too long, the watchdog logs which mod and where, then ends the process so a supervisor brings the server back.

    [:octicons-arrow-right-24: The watchdog](guide/configuration.md#watchdog)

-   :material-puzzle:{ .lg .middle } **Right at home in LeviLamina**

    ---

    LeviLamina's events, commands, players and world are yours to use, and native plugins call your services through the bridge.

    [:octicons-arrow-right-24: The bridge](guide/bridge.md)

</div>
