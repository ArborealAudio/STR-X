const std = @import("std");
const math = std.math;
const util = @import("util.zig");
const map = util.map;
const AudioBuffer = util.AudioBuffer32;
const Filter = @import("Filter.zig");
const LRFilter = @import("LRFilter.zig");

const Arena = std.heap.ArenaAllocator;
const Allocator = std.mem.Allocator;

const StrX = @import("Str_X.zig").StrX;
const StrY = @import("Str_Y.zig").StrY;
const StrZ = @import("Str_Z.zig");

const pedals = @import("pedals.zig");
const TSX = pedals.TSX;
const RXT = pedals.RXT;

/// Base Processor interface
pub const Processor = struct {
    prepare: *const fn (self: *Processor, sample_rate: f64, num_samples: u32, num_channels: u32) void,
    reset: *const fn (self: *Processor) void,
    process: *const fn (self: *Processor, buffer: AudioBuffer) void,
    paramChanged: *const fn (self: *Processor, id: []const u8, val: f32) void,
};

const AmpType = enum { StrX, StrY, StrZ };

const PedalType = enum { TSX, RXT };

// Parent struct for all possible processors, owning the arena so we can
// clean up all resources in one fell swoop
const MainProcessor = struct {
    const AmpArray = std.EnumArray(AmpType, *Processor);
    const PedalArray = std.EnumArray(PedalType, *Processor);

    amps: AmpArray,
    amp_on: bool = true,
    active_amp: AmpType = .StrX,

    pedals: PedalArray,
    pedal_on: bool = false,
    active_pedal: PedalType = .TSX,

    out_vol: f32 = 0,

    arena_impl: *Arena,
    arena: Allocator,

    max_frames: u32,

    pub fn init(num_ch: u32) !*MainProcessor {
        const default_buffer_length = 256;
        var arena = try std.heap.c_allocator.create(Arena);
        arena.* = Arena.init(std.heap.raw_c_allocator);
        const allocator = arena.allocator();

        // TODO load plugin config from user config file

        const self = try allocator.create(MainProcessor);
        self.* = .{
            .arena_impl = arena,
            .arena = allocator,
            .amps = AmpArray.init(.{
                .StrX = try StrX.init(allocator, num_ch),
                .StrY = try StrY.init(allocator, num_ch),
                .StrZ = try StrZ.init(allocator, num_ch),
            }),
            .pedals = PedalArray.init(.{
                .TSX = try TSX.init(allocator, num_ch),
                .RXT = try RXT.init(allocator, num_ch),
            }),
            .max_frames = default_buffer_length,
        };
        return self;
    }

    pub fn getCurrentAmp(self: *MainProcessor) *Processor {
        return self.amps.get(self.active_amp);
    }

    pub fn getCurrentPedal(self: *MainProcessor) *Processor {
        return self.pedals.get(self.active_pedal);
    }

    pub fn paramChange(self: *MainProcessor, id: []const u8, val: f32) void {
        // only prollem is...ain't threadsafe
        // unless we basically promise to ourself never to modify `self.params` outside
        // this fn?
        if (std.mem.eql(u8, id, "amp_on")) {
            self.amp_on = !self.amp_on;
        } else if (std.mem.eql(u8, id, "amp_type")) {
            self.active_amp = @enumFromInt(@as(u32, @intFromFloat(val)));
        } else if (std.mem.eql(u8, id, "out_vol")) {
            self.out_vol = val;
        } else if (std.mem.startsWith(u8, id, "pedal")) {
            if (std.mem.eql(u8, id, "pedal_on")) {
                self.pedal_on = !self.pedal_on;
            } else if (std.mem.eql(u8, id, "pedal_type")) {
                self.active_pedal = @enumFromInt(@as(u32, @intFromFloat(val)));
            } else {
                for (self.pedals.values) |pedal| {
                    pedal.paramChanged(pedal, id, val);
                }
            }
        } else {
            for (self.amps.values) |amp| {
                amp.paramChanged(amp, id, val);
            }
        }
    }

    pub fn process(self: *MainProcessor, c_buffer: [*]const [*]f32, num_frames: u32, num_ch: u32) void {
        const buffer: AudioBuffer = .{
            .data = &.{
                c_buffer[0][0..num_frames],
                c_buffer[1][0..num_frames],
            },
            .num_frames = num_frames,
            .num_channels = num_ch,
        };

        if (self.pedal_on) {
            const pedal = self.getCurrentPedal();
            pedal.process(pedal, buffer);
        }

        if (self.amp_on) {
            const amp = self.getCurrentAmp();
            amp.process(amp, buffer);
        }

        const out_vol_lin = math.pow(f32, 10.0, self.out_vol / 20.0);
        buffer.applyGain(out_vol_lin);
    }
};

