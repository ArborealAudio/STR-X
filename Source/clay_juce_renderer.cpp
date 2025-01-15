#include <JuceHeader.h>
#include "clay.h"
#include <stdio.h>

static Font default_font = Font();

static juce::Colour colorConvert(Clay_Color c)
{
    return {(uint8)c.r, (uint8)c.g, (uint8)c.b, (uint8)c.a};
}

static juce::Rectangle<float> rectConvert(Clay_BoundingBox b)
{
    return {b.x, b.y, b.width, b.height};
}

static Clay_Dimensions measureText(Clay_String *text, Clay_TextElementConfig *config)
{
    float width = default_font.getStringWidthFloat(String(text->chars));
    float height = default_font.getHeight();
    return {.width = width, .height = height};
}

static void ClayJuceRender(Graphics &g, Clay_RenderCommandArray renderCmd)
{
    for (int32 i = 0; i < renderCmd.length; ++i) {
        Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&renderCmd, i);
        Clay_BoundingBox bounds = cmd->boundingBox;

        switch (cmd->commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
            Clay_RectangleElementConfig *config = cmd->config.rectangleElementConfig;
            g.setColour(colorConvert(config->color));
            float radius = config->cornerRadius.topLeft;
            g.fillRoundedRectangle(bounds.x, bounds.y, bounds.width, bounds.height, radius);
        } break;
        case CLAY_RENDER_COMMAND_TYPE_BORDER: {
            fprintf(stderr, "Clay_Border currently unsupported\n");
        } break;
        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
            Clay_TextElementConfig *config = cmd->config.textElementConfig;
            Clay_String text = cmd->text;
            // TODO: Support different fonts, somehow
            g.setFont(default_font.withHeight((float)config->fontSize).withExtraKerningFactor(config->letterSpacing));
            g.setColour(colorConvert(config->textColor));
            if (config->wrapMode == CLAY_TEXT_WRAP_NONE)
                g.drawText(String(text.chars), rectConvert(bounds), Justification::centred);
            else {
                int num_lines = bounds.height / config->lineHeight;
                g.drawFittedText(String(text.chars), rectConvert(bounds).toNearestInt(), Justification::left, num_lines);
            }
        } break;
        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
            Clay_CustomElementConfig *config = cmd->config.customElementConfig;
            fprintf(stderr, "Custom elements currently unsupported\n");
        } break;
        case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
            Clay_ImageElementConfig *config = cmd->config.imageElementConfig;
            auto image = Image(Image::PixelFormat::ARGB, config->sourceDimensions.width,
                               config->sourceDimensions.height, false);
            auto bmp_data = Image::BitmapData(image, Image::BitmapData::readWrite);
            memcpy(bmp_data.data, config->imageData, config->sourceDimensions.width * config->sourceDimensions.height);
            g.drawImage(image, rectConvert(bounds));
        } break;
        default:
            fprintf(stderr, "Command unimplemented");
            break;
        }
    }
}
