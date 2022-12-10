// AmpComponent.hpp

#pragma once

struct AmpComponent : Component
{
    AmpComponent(AudioProcessorValueTreeState &vts, CustomLookAndFeel *lnf_) : apvts(vts), lnf(*lnf_),
                                                                               channel(static_cast<strix::ChoiceParameter *>(vts.getParameter("channel")))
    {
        for (auto &k : getKnobs())
        {
            k->setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
            k->setLookAndFeel(&lnf);
            k->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
            k->setPopupDisplayEnabled(true, true, this);
            addAndMakeVisible(*k);
        }

        addAndMakeVisible(brightButton);
        brightButton.setButtonText("BRIGHT");
        brightButton.setLookAndFeel(&lnf);
        brightButton.setClickingTogglesState(true);

        addAndMakeVisible(channelButton);
        channelButton.setButtonText("HI GAIN");
        channelButton.setLookAndFeel(&lnf);
        channelButton.setClickingTogglesState(true);

        StringArray modes{"THICK", "NORMAL", "OPEN"};
        mode.addItemList(modes, 1);
        mode.setSelectedItemIndex(1);
        mode.setLookAndFeel(&lnf);
        mode.setTooltip("Switches the voicing of the preamp\n\nThick = Split @ 100Hz\nNormal = Split @ 250Hz\nOpen = Split @ 400Hz");
        addAndMakeVisible(mode);

        inputGainAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, "gain", inputGain);
        ts9Attach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, "tsXgain", ts9Gain);
        modeAttach = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "mode", mode);
        bassAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, "bass", bass);
        midAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, "mid", mid);
        trebleAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, "treble", treble);
        presenceAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, "presence", presence);
        brightAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(apvts, "bright", brightButton);
        channelAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(apvts, "channel", channelButton);
        outGainAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(apvts, "master", outGain);
    }

    ~AmpComponent()
    {
        for (auto *k : getKnobs())
            k->setLookAndFeel(nullptr);
        brightButton.setLookAndFeel(nullptr);
        channelButton.setLookAndFeel(nullptr);
        mode.setLookAndFeel(nullptr);
        channel = nullptr;
    }

    void paint(Graphics &g) override
    {
        auto bounds = getLocalBounds().reduced(3);
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();

        const int ch_ = *channel;

        g.setColour(ch_ ? Colours::black : Colour(BLUE_BG));
        g.fillRoundedRectangle(bounds.toFloat(), 10.f);

        g.setColour(ch_ ? Colours::whitesmoke : Colours::wheat);

        Path ltr;
        ltr.startNewSubPath(bounds.getX() + 8, bounds.getY() + 5);
        ltr.quadraticTo(w * 0.7f, bounds.getY(), w, h);

        Path rtl;
        rtl.startNewSubPath(w - 5, bounds.getY() + 5);
        rtl.quadraticTo(w * 0.3f, bounds.getY(), bounds.getX() + 5, h);

        g.strokePath(ltr, PathStrokeType(4.f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
        g.strokePath(rtl, PathStrokeType(4.f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));

        g.setColour(ch_ ? Colour(GREEN) : Colours::wheat);
        g.drawRoundedRectangle(bounds.toFloat(), 10.f, 5.f);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(15);
        auto controls = b.removeFromBottom(b.getHeight() / 2);
        auto w = controls.getWidth();
        auto h = controls.getHeight();
        auto chunk = w / 7;

        for (auto &k : getKnobs())
            k->setBounds(controls.removeFromLeft(chunk).reduced(chunk * 0.1f));

        mode.setSize(jmax(100.f, w * 0.15f), jmax(35.f, h * 0.3f));
        mode.setCentrePosition(b.getCentreX(), b.getCentreY());

        auto rightThird = b.removeFromRight(w / 3);
        brightButton.setSize(jmax(75.f, w * 0.1f), jmax(35.f, h * 0.3f));
        brightButton.setCentrePosition(rightThird.getCentreX(), rightThird.getCentreY());

        auto leftThird = b.removeFromLeft(w / 3);
        channelButton.setSize(jmax(75.f, w * 0.1f), jmax(35.f, h * 0.3f));
        channelButton.setCentrePosition(leftThird.getCentreX(), rightThird.getCentreY());
    }

private:
    AudioProcessorValueTreeState &apvts;
    CustomLookAndFeel &lnf;

    Slider ts9Gain, inputGain, bass, mid, treble, presence, outGain;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> ts9Attach, inputGainAttach, bassAttach, midAttach, trebleAttach, presenceAttach, outGainAttach;

    ComboBox mode;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach;

    TextButton brightButton, channelButton;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> brightAttach, channelAttach;

    strix::ChoiceParameter *channel = nullptr;

    std::vector<Slider *> getKnobs()
    {
        return {
            &ts9Gain,
            &inputGain,
            &bass,
            &mid,
            &treble,
            &presence,
            &outGain};
    }
};