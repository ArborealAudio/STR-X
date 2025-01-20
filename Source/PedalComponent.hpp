#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"

static const Theme tsx_theme = {
    .main = Colours::limegreen,
    .accent = Colours::white,
    .dark_accent = Colours::grey,
};

// is this base class necessary? We can't actually implement a generic title draw that is partitioned correctly
struct PedalComponent : Component
{
    PedalComponent(const String &name) : name(name)
    {
    }

    void paint(Graphics &g) override
    {
        g.setColour(Colours::white);
        g.drawText(name, name_bounds, Justification::centred);
    }

    void resized() override
    {
        name_bounds = getLocalBounds().removeFromTop(getHeight() / 8);
    }

private:
    String name;
    Rectangle<int> name_bounds;
};

struct TSXPedalComponent : PedalComponent
{
    TSXPedalComponent(AudioProcessorValueTreeState &a) : PedalComponent("TSX")
    {
        addAndMakeVisible(gain);
        addAndMakeVisible(tone);
        addAndMakeVisible(output);
        addAndMakeVisible(on);

        gain.lnf.theme = tsx_theme;
        tone.lnf.theme = tsx_theme;
        output.lnf.theme = tsx_theme;

        on.setClickingTogglesState(true);
        on.setColour(TextButton::ColourIds::buttonColourId, Colours::transparentWhite);
        on.setColour(TextButton::ColourIds::buttonOnColourId, tsx_theme.main);

        on_attach = std::make_unique<Apvts::ButtonAttachment>(a, "pedal_on", on);
        gain_attach = std::make_unique<Apvts::SliderAttachment>(a, "pedal_gain", gain);
        tone_attach = std::make_unique<Apvts::SliderAttachment>(a, "pedal_tone", tone);
        out_attach = std::make_unique<Apvts::SliderAttachment>(a, "pedal_output", output);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const int w = bounds.getWidth();
        const int h = bounds.getHeight();
        auto top = bounds.removeFromTop(h / 2);
        auto bot = bounds;
        gain.setBounds(top.removeFromLeft(w / 2));
        tone.setBounds(top);
        output.setBounds(bot.removeFromLeft(w / 2));
        on.setBounds(bot);
    }

private:
    AmpKnob gain{"Gain"}, tone{"Tone"}, output{"Output"};
    TextButton on{"On"};

    using Apvts = AudioProcessorValueTreeState;
    std::unique_ptr<Apvts::SliderAttachment> gain_attach, tone_attach, out_attach;
    std::unique_ptr<Apvts::ButtonAttachment> on_attach;
};
