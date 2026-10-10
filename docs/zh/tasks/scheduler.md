# ⏰ 计划任务 API

想过一会儿再做某件事？或者在自己的线程里算完了，要回到主线程去调用接口？用计划任务。

### 安排任务

#### 尽快在主线程执行

Rust：`ctx.host().schedule(task)`  
Go：`levilamina.Schedule(task)`  
Zig：`levilamina.schedule(task)`  

- 参数：
    - task : 函数  
      要执行的工作，会在服务器主线程上运行
- 返回值：任务 id，以后可以用来取消
- 返回值类型：Rust `Result<TaskId>`，Go `(TaskID, error)`，Zig `levilamina.Error!TaskId`
    - 可以在任何线程调用，这正是把工作从后台线程送回主线程的办法。
- 对应槽位：`schedule_for`
- 示例：
    - Rust

      ```rust title="Rust"
      ctx.host().schedule(|| {
          Logger::get().info("现在在主线程上了");
      })?;
      ```

    - Go

      ```go title="Go"
      id, err := levilamina.Schedule(func() {
      	levilamina.Logger{}.Info("现在在主线程上了")
      })
      ```

    - Zig

      ```zig title="Zig"
      _ = try levilamina.schedule(onMainThread);

      fn onMainThread() void {
          levilamina.log(.info, "现在在主线程上了");
      }
      ```

#### 过一段时间再执行

Rust：`ctx.host().schedule_after(delay, task)`  
Go：`levilamina.ScheduleAfter(delay, task)`  
Zig：`levilamina.scheduleAfter(delay_ms, task)`  

- 参数：
    - delay : 时长  
      等多久：Rust `Duration`，Go `time.Duration`，Zig 毫秒数
    - task : 函数  
      要执行的工作
- 返回值：任务 id
- 返回值类型：Rust `Result<TaskId>`，Go `(TaskID, error)`，Zig `levilamina.Error!TaskId`
- 对应槽位：`schedule_after_for`
- 示例：
    - Rust

      ```rust title="Rust"
      let id = ctx.host().schedule_after(Duration::from_secs(30), || {
          Player::broadcast("30 秒到了").ok();
      })?;
      ```

    - Go

      ```go title="Go"
      id, err := levilamina.ScheduleAfter(30*time.Second, func() {
      	levilamina.Broadcast("30 秒到了")
      })
      ```

    - Zig

      ```zig title="Zig"
      _ = try levilamina.scheduleAfter(30_000, onTimeUp);
      ```

!!! tip "任务属于你的模组"

    如果你的模组在任务执行前就被卸载了，Pier 会把这个任务从队列里拿掉。到了原定的时间，什么都不会被调用。

### 管理任务

#### 取消一个任务

Rust：`ctx.host().cancel(id)`  
Go：`levilamina.Cancel(id)`  
Zig：`levilamina.cancel(id)`  

- 参数：
    - id : 任务 id  
      安排任务时拿到的 id
- 返回值：是否取消成功。任务已经执行过、已经取消过，或者不是你这个模组的，返回 `false`
- 返回值类型：Rust `bool`，Go `(bool, error)`，Zig `levilamina.Error!bool`
- 对应槽位：`schedule_cancel`
- 示例：
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

#### 查询还没执行的任务数

Rust：`ctx.host().try_pending_tasks()`  
Go：`levilamina.PendingTasks()`  
Zig：`levilamina.pendingTasks()`  

- 返回值：你这个模组还没执行的任务数
- 返回值类型：Rust `Result<u32>`，Go `(uint32, error)`，Zig `levilamina.Error!u32`
    - 宿主数不了的时候返回错误。你在卸载前判断「是不是 0」，拿到的会是这个错误，而不是一个让判断通过的 0。
- 对应槽位：`schedule_pending_count`
- 示例：
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
