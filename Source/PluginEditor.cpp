/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#define CLAY_IMPLEMENTATION
#include "clay.h"

void doLayout();

//==============================================================================
STRXAudioProcessorEditor::STRXAudioProcessorEditor (STRXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor(p)
{
    Clay_SetMeasureTextFunction(measureText);
    setResizable(true, true);
    setSize(DEFAULT_GUI_WIDTH, DEFAULT_GUI_HEIGHT);
}

STRXAudioProcessorEditor::~STRXAudioProcessorEditor()
{
}

//==============================================================================
void STRXAudioProcessorEditor::paint (Graphics& g)
{
    Clay_SetCurrentContext(clay_ctx.ctx);
    Clay_BeginLayout();
    doLayout();
    Clay_RenderCommandArray cmds = Clay_EndLayout();
    ClayJuceRender(g, cmds);
}

void STRXAudioProcessorEditor::resized()
{
    Clay_SetLayoutDimensions({(float)getWidth(), (float)getHeight()});
}

void STRXAudioProcessorEditor::mouseDown(const MouseEvent &e)
{}

void STRXAudioProcessorEditor::mouseDrag(const MouseEvent &e)
{}

void STRXAudioProcessorEditor::mouseMove(const MouseEvent &e)
{}

void doLayout()
{
    Clay_Sizing layoutExpand = {
        .width = CLAY_SIZING_GROW(),
        .height = CLAY_SIZING_GROW(),
    };
    
    CLAY(
        CLAY_ID("Outer"),
        CLAY_RECTANGLE({.color = {0xaa, 0xaa, 0xaa, 0xff}, .cornerRadius = {20.f}}),
        CLAY_LAYOUT({
            .sizing = layoutExpand,
            .padding = CLAY_PADDING_ALL(50),
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
        })
    ) {
        CLAY(
            CLAY_ID("inner"),
            CLAY_RECTANGLE({.color = {0, 0xff, 0, 0xff}}),
            CLAY_LAYOUT({
                .sizing = layoutExpand,
                .padding = CLAY_PADDING_ALL(100),
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            })
        ) {
            CLAY_TEXT(CLAY_STRING("I'M FARTING"), CLAY_TEXT_CONFIG({
                                                       .textColor = {0xff,0,0,0xff},
                                                       .fontSize = 20,
                                                   }));
            CLAY_TEXT(CLAY_STRING("Oh man, that was difficult"), CLAY_TEXT_CONFIG({
                                                       .textColor = {0,0,0xff,0xff},
                                                       .fontSize = 15,
                                                   }));
        }
    }
}
