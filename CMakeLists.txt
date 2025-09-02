cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9 CACHE INTERNAL "")
if (PRODUCTION_BUILD)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")
    if (WIN32)
        set(CMAKE_SYSTEM_VERSION 7 CACHE INTERNAL "")
    endif()
    set(CMAKE_BUILD_TYPE Release CACHE INTERNAL "")
else()
	set(PRODUCTION_BUILD OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(STR-X VERSION 1.2.1)

add_subdirectory(modules/juce)

add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)

set(JUCE_FORMATS AU VST3 LV2)

if (BUILD_VST2)
	juce_set_vst2_sdk_path(./vst2_sdk)
	list(APPEND JUCE_FORMATS VST)
else()
	set(BUILD_VST2 OFF)
endif()

if (NOT INSTALL)
	set(INSTALL OFF)
endif()

juce_add_plugin(STR-X
    COMPANY_NAME "Arboreal Audio"
    COPY_PLUGIN_AFTER_BUILD ${INSTALL}
    PLUGIN_MANUFACTURER_CODE Arau
    PLUGIN_CODE STRX
    FORMATS ${JUCE_FORMATS}
    VST2_CATEGORY kPlugCategEffect
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect

	LV2URI https://github.com/ArborealAudio/STR-X.git

    PRODUCT_NAME "STR-X"
    HARDENED_RUNTIME_ENABLE TRUE)

clap_juce_extensions_plugin(TARGET STR-X
    CLAP_ID "com.ArborealAudio.STR-X.clap"
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
    CLAP_FEATURES audio-effect distortion)

juce_generate_juce_header(STR-X)

juce_add_modules("modules/Arbor_modules")

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
		JUCE_DISPLAY_SPLASH_SCREEN=0)

if (UNIX AND NOT APPLE)
	target_link_libraries(STR-X PRIVATE "-static-libgcc" "-static-libstdc++")
endif()

juce_add_binary_data(BinaryData SOURCES
    "Resources/str-x.svg"
    "Resources/Menlo-Regular.ttf"
    "Resources/logo.svg")

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
)
