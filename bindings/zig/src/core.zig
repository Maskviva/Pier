//! core.zig: the handshake, the slot gate, strings, logging, tasks, events, commands and
//! the cross-mod channels of the Zig binding.
//!
//! Zig calls the host's function pointers directly, so there is no generated layer between:
//! `slot("name")` is the function pointer of that slot, gated by both checks of contract
//! section 10, and every other function here is built on it.

const std = @import("std");
const build_options = @import("build_options");

/// The ABI as translate-c read it from abi.h: every type, slot and constant.
pub const c = @import("abi");

/// The table and this mod's handle, set once by pier_main and only read after.
var host_api: ?*const c.PierApi = null;
var mod_handle: c.PierModHandle = null;
var mod_name: []const u8 = "";

/// The target this DLL declares; build with -Dclient=true for the client host.
pub const mod_flags: u32 = if (build_options.client) c.PIER_FLAG_CLIENT else 0;

/// Why a call failed. A read the host cannot answer returns NoAnswer.
pub const Error = error{
    /// The host has no such slot: it is older than this binding or lacks the capability.
    NotProvided,
    /// The host ran the call and refused it.
    Refused,
    /// The host has no answer for the read.
    NoAnswer,
    OutOfMemory,
};

/// The function pointer of slot `name`, or null when the host's table is too short to hold
/// it or the slot is NULL. Every slot of abi.h is reachable this way.
pub fn slot(comptime name: []const u8) @FieldType(c.PierApi, name) {
    const table = host_api orelse return null;
    const end = @offsetOf(c.PierApi, name) + @sizeOf(@FieldType(c.PierApi, name));
    if (table.struct_size < end) return null;
    return @field(table.*, name);
}

/// The caller's own mod handle, for a slot that takes one.
pub fn handle() c.PierModHandle {
    return mod_handle;
}

/// A Zig slice as a PierStr; the host reads it during the call only.
pub fn str(text: []const u8) c.PierStr {
    return .{ .ptr = text.ptr, .len = text.len };
}

/// A PierStr as a slice, valid only during the callback that received it.
pub fn view(s: c.PierStr) []const u8 {
    if (s.ptr == null or s.len == 0) return "";
    return s.ptr[0..s.len];
}

/// The levels of the log slot, which mirror ll::io::LogLevel.
pub const Level = enum(i32) { fatal = 0, err = 1, warn = 2, info = 3, debug = 4, trace = 5 };

/// Writes one line through this mod's LeviLamina logger. Safe from any thread.
pub fn log(level: Level, msg: []const u8) void {
    const log_fn = slot("log") orelse return;
    log_fn(mod_handle, @intFromEnum(level), str(msg));
}

/// log with std.fmt formatting, into a buffer of 2048 bytes.
pub fn logf(level: Level, comptime fmt: []const u8, args: anytype) void {
    var line_buf: [2048]u8 = undefined;
    const line = std.fmt.bufPrint(&line_buf, fmt, args) catch "(a log line longer than 2048 bytes was dropped)";
    log(level, line);
}

/// What a lifecycle step receives.
pub const Context = struct {
    pub fn info(_: *Context, msg: []const u8) void {
        log(.info, msg);
    }
    pub fn warn(_: *Context, msg: []const u8) void {
        log(.warn, msg);
    }
    pub fn err(_: *Context, msg: []const u8) void {
        log(.err, msg);
    }
};

