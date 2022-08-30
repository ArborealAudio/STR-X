// Background.hpp

#pragma once

/* component for drawing plugin title, logo, version info */
struct Background : Component, private Timer
{
    Background(AudioProcessorValueTreeState& v)
    {
        strix = Drawable::createFromImageData(BinaryData::strx_svg, BinaryData::strx_svgSize);

        channel = v.getRawParameterValue("channel");

        startTimerHz(10);
    }

    ~Background() override
    {
        stopTimer();
    }

    void timerCallback() override
    {
        if (lastChannelState != *channel)
            repaint(getLocalBounds());
        lastChannelState = *channel;
    }

    void paint(Graphics &g) override
    {
        g.setColour(*channel ? Colours::black : Colour(BLUE_BG));

        auto textbounds = getLocalBounds().reduced(getWidth() * 0.1f, 0);
        g.fillRoundedRectangle(textbounds.reduced(10).toFloat(), 10.f);

        strix->drawWithin(g, getLocalBounds().toFloat(), RectanglePlacement::xLeft, 1.f);

        g.setColour(*channel ? Colour(GREEN) : Colours::wheat);
        g.drawRoundedRectangle(textbounds.toFloat().reduced(10), 10.f, 3.f);
    }

    void resized() override
    {}

private:
    std::unique_ptr<Drawable> strix;

    std::atomic<float> *channel;
    bool lastChannelState = true;
};