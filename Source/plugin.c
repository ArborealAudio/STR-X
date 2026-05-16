#include "str_x.c"

typedef struct {
    STR_X str_x[2];
} PluginData;

void init(Plugin *plugin) {
    PluginData *data = plugin_get_user(plugin);
    str_x_init(&data->str_x[0], plugin);
    str_x_init(&data->str_x[1], plugin);
}

void deinit(Plugin *plugin) {}

void prepare(Plugin *plugin, f64 sample_rate, u32 num_frames) {
    PluginData *data = plugin_get_user(plugin);
    str_x_prepare(&data->str_x[0], sample_rate);
    str_x_prepare(&data->str_x[1], sample_rate);
}

void process(Plugin *plugin, const AudioBuffer32 in_buf, AudioBuffer32 out_buf, MidiBuffer midi) {
    PluginData *data = plugin_get_user(plugin);
    ParameterData params = get_plugin_parameters(plugin);
    for (u32 ch = 0; ch < in_buf.num_ch; ++ch) {
        str_x_process(&data->str_x[ch], &params, in_buf.data[ch], out_buf.data[ch], in_buf.num_frames);
    }
}

PluginInterface plugin_create(Allocator *alloc) {
    PluginData *data = alloc->alloc(alloc, sizeof(PluginData));
    return (PluginInterface){
        .user = data,
        .init_cb = init,
        .deinit_cb = deinit,
        .prepare_cb = prepare,
        .process_cb = process,
    };
}
