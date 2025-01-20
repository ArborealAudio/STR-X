const std = @import("std");
const math = std.math;
const Allocator = std.mem.Allocator;

const Processor = @import("processor.zig").Processor;
const Filter = @import("Filter.zig");
const util = @import("util.zig");
const AudioBuffer = util.AudioBuffer32;
const map = util.map;
const AtomicFlag = util.AtomicFlag;

/// TS9 emulation
pub const TSX = struct {
    proc: Processor,
    hpf: Filter,
    lpf: Filter,
    lpf2: Filter,

    gain: f32 = 0,
    output: f32 = 5,

    pub fn init(arena: Allocator, num_channels: u32) !*Processor {
        const tsx = try arena.create(TSX);
        tsx.* = .{
            .proc = .{
                .prepare = prepare,
                .reset = reset,
                .process = process,
                .paramChanged = paramChanged,
            },
            .hpf = try Filter.init(arena, num_channels, .FirstOrderHighpass, 720, 0),
            .lpf = try Filter.init(arena, num_channels, .FirstOrderLowpass, 5600, 0),
            .lpf2 = try Filter.init(arena, num_channels, .FirstOrderLowpass, 723.4, 0),
        };
        return &tsx.proc;
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

    fn paramChanged(p: *Processor, id: []const u8, val: f32) void {
        const self: *TSX = @fieldParentPtr("proc", p);
        if (std.mem.eql(u8, id, "pedal_gain")) {
            self.gain = val;
        } else if (std.mem.eql(u8, id, "pedal_output")) {
            self.output = val;
        }
    }

    fn process(p: *Processor, buffer: AudioBuffer) void {
        const self: *TSX = @fieldParentPtr("proc", p);
        const gain = map(f32, self.gain / 10, 1, 5);
        const inv_gain = 1.0 / gain;
        var output = (self.output / 10.0) * 2.0;
        output *= output; // square output for log knob range
        for (buffer.data, 0..) |ch, ch_idx| {
            for (ch) |*sample| {
                const x = sample.*;
                var y: f32 = x;

                y = self.hpf.processSample(ch_idx, y);
                y = self.lpf.processSample(ch_idx, y);

                y *= gain;
                if (y > 1) y = 1 else if (y < -1) y = -1;
                const y3 = y * y * y;
                y = (3.0 / 2.0) * (y + y3 / 3.0);
                if (y < 0) {
                    y += y * y * 0.5;
                }
                y *= inv_gain;

                y = self.lpf2.processSample(ch_idx, y);

                sample.* = y;
            }
        }

        buffer.applyGain(output);
    }
};

/// Rat emulation
// Measurement notes:
// Asymmetry in clipping function: one side steeper, maybe cubic, the other more gradual,
// possibly tanh, arctan, or x^2.
// DC offset increases positively with higher input signals, 0.8v acts as a threshold for this phenomenon.
// With Distortion @ 0, fr is pretty flat, with a little top rolloff around 5k, a little bump @ 2k
// Just above 0, Dist. knob introduces steep boosting HPF around 2.5k
// Tone knob is a simple LPF, taking that rolloff from inaudible to maybe 1k
// More granular details per https://www.electrosmash.com/proco-rat:
// Input DC blocker @ 7.2Hz
// A LPF before clipping that starts well above 22kHz and ends @ 16kHz when Distortion pot is maxed
// Two HPF before clipping, one @ 60Hz and the other @ 1.5kHz
// The op-amp dampens freq above 5.3kHz
pub const RXT = struct {
    proc: Processor,

    input_lpf: Filter,
    tone_filter: Filter,
    slew: Filter,
    hpf: Filter,
    hpf2: Filter,

    gain: f32 = 0,
    tone: f32 = 1,
    output: f32 = 1,

    last_sr: f32 = 44100,

    update_tone: AtomicFlag = AtomicFlag.init(false),
    update_input_lpf: AtomicFlag = AtomicFlag.init(false),

    const tone_max = 20e3;
    const tone_min = 900;
    const input_lpf_min = 16e3;
    const input_lpf_max = 22e3;

    pub fn init(arena: Allocator, num_ch: u32) !*Processor {
        const self = try arena.create(RXT);
        self.* = .{
            .proc = .{
                .prepare = prepare,
                .process = process,
                .reset = reset,
                .paramChanged = paramChanged,
            },
            .tone_filter = try Filter.init(arena, num_ch, .FirstOrderLowpass, tone_max, 0),
            .input_lpf = try Filter.init(arena, num_ch, .FirstOrderLowpass, input_lpf_max, 0),
            .slew = try Filter.init(arena, num_ch, .FirstOrderLowpass, 5.3e3, 0),
            .hpf = try Filter.init(arena, num_ch, .FirstOrderHighpass, 60, 0),
            .hpf2 = try Filter.init(arena, num_ch, .FirstOrderHighpass, 1539, 0),
        };

        return &self.proc;
    }

    fn prepare(p: *Processor, sample_rate: f64, _: u32, _: u32) void {
        const self: *RXT = @fieldParentPtr("proc", p);
        self.tone_filter.setSampleRate(@floatCast(sample_rate));
        self.input_lpf.setSampleRate(@floatCast(sample_rate));
        self.slew.setSampleRate(@floatCast(sample_rate));
        self.hpf.setSampleRate(@floatCast(sample_rate));
        self.hpf2.setSampleRate(@floatCast(sample_rate));
        self.last_sr = @floatCast(sample_rate);
    }

    fn reset(_: *Processor) void {}

    fn process(p: *Processor, buffer: AudioBuffer) void {
        const self: *RXT = @fieldParentPtr("proc", p);
        if (self.update_tone.load(.acquire)) {
            const cutoff: f32 = util.mapLog10(f32, self.tone, tone_min, tone_max);
            self.tone_filter.setCutoff(cutoff, self.last_sr);
            self.update_tone.store(false, .release);
        } else if (self.update_input_lpf.load(.acquire)) {
            const cutoff: f32 = util.mapLog10(f32, self.gain, input_lpf_min, input_lpf_max);
            self.input_lpf.setCutoff(cutoff, self.last_sr);
            self.update_input_lpf.store(false, .release);
        }

        self.input_lpf.process(buffer.data, buffer.data);
        self.hpf.process(buffer.data, buffer.data);
        self.hpf2.process(buffer.data, buffer.data);

        const gain_db = map(f32, self.gain, 0, 65);
        const gain = math.pow(f32, 10, gain_db / 20);
        buffer.applyGain(gain);

        self.slew.process(buffer.data, buffer.data);

        // clip
        for (buffer.data) |ch| {
            for (ch) |*sample| {
                const x = sample.*;
                if (x > 1) sample.* = 1 else if (x < -1) sample.* = -1;
            }
        }

        self.tone_filter.process(buffer.data, buffer.data);

        buffer.applyGain(self.output);
    }

    fn paramChanged(p: *Processor, id: []const u8, val: f32) void {
        const self: *RXT = @fieldParentPtr("proc", p);
        if (std.mem.eql(u8, id, "pedal_tone")) {
            self.tone = val / 10;
            self.update_tone.store(true, .release);
        } else if (std.mem.eql(u8, id, "pedal_output")) {
            self.output = 2 * (val / 10);
        } else if (std.mem.eql(u8, id, "pedal_gain")) {
            self.gain = val / 10;
            self.update_input_lpf.store(true, .release);
        }
    }
};
