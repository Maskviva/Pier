# 🔄 生命周期与日志 API

一个模组从被服务器发现，到最后被卸载，会经历四个阶段：**加载 → 启用 → 禁用 → 卸载**。你在对应的函数里做对应的事，Pier 会在合适的时候调用它们。

### 声明模组

#### 声明一个模组

Rust：`levilamina::register_mod!(MyMod)`  
Go：`levilamina.Register(name, mod)`  
Zig：`levilamina.exportMod(MyMod, name)`  

告诉 Pier 你的模组是哪个类型，Pier 会通过它调用四个阶段的函数。这一步会导出服务器要找的入口函数 `pier_main`，少了它，模组在加载时会被拒绝。

- 参数：
    - MyMod / mod : 类型  
      Rust 实现 `LeviMod`；Go 实现 `Mod` 接口（`Enable`、`Disable`，可选 `Load`、`Unload`）；Zig 声明 `load`、`enable`、`disable`、`unload` 中需要的几个
    - name : 字符串  
      （Go、Zig）日志里显示的模组名
    - Go 里要在 `init` 函数中调用 `Register`：服务器调用 `pier_main` 时，Go 运行时已经跑完了所有 `init`。
- 示例：
    - Rust

      ```rust title="Rust"
      use levilamina::prelude::*;

      struct MyMod;

      impl LeviMod for MyMod {
          fn on_load(ctx: &ModContext) -> Result<Self> {
              ctx.logger().info("模组加载了");
              Ok(MyMod)
          }
      }

      levilamina::register_mod!(MyMod);
      ```

    - Go

      ```go title="Go"
      type myMod struct{}

      func (m *myMod) Enable(ctx *levilamina.Context) error  { return nil }
      func (m *myMod) Disable(ctx *levilamina.Context) error { return nil }

      func init() {
      	levilamina.Register("my-mod", &myMod{})
      }

      func main() {}
      ```

    - Zig

      ```zig title="Zig"
      const levilamina = @import("levilamina");

      const MyMod = struct {
          pub fn enable(ctx: *levilamina.Context) !void {
              ctx.info("模组启用了");
          }
      };

      comptime {
          levilamina.exportMod(MyMod, "my-mod");
      }
      ```

四个阶段：

| 阶段 | Rust | Go | Zig | 什么时候 |
|---|---|---|---|---|
| 加载 | `on_load` | `Load`（可选） | `load` | 服务器发现你的模组时 |
| 启用 | `on_enable` | `Enable` | `enable` | 加载完、开始工作时 |
| 禁用 | `on_disable` | `Disable` | `disable` | 关服时，或者有人执行 `/pier disable` |
| 卸载 | `on_unload` | `Unload`（可选） | `unload` | 模组被移除前 |

这些函数都在服务器主线程上运行。返回一个错误，Pier 会把它写进日志，并拒绝这一步：启用失败时，模组保持禁用状态。订阅和命令请在启用阶段注册、在禁用阶段撤销，这样 `/pier disable` 再 `/pier enable` 时，模组能从头再来一遍。

!!! warning "别让 panic 跑出你的函数"

    Rust 和 Go 的 panic 会被 Pier 在边界上拦住并写进日志。Zig 不做栈展开，panic 会结束整个服务器进程，请用错误返回值代替。

### 日志

#### 打一条日志

Rust：`ctx.logger().info(msg)`  
Go：`ctx.Logger().Info(msg)`  
Zig：`ctx.info(msg)`  

日志会带上你模组的名字，写到服务器控制台和日志文件里。还有 warn、error 等级别；没有 `ctx` 的地方（比如回调里），Rust 用 `Logger::get()`，Go 用 `levilamina.Logger{}`，Zig 用 `levilamina.log(.info, msg)`。

- 参数：
    - msg : 字符串  
      要写的内容
    - 可以在任何线程调用。
- 对应槽位：`log`
- 示例：
    - Rust

      ```rust title="Rust"
      ctx.logger().info("普通信息");
      Logger::get().warn("在回调里也能打日志");
      ```

    - Go

      ```go title="Go"
      ctx.Logger().Info("普通信息")
      levilamina.Logger{}.Warn("在回调里也能打日志")
      ```

    - Zig

      ```zig title="Zig"
      ctx.info("普通信息");
      levilamina.logf(.warn, "玩家数：{d}", .{count});
      ```
