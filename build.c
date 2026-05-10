#include "arbor/src/build.c"

int main(int argc, char *argv[]) {
    check_rebuild();
    PluginConfig config = {
        .desc = {
            .name = "STR-X2",
            .company = "Arboreal Audio",
            .id = "com.ArborealAudio.STR-X2",
            .version = "2.0.0",
            .copyright = "(c) 2026 Arboreal Audio, LLC",
        },
        .audio_ports = {
            .inputs = 1,
            .outputs = 1,
        },
        .features = DefaultPluginFeatures,
    };

    PluginBuild pb = {
        .src_file = "Source/plugin.c",
        .config_file = "config.txt",
        .arbor_src_path = "arbor/src",
        .format = BuildFormat_VST3 | BuildFormat_CLAP,
        .config = config,
        .debug = true,
        .install = true,
    };

    build_plugin(&pb);
}
