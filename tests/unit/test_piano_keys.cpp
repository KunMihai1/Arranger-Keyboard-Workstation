#include <JuceHeader.h>
#include "MidiHandler.h"
#include "KeyboardUI.h"
#include "PianoKeyShapes.h"

namespace PianoKeysTestValues
{
    const juce::String testName     = "PianoKeys";
    const juce::String testCategory = "Unit";

    const juce::String countsAndWidths   = "52 white and 36 black keys; a white key is 22 of 1200/52 wide, a black 14 x 88 (board)";
    const juce::String blackOffsets      = "black keys sit off the boundary like a real keyboard: C# and F# 2 px left, D# and A# 2 px right, G# centred";
    const juce::String scalesWithWidth   = "at 1.6 x (1920 wide) every length is 1.6 x the board's";
    const juce::String outsideThePiano   = "notes outside A0..C8 have no shape";
    const juce::String whiteKeyMaterial  = "a white key shows the board's ivory gradient and its front lip; the gap shows the key bed";
    const juce::String pressedWhiteKey   = "a pressed white key is lit amber and pushed down, not green (owner: no green)";
    const juce::String pressedBlackKey   = "a pressed black key is lit amber too: seen at a glance (owner, 2026-11-28)";
    const juce::String blackKeyMaterial  = "a black key shows the board's ebony gradient and the lit front slope; it casts a shadow";
    const juce::String outOfRangeDimmed  = "a white key outside the device range is dimmed, as today";
    const juce::String allKeysInOrder    = "all 88 keys: whites side by side left to right, each black key between its two whites";
    const juce::String pressRepaintsAll  = "pressing any key changes no pixel outside the area it repaints, at several widths";
    const juce::String paintTime         = "paint time of the whole keyboard at 1920 x 224 (logged)";
    const juce::String paintTimeMessage  = "ms per full keyboard paint: ";
    const juce::String versus            = " vs ";

    constexpr int   whiteCount      = 52;
    constexpr int   blackCount      = 36;
    constexpr float boardWidth      = 1200.0f;
    constexpr float boardHeight     = 140.0f;
    constexpr float ownerScale      = 1.6f;
    constexpr float boardWhiteStep  = boardWidth / whiteCount;
    constexpr float boardWhiteWidth = 22.0f;
    constexpr float boardBlackWidth = 14.0f;
    constexpr float boardBlackHeight = 88.0f;
    constexpr float boardBlackHalf  = 7.0f;
    constexpr float boardOffset     = 2.0f;
    constexpr float geometryTolerance = 0.01f;

    constexpr int noteA0     = 21;
    constexpr int noteBelow  = 20;
    constexpr int noteC8     = 108;
    constexpr int noteAbove  = 109;
    constexpr int noteC4     = 60;
    constexpr int noteCSharp4 = 61;
    constexpr int noteDSharp4 = 63;
    constexpr int noteE4     = 64;
    constexpr int noteF4     = 65;
    constexpr int noteFSharp4 = 66;
    constexpr int noteGSharp4 = 68;
    constexpr int noteASharp4 = 70;
    constexpr int whitesBelowC4 = 23;   // A0, B0 and three octaves of 7

    // Board colours (StageRevised .wk / .wk.down / .bk and the keys container)
    const juce::Colour keyBed            { 0xff0b0b0c };
    const juce::Colour whiteTop          { 0xffe9e6de };
    const juce::Colour whiteMiddle       { 0xfffaf9f4 };   // between #f8f7f2 (12 %) and #fbfaf6 (80 %) at half height
    const juce::Colour whiteLip          { 0xffd8d3c6 };
    const juce::Colour pressedMiddle     { 0xffffc47c };   // between #ffbf73 (14 %) and #ffc985 (85 %) at half height
    const juce::Colour pressedLip        { 0xffd8903f };
    const juce::Colour blackTop          { 0xff151516 };   // #141415 -> #1e1e20 near the top
    const juce::Colour blackSlope        { 0xff333336 };   // between #3b3b3f (84 %) and #2a2a2d (100 %)
    constexpr int colourTolerance = 8;
    constexpr int maxGreenTint    = 6;    // a pressed key's green may exceed its red by at most this
    constexpr int topShadowDepth  = 6;    // the pressed key's top is this much darker than below its shadow
    constexpr int shadowDepth     = 3;    // a black key's shadow darkens the white key next to it by this much
    // "Seen at a glance": a pressed key differs from a resting one by more than this RGB distance (the old pressed
    // white differed by ~32, the old pressed black by ~11), and is amber: red above blue by more than this.
    constexpr int glanceDistance  = 100;
    constexpr int amberMargin     = 60;
    constexpr int blackMiddleRow  = 44;   // half of the black key's 88 rows

