/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "AmpComponent.hpp"
#include "PedalComponent.hpp"

#define DEFAULT_GUI_WIDTH 900
#define DEFAULT_GUI_HEIGHT 750

struct StereoButton : TextButton
{
    StereoButton()
    {
        setClickingTogglesState(true);
    }

    // CustomLookAndFeel *lnf;

    void paint(Graphics &g) override
    {
        auto bounds = getLocalBounds().reduced(5).toFloat();
        if (isMouseOver())
        {
            // g.setColour(lnf->buttonOutline.darker(0.6f));
            g.fillRoundedRectangle(bounds, 3.f);
        }
        // g.setColour(lnf->buttonOutline);
        g.drawRoundedRectangle(bounds, 3.f, 2.f);

        auto ellipseWidth = jmin(bounds.getHeight() * 0.75f, bounds.getWidth() * 0.75f);
        // g.setColour(lnf->accentColor);
        if (getToggleState())
        {
            g.drawEllipse(bounds.getCentreX() * 0.8f - (ellipseWidth / 2), bounds.getCentreY() - (ellipseWidth / 2), ellipseWidth, ellipseWidth, 3.f);
            g.drawEllipse(bounds.getCentreX() * 1.2f - (ellipseWidth / 2), bounds.getCentreY() - (ellipseWidth / 2), ellipseWidth, ellipseWidth, 3.f);
        }
        else
        {
            g.drawEllipse(bounds.getCentreX() - (ellipseWidth / 2), bounds.getCentreY() - (ellipseWidth / 2), ellipseWidth, ellipseWidth, 3.f);
        }
    }
};

//==============================================================================
/**
 */

class STRXAudioProcessorEditor : public AudioProcessorEditor
{
public:
    STRXAudioProcessorEditor(STRXAudioProcessor &);
    ~STRXAudioProcessorEditor() override;

    //==============================================================================
    void paint(Graphics &) override;
    void resized() override;

private:
    using Apvts = AudioProcessorValueTreeState;

    MainComponent main_comp;
    TSXPedalComponent tsx;
    AmpModeMenu pedal_type;
    std::unique_ptr<Apvts::ComboBoxAttachment> pedal_type_attach;

    Slider outVol;
    std::unique_ptr<Apvts::SliderAttachment> outVolAttachment;

    TextButton hqButton, renderHQ;
    StereoButton stereo;
    std::unique_ptr<Apvts::ButtonAttachment> hqButtonAttach, renderButtonAttach, stereoAttach;

    TooltipWindow tooltipWindow;

    std::unique_ptr<Drawable> logo;

    STRXAudioProcessor &audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(STRXAudioProcessorEditor)
};
