// LookAndFeel.h

#pragma once

struct CustomLookAndFeel : LookAndFeel_V4,
                           private AudioProcessorValueTreeState::Listener
{
    Colour mainColor, accentColor, buttonBackground;

    CustomLookAndFeel(AudioProcessorValueTreeState &v) : apvts(v)
    {
        apvts.addParameterListener("channel", this);
        getDefaultLookAndFeel().setDefaultSansSerifTypeface(getCustomFont());
        int channel = *apvts.getRawParameterValue("channel");
        mainColor = channel ? Colours::black : Colours::grey;
        accentColor = channel ? Colour(GREEN) : Colours::wheat;
        buttonBackground = channel ? Colour(GRAY) : Colours::wheat;
    }

    ~CustomLookAndFeel()
    {
        apvts.removeParameterListener("channel", this);
    }

    void parameterChanged(const String &parameterID, float newValue) override
    {
        if (parameterID == "channel")
        {
            mainColor = newValue ? Colours::black : Colours::grey;
            accentColor = newValue ? Colour(GREEN) : Colours::wheat;
            buttonBackground = newValue ? Colour(GRAY) : Colours::wheat;
        }
    }

    void drawRotarySlider(Graphics &g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, Slider &slider) override
    {
        auto radius = (float)jmin(width / 2, height / 2) - 4.f;
        auto centerX = (float)slider.getLocalBounds().getCentreX();
        auto centerY = (float)slider.getLocalBounds().getCentreY();
        auto rx = centerX - radius;
        auto ry = centerY - radius;
        auto rw = radius * 2.f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(Colours::white);
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(Colour(GRAY));
        g.drawEllipse(rx, ry, rw, rw, 3.f);

        Path p;
        const auto pointerLength = radius * 0.8f;
        const auto pointerThickness = 2.5f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(AffineTransform::rotation(angle).translated(centerX, centerY));

        g.setColour(Colour(GRAY));
        g.fillPath(p);

        if (slider.isMouseOverOrDragging())
        {
            g.setColour(accentColor);
            g.drawEllipse(rx, ry, rw, rw, 3.0f);
        }
    }

    void drawComboBox(Graphics &g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox &comboBox) override
    {
        g.setColour(mainColor);
        g.fillRoundedRectangle(comboBox.getLocalBounds().toFloat(), 5.f);
        g.setColour(accentColor);
        g.drawRoundedRectangle(comboBox.getLocalBounds().toFloat().reduced(3.f), 5.f, 3.f);
    }

    void positionComboBoxText(ComboBox &box, Label &label) override
    {
        label.setBounds(1, 1, box.getWidth(), box.getHeight() - 2);
        label.setJustificationType(Justification::centred);
    }

    void drawButtonBackground(Graphics &g, Button &button, const Colour &, bool, bool) override
    {
        g.setColour(mainColor);
        g.fillRoundedRectangle(button.getLocalBounds().reduced(5).toFloat(), 3.f);
        auto buttonArea = button.getLocalBounds().reduced(5).toFloat();
        g.setColour(buttonBackground);
        g.drawRoundedRectangle(buttonArea, 3.f, 2.f);

        if (button.isMouseOver())
        {
            g.setColour(Colour(GRAY));
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

        g.setColour(buttonBackground.contrasting());
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), Justification::centred, 2);
    }
private:
    AudioProcessorValueTreeState &apvts;
};