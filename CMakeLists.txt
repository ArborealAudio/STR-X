cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

project(STR-Z VERSION 0.1)

add_subdirectory(modules/juce)
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)
add_subdirectory(Source/zig)

set(JUCE_FORMATS AU VST3)

if (BUILD_VST2)
	juce_set_vst2_sdk_path(./vst2_sdk)
	list(APPEND JUCE_FORMATS VST)
else()
	set(BUILD_VST2 OFF)
endif()

# if (UNIX AND NOT APPLE)
# 	list(APPEND JUCE_FORMATS LV2)
# endif()

if (NOT INSTALL)
	set(INSTALL OFF)
endif()

juce_add_plugin(${CMAKE_PROJECT_NAME}
    COMPANY_NAME "Arboreal Audio"
    COPY_PLUGIN_AFTER_BUILD ${INSTALL}
    PLUGIN_MANUFACTURER_CODE Arau
    PLUGIN_CODE STRZ
    FORMATS ${JUCE_FORMATS}
    VST2_CATEGORY kPlugCategEffect
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect

		LV2URI https://github.com/ArborealAudio/STR-X.git

    PRODUCT_NAME "${CMAKE_PROJECT_NAME}"
    HARDENED_RUNTIME_ENABLE TRUE
)

clap_juce_extensions_plugin(TARGET ${CMAKE_PROJECT_NAME}
    CLAP_ID "com.ArborealAudio.${CMAKE_PROJECT_NAME}.clap"
    CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
    CLAP_FEATURES audio-effect distortion)

juce_generate_juce_header(${CMAKE_PROJECT_NAME})

target_sources(${CMAKE_PROJECT_NAME}
	PRIVATE
		Source/PluginProcessor.cpp
		Source/PluginEditor.cpp
		Source/Background.hpp
		Source/AmpComponent.hpp
		Source/LookAndFeel.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE modules/Arbor_modules)

target_compile_definitions(${CMAKE_PROJECT_NAME}
	PUBLIC
		JUCE_WEB_BROWSER=0
		JUCE_USE_CURL=0
		JUCE_VST3_CAN_REPLACE_VST2=0
		JUCE_DISPLAY_SPLASH_SCREEN=0
)

if (UNIX AND NOT APPLE)
	target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "-static-libgcc" "-static-libstdc++")
endif()

juce_add_binary_data(BinaryData SOURCES
    "Resources/str-x.svg"
    "Resources/Menlo-Regular.ttf"
    "Resources/logo.svg")

set_target_properties(BinaryData PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        BinaryData
        juce::juce_audio_utils
        juce::juce_dsp
        processor
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)
