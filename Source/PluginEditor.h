/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Background.h"

//==============================================================================
/**
*/
class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(Slider::thumbColourId, Colours::white);
    }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override
    {
        auto radius = (float)jmin(width / 2, height / 2) - 4.0f;
        auto centerX = (float)x + (float)width * 0.5f;
        auto centerY = (float)y + (float)height * 0.5f;
        auto rx = centerX - radius;
        auto ry = centerY - radius;
        auto rw = radius * 2.f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(Colours::white);
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(Colour (0xff374037));
        g.drawEllipse(rx, ry, rw, rw, 3.0f);

        Path p;
        auto pointerLength = radius * 0.5f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(AffineTransform::rotation(angle).translated(centerX, centerY));

        g.setColour(Colour (0xff374037));
        g.fillPath(p);

        if (slider.isMouseOverOrDragging())
        {
            g.setColour(Colour(0xff4e6f4e));
            g.drawEllipse(rx, ry, rw, rw, 3.0f);
        }
       
    }

    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
        int buttonW, int buttonH, ComboBox& comboBox) override
    {
        Rectangle<float> box;
        box.setSize(width, height);
        g.setColour(Colours::transparentBlack);
        g.drawRoundedRectangle(box, 10.f, 3.f);
    }

    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColor,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto buttonArea = button.getLocalBounds().toFloat();
        g.setColour(Colours::transparentBlack);
        g.fillRoundedRectangle(buttonArea, 3.f);

        if (button.isMouseOver()) {
            g.setColour(Colour(0xff374037));
            g.fillRoundedRectangle(buttonArea, 3.f);
        }

        if (button.getToggleState()) {
            g.setColour(Colour(0xff4e6f4e));
            g.fillRoundedRectangle(buttonArea, 3.f);
        }
    }

};

//=================================================================================

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

    Slider ts9Gain;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> ts9Attachment;
    
    Slider inputGain;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;

    ComboBox mode;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    Slider bassParam;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> bassParamAttachment;

    Slider midParam;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> midParamAttachment;

    Slider trebleParam;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> trebleParamAttachment;

    Slider presenceParam;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> presenceAttachment;

    TextButton brightButton;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> brightAttachment;

    Slider outGain;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;

    Slider outVol;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> outVolAttachment;
    
    TextButton hqButton;   
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> hqButtonAttach;
    
    TextButton renderHQ;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> renderButtonAttach;

    ToggleButton legacyTone;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> legacyToneAttach;
    
    Background background;

    TooltipWindow tooltipWindow;

    STRXAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (STRXAudioProcessorEditor)
};
