#include "str_x.c"

typedef struct {
    STR_X str_x[2];
} PluginData;

void init(Plugin *plugin) {
    PluginData *data = plugin_get_user(plugin);
    str_x_init(&data->str_x[0], plugin, 0);
    str_x_init(&data->str_x[1], plugin, 1);
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

void gui_init(Plugin *plugin) {
    create_plugin_gui(plugin, (PluginGuiDesc){ .create_ui = TRUE });
}

void gui_render(Plugin *plugin) {
    UICtx *ui = plugin->gui.ui;
    ui_set_background_color(ui, (av_Colorf){.g = 0.5, .b = 0.5, .a = 1});
}

PluginInterface plugin_create(Allocator *alloc) {
    PluginData *data = alloc->alloc(alloc, sizeof(PluginData));
    return (PluginInterface){
        .user = data,
        .init_cb = init,
        .deinit_cb = deinit,
        .prepare_cb = prepare,
        .process_cb = process,
        .gui_init_cb = gui_init,
        .gui_render_cb = gui_render,
        .gui_width = 600,
        .gui_height = 400,
    };
}
