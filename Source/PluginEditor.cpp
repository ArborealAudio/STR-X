/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
STRXAudioProcessorEditor::STRXAudioProcessorEditor(STRXAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), main_comp(p.apvts),
      tsx(p.apvts), pedal_type({"TSX", "RXT"}, 0)
{
    tooltipWindow.setMillisecondsBeforeTipAppears(1000);

    logo = Drawable::createFromImageData(BinaryData::logo_svg, BinaryData::logo_svgSize);

    addAndMakeVisible(main_comp);
    addAndMakeVisible(tsx);
    addAndMakeVisible(pedal_type);
    pedal_type_attach = std::make_unique<Apvts::ComboBoxAttachment>(p.apvts, "pedal_type", pedal_type);

    addAndMakeVisible(outVol);
    outVol.setSliderStyle(Slider::LinearVertical);
    outVol.setTextBoxStyle(Slider::TextBoxAbove, false, 80, 20);
    outVol.setColour(Slider::backgroundColourId, Colour(GRAY));
    outVol.setColour(Slider::thumbColourId, Colours::white);
    outVol.setColour(Slider::trackColourId, Colours::whitesmoke);
    outVol.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    outVol.setSliderSnapsToMousePosition(false);

    hqButton.setButtonText("HQ");
    hqButton.setClickingTogglesState(true);
    hqButton.setRepaintsOnMouseActivity(true);
    addAndMakeVisible(hqButton);
    hqButton.setTooltip("Enables 4x oversampling with minimal latency");
    
    renderHQ.setButtonText("HQ Rendering");
    renderHQ.setClickingTogglesState(true);
    renderHQ.setRepaintsOnMouseActivity(true);
    addAndMakeVisible(renderHQ);
    renderHQ.setTooltip("Enables 4x oversampling during rendering, using higher quality filters with fully linear phase");

    addAndMakeVisible(stereo);

    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio((float)DEFAULT_GUI_WIDTH / DEFAULT_GUI_HEIGHT);
    setSize (DEFAULT_GUI_WIDTH, DEFAULT_GUI_HEIGHT);
}

STRXAudioProcessorEditor::~STRXAudioProcessorEditor()
{
    hqButton.setLookAndFeel(nullptr);
    renderHQ.setLookAndFeel(nullptr);
}

//==============================================================================
void STRXAudioProcessorEditor::paint (Graphics& g)
{
	g.fillAll(Colours::darkgrey.darker());
    float padding = getWidth() * 0.02f;
    Rectangle<float> logoBounds(padding, padding, getWidth() * 0.075f, getWidth() * 0.075f);
    logo->drawWithin(g, logoBounds, RectanglePlacement::centred, 1.f);
}

void STRXAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    const int w = bounds.getWidth();
    const int h = bounds.getHeight();

    auto pedal_bounds = bounds.removeFromTop((float)h * 0.2f).withWidth(w / 4).withX((float)w * 0.1f);
    tsx.setBounds(pedal_bounds);
    pedal_type.setBounds(pedal_bounds.withX(pedal_bounds.getRight()));

    auto main_bounds = bounds.removeFromTop((float)h * 0.85f);
    main_comp.setBounds(main_bounds);

    hqButton.setBounds(bounds.removeFromLeft(w * 0.1f));
    renderHQ.setBounds(bounds.removeFromLeft(w * 0.15f));
    stereo.setBounds(bounds.removeFromLeft(w * 0.15f));

    audioProcessor.lastUIWidth = getWidth();
    audioProcessor.lastUIHeight = getHeight();
}
