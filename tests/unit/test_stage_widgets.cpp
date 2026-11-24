#include <JuceHeader.h>
#include "UI/Stage/StageWidgets.h"
#include "UI/Stage/StageMetrics.h"
#include "Common/AppColours.h"

namespace StageWidgetsTestValues
{
    const juce::String testName     = "StageWidgets";
    const juce::String testCategory = "Unit";

    const juce::String ledState      = "a rubber button's LED starts off and follows setLedOn";
    const juce::String styleKept     = "a rubber button keeps its style, LED colour and label";
    const juce::String capOnlyClicks = "only the cap of a key button takes clicks, not its label";
    const juce::String keyGeometry   = "a key button's cap is centred, LED left of it, label under it";
    const juce::String knobRange     = "a knob runs 0-127 in steps of 1 over -135..+135 degrees";
    const juce::String knobDrag      = "a knob is a rotary slider without a text box";
    const juce::String glowFits      = "a lit LED's glow fits inside its button on every side (was: cut off on the left)";
    const juce::String handCursor    = "every rubber button shows the pointing-hand cursor (owner)";
    const juce::String glowLikeBoard = "a lit LED's glow fades like the board's CSS blur (box-shadow 0 0 3px: Gaussian, sigma 1.5 px), not brighter";
    constexpr float cssBlurSigmaPerRadius = 0.5f;   // CSS: a blur radius B is a Gaussian with sigma B / 2
    constexpr float edgeCoverage   = 0.5f;          // a blurred edge is half covered where the shape ends
    constexpr float glowTolerance  = 0.04f;
    constexpr float firstSampleGap = 1.0f;          // px past the LED edge: clear of its antialiasing
    constexpr float lastSampleGap  = 0.5f;          // px inside the end of the glow
    const juce::String gapPrefix   = "gap ";
    const juce::String widthLikeCss = "a label is as wide as CSS makes it: each letter and its letter-spacing once (was: the last spacing twice)";
    constexpr float noTracking     = 0.0f;
    constexpr int   gapDecimals    = 2;
    const juce::String knobTurns     = "a knob turns like a real one: grabbed anywhere, it follows the hand around its centre";
    constexpr double  middleValue   = 63.0;
    constexpr double  quarterTurn   = 42.333;   // 127 x 90 / 270: a quarter turn of the 270-degree sweep
    constexpr double  turnTolerance = 1.0;           // the knob snaps to whole steps
    constexpr float   noPressure    = 0.0f;
    constexpr float   noAngle       = 0.0f;
    constexpr int     singleClick   = 1;
    const juce::String labelsFit     = "every label fits its bounds whole (was: LEFT drawn as LEF)";
    const juce::String leftLabel     = "Left";
    const juce::String rightLabel    = "Right";
    const juce::String settingsLabel = "Settings";
    const juce::String autoFillLabel = "Auto Fill";

    const juce::String sectionLabel  = "Intro 1";
    const juce::String twoLineLabel  = "Sync\nStart";
    const juce::String knobLabel     = "Volume";
    const juce::String valueLine     = "an effects knob can show its value under the label (board .ering + label + value)";
    constexpr int boardKnobHeight    = 99;   // ring 72, gap 2, label 12, gap 2, value 11
    constexpr float boardValueTop    = 88.0f;
    constexpr float boardValueHeight = 11.0f;
    constexpr float unitScale        = 1.0f;

    constexpr float ownerScale   = 1.6f;
    constexpr int   oneLine      = 1;
    constexpr int   twoLines     = 2;
    constexpr double knobMinimum = 0.0;
    constexpr double knobMaximum = 127.0;
    constexpr double knobStep    = 1.0;
    constexpr float  degreesFromTop = 135.0f;
    constexpr float  tolerance   = 0.001f;
    constexpr float  halves      = 2.0f;
    constexpr float  halfPixel   = 0.5f;
}

