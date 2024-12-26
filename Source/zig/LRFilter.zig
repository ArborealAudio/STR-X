//! Linkwitz-Riley Filter
//! Copying the JUCE implementation, licensed under GPLv3

const std = @import("std");
const math = std.math;

const LRFilter = @This();
const Allocator = std.mem.Allocator;
const AudioBuffer = @import("util.zig").AudioBuffer32;

const Type = enum {
    Lowpass,
    Highpass,
    Allpass,
};

g: f32,
r2: f32,
h: f32,

s1: []f32,
s2: []f32,
s3: []f32,
s4: []f32,

sample_rate: f32 = 44100,
cutoff: f32 = 2000,
type: Type = .Lowpass,

allocator: Allocator,

pub fn init(allocator: Allocator, num_ch: u32) !LRFilter {
    var self = LRFilter{
        .allocator = allocator,
        .g = 0,
        .r2 = 0,
        .h = 0,
        .s1 = try allocator.alloc(f32, num_ch),
        .s2 = try allocator.alloc(f32, num_ch),
        .s3 = try allocator.alloc(f32, num_ch),
        .s4 = try allocator.alloc(f32, num_ch),
    };
    self.reset();
    return self;
}

pub fn deinit(self: *LRFilter) void {
    self.allocator.free(self.s1);
    self.allocator.free(self.s2);
    self.allocator.free(self.s3);
    self.allocator.free(self.s4);
}

pub fn prepare(self: *LRFilter, sample_rate: f64, num_ch: u32) !void {
    self.sample_rate = @floatCast(sample_rate);

    self.update();

    if (num_ch != self.s1.len) {
        self.s1 = try self.allocator.realloc(self.s1, num_ch);
        self.s2 = try self.allocator.realloc(self.s2, num_ch);
        self.s3 = try self.allocator.realloc(self.s3, num_ch);
        self.s4 = try self.allocator.realloc(self.s4, num_ch);
    }

    self.reset();
}

fn update(self: *LRFilter) void {
    self.g = @tan(math.pi * self.cutoff / self.sample_rate);
    self.r2 = @sqrt(2.0);
    self.h = 1 / (1 + self.r2 * self.g + self.g * self.g);
}

pub fn setCutoff(self: *LRFilter, cutoff: f32) void {
    self.cutoff = cutoff;
    self.update();
}

pub fn reset(self: *LRFilter) void {
    @memset(self.s1, 0);
    @memset(self.s2, 0);
    @memset(self.s3, 0);
    @memset(self.s4, 0);
}

fn process(self: *LRFilter, buffer: AudioBuffer) void {
    _ = self;
    _ = buffer;
    @compileError("Unimplemented");
}

pub fn processSample(self: *LRFilter, ch: usize, input: f32, out_low: *f32, out_hi: *f32) void {
    const yh = (input - (self.r2 + self.g) * self.s1[ch] -
        self.s2[ch]) * self.h;

    const yb = self.g * yh + self.s1[ch];
    self.s1[ch] = self.g * yh + yb;

    const yl = self.g * yb + self.s2[ch];
    self.s2[ch] = self.g * yb + yl;

    const yh2 = (yl - (self.r2 + self.g) * self.s3[ch] - self.s4[ch]) * self.h;

    const yb2 = self.g * yh2 + self.s3[ch];
    self.s3[ch] = self.g * yh2 + yb2;

    const yl2 = self.g * yb2 + self.s4[ch];
    self.s4[ch] = self.g * yb2 + yl2;

    out_low.* = yl2;
    out_hi.* = yl - self.r2 * yb + yh - yl2;
}
