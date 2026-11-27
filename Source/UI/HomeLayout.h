#pragma once
#include <JuceHeader.h>

/*  The home screen (the piano picture with "Play"): where the picture lands and where the Play button sits on it.
    Pure, so the placement is unit-tested for every window shape. */
namespace HomeLayout
{
    /** Where a picture lands when it covers `bounds`: scaled to fill, centred, its overflow cropped. */
    juce::Rectangle<float> coverRect (juce::Rectangle<float> bounds, float imageWidth, float imageHeight);

    /** The Play button: on the same spot of the picture whatever the window's shape, kept inside the window. */
    juce::Rectangle<int> playButton (juce::Rectangle<int> window, float imageWidth, float imageHeight);

    /** The Play text's height: as large as the button's box lets it be drawn without being cut. */
    float playTextHeight (int buttonHeight);
}
