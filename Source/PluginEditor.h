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
    return Typeface::createSystemTypefaceFor(BinaryData::MenloRegular_ttf, BinaryData::MenloRegular_ttfSize);
}

#include "Background.hpp"
#include "LookAndFeel.h"
#include "AmpComponent.hpp"

//==============================================================================
/**
 */

class STRXAudioProcessorEditor : public AudioProcessorEditor,
                                 private AudioProcessorValueTreeState::Listener
{
public:
    STRXAudioProcessorEditor(STRXAudioProcessor &);
    ~STRXAudioProcessorEditor() override;

    //==============================================================================
    void paint(Graphics &) override;
    void resized() override;

    void parameterChanged(const String &parameterID, float newValue) override
    {
        if (parameterID == "channel")
        {
            outVol.setColour(outVol.trackColourId, newValue ? Colour(GREEN) : Colours::wheat);
            repaint();
        }
    }

private:
    std::atomic<float> *channel;

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

    STRXAudioProcessor &audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(STRXAudioProcessorEditor)
};
