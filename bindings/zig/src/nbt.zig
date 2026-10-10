//! nbt.zig: SNBT parsing and writing for Zig mods, with the rules of the Rust and Go
//! bindings: a bare number is an int, then a long, then a double; a suffix b, s, l, f or d
//! fixes the type; true and false are bytes; any other bare word is a string.

const std = @import("std");

/// The deepest nesting accepted, the cap Minecraft puts on NBT. A deeper payload is refused
/// instead of exhausting the stack of a host thread.
pub const max_depth = 512;

/// One SNBT value. Typed arrays hold their numbers as i64 whatever the element width.
pub const Value = union(enum) {
    byte: i8,
    short: i16,
    int: i32,
    long: i64,
    float: f32,
    double: f64,
    string: []const u8,
    list: []Value,
    compound: []Entry,
    byte_array: []i64,
    int_array: []i64,
    long_array: []i64,

    /// Follows a dotted path through compounds; null when a step is missing.
    pub fn get(self: Value, path: []const u8) ?Value {
        var cur = self;
        var parts = std.mem.splitScalar(u8, path, '.');
        while (parts.next()) |part| {
            switch (cur) {
                .compound => |entries| {
                    var found: ?Value = null;
                    for (entries) |entry| {
                        if (std.mem.eql(u8, entry.key, part)) found = entry.value;
                    }
                    cur = found orelse return null;
                },
                else => return null,
            }
        }
        return cur;
    }

    /// The string at path, or null when it is absent or not a string.
    pub fn getString(self: Value, path: []const u8) ?[]const u8 {
        const at = self.get(path) orelse return null;
        return switch (at) {
            .string => |text| text,
            else => null,
        };
    }

    /// The integer at path of any width, or null when it is absent or not one.
    pub fn getInt(self: Value, path: []const u8) ?i64 {
        const at = self.get(path) orelse return null;
        return switch (at) {
            .byte => |n| n,
            .short => |n| n,
            .int => |n| n,
            .long => |n| n,
            else => null,
        };
    }

    /// The number at path, integer or floating, or null when it is absent.
    pub fn getFloat(self: Value, path: []const u8) ?f64 {
        const at = self.get(path) orelse return null;
        return switch (at) {
            .float => |x| x,
            .double => |x| x,
            .byte => |n| @floatFromInt(n),
            .short => |n| @floatFromInt(n),
            .int => |n| @floatFromInt(n),
            .long => |n| @floatFromInt(n),
            else => null,
        };
    }

    /// The byte at path read as a boolean, or null when it is absent or not 0 or 1.
    pub fn getBool(self: Value, path: []const u8) ?bool {
        const at = self.get(path) orelse return null;
        return switch (at) {
            .byte => |n| if (n == 0) false else if (n == 1) true else null,
            else => null,
        };
    }
};

/// One key of a compound and its value.
pub const Entry = struct {
    key: []const u8,
    value: Value,
};

pub const ParseError = error{ Malformed, TooDeep, OutOfMemory };

/// Parses SNBT. Every node is allocated with `arena`, which the caller frees as a whole; a
/// bare string or key may also point into `text`, which must outlive the result.
pub fn parse(arena: std.mem.Allocator, text: []const u8) ParseError!Value {
    var p = Parser{ .text = text, .arena = arena };
    const root = try p.value();
    p.ws();
    if (p.pos != text.len) return error.Malformed;
    return root;
}

