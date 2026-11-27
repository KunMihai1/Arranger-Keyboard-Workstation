#include <JuceHeader.h>
#include "UI/HomeLayout.h"

/*  Owner (2026-11-27): "The play text on top of the piano photo is a bit trimmed and has to be in the same place for
    every type of width height of screen". The photo covers the window (scaled to fill, centred, cropped), but the
    button was placed at fractions of the window, so on any other shape it drifted off the piano; and its 40 px bold
    text was taller than the box JUCE leaves it (button height minus 4 px above and below). */

namespace HomeLayoutTestValues
{
    const juce::String testName     = "HomeLayout";
    const juce::String testCategory = "Unit";

    const juce::String fullHdUnchanged = "on a 1920 x 1080 window the Play text stays where it was";
    const juce::String samePlace       = "on any window shape the Play text sits on the same spot of the picture";
    const juce::String textFits        = "the Play text is never taller than the room the button gives it";
    const juce::String insideWindow    = "on a tall narrow window the button stays inside the window";

    constexpr float artworkWidth  = 1920.0f;
    constexpr float artworkHeight = 1080.0f;
    constexpr int   fullHdWidth   = 1920;
    constexpr int   fullHdHeight  = 1080;
    constexpr float oldCentreX    = 0.35625f;    // the old box: 0.325 + 1/32 of the window
    constexpr float oldCentreY    = 0.68283f;    // 0.662 + 1/48
    constexpr float onePixel      = 1.0f;
    constexpr float fittingIndent = 4.0f;        // LookAndFeel_V4::drawButtonText: min (4, 0.3 * height) above and below
    constexpr float indentShare   = 0.3f;
    constexpr float twice         = 2.0f;
    constexpr int   smallestHeight = 300;
    constexpr int   largestHeight  = 2160;
    constexpr int   heightStep     = 7;
    constexpr int   wideAspectWidth = 16;
    constexpr int   wideAspectHeight = 9;
    constexpr int   portraitWidth  = 600;
    constexpr int   portraitHeight = 1000;

    struct WindowSize { int width; int height; };
    const std::array<WindowSize, 6> windows {{ { 1280, 1024 }, { 2560, 1080 }, { 1366, 768 }, { 800, 600 }, { 3840, 2160 }, { 1024, 1366 } }};
}

class HomeLayoutTest : public juce::UnitTest
{
public:
    HomeLayoutTest() : juce::UnitTest (HomeLayoutTestValues::testName, HomeLayoutTestValues::testCategory) {}

    void runTest() override
    {
        using namespace HomeLayoutTestValues;

        beginTest (fullHdUnchanged);
        {
            const juce::Rectangle<int> window (fullHdWidth, fullHdHeight);
            const auto play = HomeLayout::playButton (window, artworkWidth, artworkHeight).toFloat();
            expectWithinAbsoluteError (play.getCentreX(), fullHdWidth * oldCentreX, onePixel);
            expectWithinAbsoluteError (play.getCentreY(), fullHdHeight * oldCentreY, onePixel);
        }

        beginTest (samePlace);
        for (const auto& size : windows)
        {
            const juce::Rectangle<int> window (size.width, size.height);
            const auto picture = HomeLayout::coverRect (window.toFloat(), artworkWidth, artworkHeight);
            const auto play = HomeLayout::playButton (window, artworkWidth, artworkHeight).toFloat();
            expectWithinAbsoluteError (play.getCentreX(), picture.getX() + picture.getWidth() * oldCentreX, onePixel);
            expectWithinAbsoluteError (play.getCentreY(), picture.getY() + picture.getHeight() * oldCentreY, onePixel);
        }

        beginTest (textFits);
        for (int height = smallestHeight; height <= largestHeight; height += heightStep)
        {
            const juce::Rectangle<int> window (height * wideAspectWidth / wideAspectHeight, height);
            const auto play = HomeLayout::playButton (window, artworkWidth, artworkHeight);
            const auto boxHeight = static_cast<float> (play.getHeight());
            const auto room = boxHeight - twice * juce::jmin (fittingIndent, boxHeight * indentShare);
            expect (HomeLayout::playTextHeight (play.getHeight()) <= room);
        }

        beginTest (insideWindow);
        {
            const juce::Rectangle<int> window (portraitWidth, portraitHeight);
            expect (window.contains (HomeLayout::playButton (window, artworkWidth, artworkHeight)));
        }
    }
};
static HomeLayoutTest homeLayoutTest;
