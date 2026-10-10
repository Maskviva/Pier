# Scoreboard

??? note "Section notes in abi.h"

    **§F scoreboard**

## Slots {#slots}

### `scoreboard_op` {#scoreboard_op}

```c
bool (*scoreboard_op)(int32_t op, PierStr a, PierStr b, int64_t n, void* ctx, PierStrSink out);
```

- Call: `api->scoreboard_op(op, a, b, n, ctx, out)`
- Parameters:
    - op : `int32_t`
    - a : `PierStr`
    - b : `PierStr`
    - n : `int64_t`
    - ctx : `void*`
    - out : `PierStrSink`
- Return type: `bool`
- Section of abi.h: §F scoreboard
- Position in the table: slot 52, counting from 0
- Callers in each binding:
    - Rust: [`Scoreboard::add_objective`](../rust/scoreboard.md#Scoreboard.add_objective), [`Scoreboard::remove_objective`](../rust/scoreboard.md#Scoreboard.remove_objective), [`Scoreboard::objectives`](../rust/scoreboard.md#Scoreboard.objectives), [`Scoreboard::score`](../rust/scoreboard.md#Scoreboard.score), [`Scoreboard::set_score`](../rust/scoreboard.md#Scoreboard.set_score), [`Scoreboard::add_score`](../rust/scoreboard.md#Scoreboard.add_score) and more, 10 in all
    - Go: [`Scoreboard`](../go/scoreboard.md#Scoreboard), [`Raw.ScoreboardOp`](../go/raw.md#Raw.ScoreboardOp)

## `PierScoreboardOp` {#PierScoreboardOp}

`scoreboard_op` verbs (args a=objective/slot, b=target, n=value).

| Name | Value | Description |
|---|---|---|
| <span id="PIER_SB_ADD_OBJECTIVE"></span>`PIER_SB_ADD_OBJECTIVE` | `0` | a=name, b=display name → out "1" `Scoreboard::addObjective`("dummy") |
| <span id="PIER_SB_REMOVE_OBJECTIVE"></span>`PIER_SB_REMOVE_OBJECTIVE` | `1` | a=name `Scoreboard::removeObjective` |
| <span id="PIER_SB_LIST_OBJECTIVES"></span>`PIER_SB_LIST_OBJECTIVES` | `2` | → out SNBT \[{name,display},…\] `Scoreboard::getObjectives` |
| <span id="PIER_SB_GET_SCORE"></span>`PIER_SB_GET_SCORE` | `3` | a=objective, b=fake-player name → out value `Objective::getPlayerScore` |
| <span id="PIER_SB_SET_SCORE"></span>`PIER_SB_SET_SCORE` | `4` | a=objective, b=name, n=value `Scoreboard::modifyPlayerScore`(Set) |
| <span id="PIER_SB_ADD_SCORE"></span>`PIER_SB_ADD_SCORE` | `5` | a=objective, b=name, n=value … (Add) |
| <span id="PIER_SB_REDUCE_SCORE"></span>`PIER_SB_REDUCE_SCORE` | `6` | a=objective, b=name, n=value … (Subtract) |
| <span id="PIER_SB_RESET_SCORE"></span>`PIER_SB_RESET_SCORE` | `7` | a=objective, b=name `Scoreboard::resetPlayerScore` |
| <span id="PIER_SB_SET_DISPLAY"></span>`PIER_SB_SET_DISPLAY` | `8` | a=slot("sidebar"/"list"/"belowname"), b=objective setDisplayObjective |
| <span id="PIER_SB_CLEAR_DISPLAY"></span>`PIER_SB_CLEAR_DISPLAY` | `9` | a=slot clearDisplayObjective |