    constexpr int topRow        = 1;
    constexpr int middleRow     = 70;
    constexpr int lipRow        = 138;
    constexpr int gapSampleRow  = 120;
    constexpr int shadowRow     = 40;
    constexpr int belowShadowRow = 20;
    constexpr int blackTopRow   = 8;
    constexpr int blackSlopeRow = 81;
    constexpr float clearOfBlackKeys = 0.75f;   // along a white key: right of the black key before it
    constexpr float keyCentre   = 0.5f;
    constexpr int   gapPixelFromRight = 1;      // the pixel left of the next key's start is the bed
    constexpr int   besideBlack = 1;            // px right of a black key's edge

    constexpr int deviceLowest  = 36;
    constexpr int deviceHighest = 96;
    constexpr int dimmedMin     = 0x70;
    constexpr int dimmedMax     = 0x90;

    // Window widths where a key's left edge falls on many pixel fractions (1075: a white key drawn 1 px past its step)
    constexpr int checkedWidths[] = { 1075, 1366, 1920 };
    constexpr int checkedHeight   = 140;
    constexpr int semitone        = 1;

    constexpr int timedWidth  = 1920;
    constexpr int timedHeight = 224;
    constexpr int timedPaints = 100;
    constexpr int origin      = 0;
}

using namespace PianoKeysTestValues;

