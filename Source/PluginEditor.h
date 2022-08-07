/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "background.h"

//==============================================================================
/**
*/
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        auto centerX = (float)x + (float)width * 0.5f;
        auto centerY = (float)y + (float)height * 0.5f;
        auto rx = centerX - radius;
        auto ry = centerY - radius;
        auto rw = radius * 2.f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colours::white);
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(juce::Colour (0xff374037));
        g.drawEllipse(rx, ry, rw, rw, 3.0f);

        juce::Path p;
        auto pointerLength = radius * 0.5f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centerX, centerY));

        g.setColour(juce::Colour (0xff374037));
        g.fillPath(p);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
        int buttonW, int buttonH, juce::ComboBox&) override
    {
        juce::Rectangle<float> box;
        box.setSize(width, height);
        g.drawRoundedRectangle(box, 10.f, 3.f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColor,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto buttonArea = button.getLocalBounds();
        g.setColour(juce::Colours::transparentBlack);
        g.fillRect(buttonArea);

        if (button.isMouseOver()) {
            g.setColour(juce::Colour(0xff374037));
            g.fillRect(buttonArea);
        }

        if (button.getToggleState()) {
            g.setColour(juce::Colour(0xff4e6f4e));
            g.fillRect(buttonArea);
        }
    }

};

class STRXAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    STRXAudioProcessorEditor (STRXAudioProcessor&);
    ~STRXAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CustomLookAndFeel customLookAndFeel;

    juce::Slider ts9Gain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ts9Attachment;
    
    juce::Slider inputGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;

    juce::ComboBox mode;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    juce::Slider bassParam;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bassParamAttachment;

    juce::Slider midParam;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midParamAttachment;

    juce::Slider trebleParam;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trebleParamAttachment;

    juce::Slider presenceParam;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> presenceAttachment;

    juce::TextButton brightButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> brightAttachment;

    juce::Slider outGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttachment;

    juce::Slider outVol;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outVolAttachment;
    
    Background background;
    
    STRXAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (STRXAudioProcessorEditor)
};
