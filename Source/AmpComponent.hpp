// AmpComponent.hpp

#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "zig/processor.h"

struct StrXComponent : Component,
    private AudioProcessorValueTreeState::Listener
{
    enum KnobList {
        Preamp,
        Bass,
        Mid,
        Treble,
        Presence,
        Master,
        NumKnobs,
    };

    const char* str_x_knobs[NumKnobs] = {
        "Preamp",
        "Bass",
        "Mid",
        "Treble",
        "Presence",
        "Master"
    };

    StrXComponent(AudioProcessorValueTreeState &apvts) : apvts(apvts),
        amp_mode({"THICK", "NORMAL", "OPEN"}, 1)
    {
        for (auto &k : knobs)
            addAndMakeVisible(k);

        addAndMakeVisible(on);
        on.setClickingTogglesState(true);
        addAndMakeVisible(gain_ch);
        addAndMakeVisible(bright);

        amp_mode.setTooltip("Switches the voicing of the preamp\n\nThick = Split @ 100Hz\nNormal = Split @ 250Hz\nOpen = Split @ 400Hz");
        addAndMakeVisible(amp_mode);

        using Sa = AudioProcessorValueTreeState::SliderAttachment;
        knob_attachments.preamp = std::make_unique<Sa>(apvts, "preamp_gain", knobs[Preamp]);
        knob_attachments.bass = std::make_unique<Sa>(apvts, "bass", knobs[Bass]);
        knob_attachments.mid = std::make_unique<Sa>(apvts, "mid", knobs[Mid]);
        knob_attachments.treble = std::make_unique<Sa>(apvts, "treble", knobs[Treble]);
        knob_attachments.presence = std::make_unique<Sa>(apvts, "presence", knobs[Presence]);
        knob_attachments.master = std::make_unique<Sa>(apvts, "master_gain", knobs[Master]);

        on_attach = std::make_unique<
            AudioProcessorValueTreeState::ButtonAttachment>(apvts, "amp_on", on);
        gain_ch_attach = std::make_unique<
            AudioProcessorValueTreeState::ButtonAttachment>(apvts, "gain_ch", gain_ch);
        bright_attach = std::make_unique<
            AudioProcessorValueTreeState::ButtonAttachment>(apvts, "bright", bright);
        amp_mode_attach = std::make_unique<
            AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "amp_mode", amp_mode);

        // set component themes based on channel value
        GainChannel cur_ch = (GainChannel)(int)apvts.getParameterAsValue("gain_ch").getValue();
        setTheme(cur_ch);

        apvts.addParameterListener("gain_ch", this);
    }

    ~StrXComponent()
    {
        apvts.removeParameterListener("gain_ch", this);
    }

    void parameterChanged(const String &id, float value) override
    {
        if (strcmp(id.toRawUTF8(), "gain_ch") == 0) {
            setTheme((GainChannel)value);
            repaint();
        }
    }

    void setTheme(GainChannel cur_ch)
    {
        const Theme new_theme = cur_ch == HiGainChannel ? str_x_higain_theme : str_x_lowgain_theme;
        for (auto &k : knobs) {
            k.lnf.theme = new_theme;
        }
        gain_ch.lnf.theme = new_theme;
        amp_mode.lnf.theme = new_theme;
        theme = new_theme;
    }

    void paint(Graphics &g) override
    {
        auto bounds = getLocalBounds().reduced(3);
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();

        g.setColour(theme.main);
        g.fillRoundedRectangle(bounds.toFloat(), 10.f);

        g.setColour(theme.accent);

        Path ltr;
        ltr.startNewSubPath(bounds.getX() + 8, bounds.getY() + 5);
        ltr.quadraticTo(w * 0.7f, bounds.getY(), w, h);

        Path rtl;
        rtl.startNewSubPath(w - 5, bounds.getY() + 5);
        rtl.quadraticTo(w * 0.3f, bounds.getY(), bounds.getX() + 5, h);

        g.strokePath(ltr, PathStrokeType(4.f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
        g.strokePath(rtl, PathStrokeType(4.f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));

        g.setColour(theme.accent);
        g.drawRoundedRectangle(bounds.toFloat(), 10.f, 5.f);

        amp_mode.setColour(ComboBox::ColourIds::textColourId, Colours::white);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        const int h = b.getHeight();
        const int w = b.getWidth();
        auto knob_bounds = b.removeFromBottom((float)h * 0.66f);

        gain_ch.setBounds(b.removeFromLeft(w / 4).reduced(10));
        amp_mode.setBounds(b.removeFromLeft(w / 4).reduced(10));
        bright.setBounds(b.removeFromLeft(w / 4).reduced(10));
        on.setBounds(b.removeFromLeft(w / 4).reduced(10));
        
        const int div = w / (sizeof(knobs) / sizeof(knobs[0]));
        for (auto &k : knobs) {
            k.setBounds(knob_bounds.removeFromLeft(div));
        }
    }

private:

    using Apvts = AudioProcessorValueTreeState;

    Apvts &apvts;

    Theme theme = str_x_higain_theme;
    
    AmpKnob knobs[NumKnobs] = {
        {str_x_knobs[Preamp]},
        {str_x_knobs[Bass]},
        {str_x_knobs[Mid]},
        {str_x_knobs[Treble]},
        {str_x_knobs[Presence]},
        {str_x_knobs[Master]},
    };

    struct {
        std::unique_ptr<Apvts::SliderAttachment> preamp;
        std::unique_ptr<Apvts::SliderAttachment> bass;
        std::unique_ptr<Apvts::SliderAttachment> mid;
        std::unique_ptr<Apvts::SliderAttachment> treble;
        std::unique_ptr<Apvts::SliderAttachment> presence;
        std::unique_ptr<Apvts::SliderAttachment> master;
    } knob_attachments;

    AmpModeMenu amp_mode; 
    GainChButton gain_ch;
    TextButton bright{"Bright"};
    TextButton on{"On"};

    std::unique_ptr<Apvts::ComboBoxAttachment> amp_mode_attach;
    std::unique_ptr<Apvts::ButtonAttachment> gain_ch_attach, bright_attach, on_attach;
};

struct MainComponent : Component
{
    MainComponent(AudioProcessorValueTreeState &vts) : apvts(vts), str_x(vts)
    {
        addAndMakeVisible(str_x);
    }

    ~MainComponent()
    {
    }

    void resized() override
    {
        str_x.setBounds(getLocalBounds());
    }

private:
    AudioProcessorValueTreeState &apvts;

    StrXComponent str_x;

};
