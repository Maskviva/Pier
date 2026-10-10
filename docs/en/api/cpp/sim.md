# Simulated players

??? note "Section notes in abi.h"

    **§I NBT binary, KvDb (thread-safe), system & server info**

## Slots {#slots}

### `sim_spawn` {#sim_spawn}

```c
bool (*sim_spawn)(PierStr name, int32_t dimension, double x, double y, double z);
```

!!! note "Group note"

    Simulated ("fake") players (additive, gated by `struct_size`). `sim_spawn` creates a real ServerPlayer with that name — every existing per-player entry (teleport, health, inventory, kick,…) works on it via the usual name selector. `sim_do` multiplexes the simulate\* verb family: the action vocabulary grows bridge-side without new table slots (verbs: despawn stop jump attack interact `use_item` drop respawn `move_to` `navigate_to` `look_at` `destroy_block` `destroy_look` `stop_destroy` `interact_block` sneak fly chat — args as SNBT, see docs). Gated on isSimulatedPlayer(): a real player can never be puppeted. False on unknown verb, malformed args, offline/non-sim target.

- Call: `api->sim_spawn(name, dimension, x, y, z)`
- Parameters:
    - name : `PierStr`
    - dimension : `int32_t`
    - x : `double`
    - y : `double`
    - z : `double`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 85, counting from 0
- Callers in each binding:
    - Rust: [`sim::spawn`](../rust/sim.md#fn.spawn)
    - Go: [`SimSpawn`](../go/sim.md#SimSpawn), [`Raw.SimSpawn`](../go/raw.md#Raw.SimSpawn)

### `sim_do` {#sim_do}

```c
bool (*sim_do)(PierPlayerSel sel, PierStr action, PierStr args_snbt);
```

!!! note "Group note"

    Simulated ("fake") players (additive, gated by `struct_size`). `sim_spawn` creates a real ServerPlayer with that name — every existing per-player entry (teleport, health, inventory, kick,…) works on it via the usual name selector. `sim_do` multiplexes the simulate\* verb family: the action vocabulary grows bridge-side without new table slots (verbs: despawn stop jump attack interact `use_item` drop respawn `move_to` `navigate_to` `look_at` `destroy_block` `destroy_look` `stop_destroy` `interact_block` sneak fly chat — args as SNBT, see docs). Gated on isSimulatedPlayer(): a real player can never be puppeted. False on unknown verb, malformed args, offline/non-sim target.

- Call: `api->sim_do(sel, action, args_snbt)`
- Parameters:
    - sel : `PierPlayerSel`
    - action : `PierStr`
    - args_snbt : `PierStr`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 86, counting from 0
- Callers in each binding:
    - Rust: [`SimPlayer::act`](../rust/sim.md#SimPlayer.act), [`SimPlayer::despawn`](../rust/sim.md#SimPlayer.despawn), [`SimPlayer::stop`](../rust/sim.md#SimPlayer.stop), [`SimPlayer::jump`](../rust/sim.md#SimPlayer.jump), [`SimPlayer::attack`](../rust/sim.md#SimPlayer.attack), [`SimPlayer::interact`](../rust/sim.md#SimPlayer.interact) and more, 19 in all
    - Go: [`SimDo`](../go/sim.md#SimDo), [`Raw.SimDo`](../go/raw.md#Raw.SimDo)

### `sim_is` {#sim_is}

```c
bool (*sim_is)(PierPlayerSel sel);
```

!!! note "Group note"

    Simulated ("fake") players (additive, gated by `struct_size`). `sim_spawn` creates a real ServerPlayer with that name — every existing per-player entry (teleport, health, inventory, kick,…) works on it via the usual name selector. `sim_do` multiplexes the simulate\* verb family: the action vocabulary grows bridge-side without new table slots (verbs: despawn stop jump attack interact `use_item` drop respawn `move_to` `navigate_to` `look_at` `destroy_block` `destroy_look` `stop_destroy` `interact_block` sneak fly chat — args as SNBT, see docs). Gated on isSimulatedPlayer(): a real player can never be puppeted. False on unknown verb, malformed args, offline/non-sim target.

True if the selector resolves to a live simulated player. Lets a mod re-validate a bot after a restart (the SimulatedPlayer persists in the world, but in-memory handles don't).

- Call: `api->sim_is(sel)`
- Parameters:
    - sel : `PierPlayerSel`
- Return type: `bool`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 87, counting from 0
- Callers in each binding:
    - Rust: [`SimPlayer::try_is_simulated`](../rust/sim.md#SimPlayer.try_is_simulated), [`SimPlayer::is_simulated`](../rust/sim.md#SimPlayer.is_simulated)
    - Go: [`IsSimulated`](../go/sim.md#IsSimulated), [`Raw.SimIs`](../go/raw.md#Raw.SimIs)

### `sim_list` {#sim_list}

```c
void (*sim_list)(void* ctx, PierStrSink name_sink);
```

!!! note "Group note"

    Simulated ("fake") players (additive, gated by `struct_size`). `sim_spawn` creates a real ServerPlayer with that name — every existing per-player entry (teleport, health, inventory, kick,…) works on it via the usual name selector. `sim_do` multiplexes the simulate\* verb family: the action vocabulary grows bridge-side without new table slots (verbs: despawn stop jump attack interact `use_item` drop respawn `move_to` `navigate_to` `look_at` `destroy_block` `destroy_look` `stop_destroy` `interact_block` sneak fly chat — args as SNBT, see docs). Gated on isSimulatedPlayer(): a real player can never be puppeted. False on unknown verb, malformed args, offline/non-sim target.

Enumerate the names of all live simulated players (sink receives each name). Rebuild a handle from a name to drive a bot that outlived the session that spawned it.

- Call: `api->sim_list(ctx, name_sink)`
- Parameters:
    - ctx : `void*`
    - name_sink : `PierStrSink`
- Section of abi.h: §I NBT binary, KvDb (thread-safe), system & server info
- Position in the table: slot 88, counting from 0
- Callers in each binding:
    - Rust: [`sim::try_list`](../rust/sim.md#fn.try_list), [`sim::list`](../rust/sim.md#fn.list)
    - Go: [`SimList`](../go/sim.md#SimList), [`Raw.SimList`](../go/raw.md#Raw.SimList)
