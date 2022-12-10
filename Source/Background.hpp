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

        auto textbounds = getLocalBounds().reduced(getWidth() * 0.1f, 0);
        g.fillRoundedRectangle(textbounds.reduced(10).toFloat(), 10.f);

        strix->drawWithin(g, getLocalBounds().toFloat(), RectanglePlacement::centred, 1.f);

        g.setColour(*channel ? Colour(GREEN) : Colours::wheat);
        g.drawRoundedRectangle(textbounds.toFloat().reduced(10), 10.f, 3.f);
    }

private:
    std::unique_ptr<Drawable> strix;

    strix::ChoiceParameter *channel = nullptr;
};