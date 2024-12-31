const std = @import("std");
const math = std.math;
const util = @import("util.zig");
const map = util.map;
const DoubleVec = util.DoubleVec;
const AudioBuffer = util.AudioBuffer32;
const Filter = @import("Filter.zig");
const LRFilter = @import("LRFilter.zig");

const Arena = std.heap.ArenaAllocator;
const Allocator = std.mem.Allocator;

const AmpMode = enum {
    Thick,
    Normal,
    Open,
};

const GainChannel = enum {
    LowGain,
    HiGain,
};

const Processor = struct {
    fn prepare(
        self: *Processor,
        sample_rate: f64,
        num_samples: u32,
        num_channels: u32,
    ) !void {
        self.max_frames = num_samples;

        self.ts9.prepare(sample_rate);
        self.preamp.prepare(sample_rate, num_channels);
        self.tone_stack.prepare(sample_rate);
    }

    fn reset(p: *Processor) void {
        p.ts9.reset();
        p.preamp.reset();
    }

    fn paramChange(self: *Processor, id: []const u8, val: f32) void {
        // only prollem is...ain't threadsafe
        // unless we basically promise to ourself never to modify `self.params` outside
        // this fn?
        const param_fields = std.meta.fields(Params);
        inline for (param_fields) |field| {
            if (std.mem.eql(u8, field.name, id)) {
                const param = &@field(self.params, field.name);
                switch (field.type) {
                    f32 => param.* = val,
                    bool => param.* = val > 0,
                    AmpMode => {
                        param.* = @enumFromInt(@as(u32, @intFromFloat(val)));
                        self.update_amp_mode.store(true, .release);
                    },
                    GainChannel => {
                        param.* = @enumFromInt(@as(u32, @intFromFloat(val)));
                    },
                    else => {},
                }
                std.debug.print("Changed {s}: {}\n", .{ field.name, param.* });
                if (std.mem.eql(u8, field.name, "bass") or
                    std.mem.eql(u8, field.name, "mid") or
                    std.mem.eql(u8, field.name, "treble") or
                    std.mem.eql(u8, field.name, "presence"))
                {
                    self.update_tone_stack.store(true, .release);
                }
            }
        }
    }

    fn process(
        p: *Processor,
        c_buffer: [*][*]f32,
        num_samples: u32,
        num_channels: u32,
    ) void {
        const buffer: AudioBuffer = .{
            .num_channels = num_channels,
            .num_frames = num_samples,
            .data = &.{
                c_buffer[0][0..num_samples],
                c_buffer[1][0..num_samples],
            },
        };
        if (p.update_amp_mode.load(.acquire)) {
            p.preamp.updateMode();
            p.update_amp_mode.store(false, .release);
        }
        if (p.update_tone_stack.load(.acquire)) {
            p.tone_stack.update();
            p.update_tone_stack.store(false, .release);
        }
        if (p.params.pedal_gain > 0)
            p.ts9.process(buffer);
        p.preamp.process(buffer);
        p.tone_stack.process(buffer);

        const out_gain: f32 = math.pow(f32, 10.0, p.params.out_vol / 20);
        for (buffer.data) |ch| {
            for (ch) |*sample| {
                sample.* = sample.* * out_gain;
            }
        }
    }

    fn process64(
        _: *Processor,
        _: [*][*]f64,
        _: u32,
        _: u32,
    ) callconv(.C) void {}

    const AtomicFlag = std.atomic.Value(bool);

    arena_impl: *Arena,
    allocator: Allocator,
    max_frames: u32,

    ts9: TSX,
    preamp: PreAmp,
    tone_stack: ToneStack,

    params: *Params,

    update_amp_mode: AtomicFlag = AtomicFlag.init(false),
    update_tone_stack: AtomicFlag = AtomicFlag.init(false),
};

const Params = struct {
    amp_mode: AmpMode = .Normal,
    gain_ch: GainChannel = .HiGain,
    bright: bool = false,

    pedal_gain: f32 = 0,
    preamp_gain: f32 = 3,
    bass: f32 = 5,
    mid: f32 = 5,
    treble: f32 = 5,
    presence: f32 = 5,
    master_gain: f32 = 5,
    out_vol: f32 = 0,
};

export fn processor_init(num_channels: u32) ?*Processor {
    var arena = std.heap.c_allocator.create(Arena) catch |e| {
        std.log.err("{!}\n", .{e});
        return null;
    };
    arena.* = Arena.init(std.heap.raw_c_allocator);
    const allocator = arena.allocator();

    const proc = allocator.create(Processor) catch |e| {
        std.log.err("{!}\n", .{e});
        return null;
    };
    const params = allocator.create(Params) catch |e| {
        std.log.err("{!}\n", .{e});
        return null;
    };
    params.* = .{};
    const default_buffer_length = 256;
    proc.* = .{
        .params = params,
        .arena_impl = arena,
        .allocator = allocator,
        .max_frames = default_buffer_length,
        .ts9 = TSX.init(allocator, num_channels, &params.pedal_gain) catch |e| {
            std.log.err("TS9 init: {!}\n", .{e});
            return null;
        },
        .preamp = PreAmp.init(params, allocator, num_channels) catch |e| {
            std.log.err("PreAmp init: {!}\n", .{e});
            return null;
        },
        .tone_stack = ToneStack.init(params, allocator, num_channels) catch |e| {
            std.log.err("ToneStack init: {!}\n", .{e});
            return null;
        },
    };
    return proc;
}

