// Background.hpp

#pragma once

/* component for drawing plugin title, logo, version info */
struct Background : Component
{
    Background(AudioProcessorValueTreeState &v) : channel(static_cast<strix::ChoiceParameter *>(v.getParameter("channel")))
    {
        strix = Drawable::createFromImageData(BinaryData::strx_svg, BinaryData::strx_svgSize);
    }

    ~Background()
    {
        channel = nullptr;
    }

    void paint(Graphics &g) override
    {
        g.setColour(*channel ? Colours::black : Colour(BLUE_BG));

        auto textbounds = getLocalBounds().reduced(getWidth() * 0.1f + 10, 10).toFloat();
        g.fillRoundedRectangle(textbounds, 10.f);

    #if LINUX // horrible!
        strix->drawWithin(g, textbounds, RectanglePlacement::centred, 1.f);
    #else
        strix->drawWithin(g, getLocalBounds().toFloat(), RectanglePlacement::xLeft, 1.f);
    #endif

        g.setColour(*channel ? Colour(GREEN) : Colour(LIGHT_ACCENT));
        g.drawRoundedRectangle(textbounds, 10.f, 3.f);
    }

private:
    std::unique_ptr<Drawable> strix;

    strix::ChoiceParameter *channel = nullptr;
};