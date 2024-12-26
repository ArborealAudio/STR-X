const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const proc = b.addStaticLibrary(.{
        .name = "Processor",
        .root_source_file = b.path("processor.zig"),
        .optimize = optimize,
        .target = target,
        .pic = true,
        .link_libc = true,
    });
    proc.bundle_compiler_rt = true;

    b.installArtifact(proc);
}
