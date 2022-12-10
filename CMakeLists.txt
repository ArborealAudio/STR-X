cmake_minimum_required(VERSION 3.15)

project(STR-X VERSION 1.2)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(modules/JUCE)

add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)
juce_set_vst2_sdk_path("$ENV{HOME}/vst2_sdk")

set(JUCE_FORMATS AU VST3)

if (UNIX AND NOT APPLE)
    list(APPEND JUCE_FORMATS LV2)
endif()

if (TARGET juce_vst2_sdk)
    list(APPEND JUCE_FORMATS VST)
endif()

juce_add_plugin(STR-X
    COMPANY_NAME "Arboreal Audio"
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Arau
    PLUGIN_CODE STRX
    FORMATS ${JUCE_FORMATS}
    VST2_CATEGORY kPlugCategEffect
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    PRODUCT_NAME "STR-X"
    HARDENED_RUNTIME_ENABLE TRUE)

clap_juce_extensions_plugin(TARGET STR-X
    CLAP_ID "com.ArborealAudio.STR-X.clap"
    CLAP_FEATURES audio-effect distortion)

juce_generate_juce_header(STR-X)

juce_add_modules(
    "modules/Arbor_modules")

target_sources(STR-X
    PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/STR-X.hpp
    Source/Background.hpp
    Source/AmpComponent.hpp
    Source/LookAndFeel.h)

target_compile_definitions(STR-X
    PUBLIC
    JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0)

juce_add_binary_data(BinaryData SOURCES
    "Resources/str-x.svg"
    "Resources/Menlo-Regular.ttf")

set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(STR-X
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_dsp
        Arbor_modules
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)