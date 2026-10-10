# levilamina::scoreboard

Scoreboards.

Everything goes through one multiplexed slot, `scoreboard_op`, where `op` decides what
happens. This layer wraps each op as a named method and separates a score that cannot be
read from a score of 0, which the bare slot reports as the same empty output
(contract §5.2).

## `Scoreboard` {#Scoreboard}

```rust
pub struct Scoreboard(/* private */);
```

The scoreboard facade. Zero sized.

- Implements: `Clone`, `Copy`

### `Scoreboard::get` {#Scoreboard.get}

```rust
pub fn get() -> Scoreboard
```

- Return type: `Scoreboard`

### `Scoreboard::add_objective` {#Scoreboard.add_objective}

```rust
pub fn add_objective(&self, name: &str, display_name: &str) -> Result<()>
```

Creates a dummy objective. It fails when one of that name already exists.

- Parameters:
    - name : `&str`
    - display_name : `&str`
- Return type: `Result<()>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::remove_objective` {#Scoreboard.remove_objective}

```rust
pub fn remove_objective(&self, name: &str) -> Result<()>
```

- Parameters:
    - name : `&str`
- Return type: `Result<()>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::objectives` {#Scoreboard.objectives}

```rust
pub fn objectives(&self) -> Result<Vec<Objective>>
```

- Return type: `Result<Vec<Objective>>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::score` {#Scoreboard.score}

```rust
pub fn score(&self, objective: &str, who: &str) -> Result<Option<i64>>
```

Reads one score.

A person with no score on that objective gives `Ok(None)`, and only an objective that
does not exist is an `Err`. The bare slot collapses both of those and a score of 0 into
the same empty output, so this separates them by whether the output is empty and leaves
a nonexistent objective for the op itself to report.

- Parameters:
    - objective : `&str`
    - who : `&str`
- Return type: `Result<Option<i64>>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::set_score` {#Scoreboard.set_score}

```rust
pub fn set_score(&self, objective: &str, who: &str, value: i64) -> Result<i64>
```

Sets it to `value` and returns the value after the write.

- Parameters:
    - objective : `&str`
    - who : `&str`
    - value : `i64`
- Return type: `Result<i64>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::add_score` {#Scoreboard.add_score}

```rust
pub fn add_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64>
```

- Parameters:
    - objective : `&str`
    - who : `&str`
    - delta : `i64`
- Return type: `Result<i64>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::reduce_score` {#Scoreboard.reduce_score}

```rust
pub fn reduce_score(&self, objective: &str, who: &str, delta: i64) -> Result<i64>
```

- Parameters:
    - objective : `&str`
    - who : `&str`
    - delta : `i64`
- Return type: `Result<i64>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::reset_score` {#Scoreboard.reset_score}

```rust
pub fn reset_score(&self, objective: &str, who: &str) -> Result<()>
```

Erases the score of this person on this objective, which is not setting it to 0.

- Parameters:
    - objective : `&str`
    - who : `&str`
- Return type: `Result<()>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::set_display` {#Scoreboard.set_display}

```rust
pub fn set_display(&self, slot: DisplaySlot, objective: &str) -> Result<()>
```

- Parameters:
    - slot : `DisplaySlot`
    - objective : `&str`
- Return type: `Result<()>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

### `Scoreboard::clear_display` {#Scoreboard.clear_display}

```rust
pub fn clear_display(&self, slot: DisplaySlot) -> Result<()>
```

- Parameters:
    - slot : `DisplaySlot`
- Return type: `Result<()>`
- Slots: [`scoreboard_op`](../cpp/scoreboard.md#scoreboard_op)

## `DisplaySlot` {#DisplaySlot}

```rust
pub enum DisplaySlot {
        Sidebar,
        List,
        BelowName,
}
```

The display slot of a scoreboard.

- Implements: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`

### `DisplaySlot::as_str` {#DisplaySlot.as_str}

```rust
pub fn as_str(self) -> &'static str
```

The host dispatches on this string.

- Return type: `&'static str`

## `Objective` {#Objective}

```rust
pub struct Objective {
    pub name: String,
    pub display_name: String,
}
```

One objective.

- Implements: `Debug`, `Clone`, `PartialEq`, `Eq`
