
static const f32 lr_cutoffs[] = { 100, 250, 400, };

// Processing state for STR_X mode. Mono-only
typedef struct {
    Plugin *plugin;
    // preamp filters
    IIR_Filter preamp_hpf;
    IIR_Filter preamp_dc;
    IIR_Filter preamp_lowshelf;

    LR_Filter lr;

    // tonestack filters
    IIR_Filter ts_hpf;
    IIR_Filter ts_bpf;
    IIR_Filter ts_lpf;
    IIR_Filter ts_bass;
    IIR_Filter ts_mid;
    IIR_Filter ts_treb;
    IIR_Filter ts_presence;
    IIR_Filter ts_bright;

    float last_bass, last_mid, last_treb, last_pres;
    AmpMode last_amp_mode;

    // poweramp filters
    IIR_Filter poweramp_dc[2];

} STR_X;

static void str_x_tonestack_update(STR_X *amp);

// Initialize filter state
static void str_x_init(STR_X *amp, Plugin *plugin) {
    *amp = (STR_X){
        .plugin = plugin,
        .preamp_hpf = (IIR_Filter){.type = IIR_Filter_Highpass, .cutoff = 65, .reso = SQRT1_2},
        .preamp_dc = (IIR_Filter){.type = IIR_Filter_Highpass, .cutoff = 10, .reso = SQRT1_2},
        .preamp_lowshelf = (IIR_Filter){
            .type = IIR_Filter_FirstOrderLowshelf,
            .cutoff = 185, .reso = 1.8, .gain = 0.5,
        },
        .lr = {.cutoff = lr_cutoffs[(uint)get_parameter_default(plugin, Param_AmpMode)]},
        .ts_hpf = (IIR_Filter){.type = IIR_Filter_FirstOrderHighpass, .cutoff = 750},
        .ts_lpf = (IIR_Filter){.type = IIR_Filter_FirstOrderLowpass, .cutoff = 10e3f},
        .ts_bpf = (IIR_Filter){.type = IIR_Filter_Bandpass, .cutoff = 80, .reso = SQRT1_2},
        .ts_bass = (IIR_Filter){.type = IIR_Filter_FirstOrderLowshelf, .cutoff = 150, .reso = 0.606f, .gain = 1},
        .ts_mid = (IIR_Filter){.type = IIR_Filter_Peak, .cutoff = 600, .reso = 0.5f, .gain = 1},
        .ts_treb = (IIR_Filter){.type = IIR_Filter_FirstOrderHighshelf, .cutoff = 1500, .reso = 0.3f, .gain = 1},
        .ts_presence = (IIR_Filter){.type = IIR_Filter_Peak, .cutoff = 4000, .reso = 0.6f, .gain = 1},
        .ts_bright = (IIR_Filter){.type = IIR_Filter_FirstOrderHighshelf, .cutoff = 2500, .reso = SQRT1_2, .gain = db2lin(12)},
        .poweramp_dc = {
            [0] = (IIR_Filter){.type = IIR_Filter_Highpass, .cutoff = 10, .reso = SQRT1_2},
            [1] = (IIR_Filter){.type = IIR_Filter_Highpass, .cutoff = 10, .reso = SQRT1_2},
        },
    };

    lr_filter_init(&amp->lr);
    str_x_tonestack_update(amp);
}

static void str_x_prepare(STR_X *amp, f64 sample_rate) {
    filter_set_sample_rate(&amp->preamp_hpf, sample_rate);
    filter_set_sample_rate(&amp->preamp_dc, sample_rate);
    filter_set_sample_rate(&amp->preamp_lowshelf, sample_rate);
    lr_filter_set_sample_rate(&amp->lr, sample_rate);
    filter_set_sample_rate(&amp->ts_hpf, sample_rate);
    filter_set_sample_rate(&amp->ts_lpf, sample_rate);
    filter_set_sample_rate(&amp->ts_bpf, sample_rate);
    filter_set_sample_rate(&amp->ts_bass, sample_rate);
    filter_set_sample_rate(&amp->ts_mid, sample_rate);
    filter_set_sample_rate(&amp->ts_treb, sample_rate);
    filter_set_sample_rate(&amp->ts_presence, sample_rate);
    filter_set_sample_rate(&amp->ts_bright, sample_rate);
    filter_set_sample_rate(&amp->poweramp_dc[0], sample_rate);
    filter_set_sample_rate(&amp->poweramp_dc[1], sample_rate);
}

