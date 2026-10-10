#include "../arbor/src/arbor.h"
#include "str_x.c"

typedef struct {
    STR_X str_x[2];
    AudioBuffer64 buffer;
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
    data->buffer = audio_buffer64_create(plugin, plugin->audio_input_count, num_frames);
}

void process_double(Plugin *plugin, const AudioBuffer64 in_buf, AudioBuffer64 out_buf, MidiBuffer midi) {
    PluginData *data = plugin_get_user(plugin);
    ParameterData params = get_audio_parameters(plugin);
    for (u32 ch = 0; ch < in_buf.num_ch; ++ch) {
        str_x_process(&data->str_x[ch], &params, in_buf.data[ch], out_buf.data[ch], in_buf.num_frames);
    }
}

void process(Plugin *plugin, const AudioBuffer32 in_buf, AudioBuffer32 out_buf, MidiBuffer midi) {
    PluginData *data = plugin_get_user(plugin);
    audio_buffer64_copy_from_32(data->buffer, in_buf);
    process_double(plugin, data->buffer, data->buffer, midi);
}

void gui_init(Plugin *plugin) {
    create_plugin_gui(plugin, (PluginGuiDesc){ .create_ui_builder = TRUE });
}

void gui_render(PluginGui *gui) {
    UiCtx *ui = gui->ui;

    ui_column_begin(ui);

    ui_row_begin(ui);
    ui_spacer(ui, Horizontal);
    ui_label(ui, STR_LIT("STR_X2"));
    ui_row_end(ui);

    arbor_quick_ui(gui);

    ui_column_end(ui);
}

PluginInterface plugin_create(Arena *arena) {
    PluginData *data = arena_alloc(arena, sizeof(PluginData));
    return (PluginInterface){
        .user = data,
        .init_cb = init,
        .deinit_cb = deinit,
        .prepare_cb = prepare,
        .process_cb = process,
        .process_double_cb = process_double,
        .gui_init_cb = gui_init,
        .gui_render_cb = gui_render,
        .gui_width = 900,
        .gui_height = 600,
    };
}