class PianoKeysTest : public juce::UnitTest
{
public:
    PianoKeysTest() : juce::UnitTest (PianoKeysTestValues::testName, PianoKeysTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (countsAndWidths);
        {
            int whites {}, blacks {};
            for (int note = noteA0; note <= noteC8; ++note)
                ++(PianoKeyShapes::isBlack (note) ? blacks : whites);
            expectEquals (whites, whiteCount);
            expectEquals (blacks, blackCount);
            expectEquals (PianoKeyShapes::whiteKeysBelow (noteC4), whitesBelowC4);

            const auto e4 = PianoKeyShapes::shapeOf (noteE4, boardWidth, boardHeight);
            expectWithinAbsoluteError (e4.getX(), PianoKeyShapes::whiteKeysBelow (noteE4) * boardWhiteStep, geometryTolerance);
            expectWithinAbsoluteError (e4.getWidth(), boardWhiteWidth, geometryTolerance);
            expectWithinAbsoluteError (e4.getHeight(), boardHeight, geometryTolerance);

            const auto cSharp = PianoKeyShapes::shapeOf (noteCSharp4, boardWidth, boardHeight);
            expectWithinAbsoluteError (cSharp.getWidth(), boardBlackWidth, geometryTolerance);
            expectWithinAbsoluteError (cSharp.getHeight(), boardBlackHeight, geometryTolerance);
        }

        beginTest (blackOffsets);
        {
            auto offsetOf = [] (int note)
            {
                const auto boundary = PianoKeyShapes::whiteKeysBelow (note) * boardWhiteStep;
                return PianoKeyShapes::shapeOf (note, boardWidth, boardHeight).getX() + boardBlackHalf - boundary;
            };
            expectWithinAbsoluteError (offsetOf (noteCSharp4), -boardOffset, geometryTolerance);
            expectWithinAbsoluteError (offsetOf (noteDSharp4),  boardOffset, geometryTolerance);
            expectWithinAbsoluteError (offsetOf (noteFSharp4), -boardOffset, geometryTolerance);
            expectWithinAbsoluteError (offsetOf (noteGSharp4),  float {}, geometryTolerance);
            expectWithinAbsoluteError (offsetOf (noteASharp4),  boardOffset, geometryTolerance);
        }

        beginTest (scalesWithWidth);
        {
            const auto board  = PianoKeyShapes::shapeOf (noteDSharp4, boardWidth, boardHeight);
            const auto scaled = PianoKeyShapes::shapeOf (noteDSharp4, boardWidth * ownerScale, boardHeight * ownerScale);
            expectWithinAbsoluteError (scaled.getX(), board.getX() * ownerScale, geometryTolerance);
            expectWithinAbsoluteError (scaled.getWidth(), board.getWidth() * ownerScale, geometryTolerance);
            expectWithinAbsoluteError (scaled.getHeight(), board.getHeight() * ownerScale, geometryTolerance);
        }

        beginTest (outsideThePiano);
        {
            expect (PianoKeyShapes::shapeOf (noteBelow, boardWidth, boardHeight).isEmpty());
            expect (PianoKeyShapes::shapeOf (noteAbove, boardWidth, boardHeight).isEmpty());
        }

        beginTest (allKeysInOrder);
        {
            float previousWhiteRight {};
            for (int note = noteA0; note <= noteC8; ++note)
            {
                const auto shape = PianoKeyShapes::shapeOf (note, boardWidth, boardHeight);
                if (! PianoKeyShapes::isBlack (note))
                {
                    expectGreaterOrEqual (shape.getX(), previousWhiteRight);
                    previousWhiteRight = shape.getRight();
                    continue;
                }
                const auto below = PianoKeyShapes::shapeOf (note - semitone, boardWidth, boardHeight);
                const auto above = PianoKeyShapes::shapeOf (note + semitone, boardWidth, boardHeight);
                expect (! PianoKeyShapes::isBlack (note - semitone) && ! PianoKeyShapes::isBlack (note + semitone));
                expectGreaterThan (shape.getX(), below.getX());
                expectLessThan (shape.getRight(), above.getRight());
                expectLessThan (shape.getX(), below.getRight());
                expectGreaterThan (shape.getRight(), above.getX());
            }
        }

        MidiDevice device;
        MidiHandler handler (device);
        KeyboardUI keyboard (handler);
        keyboard.setBounds (origin, origin, (int) boardWidth, (int) boardHeight);
        keyboard.setVisible (true);

        const auto e4 = PianoKeyShapes::shapeOf (noteE4, boardWidth, boardHeight);
        const int e4Clear = (int) (e4.getX() + e4.getWidth() * clearOfBlackKeys);

        beginTest (whiteKeyMaterial);
        {
            const auto image = render (keyboard);
            expectColour (image.getPixelAt (e4Clear, topRow), whiteTop);
            expectColour (image.getPixelAt (e4Clear, middleRow), whiteMiddle);
            expectColour (image.getPixelAt (e4Clear, lipRow), whiteLip);
            const int gapX = (int) std::ceil (PianoKeyShapes::whiteKeysBelow (noteF4) * boardWhiteStep) - gapPixelFromRight;
            expectColour (image.getPixelAt (gapX, gapSampleRow), keyBed);
        }

        beginTest (pressedWhiteKey);
        {
            keyboard.setKeyDown (noteE4, true);
            const auto image = render (keyboard);
            const auto middle = image.getPixelAt (e4Clear, middleRow);
            expectLessOrEqual ((int) middle.getGreen() - (int) middle.getRed(), maxGreenTint);
            expectColour (middle, pressedMiddle);
            expectColour (image.getPixelAt (e4Clear, lipRow), pressedLip);
            expectGreaterThan ((int) image.getPixelAt (e4Clear, belowShadowRow).getRed() - (int) image.getPixelAt (e4Clear, topRow).getRed(),
                               topShadowDepth);
            keyboard.setKeyDown (noteE4, false);
            expectSeenAtAGlance (middle, render (keyboard).getPixelAt (e4Clear, middleRow));
        }

        beginTest (pressedBlackKey);
        {
            const auto dSharp = PianoKeyShapes::shapeOf (noteDSharp4, boardWidth, boardHeight);
            const int dSharpCentre = (int) (dSharp.getX() + dSharp.getWidth() * keyCentre);
            const auto resting = render (keyboard).getPixelAt (dSharpCentre, blackMiddleRow);
            keyboard.setKeyDown (noteDSharp4, true);
            const auto pressed = render (keyboard).getPixelAt (dSharpCentre, blackMiddleRow);
            keyboard.setKeyDown (noteDSharp4, false);
            expectLessOrEqual ((int) pressed.getGreen() - (int) pressed.getRed(), maxGreenTint);
            expectSeenAtAGlance (pressed, resting);
        }

        beginTest (blackKeyMaterial);
        {
            const auto image = render (keyboard);
            const auto dSharp = PianoKeyShapes::shapeOf (noteDSharp4, boardWidth, boardHeight);
            const int dSharpCentre = (int) (dSharp.getX() + dSharp.getWidth() * keyCentre);
            expectColour (image.getPixelAt (dSharpCentre, blackTopRow), blackTop);
            expectColour (image.getPixelAt (dSharpCentre, blackSlopeRow), blackSlope);

            const int besideKey = (int) dSharp.getRight() + besideBlack;
            expectGreaterThan ((int) image.getPixelAt (e4Clear, shadowRow).getRed() - (int) image.getPixelAt (besideKey, shadowRow).getRed(),
                               shadowDepth);
        }

        beginTest (outOfRangeDimmed);
        {
            keyboard.set_min_and_max (deviceLowest, deviceHighest);
            const auto image = render (keyboard);
            const auto a0 = PianoKeyShapes::shapeOf (noteA0, boardWidth, boardHeight);
            const auto dimmed = image.getPixelAt ((int) (a0.getX() + a0.getWidth() * keyCentre), middleRow);
            expectGreaterThan ((int) dimmed.getRed(), dimmedMin);
            expectLessThan ((int) dimmed.getRed(), dimmedMax);
            keyboard.set_min_and_max (noteBelow, noteAbove);
        }

        beginTest (pressRepaintsAll);
        {
            for (const auto width : checkedWidths)
            {
                keyboard.setBounds (origin, origin, width, checkedHeight);
                const auto released = render (keyboard);
                int keysChangedOutside {};
                for (int note = noteA0; note <= noteC8; ++note)
                {
                    keyboard.setKeyDown (note, true);
                    const auto pressed = render (keyboard);
                    keyboard.setKeyDown (note, false);
                    if (changesOutside (released, pressed, keyboard.areaOf (note)))
                        ++keysChangedOutside;
                }
                expectEquals (keysChangedOutside, int {}, juce::String (width));
            }
            keyboard.setBounds (origin, origin, (int) boardWidth, (int) boardHeight);
        }

        beginTest (paintTime);
        {
            keyboard.setBounds (origin, origin, timedWidth, timedHeight);
            juce::Image image (juce::Image::ARGB, timedWidth, timedHeight, true);
            const auto start = juce::Time::getMillisecondCounterHiRes();
            for (int paint {}; paint < timedPaints; ++paint)
            {
                juce::Graphics g (image);
                keyboard.paintEntireComponent (g, false);
            }
            logMessage (paintTimeMessage + juce::String ((juce::Time::getMillisecondCounterHiRes() - start) / timedPaints));
        }
    }

private:
    static juce::Image render (KeyboardUI& keyboard)
    {
        juce::Image image (juce::Image::ARGB, keyboard.getWidth(), keyboard.getHeight(), true);
        juce::Graphics g (image);
        keyboard.paintEntireComponent (g, false);
        return image;
    }

