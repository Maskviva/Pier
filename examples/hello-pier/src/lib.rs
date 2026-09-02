//! `hello-pier`, a runnable reference for the four steps of contract §10, adding a
//! language.
//!
//! The example deliberately uses only the runtime foundation, meaning the handshake,
//! logging and the lifecycle, and touches no domain API. What it verifies is whether
//! the contract itself works, not how convenient a domain wrapper is. A mod that uses
//! only the foundation and can be loaded, log, and be unloaded proves all four steps:
//!
//! 1. Read `sdk/abi.h` alone, here through the hand-written mirror in
//!    `levilamina_sys`.
//! 2. Declare the whole table unconditionally, which the mirror does and
//!    `sys-mirrors-abi` guards.
//! 3. Export `pier_main` with `struct_size`, `abi_version`, `mod_flags` and the three
//!    lifecycle callbacks filled in, which is what `register_mod!` expands to.
//! 4. Check both gates before calling any non-core slot, shown here with `host_abi()`.
//!
//! Once installed the console shows three `[hello-pier]` lines, one each for load,
//! enable and unload. A missing line means that part of the lifecycle did not
//! complete, which is what this example is for.

use levilamina::prelude::*;

struct HelloPier {
    /// Counts how many times the mod was enabled. A reload produces repeated
    /// enable and disable pairs, so printing the count shows whether the host
    /// completed a full round.
    enabled_times: u32,
}

impl LeviMod for HelloPier {
    fn on_load(ctx: &ModContext) -> Result<Self> {
        let (abi, table_len) = ctx.host_abi();
        ctx.logger().info(&format!(
            "loaded. host ABI v{abi}, function table {table_len} bytes, \
             target={}.",
            if ctx.host_is_client() {
                "client"
            } else {
                "server"
            }
        ));

        // Step 4 of contract §10 in practice: check both gates before calling a
        // non-core slot. `has_slot!` checks that the table is long enough and that
        // the slot is non-null. The first guards against an out-of-bounds read, the
        // second against the capability package not being compiled into the host.
        if levilamina::has_slot!(md_is_available) {
            ctx.logger()
                .info("host has the custom dimension capability, md_* slots are set.");
        } else {
            // Not an error but a deliberate degradation. pier-dimensions is an
            // optional package (contract §1 rule 3) and its slots are NULL when it
            // is not compiled in.
            ctx.logger()
                .info("host lacks the custom dimension capability, slots are empty, treating as unsupported.");
        }

        Ok(HelloPier { enabled_times: 0 })
    }

    fn on_enable(&mut self, ctx: &ModContext) -> Result<()> {
        self.enabled_times += 1;
        let host = Host::get();
        ctx.logger().info(&format!(
            "enabled (time {}). server stage={:?}, {} player(s) online, tick {}.",
            self.enabled_times,
            host.gaming_status(),
            host.player_count()
                .map_or("?".to_owned(), |n| n.to_string()),
            host.current_tick()
                .map_or("?".to_owned(), |t| t.to_string()),
        ));

        // Contract §5.3 in practice: when a subscription or resolution fails, listing
        // the ids the host does recognize helps far more than the word "failed". Only
        // the count is printed here; a real subscription would print the near matches.
        let events = host.list_events();
        ctx.logger()
            .info(&format!("host currently resolves {} event id(s).", events.len()));

        // Hand a piece of work back to the server thread. The closure is boxed for
        // the host and freed as soon as it has run, and ownership stays on the mod
        // side throughout (contract §3).
        //
        // What comes back is a ticket. This path uses `schedule_for` with the mod
        // handle, so the host accounts tasks per mod and discards the whole pending
        // batch on unload. An ownerless slot cannot do that and its timer would jump
        // into an unmapped code section after the mod is gone. `host.cancel(task)`
        // cancels early.
        let task = host.schedule_after(std::time::Duration::from_secs(1), || {
            Logger::get().info("scheduled task fired one second later.");
        })?;
        ctx.logger().info(&format!(
            "schedule ticket {}, {} task(s) pending under this mod.",
            task.raw(),
            host.pending_tasks()
        ));
        Ok(())
    }

    fn on_disable(&mut self, ctx: &ModContext) -> Result<()> {
        ctx.logger().info("disabled.");
        Ok(())
    }

    fn on_unload(&mut self, ctx: &ModContext) -> Result<()> {
        // `&mut self` is still available here, so final cleanup belongs in this
        // callback. The host runs its teardown steps and finally FreeLibrary only
        // after this returns.
        ctx.logger()
            .info(&format!("unloaded. enabled {} time(s) this session.", self.enabled_times));
        Ok(())
    }
}

levilamina::register_mod!(HelloPier);
