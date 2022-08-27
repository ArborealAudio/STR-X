/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
STRXAudioProcessorEditor::STRXAudioProcessorEditor (STRXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), amp(p.apvts)
{
    tooltipWindow.setMillisecondsBeforeTipAppears(1000);

    addAndMakeVisible(background);

    addAndMakeVisible(amp);

    addAndMakeVisible(outVol);
    outVol.setSliderStyle(Slider::LinearVertical);
    outVol.setTextBoxStyle(Slider::TextBoxAbove, false, 80, 20);
    outVol.setColour(Slider::backgroundColourId, Colour(0xff394c57));
    outVol.setColour(Slider::thumbColourId, Colours::white);
    outVol.setColour(Slider::trackColourId, Colour(0xff78ab78));
    outVol.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

    hqButton.setButtonText(TRANS("HQ"));
    hqButton.setClickingTogglesState(true);
    hqButton.setRepaintsOnMouseActivity(true);
    hqButton.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(hqButton);
    hqButton.setTooltip(TRANS("Enables 4x oversampling with minimal latency"));
    
    renderHQ.setButtonText(TRANS("HQ Rendering"));
    renderHQ.setClickingTogglesState(true);
    renderHQ.setRepaintsOnMouseActivity(true);
    renderHQ.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(renderHQ);
    renderHQ.setTooltip(TRANS("Enables 4x oversampling during rendering, using higher quality filters with fully linear phase"));

    legacyTone.setButtonText(TRANS("Use v1.0 tone controls"));
    legacyTone.setClickingTogglesState(true);
    legacyTone.setRepaintsOnMouseActivity(true);
    addAndMakeVisible(legacyTone);

    outVolAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "outVol", outVol);
    hqButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "hq", hqButton);
    renderButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "renderHQ", renderHQ);
    legacyToneAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "legacyTone", legacyTone);
    
    setResizable(true, true);
    getConstrainer()->setMinimumSize(500, 323);
    getConstrainer()->setFixedAspectRatio(1.55);
    setSize (p.lastUIWidth, p.lastUIHeight);
}

STRXAudioProcessorEditor::~STRXAudioProcessorEditor()
{
}

//==============================================================================
void STRXAudioProcessorEditor::paint (Graphics& g)
{
}

void STRXAudioProcessorEditor::resized()
{
    auto children = getChildren();
    children.removeLast();

    auto width = getWidth();
    auto scale = float(width) / float(775);


    amp.setSize(getWidth() * 0.85f, getHeight() * 0.5f);
    amp.setCentrePosition(getLocalBounds().getCentreX(), getLocalBounds().getCentreY());

    background.setBounds(getLocalBounds().removeFromTop(getHeight() / 2));
    
    outVol.setBounds(718, 28, 46, 125);
    hqButton.setBounds(20, 460, 48, 32);
    renderHQ.setBounds(100, 460, 104, 32);
    legacyTone.setBounds(625, 450, 120, 50);

    // for (auto child : children)
    // {
    //     child->setTransform(AffineTransform::scale(scale));
    // }

    audioProcessor.lastUIWidth = getWidth();
    audioProcessor.lastUIHeight = getHeight();
}
