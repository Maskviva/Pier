# levilamina::scoreboard · 计分板

计分板。

所有操作都经过一个复用的槽位 `scoreboard_op`，由 `op` 决定做什么。这一层把每个 op 包成一个具名方法，并把「读不到分数」和「分数为 0」分开；直接用这个槽位时，两者给出的是同样的空输出（契约 §5.2）。

## `Scoreboard` {#Scoreboard}

```rust
pub struct Scoreboard(/* private */);
```

计分板门面。零大小。

- 实现的 trait：`Clone`、`Copy`

### `Scoreboard::get` {#Scoreboard.get}

```rust
pub fn get() -> Scoreboard
```

- 返回值类型：`Scoreboard`

### `Scoreboard::add_objective` {#Scoreboard.add_objective}

```rust
pub fn add_objective(&self, name: &str, display_name: &str) -> Result<()>
```

创建一个 dummy 计分项。同名的计分项已经存在时失败。

- 参数：
    - name : `&str`
    - display_name : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::remove_objective` {#Scoreboard.remove_objective}

```rust
pub fn remove_objective(&self, name: &str) -> Result<()>
```

- 参数：
    - name : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::objectives` {#Scoreboard.objectives}

```rust
pub fn objectives(&self) -> Result<Vec<Objective>>
```

- 返回值类型：`Result<Vec<Objective>>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::score` {#Scoreboard.score}

```rust
pub fn score(&self, objective: &str, who: &str) -> Result<Option<i64>>
```

读取一个分数。

这个人在这个计分项上没有分数时返回 `Ok(None)`，只有计分项不存在才是 `Err`。直接用这个槽位时，这两种情况和分数为 0 会合成同一个空输出，所以这里按输出是否为空把它们分开，计分项不存在的情况留给 op 本身去报告。

- 参数：
    - objective : `&str`
    - who : `&str`
- 返回值类型：`Result<Option<i64>>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::set_score` {#Scoreboard.set_score}

```rust
pub fn set_score(&self, objective: &str, who: &str, value: i64) -> Result<i64>
```

设为 `value`，返回写入以后的值。

- 参数：
    - objective : `&str`
    - who : `&str`
    - value : `i64`
- 返回值类型：`Result<i64>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::add_score` {#Scoreboard.add_score}

```rust
pub fn add_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64>
```

- 参数：
    - objective : `&str`
    - who : `&str`
    - delta : `i64`
- 返回值类型：`Result<i64>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::reduce_score` {#Scoreboard.reduce_score}

```rust
pub fn reduce_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64>
```

- 参数：
    - objective : `&str`
    - who : `&str`
    - delta : `i64`
- 返回值类型：`Result<i64>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::reset_score` {#Scoreboard.reset_score}

```rust
pub fn reset_score(&self, objective: &str, who: &str) -> Result<()>
```

抹掉这个人在这个计分项上的分数，这和设为 0 是两回事。

- 参数：
    - objective : `&str`
    - who : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::set_display` {#Scoreboard.set_display}

```rust
pub fn set_display(&self, slot: DisplaySlot, objective: &str) -> Result<()>
```

- 参数：
    - slot : `DisplaySlot`
    - objective : `&str`
- 返回值类型：`Result<()>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::clear_display` {#Scoreboard.clear_display}

```rust
pub fn clear_display(&self, slot: DisplaySlot) -> Result<()>
```

- 参数：
    - slot : `DisplaySlot`
- 返回值类型：`Result<()>`
- 对应槽位：[`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

## `DisplaySlot` {#DisplaySlot}

```rust
pub enum DisplaySlot {
        Sidebar,
        List,
        BelowName,
}
```

计分板的显示位置。

- 实现的 trait：`Debug`、`Clone`、`Copy`、`PartialEq`、`Eq`

### `DisplaySlot::as_str` {#DisplaySlot.as_str}

```rust
pub fn as_str(self) -> &'static str
```

宿主按这个字符串分派。

- 返回值类型：`&'static str`

## `Objective` {#Objective}

```rust
pub struct Objective {
    pub name: String,
    pub display_name: String,
}
```

一个计分项。

- 实现的 trait：`Debug`、`Clone`、`PartialEq`、`Eq`