/// Exports pier_main for mod type M, named `name` in the log. Call it from the root file:
///
///     comptime { levilamina.exportMod(MyMod, "my-mod"); }
///
/// M declares any of `pub fn load`, `enable`, `disable` and `unload`, each taking a
/// `*levilamina.Context` and returning `!void`; an error is logged and refuses the step.
pub fn exportMod(comptime M: type, comptime name: []const u8) void {
    const Entry = struct {
        fn main(table_opt: ?*const c.PierApi, own_handle: c.PierModHandle, out_opt: ?*c.PierModVTable) callconv(.c) bool {
            const table = table_opt orelse return false;
            const out = out_opt orelse return false;
            host_api = table;
            // Until the table is known to reach log nothing can be said, so this failure
            // is silent here and reported by the host.
            if (slot("log") == null) {
                host_api = null;
                return false;
            }
            mod_handle = own_handle;
            mod_name = name;
            if (table.abi_version < c.PIER_ABI_VERSION) {
                logf(.err, "this host speaks Pier ABI v{d} and this mod was built against v{d}; upgrade Pier", .{ table.abi_version, @as(u32, c.PIER_ABI_VERSION) });
                return false;
            }
            if ((table.host_flags & c.PIER_FLAG_CLIENT) != (mod_flags & c.PIER_FLAG_CLIENT)) {
                log(.err, "this mod was built for the other target, server or client, than this host");
                return false;
            }
            if (!stage("load")) return false;
            out.* = .{
                .struct_size = @sizeOf(c.PierModVTable),
                .abi_version = c.PIER_ABI_VERSION,
                .mod_flags = mod_flags,
                ._reserved0 = 0,
                .instance = null,
                .on_enable = &onEnable,
                .on_disable = &onDisable,
                .on_unload = &onUnload,
            };
            return true;
        }

        fn stage(comptime step: []const u8) bool {
            if (!@hasDecl(M, step)) return true;
            var ctx: Context = .{};
            // Widened first, so a step whose inferred error set is empty compiles the same.
            const result: anyerror!void = @field(M, step)(&ctx);
            result catch |step_err| {
                logf(.err, "{s}: {s} failed: {s}", .{ name, step, @errorName(step_err) });
                return false;
            };
            return true;
        }

        fn onEnable(_: ?*anyopaque) callconv(.c) bool {
            return stage("enable");
        }

        fn onDisable(_: ?*anyopaque) callconv(.c) bool {
            return stage("disable");
        }

        fn onUnload(_: ?*anyopaque) callconv(.c) bool {
            return stage("unload");
        }
    };
    @export(&Entry.main, .{ .name = "pier_main" });
}

/// Gathers what a PierStrSink receives during one call, each piece copied with allocator.
pub const Collector = struct {
    allocator: std.mem.Allocator,
    items: std.ArrayList([]u8) = .empty,
    failed: bool = false,

    pub fn init(allocator: std.mem.Allocator) Collector {
        return .{ .allocator = allocator };
    }

    pub fn deinit(self: *Collector) void {
        for (self.items.items) |piece| self.allocator.free(piece);
        self.items.deinit(self.allocator);
    }

    /// The PierStrSink to hand the host, with a *Collector as its ctx.
    pub fn sink(ctx: ?*anyopaque, s: c.PierStr) callconv(.c) void {
        const self: *Collector = @ptrCast(@alignCast(ctx orelse return));
        const piece = self.allocator.dupe(u8, view(s)) catch {
            self.failed = true;
            return;
        };
        self.items.append(self.allocator, piece) catch {
            self.allocator.free(piece);
            self.failed = true;
        };
    }

    /// The last piece, owned by the caller; NoAnswer when the sink received nothing.
    pub fn take(self: *Collector) Error![]u8 {
        if (self.failed) return error.OutOfMemory;
        return self.items.pop() orelse error.NoAnswer;
    }

    /// The last piece, or an empty owned slice when the sink received nothing.
    pub fn takeOrEmpty(self: *Collector) Error![]u8 {
        if (self.failed) return error.OutOfMemory;
        if (self.items.pop()) |piece| return piece;
        return self.allocator.alloc(u8, 0);
    }

    /// Every piece, owned by the caller with the slice holding them.
    pub fn takeAll(self: *Collector) Error![][]u8 {
        if (self.failed) return error.OutOfMemory;
        return self.items.toOwnedSlice(self.allocator);
    }
};

/// A task id, for cancel.
pub const TaskId = u64;

