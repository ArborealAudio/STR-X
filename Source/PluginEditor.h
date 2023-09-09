/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#define GREEN 0xff4e6f4e
#define GRAY 0xff373c40
#define BLUE_BG 0xff3b537a
#define LIGHT_ACCENT 0xffdedece

static Typeface::Ptr getCustomFont()
{
    return Typeface::createSystemTypefaceFor(BinaryData::MenloRegular_ttf, BinaryData::MenloRegular_ttfSize);
}

#include "Background.hpp"
#include "LookAndFeel.h"
#include "AmpComponent.hpp"

struct StereoButton : TextButton
{
    StereoButton()
    {
        setClickingTogglesState(true);
    }

    CustomLookAndFeel *lnf;

    void paint(Graphics &g) override
    {
        auto bounds = getLocalBounds().reduced(5).toFloat();
        if (isMouseOver())
        {
            g.setColour(lnf->buttonOutline.darker(0.6f));
            g.fillRoundedRectangle(bounds, 3.f);
        }
        g.setColour(lnf->buttonOutline);
        g.drawRoundedRectangle(bounds, 3.f, 2.f);

        auto ellipseWidth = jmin(bounds.getHeight() * 0.75f, bounds.getWidth() * 0.75f);
        g.setColour(lnf->accentColor);
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

class STRXAudioProcessorEditor : public AudioProcessorEditor,
                                 private AudioProcessorValueTreeState::Listener
{
public:
    STRXAudioProcessorEditor(STRXAudioProcessor &);
    ~STRXAudioProcessorEditor() override;

    //==============================================================================
    void paint(Graphics &) override;
    void resized() override;

    void parameterChanged(const String &parameterID, float newValue) override
    {
        if (parameterID == "channel")
        {
            outVol.setColour(outVol.trackColourId, (bool)newValue ? Colour(GREEN) : Colour(LIGHT_ACCENT));
            backgroundColor = (bool)newValue ? Colours::black : Colours::grey;
			repaint();
        }
    }

private:
    std::atomic<float> *channel;

    CustomLookAndFeel customLookAndFeel;

    Slider outVol;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> outVolAttachment;

    TextButton hqButton, renderHQ;
    StereoButton stereo;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> hqButtonAttach, renderButtonAttach, stereoAttach;

    ToggleButton legacyTone;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> legacyToneAttach;

    Background background;
	Colour backgroundColor;

    AmpComponent amp;

    TooltipWindow tooltipWindow;

    std::unique_ptr<Drawable> logo;

    STRXAudioProcessor &audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(STRXAudioProcessorEditor)
};
