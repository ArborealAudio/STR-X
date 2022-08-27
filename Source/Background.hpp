// Background.hpp

#pragma once

/* component for drawing plugin title, logo, version info */
struct Background : Component
{
    Background()
    {
        strix = Drawable::createFromImageData(BinaryData::strix_svg, BinaryData::strix_svgSize);
    }

    void paint(Graphics &g) override
    {
        g.fillAll(Colours::black);

        g.setColour(Colours::whitesmoke);
        g.drawFittedText("STR_X", getLocalBounds().reduced(10), Justification::centred, 1, 1.f);
    }

    void resized() override
    {}

private:
    std::unique_ptr<Drawable> strix;
};