/// Runs `task` on the server thread as soon as it can. Safe from any thread. The task
/// belongs to this mod: if the mod unloads first, the host drops it and it never runs.
pub fn schedule(comptime task: fn () void) Error!TaskId {
    const sched = slot("schedule_for") orelse return error.NotProvided;
    const Task = struct {
        fn run(_: ?*anyopaque) callconv(.c) void {
            task();
        }
    };
    const id = sched(mod_handle, &Task.run, null);
    if (id == 0) return error.Refused;
    return id;
}

/// Runs `task` on the server thread after delay_ms, under the ownership of schedule.
pub fn scheduleAfter(delay_ms: u64, comptime task: fn () void) Error!TaskId {
    const sched = slot("schedule_after_for") orelse return error.NotProvided;
    const Task = struct {
        fn run(_: ?*anyopaque) callconv(.c) void {
            task();
        }
    };
    const id = sched(mod_handle, &Task.run, null, delay_ms);
    if (id == 0) return error.Refused;
    return id;
}

/// Voids a task that has not run; false for one that ran, was cancelled or is not this mod's.
pub fn cancel(id: TaskId) Error!bool {
    const cancel_fn = slot("schedule_cancel") orelse return error.NotProvided;
    return cancel_fn(mod_handle, id);
}

/// This mod's tasks that have not run. A host that cannot count returns an error.
pub fn pendingTasks() Error!u32 {
    const count_fn = slot("schedule_pending_count") orelse return error.NotProvided;
    return count_fn(mod_handle);
}

/// Listener order, mirroring ll::event::EventPriority.
pub const Priority = enum(i32) { highest = 0, high = 1, normal = 2, low = 3, lowest = 4 };

/// One event as a listener receives it; id and snbt are valid during the callback only.
pub const Event = struct {
    id: []const u8,
    snbt: []const u8,
    write_ctx: ?*anyopaque,
    write_back: c.PierStrSink,

    /// Writes keys back into the event, as SNBT of only the keys that change; the host
    /// merges them, so two listeners writing different keys keep both.
    pub fn writeBack(self: *const Event, edit: []const u8) void {
        if (self.write_back) |wb| wb(self.write_ctx, str(edit));
    }

    /// Asks the host to cancel the event. An event that cannot be cancelled ignores it.
    pub fn cancel(self: *const Event) void {
        self.writeBack("{cancelled:1b}");
    }
};

/// One subscription, kept to end it.
pub const Listener = struct {
    raw: c.PierListenerHandle,

    /// Ends the subscription; calling it twice is harmless. Server thread only.
    pub fn unsubscribe(self: *Listener) Error!void {
        const raw = self.raw orelse return;
        const unsub = slot("unsubscribe_event") orelse return error.NotProvided;
        self.raw = null;
        if (!unsub(mod_handle, raw)) return error.Refused;
    }
};

/// Calls `handler` for every event with this id, on the server thread. Server thread only.
pub fn subscribe(id: []const u8, priority: Priority, comptime handler: fn (*const Event) void) Error!Listener {
    const sub = slot("subscribe_event") orelse return error.NotProvided;
    const Trampoline = struct {
        fn call(_: ?*anyopaque, event_id: c.PierStr, snbt: c.PierStr, write_ctx: ?*anyopaque, write_back: c.PierStrSink) callconv(.c) void {
            const ev = Event{ .id = view(event_id), .snbt = view(snbt), .write_ctx = write_ctx, .write_back = write_back };
            handler(&ev);
        }
    };
    const raw = sub(mod_handle, str(id), @intFromEnum(priority), &Trampoline.call, null);
    if (raw == null) return error.Refused;
    return .{ .raw = raw };
}

/// Who may run a command, mirroring CommandPermissionLevel.
pub const Permission = enum(i32) { any = 0, game_directors = 1, admin = 2, host = 3, owner = 4 };