const Parser = struct {
    text: []const u8,
    arena: std.mem.Allocator,
    pos: usize = 0,
    depth: usize = 0,

    fn ws(self: *Parser) void {
        while (self.pos < self.text.len and std.mem.indexOfScalar(u8, " \t\r\n", self.text[self.pos]) != null) self.pos += 1;
    }

    fn peek(self: *Parser) ?u8 {
        if (self.pos < self.text.len) return self.text[self.pos];
        return null;
    }

    fn expect(self: *Parser, ch: u8) ParseError!void {
        self.ws();
        if (self.peek() != ch) return error.Malformed;
        self.pos += 1;
    }

    fn value(self: *Parser) ParseError!Value {
        self.ws();
        const ch = self.peek() orelse return error.Malformed;
        return switch (ch) {
            '{' => self.nested(.compound),
            '[' => self.nested(.list),
            '"', '\'' => .{ .string = try self.quoted() },
            else => self.scalar(),
        };
    }

    const Shape = enum { compound, list };

    fn nested(self: *Parser, shape: Shape) ParseError!Value {
        if (self.depth >= max_depth) return error.TooDeep;
        self.depth += 1;
        defer self.depth -= 1;
        return switch (shape) {
            .compound => self.compound(),
            .list => self.listOrArray(),
        };
    }

    fn compound(self: *Parser) ParseError!Value {
        try self.expect('{');
        var entries: std.ArrayList(Entry) = .empty;
        self.ws();
        if (self.peek() == '}') {
            self.pos += 1;
            return .{ .compound = try entries.toOwnedSlice(self.arena) };
        }
        while (true) {
            self.ws();
            const name = try self.key();
            try self.expect(':');
            const item = try self.value();
            var replaced = false;
            for (entries.items) |*entry| {
                if (std.mem.eql(u8, entry.key, name)) {
                    entry.value = item;
                    replaced = true;
                }
            }
            if (!replaced) try entries.append(self.arena, .{ .key = name, .value = item });
            self.ws();
            const ch = self.peek() orelse return error.Malformed;
            self.pos += 1;
            if (ch == '}') return .{ .compound = try entries.toOwnedSlice(self.arena) };
            if (ch != ',') return error.Malformed;
        }
    }

    fn key(self: *Parser) ParseError![]const u8 {
        const ch = self.peek() orelse return error.Malformed;
        if (ch == '"' or ch == '\'') return self.quoted();
        const start = self.pos;
        while (self.pos < self.text.len and isBare(self.text[self.pos])) self.pos += 1;
        if (self.pos == start) return error.Malformed;
        return self.text[start..self.pos];
    }

    fn listOrArray(self: *Parser) ParseError!Value {
        try self.expect('[');
        if (self.pos + 1 < self.text.len and self.text[self.pos + 1] == ';') {
            const tag = self.text[self.pos];
            if (tag == 'B' or tag == 'I' or tag == 'L') {
                self.pos += 2;
                const nums = try self.typedArray();
                return switch (tag) {
                    'B' => .{ .byte_array = nums },
                    'I' => .{ .int_array = nums },
                    else => .{ .long_array = nums },
                };
            }
        }
        var items: std.ArrayList(Value) = .empty;
        self.ws();
        if (self.peek() == ']') {
            self.pos += 1;
            return .{ .list = try items.toOwnedSlice(self.arena) };
        }
        while (true) {
            const item = try self.value();
            try items.append(self.arena, item);
            self.ws();
            const ch = self.peek() orelse return error.Malformed;
            self.pos += 1;
            if (ch == ']') return .{ .list = try items.toOwnedSlice(self.arena) };
            if (ch != ',') return error.Malformed;
        }
    }

    fn typedArray(self: *Parser) ParseError![]i64 {
        var nums: std.ArrayList(i64) = .empty;
        self.ws();
        if (self.peek() == ']') {
            self.pos += 1;
            return nums.toOwnedSlice(self.arena);
        }
        while (true) {
            const item = try self.value();
            const num: i64 = switch (item) {
                .byte => |n| n,
                .short => |n| n,
                .int => |n| n,
                .long => |n| n,
                else => return error.Malformed,
            };
            try nums.append(self.arena, num);
            self.ws();
            const ch = self.peek() orelse return error.Malformed;
            self.pos += 1;
            if (ch == ']') return nums.toOwnedSlice(self.arena);
            if (ch != ',') return error.Malformed;
        }
    }

    fn quoted(self: *Parser) ParseError![]const u8 {
        const quote = self.text[self.pos];
        self.pos += 1;
        var out: std.ArrayList(u8) = .empty;
        while (true) {
            if (self.pos >= self.text.len) return error.Malformed;
            const ch = self.text[self.pos];
            self.pos += 1;
            if (ch == quote) return out.toOwnedSlice(self.arena);
            if (ch != '\\') {
                try out.append(self.arena, ch);
                continue;
            }
            if (self.pos >= self.text.len) return error.Malformed;
            const esc = self.text[self.pos];
            self.pos += 1;
            switch (esc) {
                'n' => try out.append(self.arena, '\n'),
                'r' => try out.append(self.arena, '\r'),
                't' => try out.append(self.arena, '\t'),
                'b' => try out.append(self.arena, 8),
                'f' => try out.append(self.arena, 12),
                '0' => try out.append(self.arena, 0),
                'u' => {
                    if (self.pos + 4 > self.text.len) return error.Malformed;
                    const point = std.fmt.parseInt(u21, self.text[self.pos .. self.pos + 4], 16) catch return error.Malformed;
                    self.pos += 4;
                    var enc: [4]u8 = undefined;
                    const len = std.unicode.utf8Encode(point, &enc) catch return error.Malformed;
                    try out.appendSlice(self.arena, enc[0..len]);
                },
                else => try out.append(self.arena, esc),
            }
        }
    }

    fn scalar(self: *Parser) ParseError!Value {
        const start = self.pos;
        while (self.pos < self.text.len and isBare(self.text[self.pos])) self.pos += 1;
        if (self.pos == start) return error.Malformed;
        const raw = self.text[start..self.pos];
        if (std.mem.eql(u8, raw, "true")) return .{ .byte = 1 };
        if (std.mem.eql(u8, raw, "false")) return .{ .byte = 0 };
        var body = raw;
        var suffix: u8 = 0;
        const last = raw[raw.len - 1];
        if (std.mem.indexOfScalar(u8, "bBsSlLfFdD", last) != null) {
            body = raw[0 .. raw.len - 1];
            suffix = last | 0x20;
        }
        if (isNumeric(body)) {
            switch (suffix) {
                'b' => if (std.fmt.parseInt(i64, body, 10)) |n| return .{ .byte = @truncate(n) } else |_| {},
                's' => if (std.fmt.parseInt(i64, body, 10)) |n| return .{ .short = @truncate(n) } else |_| {},
                'l' => if (std.fmt.parseInt(i64, body, 10)) |n| return .{ .long = n } else |_| {},
                'f' => if (std.fmt.parseFloat(f32, body)) |x| return .{ .float = x } else |_| {},
                'd' => if (std.fmt.parseFloat(f64, body)) |x| return .{ .double = x } else |_| {},
                else => {
                    if (std.fmt.parseInt(i32, raw, 10)) |n| return .{ .int = n } else |_| {}
                    if (std.fmt.parseInt(i64, raw, 10)) |n| return .{ .long = n } else |_| {}
                    if (std.fmt.parseFloat(f64, raw)) |x| return .{ .double = x } else |_| {}
                },
            }
        }
        return .{ .string = raw };
    }
};

