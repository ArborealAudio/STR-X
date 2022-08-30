/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#define GREEN 0xff4e6f4e
#define GRAY 0xff374037
#define BLUE_BG 0xff5c7eb5
#define LIGHT_GREEN 0xff448771

static Typeface::Ptr getCustomFont()
{
    return Typeface::createSystemTypefaceFor(BinaryData::menlo_ttc, BinaryData::menlo_ttcSize);
}

#include "Background.hpp"
#include "LookAndFeel.h"
#include "AmpComponent.hpp"

//==============================================================================
/**
*/

class STRXAudioProcessorEditor  : public AudioProcessorEditor, private Timer
{
public:
    STRXAudioProcessorEditor (STRXAudioProcessor&);
    ~STRXAudioProcessorEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override
    {
        if (channel && *channel != lastChannelState) {
            repaint();
            lastChannelState = *channel;
            outVol.setColour(outVol.trackColourId, lastChannelState ? Colour(GREEN) : Colours::wheat);
        }
    }

private:
    std::atomic<float> *channel;
    bool lastChannelState = false;

    CustomLookAndFeel customLookAndFeel;

    Slider outVol;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> outVolAttachment;

    TextButton hqButton, renderHQ;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> hqButtonAttach, renderButtonAttach;
    
    ToggleButton legacyTone;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> legacyToneAttach;
    
    Background background;

    AmpComponent amp;

    TooltipWindow tooltipWindow;

    STRXAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (STRXAudioProcessorEditor)
};