export fn processor_init(num_channels: u32) ?*MainProcessor {
    return MainProcessor.init(num_channels) catch |e| {
        std.log.err("Processor init fail: {!}\n", .{e});
        return null;
    };
}

export fn processor_deinit(p: ?*MainProcessor) void {
    if (p) |proc| {
        proc.arena_impl.deinit();
    }
}

export fn processor_prepare(
    p: ?*MainProcessor,
    sample_rate: f64,
    num_samples: u32,
    num_channels: u32,
) void {
    if (p) |proc| {
        for (proc.amps.values) |amp| {
            amp.prepare(amp, sample_rate, num_samples, num_channels);
        }
        for (proc.pedals.values) |pedal| {
            pedal.prepare(pedal, sample_rate, num_samples, num_channels);
        }
    }
}

export fn processor_reset(p: ?*MainProcessor) void {
    if (p) |proc| {
        for (proc.amps.values) |amp| {
            amp.reset(amp);
        }
        for (proc.pedals.values) |pedal| {
            pedal.reset(pedal);
        }
    }
}

export fn processor_process(
    p: ?*MainProcessor,
    buffer: [*][*]f32,
    num_samples: u32,
    num_channels: u32,
) void {
    if (p) |proc| {
        proc.process(buffer, num_samples, num_channels);
    }
}

export fn processor_param_change(p: ?*MainProcessor, id: [*:0]const u8, val: f32) void {
    if (p) |proc| {
        proc.paramChange(std.mem.span(id), val);
    }
}

/// Float parameter with per-sample smoothing
/// Call `set()` to update to a new target value
/// Call `next()` each frame to read the next smooth value
const FloatParam = struct {
    target_val: f32,
    val: []f32,
    step: f32,
    is_smoothing: bool = false,

    pub fn init(allocator: Allocator, val: f32, step: f32, num_ch: u32) !FloatParam {
        const self = FloatParam{
            .target_val = val,
            .val = try allocator.alloc(f32, num_ch),
            .step = step,
        };
        for (self.val) |*v| {
            v.* = val;
        }
        return self;
    }

    pub fn initFromSR(
        allocator: Allocator,
        val: f32,
        sample_rate: f32,
        step_ms: f32,
        num_ch: u32,
    ) FloatParam {
        const self = FloatParam{
            .target_val = val,
            .val = try allocator.alloc(f32, num_ch),
            .step = val / ((step_ms / 1000) * sample_rate),
        };
        for (self.val) |*v| {
            v.* = val;
        }
        return self;
    }

    pub fn next(self: *FloatParam, ch: u32) f32 {
        if (!self.is_smoothing) {
            return self.target_val;
        }

        self.val[ch] += self.step;
        if (self.val[ch] == self.target_val) {
            self.is_smoothing = false;
        }
        return self.val[ch];
    }

    pub fn set(self: *FloatParam, new: f32) void {
        if (new == self.target_val) return;

        self.target_val = new;
        self.is_smoothing = true;
    }
};
