# 计分板

??? note "abi.h 里的分节说明"

    **§F 计分板（`§F scoreboard`）**

## 槽位 {#slots}

### `scoreboard_op` {#scoreboard_op}

```c
bool (*scoreboard_op)(int32_t op, PierStr a, PierStr b, int64_t n, void* ctx, PierStrSink out);
```

- 调用形式：`api->scoreboard_op(op, a, b, n, ctx, out)`
- 参数：
    - op : `int32_t`
    - a : `PierStr`
    - b : `PierStr`
    - n : `int64_t`
    - ctx : `void*`
    - out : `PierStrSink`
- 返回值类型：`bool`
- 所在分节：§F 计分板（`§F scoreboard`）
- 表内序号：第 52 个槽位（从 0 数起）
- 各绑定里的调用方：
    - Rust：[`Scoreboard::add_objective`](../rust/scoreboard.md#Scoreboard.add_objective)、[`Scoreboard::remove_objective`](../rust/scoreboard.md#Scoreboard.remove_objective)、[`Scoreboard::objectives`](../rust/scoreboard.md#Scoreboard.objectives)、[`Scoreboard::score`](../rust/scoreboard.md#Scoreboard.score)、[`Scoreboard::set_score`](../rust/scoreboard.md#Scoreboard.set_score)、[`Scoreboard::add_score`](../rust/scoreboard.md#Scoreboard.add_score) 等，共 10 个
    - Go：[`Scoreboard`](../go/scoreboard.md#Scoreboard)、[`Raw.ScoreboardOp`](../go/raw.md#Raw.ScoreboardOp)

## `PierScoreboardOp` {#PierScoreboardOp}

`scoreboard_op` 的操作（参数：`a` 为计分项或显示位置，`b` 为目标，`n` 为数值）。

| 名称 | 值 | 说明 |
|---|---|---|
| <span id="PIER_SB_ADD_OBJECTIVE"></span>`PIER_SB_ADD_OBJECTIVE` | `0` | `a` 为名字，`b` 为显示名，输出 `"1"`，调用 `Scoreboard::addObjective("dummy")` |
| <span id="PIER_SB_REMOVE_OBJECTIVE"></span>`PIER_SB_REMOVE_OBJECTIVE` | `1` | `a` 为名字，调用 `Scoreboard::removeObjective` |
| <span id="PIER_SB_LIST_OBJECTIVES"></span>`PIER_SB_LIST_OBJECTIVES` | `2` | 输出 SNBT `[{name,display},…]`，取自 `Scoreboard::getObjectives` |
| <span id="PIER_SB_GET_SCORE"></span>`PIER_SB_GET_SCORE` | `3` | `a` 为计分项，`b` 为虚拟玩家名，输出分数，取自 `Objective::getPlayerScore` |
| <span id="PIER_SB_SET_SCORE"></span>`PIER_SB_SET_SCORE` | `4` | `a` 为计分项，`b` 为名字，`n` 为数值，调用 `Scoreboard::modifyPlayerScore(Set)` |
| <span id="PIER_SB_ADD_SCORE"></span>`PIER_SB_ADD_SCORE` | `5` | `a` 为计分项，`b` 为名字，`n` 为数值，同上（Add） |
| <span id="PIER_SB_REDUCE_SCORE"></span>`PIER_SB_REDUCE_SCORE` | `6` | `a` 为计分项，`b` 为名字，`n` 为数值，同上（Subtract） |
| <span id="PIER_SB_RESET_SCORE"></span>`PIER_SB_RESET_SCORE` | `7` | `a` 为计分项，`b` 为名字，调用 `Scoreboard::resetPlayerScore` |
| <span id="PIER_SB_SET_DISPLAY"></span>`PIER_SB_SET_DISPLAY` | `8` | `a` 为显示位置（`"sidebar"`、`"list"` 或 `"belowname"`），`b` 为计分项，调用 `setDisplayObjective` |
| <span id="PIER_SB_CLEAR_DISPLAY"></span>`PIER_SB_CLEAR_DISPLAY` | `9` | `a` 为显示位置，调用 `clearDisplayObjective` |