export fn processor_deinit(p: ?*Processor) void {
    if (p) |proc| {
        proc.arena_impl.deinit();
    }
}

export fn processor_prepare(
    p: ?*Processor,
    sample_rate: f64,
    num_samples: u32,
    num_channels: u32,
) void {
    if (p) |proc| {
        proc.prepare(sample_rate, num_samples, num_channels) catch |e| {
            std.log.err("{!}\n", .{e});
            return;
        };
    }
}

export fn processor_reset(p: ?*Processor) void {
    if (p) |proc| {
        proc.reset();
    }
}

export fn processor_process(
    p: ?*Processor,
    buffer: [*][*]f32,
    num_samples: u32,
    num_channels: u32,
) void {
    if (p) |proc| {
        proc.process(buffer, num_samples, num_channels);
    }
}

export fn processor_param_change(p: ?*Processor, id: [*:0]const u8, val: f32) void {
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

const PreAmp = struct {
    hpf: Filter,
    dc_removal: Filter,
    low_shelf: Filter,

    lr: LRFilter,

    state: *const Params,

    pub fn init(state: *const Params, arena: Allocator, num_ch: u32) !PreAmp {
        var preamp: PreAmp = .{
            .state = state,
            .hpf = try Filter.init(arena, num_ch, .Highpass, 65, std.math.phi),
            .dc_removal = try Filter.init(arena, num_ch, .Highpass, 10, std.math.phi),
            .low_shelf = try Filter.init(arena, num_ch, .FirstOrderLowshelf, 185, 1.8),
            .lr = try LRFilter.init(arena, num_ch),
        };
        preamp.lr.type = .Lowpass;
        preamp.low_shelf.gain = 0.5;
        return preamp;
    }

    fn prepare(self: *PreAmp, sample_rate: f64, num_ch: u32) void {
        self.hpf.setSampleRate(@floatCast(sample_rate));
        self.dc_removal.setSampleRate(@floatCast(sample_rate));
        self.low_shelf.setSampleRate(@floatCast(sample_rate));
        self.lr.prepare(sample_rate, num_ch) catch |e| {
            std.log.err("LR Filter prepare: {!}\n", .{e});
        };
        self.updateMode();
    }

    fn reset(self: *PreAmp) void {
        self.hpf.reset();
        self.low_shelf.reset();
        self.dc_removal.reset();
        self.lr.reset();
    }

    fn updateMode(self: *PreAmp) void {
        switch (self.state.amp_mode) {
            .Thick => {
                self.lr.setCutoff(100);
            },
            .Normal => {
                self.lr.setCutoff(250);
            },
            .Open => {
                self.lr.setCutoff(400);
            },
        }
    }

    fn process(self: *PreAmp, buffer: AudioBuffer) void {
        switch (self.state.gain_ch) {
            .LowGain => self.processLoGain(buffer),
            .HiGain => self.processHiGain(buffer),
        }
    }

    fn processHiGain(self: *PreAmp, buffer: AudioBuffer) void {
        const gain = self.state.preamp_gain;
        for (buffer.data, 0..) |ch, ch_idx| {
            for (ch) |*sample| {
                const x = sample.*;
                var y: f32 = x;
                var yl: f32 = 0;
                var yh: f32 = 0;

                y *= gain;
                self.lr.processSample(ch_idx, y, &yl, &yh);
                yl = self.hpf.processSample(@intCast(ch_idx), yl);

                yl = saturateHi(gain / 3, yl);
                yh = saturateHi(gain / 3, yh);

                y = yl + yh;

                y = self.dc_removal.processSample(@intCast(ch_idx), y);
                y = self.low_shelf.processSample(@intCast(ch_idx), y);

                sample.* = y;
            }
        }
    }

    fn processLoGain(self: *PreAmp, buffer: AudioBuffer) void {
        const gain = self.state.preamp_gain;
        for (buffer.data, 0..) |ch, ch_idx| {
            for (ch) |*sample| {
                const x = sample.*;
                var y: f32 = x;
                var yl: f32 = 0;
                var yh: f32 = 0;

                y *= gain;
                self.lr.processSample(ch_idx, y, &yl, &yh);
                yl = saturateLo(yl);
                yh = saturateLo(yh);

                y = yl + yh;
                y = self.dc_removal.processSample(ch_idx, y);
                y = self.low_shelf.processSample(ch_idx, y);

                sample.* = y;
            }
        }
    }

    /// k = saturation curve parameter
    fn saturateHi(k: f32, x: f32) f32 {
        const nk = k / 0.9;

        if (x > 0) {
            return math.atan(k * x) / math.atan(k);
        } else {
            return 0.9 * math.atan(nk * x) / math.atan(nk);
        }
    }

    fn saturateLo(x: f32) f32 {
        if (x > 0) {
            return (x / (1 + @abs(x))) * 2;
        } else {
            return (2 * x) / (1 + @abs(2 * x));
        }
    }
};

const ToneStack = struct {
    const FilterList = enum {
        hpf,
        bpf,
        lpf,
        bass,
        mid,
        treble,
        presence,
        bright,
    };
    const FilterArray = std.EnumArray(FilterList, Filter);

    filters: FilterArray,
    sample_rate: f32 = 44100,

    state: *const Params,

    pub fn init(params: *const Params, arena: Allocator, num_ch: u32) !ToneStack {
        var ts: ToneStack = .{
            .filters = FilterArray.init(.{
                .hpf = try Filter.init(arena, num_ch, .FirstOrderHighpass, 750, 0),
                .lpf = try Filter.init(arena, num_ch, .FirstOrderLowpass, 10e3, 0),
                .bpf = try Filter.init(arena, num_ch, .Bandpass, 80, math.sqrt1_2),
                .bright = try Filter.init(arena, num_ch, .FirstOrderHighshelf, 2500, math.sqrt1_2),
                .bass = try Filter.init(arena, num_ch, .FirstOrderLowshelf, 150, 0.606),
                .mid = try Filter.init(arena, num_ch, .Peak, 600, 0.5),
                .treble = try Filter.init(arena, num_ch, .FirstOrderHighshelf, 1500, 0.3),
                .presence = try Filter.init(arena, num_ch, .Peak, 4000, 0.6),
            }),
            .state = params,
        };
        ts.update();
        return ts;
    }

    pub fn update(self: *ToneStack) void {
        // remap user-facing values to dB-based values
        var bass = map(f32, self.state.bass / 10, -12, 12);
        var mid = map(f32, self.state.mid / 10, -7, 7);
        var treble = map(f32, self.state.treble / 10, -14, 14);
        var presence = map(f32, self.state.presence / 10, -8, 8);

        // convert to linear
        bass = math.pow(f32, 10, bass / 20);
        self.filters.getPtr(.bass).setGain(bass, self.sample_rate);
        mid = math.pow(f32, 10, mid / 20);
        self.filters.getPtr(.mid).setGain(mid, self.sample_rate);
        treble = math.pow(f32, 10, treble / 20);
        self.filters.getPtr(.treble).setGain(treble, self.sample_rate);
        presence = math.pow(f32, 10, presence / 20);
        self.filters.getPtr(.presence).setGain(presence, self.sample_rate);
    }

    pub fn prepare(self: *ToneStack, sample_rate: f64) void {
        self.sample_rate = @floatCast(sample_rate);
        for (&self.filters.values) |*f| {
            f.setSampleRate(@floatCast(sample_rate));
        }
    }

    pub fn process(self: *ToneStack, buffer: AudioBuffer) void {
        for (buffer.data, 0..) |ch, ch_idx| {
            for (ch) |*sample| {
                const in = sample.*;
                var y = self.filters.getPtr(.lpf).processSample(ch_idx, in);
                const yhp = self.filters.getPtr(.hpf).processSample(ch_idx, y);
                const ybp = self.filters.getPtr(.bpf).processSample(ch_idx, y);
                y = yhp + ybp;
                y = self.filters.getPtr(.bass).processSample(ch_idx, y);
                y = self.filters.getPtr(.mid).processSample(ch_idx, y);
                y = self.filters.getPtr(.treble).processSample(ch_idx, y);
                y = self.filters.getPtr(.presence).processSample(ch_idx, y);
                if (self.state.bright) {
                    y = self.filters.getPtr(.bright).processSample(ch_idx, y);
                }

                sample.* = y;
            }
        }
    }
};

// TS9 guitar pedal
const TSX = struct {
    hpf: Filter,
    lpf: Filter,
    lpf2: Filter,

    gain: *const f32,

    pub fn init(arena: Allocator, num_channels: u32, gain: *const f32) !TSX {
        const ts9: TSX = .{
            .hpf = try Filter.init(arena, num_channels, .FirstOrderHighpass, 720, 0),
            .lpf = try Filter.init(arena, num_channels, .FirstOrderLowpass, 5600, 0),
            .lpf2 = try Filter.init(arena, num_channels, .FirstOrderLowpass, 723.4, 0),
            .gain = gain,
        };
        return ts9;
    }

    fn prepare(self: *TSX, sample_rate: f64) void {
        self.hpf.setSampleRate(@floatCast(sample_rate));
        self.lpf.setSampleRate(@floatCast(sample_rate));
        self.lpf2.setSampleRate(@floatCast(sample_rate));
    }

    fn reset(self: *TSX) void {
        self.hpf.reset();
        self.lpf.reset();
        self.lpf2.reset();
    }

    fn process(self: *TSX, buffer: AudioBuffer) void {
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
