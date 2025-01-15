/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "clay.h"
#include "clay_juce_renderer.cpp"

#define DEFAULT_GUI_WIDTH 900
#define DEFAULT_GUI_HEIGHT 600

static void handleClayError(Clay_ErrorData error) {
    fprintf(stderr, "%s\n", error.errorText.chars);
}

struct Context {
    Clay_Arena arena;
    Clay_Context *ctx;

    Context()
    {
        uint32 req_mem_size = 2 * Clay_MinMemorySize();
        arena = Clay_CreateArenaWithCapacityAndMemory(req_mem_size, malloc(req_mem_size));
        ctx = Clay_Initialize(arena, {
                            .width = DEFAULT_GUI_WIDTH, .height = DEFAULT_GUI_HEIGHT,
                        }, {handleClayError});
        
    }

    ~Context()
    {
        free(arena.memory);
    }
};

//==============================================================================
/**
 */

class STRXAudioProcessorEditor : public AudioProcessorEditor
{
public:
    STRXAudioProcessorEditor(STRXAudioProcessor &);
    ~STRXAudioProcessorEditor() override;

    //==============================================================================
    void paint(Graphics &) override;
    void resized() override;
    void mouseMove(const MouseEvent &e) override;
    void mouseDown(const MouseEvent &e) override;
    void mouseDrag(const MouseEvent &e) override;

private:

    Context clay_ctx;
    
    STRXAudioProcessor &audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(STRXAudioProcessorEditor)
};
