const std = @import("std");
const math = std.math;
const Allocator = std.mem.Allocator;

const processor = @import("processor.zig");
const Processor = processor.Processor;
const Filter = @import("Filter.zig");
const LRFilter = @import("LRFilter.zig");
const util = @import("util.zig");
const map = util.map;
const AudioBuffer = util.AudioBuffer32;
const AtomicFlag = util.AtomicFlag;

const TSX = processor.TSX;

pub const StrX = struct {
    pub const AmpMode = enum {
        Thick,
        Normal,
        Open,
    };

    pub const GainChannel = enum {
        LowGain,
        HiGain,
    };

    const Params = struct {
        amp_mode: AmpMode = .Normal,
        gain_ch: GainChannel = .HiGain,
        bright: bool = false,

        preamp_gain: f32 = 3,
        bass: f32 = 5,
        mid: f32 = 5,
        treble: f32 = 5,
        presence: f32 = 5,
        master_gain: f32 = 5,
    };

    pub fn init(arena: Allocator, num_ch: u32) !*Processor {
        const self: *StrX = try arena.create(StrX);
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
            .preamp = PreAmp.init(params, arena, num_ch) catch |e| {
                std.log.err("PreAmp init: {!}\n", .{e});
                return error.PreAmpInitFailed;
            },
            .tone_stack = ToneStack.init(params, arena, num_ch) catch |e| {
                std.log.err("ToneStack init: {!}\n", .{e});
                return error.ToneStackInitFailed;
            },
            .poweramp = PowerAmp.init(params, arena, num_ch) catch |e| {
                std.log.err("PowerAmp init: {!}\n", .{e});
                return error.PowerAmpInitFailed;
            },
        };

        return &self.proc;
    }

    fn prepare(
        proc: *Processor,
        sample_rate: f64,
        _: u32,
        num_channels: u32,
    ) void {
        const self: *StrX = @fieldParentPtr("proc", proc);

        self.preamp.prepare(sample_rate, num_channels);
        self.tone_stack.prepare(sample_rate);
        self.poweramp.prepare(sample_rate);
    }

    fn reset(p: *Processor) void {
        const self: *StrX = @fieldParentPtr("proc", p);
        self.preamp.reset();
        self.poweramp.reset();
    }

    fn process(
        p: *Processor,
        buffer: AudioBuffer,
    ) void {
        const self: *StrX = @fieldParentPtr("proc", p);
        if (self.update_amp_mode.load(.acquire)) {
            self.preamp.updateMode();
            self.update_amp_mode.store(false, .release);
        }
        if (self.update_tone_stack.load(.acquire)) {
            self.tone_stack.update();
            self.update_tone_stack.store(false, .release);
        }
        self.preamp.process(buffer);
        self.tone_stack.process(buffer);
        self.poweramp.process(buffer);
    }

    fn paramChanged(p: *Processor, id: []const u8, val: f32) void {
        const self: *StrX = @fieldParentPtr("proc", p);
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

    proc: Processor,

    preamp: PreAmp,
    tone_stack: ToneStack,
    poweramp: PowerAmp,

    params: *Params,

    update_amp_mode: AtomicFlag = AtomicFlag.init(false),
    update_tone_stack: AtomicFlag = AtomicFlag.init(false),

    const PreAmp = struct {
        hpf: Filter,
        dc_removal: Filter,
        low_shelf: Filter,

        lr: LRFilter,

        state: *const Params,

        pub fn init(state: *const Params, arena: Allocator, num_ch: u32) !PreAmp {
            return .{
                .state = state,
                .hpf = try Filter.init(arena, num_ch, .Highpass, 65, std.math.sqrt1_2),
                .dc_removal = try Filter.init(arena, num_ch, .Highpass, 10, std.math.sqrt1_2),
                // TODO: Implement 2nd-order shelving filters to accurately copy original sound
                .low_shelf = try Filter.init(arena, num_ch, .FirstOrderLowshelf, 185, 1.8),
                .lr = try LRFilter.init(arena, num_ch, .Lowpass),
            };
        }

        fn prepare(self: *PreAmp, sample_rate: f64, num_ch: u32) void {
            self.hpf.setSampleRate(@floatCast(sample_rate));
            self.dc_removal.setSampleRate(@floatCast(sample_rate));
            self.low_shelf.setGain(0.5, @floatCast(sample_rate));
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
            const gain = self.state.preamp_gain * 8;
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
            const gain = self.state.preamp_gain * 4;
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
            self.filters.getPtr(.bright).gain = 2;
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

    const PowerAmp = struct {
        /// filters for sep. pos & neg asym. saturation
        dc_removal: [2]Filter,
        state: *const Params,

        pub fn init(params: *const Params, arena: Allocator, num_ch: u32) !PowerAmp {
            return .{
                .dc_removal = .{
                    try Filter.init(arena, num_ch, .Highpass, 10, math.sqrt1_2),
                    try Filter.init(arena, num_ch, .Highpass, 10, math.sqrt1_2),
                },
                .state = params,
            };
        }

        pub fn prepare(self: *PowerAmp, sample_rate: f64) void {
            for (&self.dc_removal) |*f| {
                f.setSampleRate(@floatCast(sample_rate));
            }
        }

        pub fn reset(self: *PowerAmp) void {
            for (&self.dc_removal) |*f| {
                f.reset();
            }
        }

        pub fn process(self: *PowerAmp, buffer: AudioBuffer) void {
            const gain = self.state.master_gain;
            switch (self.state.gain_ch) {
                .HiGain => {
                    for (buffer.data, 0..) |ch, ch_idx| {
                        for (ch) |*sample| {
                            sample.* = self.processSampleHiGain(sample.*, gain, ch_idx);
                        }
                    }
                },
                .LowGain => {
                    for (buffer.data, 0..) |ch, ch_idx| {
                        for (ch) |*sample| {
                            sample.* = self.processSampleLoGain(sample.*, gain, ch_idx);
                        }
                    }
                },
            }
        }

        fn processSampleHiGain(self: *PowerAmp, x: f32, gain: f32, ch: usize) f32 {
            const g = gain * 0.6;
            var y = x * g;

            // asym waveshaping
            var yp = saturate(y, 1.7, 23.6, 1.01);
            var yn = saturate(y, 1.7, 1.01, 23.6);

            yp = self.dc_removal[0].processSample(ch, yp);
            yn = self.dc_removal[1].processSample(ch, yn);

            yp = saturate(yp, 4.0, 1.01, 1.01);
            yn = saturate(yn, 4.0, 1.01, 1.01);

            y = yp + yn;

            y *= 0.1767;

            return y;
        }

        fn processSampleLoGain(self: *PowerAmp, x: f32, gain: f32, ch: usize) f32 {
            const g = gain * 0.6;
            var y = x * g;

            // asym waveshaping
            var yp = saturate(y, 1.7, 23.6, 1.01);
            var yn = saturate(y, 1.7, 1.01, 23.6);

            yp = self.dc_removal[0].processSample(ch, yp);
            yn = self.dc_removal[1].processSample(ch, yn);

            yp = saturate(yp, 2, 2.01, 2.01);
            yn = saturate(yn, 2, 2.01, 2.01);

            y = yp + yn;

            y *= 0.1767;

            return y;
        }

        fn saturate(x: f32, g: f32, ln: f32, lp: f32) f32 {
            const gx = g * x;
            if (x <= 0)
                return gx / (1 - (gx / ln))
            else
                return gx / (1 + (gx / lp));
        }
    };
};
