// LookAndFeel.h

#pragma once

class CustomLookAndFeel : public LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(Slider::thumbColourId, Colours::white);
    }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override
    {
        auto radius = (float)jmin(width / 2, height / 2) - 4.0f;
        auto centerX = (float)x + (float)width * 0.5f;
        auto centerY = (float)y + (float)height * 0.5f;
        auto rx = centerX - radius;
        auto ry = centerY - radius;
        auto rw = radius * 2.f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(Colours::white);
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(Colour (GRAY));
        g.drawEllipse(rx, ry, rw, rw, 3.0f);

        Path p;
        auto pointerLength = radius * 0.5f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(AffineTransform::rotation(angle).translated(centerX, centerY));

        g.setColour(Colour (GRAY));
        g.fillPath(p);

        if (slider.isMouseOverOrDragging())
        {
            g.setColour(Colour(GREEN));
            g.drawEllipse(rx, ry, rw, rw, 3.0f);
        }
       
    }

    void drawComboBox(Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
        int buttonW, int buttonH, ComboBox& comboBox) override
    {
        g.fillAll(Colours::black);
        g.setColour(Colour(GREEN));
        g.drawRoundedRectangle(comboBox.getLocalBounds().toFloat().reduced(3.f), 5.f, 3.f);
    }

    void positionComboBoxText(ComboBox& box, Label& label) override
    {
        label.setBounds (1, 1, box.getWidth(), box.getHeight() - 2);
        label.setJustificationType(Justification::centred);
    }

    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColor,
        bool isMouseOverButton, bool isButtonDown) override
    {
        g.fillAll(Colours::black);
        auto buttonArea = button.getLocalBounds().reduced(5).toFloat();
        g.setColour(Colour(GRAY));
        g.drawRoundedRectangle(buttonArea, 3.f, 2.f);

        if (button.isMouseOver()) {
            g.setColour(Colour(GRAY));
            g.fillRoundedRectangle(buttonArea, 3.f);
        }

        if (button.getToggleState()) {
            g.setColour(Colour(GREEN));
            g.fillRoundedRectangle(buttonArea, 3.f);
        }
    }

};