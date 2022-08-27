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

#include "Background.hpp"
#include "LookAndFeel.h"
#include "AmpComponent.hpp"

//==============================================================================
/**
*/

class STRXAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    STRXAudioProcessorEditor (STRXAudioProcessor&);
    ~STRXAudioProcessorEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    CustomLookAndFeel customLookAndFeel;

    Slider outVol;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> outVolAttachment;
    
    TextButton hqButton;   
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> hqButtonAttach;
    
    TextButton renderHQ;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> renderButtonAttach;

    ToggleButton legacyTone;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> legacyToneAttach;
    
    Background background;

    AmpComponent amp;

    TooltipWindow tooltipWindow;

    STRXAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (STRXAudioProcessorEditor)
};
