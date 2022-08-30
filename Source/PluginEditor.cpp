/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
STRXAudioProcessorEditor::STRXAudioProcessorEditor (STRXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), amp(p.apvts), background(p.apvts)
{
    tooltipWindow.setMillisecondsBeforeTipAppears(1000);

    channel = p.apvts.getRawParameterValue("channel");

    customLookAndFeel.channel = channel;

    addAndMakeVisible(background);

    addAndMakeVisible(amp);

    addAndMakeVisible(outVol);
    outVol.setSliderStyle(Slider::LinearVertical);
    outVol.setTextBoxStyle(Slider::TextBoxAbove, false, 80, 20);
    outVol.setColour(Slider::backgroundColourId, Colour(GRAY));
    outVol.setColour(Slider::thumbColourId, Colours::white);
    outVol.setColour(Slider::trackColourId, Colour(GREEN));
    outVol.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    outVol.setSliderSnapsToMousePosition(false);

    hqButton.setButtonText("HQ");
    hqButton.setClickingTogglesState(true);
    hqButton.setRepaintsOnMouseActivity(true);
    hqButton.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(hqButton);
    hqButton.setTooltip("Enables 4x oversampling with minimal latency");
    
    renderHQ.setButtonText("HQ Rendering");
    renderHQ.setClickingTogglesState(true);
    renderHQ.setRepaintsOnMouseActivity(true);
    renderHQ.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(renderHQ);
    renderHQ.setTooltip("Enables 4x oversampling during rendering, using higher quality filters with fully linear phase");

    legacyTone.setButtonText("Use v1.0 tone controls");
    legacyTone.setClickingTogglesState(true);
    legacyTone.setRepaintsOnMouseActivity(true);
    addAndMakeVisible(legacyTone);

    outVolAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "outVol", outVol);
    hqButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "hq", hqButton);
    renderButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "renderHQ", renderHQ);
    legacyToneAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "legacyTone", legacyTone);

    setResizable(true, true);
    getConstrainer()->setMinimumSize(500, 323);
    getConstrainer()->setFixedAspectRatio(1.55);
    setSize (p.lastUIWidth, p.lastUIHeight);

    startTimerHz(10);
}

STRXAudioProcessorEditor::~STRXAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void STRXAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(lastChannelState ? Colours::black : Colours::grey);
}

void STRXAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto w = bounds.getWidth();
    auto h = bounds.getHeight();

    background.setBounds(bounds.removeFromTop(h / 3));

    amp.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.85f));

    outVol.setBounds(background.getBounds().withTrimmedLeft(w * 0.9f).reduced(5));

    hqButton.setBounds(bounds.removeFromLeft(w * 0.1f));
    renderHQ.setBounds(bounds.removeFromLeft(w * 0.15f));
    legacyTone.setBounds(bounds.removeFromRight(w * 0.2f));

    audioProcessor.lastUIWidth = w;
    audioProcessor.lastUIHeight = h;
}
