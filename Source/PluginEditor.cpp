/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
STRXAudioProcessorEditor::STRXAudioProcessorEditor (STRXAudioProcessor& p)
    : AudioProcessorEditor (&p), customLookAndFeel(p.apvts), background(p.apvts), amp(p.apvts, &customLookAndFeel), audioProcessor(p)
{
    tooltipWindow.setMillisecondsBeforeTipAppears(1000);

    logo = Drawable::createFromImageData(BinaryData::logo_svg, BinaryData::logo_svgSize);

    channel = p.apvts.getRawParameterValue("channel");
    p.apvts.addParameterListener("channel", this);

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

    addAndMakeVisible(stereo);
    stereo.lnf = &customLookAndFeel;
    stereoAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "stereo", stereo);

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
}

STRXAudioProcessorEditor::~STRXAudioProcessorEditor()
{
    audioProcessor.apvts.removeParameterListener("channel", this);
    hqButton.setLookAndFeel(nullptr);
    renderHQ.setLookAndFeel(nullptr);
}

//==============================================================================
void STRXAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll(*channel ? Colours::black : Colours::grey);
    float padding = getWidth() * 0.02f;
    Rectangle<float> logoBounds(padding, padding, getWidth() * 0.075f, getWidth() * 0.075f);
    logo->drawWithin(g, logoBounds, RectanglePlacement::centred, 1.f);
}

void STRXAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    const auto w = bounds.getWidth();
    const auto h = bounds.getHeight();

    background.setBounds(bounds.removeFromTop(h / 3));

    amp.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.85f));

    outVol.setBounds(background.getBounds().withTrimmedLeft(w * 0.9f).reduced(5));

    hqButton.setBounds(bounds.removeFromLeft(w * 0.1f));
    renderHQ.setBounds(bounds.removeFromLeft(w * 0.15f));
    stereo.setBounds(bounds.removeFromLeft(w * 0.15f));
    legacyTone.setBounds(bounds.removeFromRight(w * 0.2f));

    audioProcessor.lastUIWidth = getWidth();
    audioProcessor.lastUIHeight = getHeight();
}