static void str_x_tonestack_update(STR_X *amp) {
    f32 bass = get_parameter(amp->plugin, Param_Bass);
    f32 mid = get_parameter(amp->plugin, Param_Mid);
    f32 treb = get_parameter(amp->plugin, Param_Treble);
    f32 pres = get_parameter(amp->plugin, Param_Presence);

    if (amp->last_bass != bass) {
        f32 map = db2linf(mapf(bass / 10.f, -12, 12));
        filter_set_gain(&amp->ts_bass, map);
        amp->last_bass = bass;
    }

    if (amp->last_mid != mid) {
        f32 map = db2linf(mapf(mid / 10.f, -7, 7));
        filter_set_gain(&amp->ts_mid, map);
        amp->last_mid = mid;
    }

    if (amp->last_treb != treb) {
        f32 map = db2linf(mapf(treb / 10.f, -14, 14));
        filter_set_gain(&amp->ts_treb, map);
        amp->last_treb = treb;
    }

    if (amp->last_pres != pres) {
        f32 map = db2linf(mapf(pres / 10.f, -8, 8));
        filter_set_gain(&amp->ts_presence, map);
        amp->last_pres = pres;
    }
}

static void str_x_mode_update(STR_X *amp, AmpMode new_mode) {
    lr_filter_set_cutoff(&amp->lr, lr_cutoffs[new_mode]);
    amp->last_amp_mode = new_mode;
}

static f64 str_x_preamp_saturate_hi(f64 k, f64 x) {
    const f64 nk = k / 0.9;

    if (x > 0) {
        return atan(k * x) / fmax(atan(k), 1.0);
    } else {
        return 0.9 * atan(nk * x) / fmax(atan(nk), 1.0);
    }
}

static f64 str_x_preamp_saturate_low(f64 x) {
    if (x > 0) {
        return (x / (1.0 + fabs(x))) * 2.0;
    } else {
        return (2.0 * x) / (1.0 + fabs(2.0 * x));
    }
}

static void str_x_process_preamp_hi(STR_X *amp, const f32 *in, f32 *out, u32 num_frames) {
    const f32 gain = get_parameter(amp->plugin, Param_PreampGain) * 2.667f;
    for (u32 i = 0; i < num_frames; ++i) {
        const f32 x = in[i];
        f32 y = x * gain;
        f64 yl, yh;
        lr_filter_process_sample(&amp->lr, y, &yl, &yh);
        yl = filter_process_sample(&amp->preamp_hpf, yl);

        yl = str_x_preamp_saturate_hi(gain, yl);
        yh = str_x_preamp_saturate_hi(gain, yh);

        y = yl + yh;

        y = filter_process_sample(&amp->preamp_dc, y);
        y = filter_process_sample(&amp->preamp_lowshelf, y);

        out[i] = y;
    }
}

static void str_x_process_preamp_low(STR_X *amp, const f32 *in, f32 *out, u32 num_frames) {
    const f64 gain = get_parameter(amp->plugin, Param_PreampGain) * 4.f;
    for (u32 i = 0; i < num_frames; ++i) {
        const f32 x = in[i];
        f32 y = x * gain;
        f64 yl, yh;
        lr_filter_process_sample(&amp->lr, y, &yl, &yh);

        yl = str_x_preamp_saturate_low(yl);
        yh = str_x_preamp_saturate_low(yh);

        y = yl + yh;

        y = filter_process_sample(&amp->preamp_dc, y);
        y = filter_process_sample(&amp->preamp_lowshelf, y);

        out[i] = y;
    }
}

