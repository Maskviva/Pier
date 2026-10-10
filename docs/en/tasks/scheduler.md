# ⏰ Scheduled tasks API

Something to do a little later? Or work finished on a thread of your own that now needs the API on the server thread? Use a scheduled task.

### Scheduling

#### On the server thread, as soon as possible

Rust: `ctx.host().schedule(task)`  
Go: `levilamina.Schedule(task)`  
Zig: `levilamina.schedule(task)`  

- Parameters:
    - task : function  
      the work, run on the server thread
- Return value: the task id, to cancel it later
- Return type: Rust `Result<TaskId>`, Go `(TaskID, error)`, Zig `levilamina.Error!TaskId`
    - Callable from any thread: this is how work gets back from a background thread to the server thread.
- Slot: `schedule_for`
- Example:
    - Rust

      ```rust title="Rust"
      ctx.host().schedule(|| {
          Logger::get().info("on the server thread now");
      })?;
      ```

    - Go

      ```go title="Go"
      id, err := levilamina.Schedule(func() {
      	levilamina.Logger{}.Info("on the server thread now")
      })
      ```

    - Zig

      ```zig title="Zig"
      _ = try levilamina.schedule(onMainThread);

      fn onMainThread() void {
          levilamina.log(.info, "on the server thread now");
      }
      ```

#### After a while

Rust: `ctx.host().schedule_after(delay, task)`  
Go: `levilamina.ScheduleAfter(delay, task)`  
Zig: `levilamina.scheduleAfter(delay_ms, task)`  

- Parameters:
    - delay : duration  
      how long to wait: Rust `Duration`, Go `time.Duration`, Zig milliseconds
    - task : function  
      the work
- Return value: the task id
- Return type: Rust `Result<TaskId>`, Go `(TaskID, error)`, Zig `levilamina.Error!TaskId`
- Slot: `schedule_after_for`
- Example:
    - Rust

      ```rust title="Rust"
      let id = ctx.host().schedule_after(Duration::from_secs(30), || {
          Player::broadcast("30 seconds are up").ok();
      })?;
      ```

    - Go

      ```go title="Go"
      id, err := levilamina.ScheduleAfter(30*time.Second, func() {
      	levilamina.Broadcast("30 seconds are up")
      })
      ```

    - Zig

      ```zig title="Zig"
      _ = try levilamina.scheduleAfter(30_000, onTimeUp);
      ```

!!! tip "A task belongs to your mod"

    When your mod is unloaded before a task runs, Pier takes the task off the queue. When its time comes, nothing is called.

### Managing tasks

#### Cancelling a task

Rust: `ctx.host().cancel(id)`  
Go: `levilamina.Cancel(id)`  
Zig: `levilamina.cancel(id)`  

- Parameters:
    - id : task id  
      the id scheduling returned
- Return value: whether it was cancelled. A task that already ran, was already cancelled, or is not your mod's returns `false`
- Return type: Rust `bool`, Go `(bool, error)`, Zig `levilamina.Error!bool`
- Slot: `schedule_cancel`
- Example:
    - Rust

      ```rust title="Rust"
      ctx.host().cancel(id);
      ```

    - Go

      ```go title="Go"
      cancelled, err := levilamina.Cancel(id)
      ```

    - Zig

      ```zig title="Zig"
      const cancelled = try levilamina.cancel(id);
      ```

#### How many tasks are waiting

Rust: `ctx.host().try_pending_tasks()`  
Go: `levilamina.PendingTasks()`  
Zig: `levilamina.pendingTasks()`  

- Return value: how many of your mod's tasks have not run
- Return type: Rust `Result<u32>`, Go `(uint32, error)`, Zig `levilamina.Error!u32`
    - A host that cannot count returns an error. A check for 0 before unloading receives that error, not a 0 that lets the check pass.
- Slot: `schedule_pending_count`
- Example:
    - Rust

      ```rust title="Rust"
      let n = ctx.host().try_pending_tasks()?;
      ```

    - Go

      ```go title="Go"
      n, err := levilamina.PendingTasks()
      ```

    - Zig

      ```zig title="Zig"
      const n = try levilamina.pendingTasks();
      ```
