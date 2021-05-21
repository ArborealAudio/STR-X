/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
STRXAudioProcessorEditor::STRXAudioProcessorEditor (STRXAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible(background);

    inputGain.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    inputGain.setLookAndFeel(&customLookAndFeel);
    inputGain.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    inputGain.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(inputGain);

    ts9Gain.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    ts9Gain.setLookAndFeel(&customLookAndFeel);
    ts9Gain.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    ts9Gain.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(ts9Gain);

    StringArray modes { "THICK", "NORMAL", "OPEN" };
    mode.addItemList(modes, 1);
    mode.setSelectedItemIndex (1);
    mode.setLookAndFeel(&customLookAndFeel);
    mode.setJustificationType(Justification::centred);
    addAndMakeVisible(mode);

    bassParam.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    bassParam.setLookAndFeel(&customLookAndFeel);
    bassParam.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    bassParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(bassParam);

    midParam.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    midParam.setLookAndFeel(&customLookAndFeel);
    midParam.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    midParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(midParam);

    trebleParam.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    trebleParam.setLookAndFeel(&customLookAndFeel);
    trebleParam.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    trebleParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(trebleParam);

    presenceParam.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    presenceParam.setLookAndFeel(&customLookAndFeel);
    presenceParam.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    presenceParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(presenceParam);
    
    brightButton.setButtonText(TRANS("BRIGHT"));
    brightButton.setClickingTogglesState(true);
    brightButton.setRepaintsOnMouseActivity(true);
    brightButton.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(brightButton);

    outGain.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    outGain.setLookAndFeel(&customLookAndFeel);
    outGain.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    outGain.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(outGain);

    addAndMakeVisible(outVol);
    outVol.setSliderStyle(Slider::LinearVertical);
    outVol.setTextBoxStyle(Slider::TextBoxAbove, false, 80, 20);
    outVol.setColour(Slider::backgroundColourId, Colour(0xff394c57));
    outVol.setColour(Slider::thumbColourId, Colours::white);
    outVol.setColour(Slider::trackColourId, Colour(0xff78ab78));
    outVol.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

    hqButton.setButtonText(TRANS("HQ"));
    hqButton.setTooltip(TRANS("Enables 4x oversampling with minimal latency"));
    hqButton.setClickingTogglesState(true);
    hqButton.setRepaintsOnMouseActivity(true);
    hqButton.setLookAndFeel(&customLookAndFeel);
    /*hqButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00232823));
    hqButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x004d8f4d));
    hqButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff549054));*/
    addAndMakeVisible(hqButton);

    renderHQ.setButtonText(TRANS("RenderHQ"));
    renderHQ.setTooltip(TRANS("Enables 4x oversampling during rendering, using higher quality filters with fully linear phase"));
    renderHQ.setButtonText(TRANS("HQ Rendering"));
    renderHQ.setClickingTogglesState(true);
    renderHQ.setRepaintsOnMouseActivity(true);
    renderHQ.setLookAndFeel(&customLookAndFeel);
    /*renderHQ.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00232823));
    renderHQ.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x004d8f4d));
    renderHQ.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff549054));*/
    addAndMakeVisible(renderHQ);

    inputGainAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, 
        "gain", inputGain);
    ts9Attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "tsXgain", ts9Gain);
    modeAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,
        "mode", mode);
    bassParamAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "bass", bassParam);
    midParamAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "mid", midParam);
    trebleParamAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "treble", trebleParam);
    presenceAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "presence", presenceParam);
    brightAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "bright", brightButton);
    outGainAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "master", outGain);
    outVolAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "outVol", outVol);
    hqButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "hq", hqButton);
    renderButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "renderHQ", renderHQ);
    
        
    setSize (775, 435);
}

STRXAudioProcessorEditor::~STRXAudioProcessorEditor()
{
}

//==============================================================================
void STRXAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
    g.setFont (15.0f);
    
}

void STRXAudioProcessorEditor::resized()
{
    ts9Gain.setBounds(50, 307, 75, 75);
    inputGain.setBounds(150, 307, 75, 75);
    mode.setBounds(350, 240, 100, 25);
    bassParam.setBounds(250, 307, 75, 75);
    midParam.setBounds(350, 307, 75, 75);
    trebleParam.setBounds(450, 307, 75, 75);
    presenceParam.setBounds(550, 307, 75, 75);
    brightButton.setBounds(600, 240, 62, 24);
    outGain.setBounds(650, 307, 75, 75);
    outVol.setBounds(718, 8, 46, 125);
    hqButton.setBounds(8, 8, 48, 24);
    renderHQ.setBounds(580, 0, 104, 32);
}
