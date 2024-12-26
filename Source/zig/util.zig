pub const AudioBuffer32 = struct {
    num_channels: u32,
    num_frames: u32,
    data: []const []f32,
};

pub const AudioBuffer64 = struct {
    num_channels: u32,
    data: [][]f64,
};

pub const DoubleVec = @Vector(2, f64);

pub const AudioBufferVector = struct {
    data: []DoubleVec,
};
