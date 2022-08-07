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

    inputGain.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    inputGain.setLookAndFeel(&customLookAndFeel);
    inputGain.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    inputGain.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(inputGain);

    ts9Gain.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    ts9Gain.setLookAndFeel(&customLookAndFeel);
    ts9Gain.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    ts9Gain.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(ts9Gain);

    juce::StringArray modes { "THICK", "NORMAL", "OPEN" };
    mode.addItemList(modes, 1);
    mode.setSelectedItemIndex (1);
    mode.setLookAndFeel(&customLookAndFeel);
    mode.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mode);

    bassParam.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    bassParam.setLookAndFeel(&customLookAndFeel);
    bassParam.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    bassParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(bassParam);

    midParam.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    midParam.setLookAndFeel(&customLookAndFeel);
    midParam.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    midParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(midParam);

    trebleParam.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    trebleParam.setLookAndFeel(&customLookAndFeel);
    trebleParam.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    trebleParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(trebleParam);

    presenceParam.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    presenceParam.setLookAndFeel(&customLookAndFeel);
    presenceParam.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    presenceParam.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(presenceParam);
    
    brightButton.setButtonText(TRANS("BRIGHT"));
    brightButton.setClickingTogglesState(true);
    brightButton.setRepaintsOnMouseActivity(true);
    brightButton.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(brightButton);

    outGain.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    outGain.setLookAndFeel(&customLookAndFeel);
    outGain.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    outGain.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(outGain);

    addAndMakeVisible(outVol);
    outVol.setSliderStyle(juce::Slider::LinearVertical);
    outVol.setTextBoxStyle(juce::Slider::TextBoxAbove, false, 80, 20);
    outVol.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff394c57));
    outVol.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    outVol.setColour(juce::Slider::trackColourId, juce::Colour(0xff78ab78));
    outVol.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);


    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, 
        "gain", inputGain);
    ts9Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "tsXgain", ts9Gain);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,
        "mode", mode);
    bassParamAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "bass", bassParam);
    midParamAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "mid", midParam);
    trebleParamAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "treble", trebleParam);
    presenceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "presence", presenceParam);
    brightAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "bright", brightButton);
    outGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "master", outGain);
    outVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
        "outVol", outVol);
        
    setSize (775, 400);
}

STRXAudioProcessorEditor::~STRXAudioProcessorEditor()
{
}

//==============================================================================
void STRXAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    
}

void STRXAudioProcessorEditor::resized()
{
    ts9Gain.setBounds(50, 275, 75, 75);
    inputGain.setBounds(150, 275, 75, 75);
    mode.setBounds(350, 208, 100, 25);
    bassParam.setBounds(250, 275, 75, 75);
    midParam.setBounds(350, 275, 75, 75);
    trebleParam.setBounds(450, 275, 75, 75);
    presenceParam.setBounds(550, 275, 75, 75);
    brightButton.setBounds(600, 208, 62, 24);
    outGain.setBounds(650, 275, 75, 75);
    outVol.setBounds(721, 5, 46, 115);
}