    /** True if any pixel outside `area` differs between the two renders (row by row, left and right of the area). */
    static bool changesOutside (const juce::Image& before, const juce::Image& after, juce::Rectangle<int> area)
    {
        const juce::Image::BitmapData beforeData (before, juce::Image::BitmapData::readOnly);
        const juce::Image::BitmapData afterData (after, juce::Image::BitmapData::readOnly);
        const auto clipped = area.getIntersection (before.getBounds());
        for (int row {}; row < before.getHeight(); ++row)
        {
            const auto rowInside = row >= clipped.getY() && row < clipped.getBottom();
            const auto spans = rowInside ? std::initializer_list<juce::Range<int>> { { origin, clipped.getX() }, { clipped.getRight(), before.getWidth() } }
                                         : std::initializer_list<juce::Range<int>> { { origin, before.getWidth() } };
            for (const auto span : spans)
                if (! span.isEmpty()
                    && std::memcmp (beforeData.getPixelPointer (span.getStart(), row), afterData.getPixelPointer (span.getStart(), row),
                                    static_cast<size_t> (span.getLength() * beforeData.pixelStride)) != int {})
                    return true;
        }
        return false;
    }

    /** The pressed colour is amber and far from the resting one. */
    void expectSeenAtAGlance (juce::Colour pressed, juce::Colour resting)
    {
        const auto squared = [] (juce::uint8 left, juce::uint8 right) { const int step = (int) left - (int) right; return step * step; };
        const auto distance = std::sqrt ((double) (squared (pressed.getRed(), resting.getRed()) + squared (pressed.getGreen(), resting.getGreen())
                                                   + squared (pressed.getBlue(), resting.getBlue())));
        expectGreaterThan (distance, (double) glanceDistance, pressed.toDisplayString (false) + versus + resting.toDisplayString (false));
        expectGreaterThan ((int) pressed.getRed() - (int) pressed.getBlue(), amberMargin, pressed.toDisplayString (false));
    }

    void expectColour (juce::Colour actual, juce::Colour expected)
    {
        const auto near = [] (juce::uint8 left, juce::uint8 right) { return std::abs ((int) left - (int) right) <= colourTolerance; };
        expect (near (actual.getRed(), expected.getRed()) && near (actual.getGreen(), expected.getGreen())
                    && near (actual.getBlue(), expected.getBlue()),
                actual.toDisplayString (false) + versus + expected.toDisplayString (false));
    }
};

static PianoKeysTest pianoKeysTest;
