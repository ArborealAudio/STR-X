const std = @import("std");
const math = std.math;
const Allocator = std.mem.Allocator;

const Processor = @import("processor.zig").Processor;
const Filter = @import("Filter.zig");
const util = @import("util.zig");
const AudioBuffer = util.AudioBuffer32;

const StrZ = @This();

const GainChannel = enum {
    LowGain,
    HiGain,
};

const Params = struct {
    gain_ch: GainChannel = .HiGain,
    bright: bool = false,

    preamp_gain: f32 = 3,
    bass: f32 = 5,
    mid: f32 = 5,
    treble: f32 = 5,
    presence: f32 = 5,
    master_gain: f32 = 5,
};

const AtomicFlag = std.atomic.Value(bool);

proc: Processor,

tone_stack: ToneStack,
hpf: Filter,

params: *Params,

update_tone_stack: AtomicFlag = AtomicFlag.init(false),

pub fn init(arena: Allocator, num_ch: u32) !*Processor {
    const self: *StrZ = try arena.create(StrZ);
    const params = try arena.create(Params);
    params.* = .{};
    self.* = .{
        .proc = .{
            .prepare = prepare,
            .reset = reset,
            .process = process,
            .paramChanged = paramChanged,
        },
        .params = params,
        .tone_stack = try ToneStack.init(arena, num_ch),
        .hpf = try Filter.init(arena, num_ch, .Highpass, 30, std.math.sqrt1_2),
    };

    return &self.proc;
}

fn prepare(p: *Processor, sample_rate: f64, _: u32, num_ch: u32) void {
    const self: *StrZ = @fieldParentPtr("proc", p);
    self.tone_stack.prepare(sample_rate, num_ch);
    self.hpf.setSampleRate(@floatCast(sample_rate));
}

fn reset(p: *Processor) void {
    const self: *StrZ = @fieldParentPtr("proc", p);
    self.tone_stack.reset();
    self.hpf.reset();
}

fn process(p: *Processor, buffer: AudioBuffer) void {
    const self: *StrZ = @fieldParentPtr("proc", p);
    if (self.update_tone_stack.load(.acquire)) {
        self.tone_stack.gain.bass = self.params.bass / 10;
        self.tone_stack.gain.mid = self.params.mid / 10;
        self.tone_stack.gain.treble = self.params.treble / 10;
        self.update_tone_stack.store(false, .release);
    }

    const in_ceil = math.pow(f32, 10.0, -18 / 20);
    saturateSymmetric(buffer, in_ceil);

    self.tone_stack.process(buffer);

    buffer.applyGain(self.params.preamp_gain * if (self.params.gain_ch == .HiGain)
        @as(f32, 8.0)
    else
        @as(f32, 2.0));

    const p_bias: f32 = if (self.params.gain_ch == .HiGain) 6.0 else 1.0;
    const n_bias: f32 = if (self.params.gain_ch == .HiGain) 8.0 else 3.0;

    saturateAsymmetric(buffer, p_bias, n_bias);
    saturateAsymmetric(buffer, p_bias, n_bias);

    buffer.applyGain(self.params.master_gain);

    saturateSymmetric(buffer, 1.0);
    self.hpf.process(buffer.data, buffer.data);
}

fn paramChanged(p: *Processor, id: []const u8, val: f32) void {
    const self: *StrZ = @fieldParentPtr("proc", p);
    const param_fields = std.meta.fields(Params);
    inline for (param_fields) |field| {
        if (std.mem.eql(u8, field.name, id)) {
            const param = &@field(self.params, field.name);
            switch (field.type) {
                f32 => param.* = val,
                bool => param.* = val > 0,
                GainChannel => {
                    param.* = @enumFromInt(@as(u32, @intFromFloat(val)));
                },
                else => {},
            }
            std.debug.print("Changed {s}: {}\n", .{ field.name, param.* });
            if (std.mem.eql(u8, field.name, "bass") or
                std.mem.eql(u8, field.name, "mid") or
                std.mem.eql(u8, field.name, "treble"))
                // std.mem.eql(u8, field.name, "presence"))
            {
                self.update_tone_stack.store(true, .release);
            }
        }
    }
}

fn saturateSymmetric(buffer: AudioBuffer, limit: f32) void {
    for (buffer.data) |ch| {
        for (ch) |*sample| {
            var x = sample.*;
            x /= limit;
            if (x > 1)
                x = 1
            else if (x < -1)
                x = -1;
            const x5 = x * x * x * x * x;
            x = (5.0 / 4.0) * (x - x5 / 5.0);
            sample.* = x * limit;
        }
    }
}

fn saturateAsymmetric(buffer: AudioBuffer, p_bias: f32, n_bias: f32) void {
    for (buffer.data) |ch| {
        for (ch) |*sample| {
            var x = sample.*;
            if (x >= 0) {
                const x2 = x * x;
                x = (x + x2) / (1 + p_bias * x2);
            } else {
                x = x / (1 - n_bias * x);
            }
            // invert output phase
            sample.* = -x;
        }
    }
}

const ToneStack = struct {
    treble: Filter,
    bass: Filter,
    mid: Filter,

    hpf: Filter,

    gain: struct {
        bass: f32 = 0.5,
        mid: f32 = 0.5,
        treble: f32 = 0.5,
    },

    pub fn init(arena: Allocator, num_ch: u32) !ToneStack {
        return .{
            .gain = .{},
            .treble = try Filter.init(arena, num_ch, .FirstOrderHighpass, 4000, 0),
            .mid = try Filter.init(arena, num_ch, .FirstOrderHighpass, 900, 0.6),
            .bass = try Filter.init(arena, num_ch, .FirstOrderLowpass, 330, 0),
            .hpf = try Filter.init(arena, num_ch, .FirstOrderHighpass, 90, 0),
        };
    }

    pub fn prepare(self: *ToneStack, sample_rate: f64, _: u32) void {
        self.treble.setSampleRate(@floatCast(sample_rate));
        self.bass.setSampleRate(@floatCast(sample_rate));
        self.mid.setSampleRate(@floatCast(sample_rate));
        self.hpf.setSampleRate(@floatCast(sample_rate));
    }

    pub fn reset(self: *ToneStack) void {
        self.treble.reset();
        self.mid.reset();
        self.bass.reset();
        self.hpf.reset();
    }

    pub fn process(self: *ToneStack, buffer: AudioBuffer) void {
        for (buffer.data, 0..) |ch, ch_idx| {
            for (ch) |*sample| {
                var y = sample.*;
                const t = self.treble.processSample(ch_idx, y) *
                    self.gain.treble * 3;
                const m = self.mid.processSample(ch_idx, y) *
                    self.gain.mid * 2;
                const b = self.bass.processSample(ch_idx, y) *
                    self.gain.bass;

                y = (t + m + b) / 3.0;

                // y = self.hpf.processSample(ch_idx, y);

                sample.* = y;
            }
        }
    }
};
