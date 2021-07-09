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
    tooltipWindow.setMillisecondsBeforeTipAppears(1000);

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
    mode.setTooltip(TRANS("Switches multiband crossover point. Thick = 100Hz | Normal = 250Hz | Open = 400Hz"));
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
    legacyToneAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "legacyTone", legacyTone);
    
    setResizable(true, false); /*switched second bool back to false*/
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
    g.setColour(Colours::black);
    g.fillAll();
    
}

void STRXAudioProcessorEditor::resized()
{
    auto children = getChildren();

    auto width = getWidth();
    auto scale = float(width) / float(775);
    
    ts9Gain.setBounds(50, 327, 75, 75);
    inputGain.setBounds(150, 327, 75, 75);
    mode.setBounds(350, 260, 100, 25);
    bassParam.setBounds(250, 327, 75, 75);
    midParam.setBounds(350, 327, 75, 75);
    trebleParam.setBounds(450, 327, 75, 75);
    presenceParam.setBounds(550, 327, 75, 75);
    brightButton.setBounds(600, 260, 62, 24);
    outGain.setBounds(650, 327, 75, 75);
    outVol.setBounds(718, 28, 46, 125);
    hqButton.setBounds(20, 460, 48, 32);
    renderHQ.setBounds(100, 460, 104, 32);
    legacyTone.setBounds(625, 450, 120, 50);

    for (auto child : children)
    {
        child->setTransform(AffineTransform::scale(scale));
    }

    audioProcessor.lastUIWidth = getWidth();
    audioProcessor.lastUIHeight = getHeight();
}