/// One run of a command; valid during the callback only.
pub const Invocation = struct {
    args: []const u8,
    origin: []const u8,
    ctx: ?*anyopaque,
    ok: c.PierStrSink,
    fail: c.PierStrSink,

    /// Sends a line of output.
    pub fn success(self: *const Invocation, msg: []const u8) void {
        if (self.ok) |out| out(self.ctx, str(msg));
    }

    /// Sends a line of error output.
    pub fn failure(self: *const Invocation, msg: []const u8) void {
        if (self.fail) |out| out(self.ctx, str(msg));
    }
};

/// Registers /name, whose text after the name reaches handler as args. Call it from enable.
pub fn registerCommand(name: []const u8, description: []const u8, permission: Permission, comptime handler: fn (*const Invocation) void) Error!void {
    const reg = slot("register_command") orelse return error.NotProvided;
    const Trampoline = struct {
        fn call(_: ?*anyopaque, args: c.PierStr, origin: c.PierStr, ctx: ?*anyopaque, ok: c.PierStrSink, fail: c.PierStrSink) callconv(.c) void {
            const inv = Invocation{ .args = view(args), .origin = view(origin), .ctx = ctx, .ok = ok, .fail = fail };
            handler(&inv);
        }
    };
    if (!reg(mod_handle, str(name), str(description), @intFromEnum(permission), &Trampoline.call, null)) return error.Refused;
}

/// One bus subscription, kept to end it.
pub const Subscription = struct {
    id: u64,

    /// Ends the subscription; calling it twice is harmless.
    pub fn unsubscribe(self: *Subscription) Error!void {
        if (self.id == 0) return;
        const unsub = slot("bus_unsubscribe") orelse return error.NotProvided;
        const id = self.id;
        self.id = 0;
        if (!unsub(mod_handle, id)) return error.Refused;
    }
};

/// Calls `handler(topic, payload)` for every message on topic, from mods of any language, on
/// the publisher's thread. It returns a veto: true refuses a vetoable publish and false has
/// no opinion, and a plain publish ignores it. Namespace topics, as "plot:enter".
pub fn busSubscribe(topic: []const u8, comptime handler: fn ([]const u8, []const u8) bool) Error!Subscription {
    const sub = slot("bus_subscribe") orelse return error.NotProvided;
    const Trampoline = struct {
        fn call(_: ?*anyopaque, msg_topic: c.PierStr, payload: c.PierStr) callconv(.c) bool {
            return handler(view(msg_topic), view(payload));
        }
    };
    const id = sub(mod_handle, str(topic), &Trampoline.call, null);
    if (id == 0) return error.Refused;
    return .{ .id = id };
}

/// Sends payload to every subscriber of topic and returns how many ran; 0 means nobody is
/// listening, which is not an error.
pub fn busPublish(topic: []const u8, payload: []const u8) Error!u32 {
    const publish = slot("bus_publish") orelse return error.NotProvided;
    return publish(mod_handle, str(topic), str(payload));
}

/// The result of one vetoable publish. There is no short circuit: after a veto the later
/// subscribers still receive the message.
pub const Vetoable = struct { vetoed: bool, delivered: u32 };

/// Sends payload and collects the subscribers' vetoes.
pub fn busPublishVetoable(topic: []const u8, payload: []const u8) Error!Vetoable {
    const publish = slot("bus_publish_vetoable") orelse return error.NotProvided;
    var delivered: u32 = 0;
    const vetoed = publish(mod_handle, str(topic), str(payload), &delivered);
    return .{ .vetoed = vetoed, .delivered = delivered };
}

/// How many subscribers topic has now.
pub fn busSubscriberCount(topic: []const u8) Error!u32 {
    const count_fn = slot("bus_subscriber_count") orelse return error.NotProvided;
    return count_fn(str(topic));
}

/// The way a service provider answers: send the reply and return true, or send the reason
/// and return false, which the caller receives unchanged as the provider's message.
pub const Reply = struct {
    ctx: ?*anyopaque,
    sink: c.PierStrSink,

    pub fn send(self: *const Reply, text: []const u8) void {
        if (self.sink) |out| out(self.ctx, str(text));
    }
};