class StageWidgetsTest : public juce::UnitTest
{
public:
    StageWidgetsTest() : juce::UnitTest (StageWidgetsTestValues::testName, StageWidgetsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StageWidgetsTestValues;

        beginTest (valueLine);
        {
            StageKnob knob { knobLabel, StageKnob::Size::effects };
            knob.setScale (unitScale);
            knob.setShowsValue (true);
            knob.setSize (knob.getPreferredWidth(), knob.getPreferredHeight());
            expectEquals (knob.getPreferredHeight(), boardKnobHeight);
            expectWithinAbsoluteError (knob.getValueBounds().getY(), boardValueTop, halfPixel);
            expectWithinAbsoluteError (knob.getValueBounds().getHeight(), boardValueHeight, halfPixel);
        }

        beginTest (ledState);
        {
            RubberButton button { sectionLabel, RubberButton::Style::key, RubberButton::LedColour::red };
            expect (! button.isLedOn());
            button.setLedOn (true);
            expect (button.isLedOn());
            button.setLedOn (false);
            expect (! button.isLedOn());
        }

        beginTest (styleKept);
        {
            RubberButton button { twoLineLabel, RubberButton::Style::key, RubberButton::LedColour::green };
            expect (button.getStyle() == RubberButton::Style::key);
            expect (button.getLedColour() == RubberButton::LedColour::green);
            expectEquals (button.getLabel(), twoLineLabel);
            expectEquals (button.getLabelLineCount(), twoLines);

            RubberButton oneLiner { sectionLabel, RubberButton::Style::key, RubberButton::LedColour::red };
            expectEquals (oneLiner.getLabelLineCount(), oneLine);
        }

        beginTest (keyGeometry);
        {
            RubberButton button { sectionLabel, RubberButton::Style::key, RubberButton::LedColour::red };
            button.setScale (ownerScale);
            button.setBounds (button.boundsForBoardBox ({ button.getPreferredWidth(), button.getPreferredHeight() }));   // placed like the panel does

            const auto cap = button.getCapBounds();
            expectWithinAbsoluteError (cap.getWidth(),  StageMetrics::buttonWidth  * ownerScale, tolerance);
            expectWithinAbsoluteError (cap.getHeight(), StageMetrics::buttonHeight * ownerScale, tolerance);
            const auto box = button.getBoardBox();
            expectWithinAbsoluteError (cap.getCentreX(), box.getCentreX(), tolerance);
            expectWithinAbsoluteError (cap.getY(), box.getY(), tolerance);

            const auto led = button.getLedBounds();
            expectWithinAbsoluteError (led.getWidth(), StageMetrics::ledDiameter * ownerScale, tolerance);
            expectWithinAbsoluteError (cap.getX() - led.getRight(), StageMetrics::ledToButtonGap * ownerScale, tolerance);
            expectWithinAbsoluteError (led.getCentreY(), cap.getCentreY(), tolerance);

            const auto label = button.getLabelBounds();
            expectWithinAbsoluteError (label.getY() - cap.getBottom(), StageMetrics::buttonToLabelGap * ownerScale, tolerance);
            expectWithinAbsoluteError (label.getCentreX(), cap.getCentreX(), tolerance);
            expectWithinAbsoluteError (label.getBottom(), box.getBottom(), tolerance);
        }

        beginTest (capOnlyClicks);
        {
            RubberButton button { sectionLabel, RubberButton::Style::key, RubberButton::LedColour::red };
            button.setScale (ownerScale);
            button.setBounds (button.boundsForBoardBox ({ button.getPreferredWidth(), button.getPreferredHeight() }));   // placed like the panel does

            const auto capCentre   = button.getCapBounds().getCentre().roundToInt();
            const auto labelCentre = button.getLabelBounds().getCentre().roundToInt();
            expect (button.hitTest (capCentre.x, capCentre.y));
            expect (! button.hitTest (labelCentre.x, labelCentre.y));
        }

        beginTest (knobRange);
        {
            StageKnob knob { knobLabel, StageKnob::Size::quick };
            expectWithinAbsoluteError (knob.getMinimum(), knobMinimum, static_cast<double> (tolerance));
            expectWithinAbsoluteError (knob.getMaximum(), knobMaximum, static_cast<double> (tolerance));
            expectWithinAbsoluteError (knob.getInterval(), knobStep, static_cast<double> (tolerance));

            const auto rotary = knob.getRotaryParameters();
            const auto fullTurn = juce::MathConstants<float>::twoPi;
            expectWithinAbsoluteError (rotary.startAngleRadians, fullTurn - juce::degreesToRadians (degreesFromTop), tolerance);
            expectWithinAbsoluteError (rotary.endAngleRadians,   fullTurn + juce::degreesToRadians (degreesFromTop), tolerance);
            expect (rotary.stopAtEnd);
            expectEquals (knob.getLabel(), knobLabel);
        }

        beginTest (labelsFit);
        {
            const std::vector<std::pair<juce::String, RubberButton::Style>> labelled {
                { leftLabel, RubberButton::Style::ledToggle }, { rightLabel, RubberButton::Style::ledToggle },
                { settingsLabel, RubberButton::Style::screen }, { autoFillLabel, RubberButton::Style::key } };
            for (const auto& [labelText, labelStyle] : labelled)
            {
                RubberButton button { labelText, labelStyle, RubberButton::LedColour::red };
                button.setScale (ownerScale);
                button.setBounds (button.boundsForBoardBox ({ button.getPreferredWidth(), button.getPreferredHeight() }));   // placed like the panel does
                const auto area = button.getLabelBounds();
                const auto upper = labelText.toUpperCase();
                juce::GlyphArrangement laidOut;   // what Graphics::drawText does: glyphs that do not fit are dropped
                laidOut.addCurtailedLineOfText (button.getLabelFont(), upper, area.getX(), area.getBottom(), area.getWidth(), false);
                expectEquals (laidOut.getNumGlyphs(), upper.length(), upper);
            }
        }

        beginTest (glowFits);
        {
            using Style = RubberButton::Style;
            using Side  = RubberButton::LabelSide;
            for (const auto& [buttonStyle, labelSide] : { std::pair { Style::key, Side::left }, std::pair { Style::screen, Side::left },
                                                           std::pair { Style::screen, Side::right }, std::pair { Style::ledToggle, Side::left } })
            {
                RubberButton button { sectionLabel, buttonStyle, RubberButton::LedColour::red };
                button.setLabelSide (labelSide);
                button.setScale (ownerScale);
                button.setBounds (button.boundsForBoardBox ({ button.getPreferredWidth(), button.getPreferredHeight() }));
                const auto glow = button.getLedBounds().expanded (StageMetrics::ledGlowRadius * ownerScale);
                expect (button.getLocalBounds().toFloat().contains (glow), glow.toString());
                expectEquals (button.getBoardBox().getWidth(), static_cast<float> (button.getPreferredWidth()));
            }
        }

        beginTest (glowLikeBoard);
        {
            RubberButton button { sectionLabel, RubberButton::Style::key, RubberButton::LedColour::red };
            button.setScale (ownerScale);
            button.setBounds (button.boundsForBoardBox ({ button.getPreferredWidth(), button.getPreferredHeight() }));
            button.setLedOn (true);
            juce::Image canvas (juce::Image::ARGB, button.getWidth(), button.getHeight(), true, juce::SoftwareImageType());
            {
                juce::Graphics g (canvas);
                g.fillAll (juce::Colours::black);
                button.paintEntireComponent (g, false);
            }
            const auto led    = button.getLedBounds();
            const auto radius = led.getWidth() / halves;
            const auto blur   = StageMetrics::ledGlowRadius * ownerScale;
            const auto sigma  = blur * cssBlurSigmaPerRadius;
            const auto column = static_cast<int> (led.getCentreX());
            for (auto row = static_cast<int> (led.getY() - firstSampleGap); row > led.getCentreY() - radius - blur + lastSampleGap; --row)
            {
                const auto gap = led.getCentreY() - (static_cast<float> (row) + halfPixel) - radius;   // past the LED edge, straight up
                if (gap < firstSampleGap)
                    continue;
                const auto expected = StagePalette::ledGlowAlpha * edgeCoverage
                                    * static_cast<float> (std::erfc (gap / (sigma * juce::MathConstants<float>::sqrt2)));
                const auto drawn = canvas.getPixelAt (column, row).getFloatRed();   // over black, red = the glow's alpha (the LED's red is full)
                expectWithinAbsoluteError (drawn, expected, glowTolerance, gapPrefix + juce::String (gap, gapDecimals));
            }
        }

        beginTest (widthLikeCss);
        {
            RubberButton button { leftLabel, RubberButton::Style::ledToggle, RubberButton::LedColour::red };
            button.setScale (ownerScale);
            const auto upper = leftLabel.toUpperCase();
            const auto untracked = juce::GlyphArrangement::getStringWidth (button.getLabelFont().withExtraKerningFactor (noTracking), upper);
            const auto tracking  = StageMetrics::silkLabelSize * ownerScale * StageMetrics::silkLabelSpacing;
            const auto cssLabel  = std::ceil (untracked + tracking * static_cast<float> (upper.length()));
            expectEquals (button.getPreferredWidth(),
                          juce::roundToInt ((StageMetrics::ledDiameter + StageMetrics::toggleGap) * ownerScale + cssLabel));
        }

        beginTest (handCursor);
        {
            for (const auto buttonStyle : { RubberButton::Style::key, RubberButton::Style::screen, RubberButton::Style::ledToggle })
            {
                RubberButton button { sectionLabel, buttonStyle, RubberButton::LedColour::red };
                expect (button.getMouseCursor() == juce::MouseCursor::PointingHandCursor);
            }
        }

        beginTest (knobTurns);
        {
            StageKnob knob { knobLabel, StageKnob::Size::quick };
            knob.setScale (ownerScale);
            knob.setSize (knob.getPreferredWidth(), knob.getPreferredHeight());
            const auto ring   = knob.getRingBounds();
            const auto top    = juce::Point<float> (ring.getCentreX(), ring.getY());
            const auto right  = juce::Point<float> (ring.getRight(), ring.getCentreY());
            const auto bottom = juce::Point<float> (ring.getCentreX(), ring.getBottom());
            const auto left   = juce::Point<float> (ring.getX(), ring.getCentreY());
            auto turn = [&knob] (juce::Point<float> from, juce::Point<float> to)
            {
                const auto source = juce::Desktop::getInstance().getMainMouseSource();
                const auto now = juce::Time::getCurrentTime();
                const juce::MouseEvent press (source, from, juce::ModifierKeys::leftButtonModifier, noPressure, noAngle, noAngle, noAngle, noAngle,
                                              &knob, &knob, now, from, now, singleClick, false);
                const juce::MouseEvent drag (source, to, juce::ModifierKeys::leftButtonModifier, noPressure, noAngle, noAngle, noAngle, noAngle,
                                             &knob, &knob, now, from, now, singleClick, true);
                knob.mouseDown (press);
                knob.mouseDrag (drag);
                knob.mouseUp (drag);
            };

            knob.setValue (middleValue, juce::dontSendNotification);
            turn (top, right);   // a quarter turn clockwise from the top
            expectWithinAbsoluteError (knob.getValue(), middleValue + quarterTurn, turnTolerance);

            knob.setValue (middleValue, juce::dontSendNotification);
            turn (bottom, left); // grabbed opposite its pointer: still a quarter turn clockwise, no jump
            expectWithinAbsoluteError (knob.getValue(), middleValue + quarterTurn, turnTolerance);

            knob.setValue (middleValue, juce::dontSendNotification);
            turn (right, top);   // anticlockwise lowers it
            expectWithinAbsoluteError (knob.getValue(), middleValue - quarterTurn, turnTolerance);

            knob.setValue (knobMaximum, juce::dontSendNotification);
            turn (top, right);   // stops at the end
            expectWithinAbsoluteError (knob.getValue(), knobMaximum, turnTolerance);
        }

        beginTest (knobDrag);
        {
            StageKnob knob { knobLabel, StageKnob::Size::effects };
            expect (knob.isRotary());
            expect (knob.getTextBoxPosition() == juce::Slider::NoTextBox);
            expect (knob.getSize() == StageKnob::Size::effects);
        }
    }
};
static StageWidgetsTest stageWidgetsTest;
