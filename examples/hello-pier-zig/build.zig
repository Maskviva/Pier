const std = @import("std");

/// Builds hello_pier_zig.dll into zig-out/bin. The default target is x86_64-windows-gnu,
/// so the DLL needs no C runtime beyond the system's; Zig cross-compiles it from any host.
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{
        .default_target = .{ .cpu_arch = .x86_64, .os_tag = .windows, .abi = .gnu },
    });
    const optimize = b.standardOptimizeOption(.{ .preferred_optimize_mode = .ReleaseSafe });
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
}
