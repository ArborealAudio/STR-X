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

/// Base Processor interface
pub const Processor = struct {
    prepare: *const fn (self: *Processor, sample_rate: f64, num_samples: u32, num_channels: u32) void,
    reset: *const fn (self: *Processor) void,
    process: *const fn (self: *Processor, buffer: AudioBuffer) void,
    paramChanged: *const fn (self: *Processor, id: []const u8, val: f32) void,
};

const AmpType = enum {
    StrX,
    StrY,
    StrZ,
};

// Parent struct for all possible processors, owning the arena so we can
// clean up all resources in one fell swoop
const MainProcessor = struct {
    const ProcArray = std.EnumArray(AmpType, *Processor);

    procs: ProcArray,
    active_proc: AmpType = .StrX,

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
            .procs = ProcArray.init(.{
                .StrX = try StrX.init(allocator, num_ch),
                .StrY = try StrY.init(allocator, num_ch),
                .StrZ = try StrZ.init(allocator, num_ch),
            }),
            .max_frames = default_buffer_length,
        };
        return self;
    }

    pub fn getCurrentProc(self: *MainProcessor) *Processor {
        return self.procs.get(self.active_proc);
    }

    pub fn paramChange(self: *MainProcessor, id: []const u8, val: f32) void {
        // only prollem is...ain't threadsafe
        // unless we basically promise to ourself never to modify `self.params` outside
        // this fn?
        if (std.mem.eql(u8, id, "amp")) {
            self.active_proc = @enumFromInt(@as(u32, @intFromFloat(val)));
        } else if (std.mem.eql(u8, id, "out_vol")) {
            self.out_vol = val;
        } else {
            for (self.procs.values) |amp| {
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

        const amp = self.getCurrentProc();
        amp.process(amp, buffer);

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
        for (proc.procs.values) |amp| {
            amp.prepare(amp, sample_rate, num_samples, num_channels);
        }
    }
}

export fn processor_reset(p: ?*MainProcessor) void {
    if (p) |proc| {
        for (proc.procs.values) |amp| {
            amp.reset(amp);
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

// TS9 guitar pedal
pub const TSX = struct {
    proc: Processor,
    hpf: Filter,
    lpf: Filter,
    lpf2: Filter,

    gain: *const f32,

    pub fn init(arena: Allocator, num_channels: u32, gain: *const f32) !TSX {
        const ts9: TSX = .{
            .proc = .{
                .prepare = prepare,
                .reset = reset,
                .process = process,
            },
            .hpf = try Filter.init(arena, num_channels, .FirstOrderHighpass, 720, 0),
            .lpf = try Filter.init(arena, num_channels, .FirstOrderLowpass, 5600, 0),
            .lpf2 = try Filter.init(arena, num_channels, .FirstOrderLowpass, 723.4, 0),
            .gain = gain,
        };
        return ts9;
    }

    fn prepare(p: *Processor, sample_rate: f64, _: u32, _: u32) void {
        const self: *TSX = @fieldParentPtr("proc", p);
        self.hpf.setSampleRate(@floatCast(sample_rate));
        self.lpf.setSampleRate(@floatCast(sample_rate));
        self.lpf2.setSampleRate(@floatCast(sample_rate));
    }

    fn reset(p: *Processor) void {
        const self: *TSX = @fieldParentPtr("proc", p);
        self.hpf.reset();
        self.lpf.reset();
        self.lpf2.reset();
    }

    fn process(p: *Processor, buffer: AudioBuffer) void {
        const self: *TSX = @fieldParentPtr("proc", p);
        const gain = self.gain.*;
        const k = gain * 2;
        for (buffer.data, 0..) |ch, ch_idx| {
            for (ch) |*sample| {
                const x = sample.*;
                var y: f32 = x;
                y *= gain / 2;

                y = self.hpf.processSample(ch_idx, y);
                y = self.lpf.processSample(ch_idx, y);

                y = math.tanh(k * y) / math.tanh(k);

                y = self.lpf2.processSample(ch_idx, y);

                y = (y * gain / 10) + (x * (1 - (gain / 10)));

                sample.* = y;
            }
        }
    }
};
