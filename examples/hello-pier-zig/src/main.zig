//! hello-pier-zig: the smallest Zig mod that does something: one command, one event
//! listener, one delayed task, and a service and a bus topic any other mod can use, in
//! whatever language it is written, with a log line at every lifecycle step.

const std = @import("std");
const levilamina = @import("levilamina");

const Hello = struct {
    var joins: ?levilamina.Listener = null;
    var greet: ?levilamina.ServiceRegistration = null;
    var pings: ?levilamina.Subscription = null;

    pub fn load(ctx: *levilamina.Context) !void {
        ctx.info("loaded");
    }

    pub fn enable(ctx: *levilamina.Context) !void {
        ctx.info("enabled");
        joins = try levilamina.subscribe("ll::event::PlayerJoinEvent", .normal, onJoin);

        // Not fatal: the mod still works without its command.
        levilamina.registerCommand("hellozig", "Says hello from a Zig mod.", .any, onHello) catch |reg_err| {
            levilamina.logf(.warn, "/hellozig was not registered: {s}", .{@errorName(reg_err)});
        };

        // Another mod, in any language, calls service "hello-pier-zig:greet" and publishes
        // on "hello:ping"; this one answers on "hello:pong".
        greet = try levilamina.registerService("hello-pier-zig:greet", greetProvider);
        pings = try levilamina.busSubscribe("hello:ping", onPing);

        _ = try levilamina.scheduleAfter(1000, later);
    }

    pub fn disable(ctx: *levilamina.Context) !void {
        ctx.info("disabled");
        if (joins) |*listener| try listener.unsubscribe();
        if (pings) |*sub| try sub.unsubscribe();
        if (greet) |*reg| try reg.unregister();
    }

    pub fn unload(ctx: *levilamina.Context) !void {
        ctx.info("unloaded");
    }

    fn onJoin(ev: *const levilamina.Event) void {
        levilamina.logf(.info, "a player joined: {s}", .{ev.snbt});
    }

    fn onHello(inv: *const levilamina.Invocation) void {
        var line_buf: [256]u8 = undefined;
        const line = std.fmt.bufPrint(&line_buf, "hello from a Zig mod, {s}", .{inv.origin}) catch "hello from a Zig mod";
        inv.success(line);
    }

    fn greetProvider(request: []const u8, reply: *const levilamina.Reply) bool {
        if (request.len == 0) {
            reply.send("say who to greet");
            return false;
        }
        var text_buf: [256]u8 = undefined;
        const text = std.fmt.bufPrint(&text_buf, "hello, {s}, from a Zig mod", .{request}) catch {
            reply.send("the name is too long to greet");
            return false;
        };
        reply.send(text);
        return true;
    }

    fn onPing(_: []const u8, payload: []const u8) bool {
        levilamina.logf(.info, "ping from another mod: {s}", .{payload});
        _ = levilamina.busPublish("hello:pong", payload) catch 0;
        return false;
    }

    fn later() void {
        levilamina.log(.info, "a task scheduled one second ago ran on the server thread");
    }
};

comptime {
    levilamina.exportMod(Hello, "hello-pier-zig");
}
