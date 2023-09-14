// LookAndFeel.h

#pragma once

struct AmpKnob : Slider
{
    AmpKnob() = default;

    String label = "";
};

struct CustomLookAndFeel : LookAndFeel_V4,
                           private AudioProcessorValueTreeState::Listener
{
    Colour mainColor, accentColor, buttonOutline;

    CustomLookAndFeel(AudioProcessorValueTreeState &v) : apvts(v)
    {
        apvts.addParameterListener("channel", this);
        getDefaultLookAndFeel().setDefaultSansSerifTypeface(getCustomFont());
        int channel = *apvts.getRawParameterValue("channel");
        mainColor = channel ? Colours::black : Colour(BLUE_BG);
        accentColor = channel ? Colour(GREEN) : Colour(LIGHT_ACCENT);
        buttonOutline = channel ? Colour(GREEN) : Colour(LIGHT_ACCENT);
    }

    ~CustomLookAndFeel()
    {
        apvts.removeParameterListener("channel", this);
    }

    void parameterChanged(const String &parameterID, float newValue) override
    {
        if (parameterID == "channel")
        {
            mainColor = newValue ? Colours::black : Colour(BLUE_BG);
            accentColor = newValue ? Colour(GREEN) : Colour(LIGHT_ACCENT);
            buttonOutline = newValue ? Colour(GRAY) : Colour(LIGHT_ACCENT);
        }
    }

    void drawRotarySlider(Graphics &g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, Slider &slider) override
    {
        width *= 0.85f;
        height *= 0.85f;

        auto radius = (float)jmin(width / 2, height / 2) - 4.f;
        auto centerX = (float)slider.getLocalBounds().getCentreX();
        auto centerY = (float)slider.getLocalBounds().getCentreY() + (height / 13.33);
        auto rx = centerX - radius;
        auto ry = centerY - radius;
        auto rw = radius * 2.f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(Colours::white);
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(slider.isMouseOverOrDragging() ? accentColor : Colour(GRAY));
        g.drawEllipse(rx, ry, rw, rw, 3.f);

        Path p;
        const auto pointerLength = radius * 0.8f;
        const auto pointerThickness = 2.5f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(AffineTransform::rotation(angle).translated(centerX, centerY));

        g.setColour(Colour(GRAY));
        g.fillPath(p);

        if (auto *knob = dynamic_cast<AmpKnob*>(&slider))
        {
            String text;
            if (slider.isMouseOverOrDragging())
                text = String(slider.getValue(), 0);
            else
                text = knob->label;
            g.setColour(mainColor.contrasting(0.1f));
            g.fillRoundedRectangle(slider.getLocalBounds().removeFromTop(slider.getHeight() / 6.66).toFloat(), 5.f);
            g.setColour(mainColor.contrasting());
            g.setFont(slider.getHeight() * 0.15f);
            g.drawFittedText(text, slider.getLocalBounds().removeFromTop(slider.getHeight() / 6.66), Justification::centred, 1);
        }
    }

    void drawComboBox(Graphics &g, int width, int height, bool isButtonDown,
					  int buttonX, int buttonY, int buttonW, int buttonH,
					  ComboBox &comboBox) override
    {
        g.setColour(accentColor);
        g.drawRoundedRectangle(comboBox.getLocalBounds().toFloat().reduced(3.f), 5.f, 3.f);
    }

    void positionComboBoxText(ComboBox &box, Label &label) override
    {
        label.setBounds(1, 1, box.getWidth(), box.getHeight() - 2);
        label.setJustificationType(Justification::centred);
    }

    void drawPopupMenuBackground(Graphics &g, int width, int height) override
    {
        g.fillAll(Colours::darkgrey);
    }

    void drawPopupMenuItem(Graphics &g, const Rectangle<int> &area, bool isSeparator,
						   bool isActive, bool isHighlighted, bool isTicked,
						   bool hasSubMenu, const String &text,
						   const String &shortcutKeyText, const Drawable *icon,
						   const Colour *textColour) override
    {
        if (isHighlighted)
        {
            g.setColour(Colours::grey);
            g.fillRoundedRectangle(area.toFloat(), 10.f);
        }

        if (isTicked)
        {
            g.setColour(Colours::white);
            g.fillEllipse(area.
						  withTrimmedRight(area.getWidth() - area.getHeight())
						  .reduced(area.getHeight() * 0.25f).toFloat());
        }

        g.setColour(Colours::white);
        g.drawFittedText(text, area, Justification::centred, 1);
    }

    void drawButtonBackground(Graphics &g, Button &button, const Colour &, bool, bool) override
    {
        auto buttonArea = button.getLocalBounds().reduced(5).toFloat();
        g.setColour(buttonOutline);
        g.drawRoundedRectangle(buttonArea, 3.f, 2.f);

        if (button.isMouseOver())
        {
            g.setColour(accentColor.darker(0.6f));
            g.fillRoundedRectangle(buttonArea, 3.f);
        }

        if (button.getToggleState())
        {
            g.setColour(accentColor);
            g.fillRoundedRectangle(buttonArea, 3.f);
        }
    }

    void drawButtonText(Graphics &g, TextButton &button, bool, bool) override
    {
        auto text = button.getButtonText();
        auto font = getTextButtonFont(button, button.getHeight());
        const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
        const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

        const int fontHeight = roundToInt(font.getHeight() * 0.6f);
        const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
        const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
        const int textWidth = button.getWidth() - leftIndent - rightIndent;

        g.setFont(jmin(button.getHeight() * 0.3f, (float)textWidth));

		auto state = button.getToggleState();
        g.setColour(state ? accentColor.contrasting() : mainColor.contrasting());
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), Justification::centred, 2);
    }
private:
    AudioProcessorValueTreeState &apvts;
};
