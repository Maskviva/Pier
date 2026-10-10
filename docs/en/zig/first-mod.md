# Your first Zig mod

This builds `examples/hello-pier-zig`: a command, a listener on player joins, a delayed
task, and a service and a bus topic any other mod can use.

## The package

```
build.zig
build.zig.zon
src/main.zig
manifest.json
```

`build.zig.zon` depends on the binding. Inside the Pier repository the example points at
it by path; a mod of your own fetches it with `zig fetch --save` instead.

```zig
.dependencies = .{
    .pier = .{ .path = "../../bindings/zig" },
},
```

`build.zig` builds a dynamic library with the `pier` module imported:

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

`exportMod` exports `pier_main` for the mod type. It runs the handshake, then calls `load`
before the vtable is filled in, and the host calls `enable`, `disable` and `unload` later.
Each one the type declares takes a `*levilamina.Context` and returns `!void`; an error is logged
with its name and refuses that step.

## Build and install

```
zig build
```

`zig-out/bin/hello_pier_zig.dll` is the mod. Put it with `manifest.json` in
`plugins/hello-pier-zig/` and start the server. The log shows the mod loading and enabling,
and one second later the line from the task. `/hellozig` answers in game and from the
console.

If the mod does not load, [Troubleshooting](../guide/troubleshooting.md) lists the log lines and
what each means.
