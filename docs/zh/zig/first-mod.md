# 第一个 Zig 模组

这里构建的是 `examples/hello-pier-zig`：一个命令、一个玩家加入事件的监听器、一个延时任务，以及一个任何模组都能用的服务和总线主题。

## 包结构

```
build.zig
build.zig.zon
src/main.zig
manifest.json
```

`build.zig.zon` 依赖绑定。在 Pier 仓库里，示例按路径指向它；你自己的模组用 `zig fetch --save` 获取。

```zig
.dependencies = .{
    .pier = .{ .path = "../../bindings/zig" },
},
```

`build.zig` 构建一个导入了 `pier` 模块的动态库：

```zig
const pier_dep = b.dependency("pier", .{ .target = target, .optimize = optimize });
const lib = b.addLibrary(.{
    .linkage = .dynamic,
    .name = "hello_pier_zig",
    .root_module = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
        .imports = &.{.{ .name = "levilamina", .module = pier_dep.module("levilamina") }},
    }),
});
b.installArtifact(lib);
```

## src/main.zig

```zig
const std = @import("std");
const levilamina = @import("levilamina");

const Hello = struct {
    var joins: ?levilamina.Listener = null;

    pub fn enable(ctx: *levilamina.Context) !void {
        ctx.info("enabled");
        joins = try levilamina.subscribe("ll::event::PlayerJoinEvent", .normal, onJoin);
        _ = try levilamina.scheduleAfter(1000, later);
    }

    pub fn disable(_: *levilamina.Context) !void {
        if (joins) |*listener| try listener.unsubscribe();
    }

    fn onJoin(ev: *const levilamina.Event) void {
        levilamina.logf(.info, "a player joined: {s}", .{ev.snbt});
    }

    fn later() void {
        levilamina.log(.info, "one second later");
    }
};

comptime {
    levilamina.exportMod(Hello, "hello-pier-zig");
}
```

`exportMod` 为这个模组类型导出 `pier_main`。它先完成握手，在填写 vtable 之前调用 `load`；宿主之后再调用 `enable`、`disable` 和 `unload`。
类型里声明的每一个都接收 `*levilamina.Context` 并返回 `!void`；返回的错误会连同名字记进日志，并拒绝这一步。

## 构建和安装

```
zig build
```

`zig-out/bin/hello_pier_zig.dll` 就是模组。把它和 `manifest.json` 放进 `plugins/hello-pier-zig/`，再启动服务器。
日志会显示模组加载、启用，一秒后出现任务打印的那一行。`/hellozig` 在游戏里和控制台都能用。

如果模组没有加载，[排错](../guide/troubleshooting.md)列出了各条日志的含义。
