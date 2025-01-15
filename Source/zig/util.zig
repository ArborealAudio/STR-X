const std = @import("std");

pub const AudioBuffer32 = struct {
    num_channels: u32,
    num_frames: u32,
    data: []const []f32,

    pub fn applyGain(b: AudioBuffer32, gain: f32) void {
        for (b.data) |ch| {
            for (ch) |*sample| {
                sample.* *= gain;
            }
        }
        // const vec_len = std.simd.suggestVectorLength(f32) orelse 4;
        // const Vec = @Vector(vec_len, f32);
        // const gv: Vec = @splat(gain);
        // for (b.data) |ch| {
        //     var i: u32 = 0;
        //     const buffer_len = ch.len / vec_len;
        //     while (i < buffer_len) : (i += vec_len) {
        //         var x: Vec = ch[i..][0..vec_len].*;
        //         x *= gv;
        //     }
        // }
    }
};

pub const AudioBuffer64 = struct {
    num_channels: u32,
    data: [][]f64,
};

pub const DoubleVec = @Vector(2, f64);

pub const AudioBufferVector = struct {
    data: []DoubleVec,
};

// map normalized float to new range
pub fn map(comptime T: type, x: T, min: T, max: T) T {
    return (x * (max - min)) + min;
}
