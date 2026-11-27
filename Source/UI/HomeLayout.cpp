#include "HomeLayout.h"

namespace
{
    constexpr float half = 0.5f;

    // The Play text's spot on the artwork, as fractions of the picture (where it sat on a 16:9 window). The box is
    // transparent, a little taller than before so the text fits, and centred on the same spot.
    namespace Play
    {
        constexpr float centreX      = 0.35625f;
        constexpr float centreY      = 0.68283f;
        constexpr float width        = 0.0714286f;   // 1/14 of the picture
        constexpr float height       = 0.0454545f;   // 1/22
        constexpr float edgeIndent   = 4.0f;    // LookAndFeel_V4::drawButtonText leaves min (4, 0.3 * height) above and below
        constexpr float indentShare  = 0.3f;
        constexpr float twice        = 2.0f;
        constexpr float textShare    = 0.95f;   // of the room left, so the glyphs never touch its edge
    }
}

namespace HomeLayout
{
    juce::Rectangle<float> coverRect (juce::Rectangle<float> bounds, float imageWidth, float imageHeight)
    {
        if (imageWidth <= float {} || imageHeight <= float {})
            return bounds;   // no picture (it failed to load): lay out on the window rather than divide by zero
        const auto scale = juce::jmax (bounds.getWidth() / imageWidth, bounds.getHeight() / imageHeight);
        const auto width = imageWidth * scale;
        const auto height = imageHeight * scale;
        return { bounds.getX() + (bounds.getWidth() - width) * half, bounds.getY() + (bounds.getHeight() - height) * half, width, height };
    }

    juce::Rectangle<int> playButton (juce::Rectangle<int> window, float imageWidth, float imageHeight)
    {
        const auto picture = coverRect (window.toFloat(), imageWidth, imageHeight);
        const auto box = juce::Rectangle<float> (picture.getWidth() * Play::width, picture.getHeight() * Play::height)
                             .withCentre ({ picture.getX() + picture.getWidth() * Play::centreX, picture.getY() + picture.getHeight() * Play::centreY });
        return box.toNearestInt().constrainedWithin (window);
    }

    float playTextHeight (int buttonHeight)
    {
        const auto height = static_cast<float> (buttonHeight);
        return (height - Play::twice * juce::jmin (Play::edgeIndent, height * Play::indentShare)) * Play::textShare;
    }
}
