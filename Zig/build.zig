const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{
        .default_target = .{
            .cpu_arch = .x86_64,
            .os_tag = .windows,
        },
    });
    if (target.result.os.tag != .windows) {
        std.log.err("Windows only application; {s} not supported", .{@tagName(target.result.os.tag)});
        return;
    }
    const optimize = b.standardOptimizeOption(.{});

    const win_c = b.addTranslateC(.{
        .root_source_file = b.path("win.h"),
        .target = target,
        .optimize = optimize,
    });

    const exe_module = b.createModule(.{
        .root_source_file = b.path("WinGDITemplate.zig"),
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .imports = &.{
            .{ .name = "win", .module = win_c.createModule() },
        },
    });
    exe_module.linkSystemLibrary("gdi32", .{});
    exe_module.linkSystemLibrary("user32", .{});
    exe_module.linkSystemLibrary("kernel32", .{});

    const exe = b.addExecutable(.{
        .name = "WinGDITemplate",
        .root_module = exe_module,
    });

    exe.subsystem = .windows;
    exe.mingw_unicode_entry_point = true;

    b.installArtifact(exe);
}