fn isBare(ch: u8) bool {
    return std.ascii.isAlphanumeric(ch) or ch == '_' or ch == '-' or ch == '+' or ch == '.';
}

fn isNumeric(text: []const u8) bool {
    if (text.len == 0) return false;
    for (text) |ch| {
        if (!std.ascii.isDigit(ch) and std.mem.indexOfScalar(u8, "-+.eE", ch) == null) return false;
    }
    return true;
}

test "scalars and containers follow the binding rules" {
    var arena_state = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena_state.deinit();
    const arena = arena_state.allocator();
    const root = try parse(arena, "{a:1,b:2b,c:3L,d:1.5,e:1.5f,f:true,g:word,h:\"q\\\"x\",i:[I;1,2],j:[1,2],k:{m:-4s}}");
    try std.testing.expectEqual(@as(?i64, 1), root.getInt("a"));
    try std.testing.expectEqual(@as(?i64, 2), root.getInt("b"));
    try std.testing.expectEqual(@as(?i64, 3), root.getInt("c"));
    try std.testing.expectEqual(@as(?f64, 1.5), root.getFloat("d"));
    try std.testing.expectEqual(@as(?f64, 1.5), root.getFloat("e"));
    try std.testing.expectEqual(@as(?bool, true), root.getBool("f"));
    try std.testing.expectEqualStrings("word", root.getString("g").?);
    try std.testing.expectEqualStrings("q\"x", root.getString("h").?);
    try std.testing.expectEqual(@as(usize, 2), root.get("i").?.int_array.len);
    try std.testing.expectEqual(@as(usize, 2), root.get("j").?.list.len);
    try std.testing.expectEqual(@as(?i64, -4), root.getInt("k.m"));
    try std.testing.expectEqual(@as(?i64, null), root.getInt("missing"));
}

test "a bare name is a string, and a big integer is a long" {
    var arena_state = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena_state.deinit();
    const arena = arena_state.allocator();
    const name = try parse(arena, "Steve");
    try std.testing.expectEqualStrings("Steve", name.string);
    const big = try parse(arena, "3000000000");
    try std.testing.expectEqual(@as(i64, 3000000000), big.long);
}

test "nesting past the cap is an error and not a stack overflow" {
    var arena_state = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena_state.deinit();
    const arena = arena_state.allocator();
    try std.testing.expectError(error.TooDeep, parse(arena, "[" ** 600));
    const ok_depth = ("[" ** max_depth) ++ ("]" ** max_depth);
    _ = try parse(arena, ok_depth);
}
