const std = @import("std");

/// The levilamina module: `b.dependency("pier", .{ .target = ..., .optimize = ... }).module("levilamina")`.
/// -Dclient=true builds mods for the client host. `zig build test` type-checks the whole
/// binding against abi.h and runs the SNBT tests.
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const client = b.option(bool, "client", "Build mods for the client host: sets the client bit of mod_flags") orelse false;

    const options = b.addOptions();
    options.addOption(bool, "client", client);

    const abi = b.addTranslateC(.{
        .root_source_file = b.path("include/sdk/abi.h"),
        .target = target,
        .optimize = optimize,
    });

    const levilamina = b.addModule("levilamina", .{
        .root_source_file = b.path("src/levilamina.zig"),
        .target = target,
        .optimize = optimize,
        .imports = &.{
            .{ .name = "abi", .module = abi.createModule() },
            .{ .name = "build_options", .module = options.createModule() },
        },
    });

    const tests = b.addTest(.{ .root_module = levilamina });
    const run_tests = b.addRunArtifact(tests);
    const test_step = b.step("test", "Type-check the whole binding against abi.h and run its tests");
    test_step.dependOn(&run_tests.step);
}
