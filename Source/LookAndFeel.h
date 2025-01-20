// LookAndFeel.h

#pragma once
#include <JuceHeader.h>
#include "zig/processor.h"
#define GREEN 0xff4e6f4e
#define GRAY 0xff373c40
#define BLUE_BG 0xff3b537a
#define LIGHT_ACCENT 0xffdedece

struct Theme
{
    Colour main, accent, dark_accent;
};

static const Theme str_x_higain_theme = {
    .main = Colours::black,
    .accent = Colour(GREEN),
    .dark_accent = Colour(GRAY),
};

static const Theme str_x_lowgain_theme = {
    .main = Colour(BLUE_BG),
    .accent = Colour(LIGHT_ACCENT),
    .dark_accent = Colour(GRAY),
};

struct CustomLookAndFeel : LookAndFeel_V4
{
    Theme theme;

    void drawComboBox(Graphics &g, int width, int height, bool isButtonDown,
					  int buttonX, int buttonY, int buttonW, int buttonH,
					  ComboBox &comboBox) override
    {
        g.setColour(theme.accent);
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
        g.setColour(theme.accent);
        g.drawRoundedRectangle(buttonArea, 3.f, 2.f);

        if (button.isMouseOver())
        {
            g.setColour(theme.accent.darker(0.6f));
            g.fillRoundedRectangle(buttonArea, 3.f);
        }

        if (button.getToggleState())
        {
            g.setColour(theme.accent);
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
        g.setColour(state ? theme.accent.contrasting() : theme.main.contrasting());
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(), Justification::centred, 2);
    }

    void drawRotarySlider(Graphics &g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, Slider &slider) override;
};

struct AmpKnob : Slider
{
    AmpKnob(const String &label) : label(label)
    {
        setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
        setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
        setLookAndFeel(&lnf);
    }

    ~AmpKnob()
    {
        setLookAndFeel(nullptr);
    }

    CustomLookAndFeel lnf;
    String label;
};

struct GainChButton : TextButton
{
    GainChButton() : TextButton("Channel")
    {
        setClickingTogglesState(true);
        setRepaintsOnMouseActivity(true);
        setLookAndFeel(&lnf);
    }

    ~GainChButton()
    {
        setLookAndFeel(nullptr);
    }

    struct LNF : CustomLookAndFeel
    {
        void drawButtonText(Graphics &g, TextButton &button, bool, bool) override
        {
            auto bounds = button.getLocalBounds().reduced(10);
            g.setColour(theme.dark_accent);
            g.fillRoundedRectangle(bounds.toFloat(), 10.f);

            const int w = bounds.getWidth();
            const int h = bounds.getHeight();

            auto toggle_bounds = bounds.removeFromRight((float)w * 0.25f).reduced(5, 10);

            g.setColour(Colours::white);
            auto label_bounds = bounds;
    		g.setFont((float)bounds.getHeight() * 0.3f);
            g.drawText("Low", label_bounds.removeFromTop(toggle_bounds.getHeight() / 2), Justification::centred);
            g.drawText("High", label_bounds, Justification::centred);
            g.drawRoundedRectangle(toggle_bounds.toFloat(), 10.f, 2.f);
    		GainChannel state = (GainChannel)button.getToggleState();
    		auto circle = state == HiGainChannel ? toggle_bounds.removeFromTop(toggle_bounds.getHeight() / 2) :
    		                              toggle_bounds.removeFromBottom(toggle_bounds.getHeight() / 2);
    		// circle.setHeight(circle.getWidth());
    		g.fillEllipse(circle.toFloat());
        }
    };

    LNF lnf;
};

struct AmpModeMenu : ComboBox
{
    AmpModeMenu(const StringArray &labels, int default_item)
    {
        addItemList(labels, 1);
        setSelectedItemIndex(default_item + 1);
    }

    CustomLookAndFeel lnf;
};

inline void CustomLookAndFeel::drawRotarySlider(Graphics &g, int x, int y, int width, int height, float sliderPos,
                      const float rotaryStartAngle, const float rotaryEndAngle, Slider &slider)
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
    g.setColour(slider.isMouseOverOrDragging() ? theme.accent : theme.dark_accent);
    g.drawEllipse(rx, ry, rw, rw, 3.f);

    Path p;
    const auto pointerLength = radius * 0.8f;
    const auto pointerThickness = 2.5f;
    p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    p.applyTransform(AffineTransform::rotation(angle).translated(centerX, centerY));

    g.setColour(theme.accent);
    g.fillPath(p);

    if (auto *knob = static_cast<AmpKnob*>(&slider))
    {
        String text;
        if (slider.isMouseOverOrDragging())
            text = String(slider.getValue(), 0);
        else
            text = knob->label;
        g.setColour(theme.main.contrasting(0.1f));
        g.fillRoundedRectangle(slider.getLocalBounds().removeFromTop(slider.getHeight() / 6.66).toFloat(), 5.f);
        g.setColour(theme.main.contrasting());
        g.setFont(slider.getHeight() * 0.15f);
        g.drawFittedText(text, slider.getLocalBounds().removeFromTop(slider.getHeight() / 6.66), Justification::centred, 1);
    }
}