static f64 str_x_poweramp_saturate(f64 x, f64 g, f64 ln, f64 lp) {
    const f64 gx = x * g;
    if (gx <= 0) {
        return gx / (1 - (gx / ln));
    } else {
        return gx / (1 + (gx / lp));
    }
}

static void str_x_process_poweramp_hi(STR_X *amp, const f32 *in, f32 *out, u32 num_frames) {
    const f64 gain = get_parameter(amp->plugin, Param_MasterGain) * 0.6;
    for (u32 i = 0; i < num_frames; ++i) {
        f64 y = in[i] * gain;
        f64 yp = str_x_poweramp_saturate(y, 1.7, 23.6, 1.01);
        f64 yn = str_x_poweramp_saturate(y, 1.7, 1.01, 23.6);
        yp = filter_process_sample(&amp->poweramp_dc[0], yp);
        yn = filter_process_sample(&amp->poweramp_dc[1], yn);
        yp = str_x_poweramp_saturate(yp, 4, 1.01, 1.01);
        yn = str_x_poweramp_saturate(yn, 4, 1.01, 1.01);

        y = yp + yn;
        y *= 0.1767;
        out[i] = y;
    }
}

static void str_x_process_poweramp_low(STR_X *amp, const f32 *in, f32 *out, u32 num_frames) {
    const f64 gain = get_parameter(amp->plugin, Param_MasterGain) * 0.6;
    for (u32 i = 0; i < num_frames; ++i) {
        f64 y = in[i] * gain;
        f64 yp = str_x_poweramp_saturate(y, 1.7, 23.6, 1.01);
        f64 yn = str_x_poweramp_saturate(y, 1.7, 1.01, 23.6);
        yp = filter_process_sample(&amp->poweramp_dc[0], yp);
        yn = filter_process_sample(&amp->poweramp_dc[1], yn);
        yp = str_x_poweramp_saturate(yp, 2, 2.01, 2.01);
        yn = str_x_poweramp_saturate(yn, 2, 2.01, 2.01);

        y = yp + yn;
        y *= 0.1767;
        out[i] = y;
    }
}

static void str_x_process(STR_X *amp, const f32 *in, f32 *out, u32 num_frames) {
    str_x_tonestack_update(amp);
    // TODO This should look like:
    // GainChannel gain_ch = amp->params->gain_ch;
    GainChannel gain_ch = (GainChannel)get_parameter(amp->plugin, Param_GainChannel);
    AmpMode amp_mode = (AmpMode)get_parameter(amp->plugin, Param_AmpMode);
    if (amp_mode != amp->last_amp_mode)
        str_x_mode_update(amp, amp_mode);

    // Preamp
    // if (gain_ch == High) {
    //     str_x_process_preamp_hi(amp, in, out, num_frames);
    // } else {
    //     str_x_process_preamp_low(amp, in, out, num_frames);
    // }
    // Tone stack
    // ISSUE I thought I solved the excessive bassiness but i actually failed to capture the output
    // of the tonestack filters. The issue persists
    for (u32 i = 0; i < num_frames; ++i) {
        f64 y = out[i];
        // y = filter_process_sample(&amp->ts_lpf, y);
        // f64 yhp = filter_process_sample(&amp->ts_hpf, y);
        // f64 ybp = filter_process_sample(&amp->ts_bpf, y);
        // y = yhp + ybp;
        y = filter_process_sample(&amp->ts_bass, y);
        y = filter_process_sample(&amp->ts_mid, y);
        y = filter_process_sample(&amp->ts_treb, y);
        y = filter_process_sample(&amp->ts_presence, y);
        out[i] = y;
    }
    if ((bool)get_parameter(amp->plugin, Param_Bright)) {
        filter_process(&amp->ts_bright, out, out, num_frames);
    }
    // Poweramp
    // if (gain_ch == High) {
    //     str_x_process_poweramp_hi(amp, out, out, num_frames);
    // } else {
    //     str_x_process_poweramp_low(amp, out, out, num_frames);
    // }
}