/// One service this mod provides, kept to withdraw it.
pub const ServiceRegistration = struct {
    id: u64,

    /// Withdraws the service; calling it twice is harmless.
    pub fn unregister(self: *ServiceRegistration) Error!void {
        if (self.id == 0) return;
        const unreg = slot("service_unregister") orelse return error.NotProvided;
        const id = self.id;
        self.id = 0;
        if (!unreg(mod_handle, id)) return error.Refused;
    }
};

/// Answers calls to name from mods of any language, and from native plugins through the
/// bridge. `provider(request, reply)` runs on the caller's thread, also while this mod is
/// disabled but loaded, since consumers resolve services in their own on_load.
pub fn registerService(name: []const u8, comptime provider: fn ([]const u8, *const Reply) bool) Error!ServiceRegistration {
    const reg = slot("service_register") orelse return error.NotProvided;
    const Trampoline = struct {
        fn call(_: ?*anyopaque, _: c.PierStr, request: c.PierStr, ctx: ?*anyopaque, sink: c.PierStrSink) callconv(.c) bool {
            const reply = Reply{ .ctx = ctx, .sink = sink };
            return provider(view(request), &reply);
        }
    };
    const id = reg(mod_handle, str(name), &Trampoline.call, null);
    if (id == 0) return error.Refused;
    return .{ .id = id };
}

/// How a service call ended, matching the PIER_SERVICE_* results.
pub const CallCode = enum { ok, not_found, provider_error, refused };

/// One service call's outcome. body is the reply for ok and the provider's message for
/// provider_error, owned by the caller.
pub const CallOutcome = struct { code: CallCode, body: []u8 };

/// Calls another mod's service, whatever language it is written in.
pub fn callService(allocator: std.mem.Allocator, name: []const u8, request: []const u8) Error!CallOutcome {
    const call_fn = slot("service_call") orelse return error.NotProvided;
    var col = Collector.init(allocator);
    defer col.deinit();
    const code = call_fn(mod_handle, str(name), str(request), &col, &Collector.sink);
    const body = try col.takeOrEmpty();
    const outcome: CallCode = switch (code) {
        c.PIER_SERVICE_OK => .ok,
        c.PIER_SERVICE_NOT_FOUND => .not_found,
        c.PIER_SERVICE_ERROR => .provider_error,
        else => .refused,
    };
    return .{ .code = outcome, .body = body };
}

/// Every registered service as the host's JSON array of {"name","mod"}, owned by the caller.
pub fn listServicesJson(allocator: std.mem.Allocator) Error![]u8 {
    const list_fn = slot("service_list") orelse return error.NotProvided;
    var col = Collector.init(allocator);
    defer col.deinit();
    list_fn(&col, &Collector.sink);
    return col.takeOrEmpty();
}

/// Inside a provider, the mod whose call is running; null outside one and for a caller with
/// no mod, such as a native plugin through the bridge.
pub fn serviceCaller(allocator: std.mem.Allocator) Error!?[]u8 {
    const caller_fn = slot("service_caller") orelse return error.NotProvided;
    var col = Collector.init(allocator);
    defer col.deinit();
    caller_fn(&col, &Collector.sink);
    const caller = col.take() catch |take_err| {
        if (take_err == error.NoAnswer) return null;
        return take_err;
    };
    return caller;
}

/// Runs cmd as the console. Server thread only. The output lines are logged at debug level;
/// a false from the host, a level that is not ready, is Refused.
pub fn executeCommand(cmd: []const u8) Error!void {
    const exec = slot("execute_command") orelse return error.NotProvided;
    const Out = struct {
        fn line(_: ?*anyopaque, _: bool, output: c.PierStr) callconv(.c) void {
            log(.debug, view(output));
        }
    };
    if (!exec(str(cmd), null, &Out.line)) return error.Refused;
}
