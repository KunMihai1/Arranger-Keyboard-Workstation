#include <JuceHeader.h>
#include "UI/Stage/StageWidgets.h"
#include "UI/Stage/StageMetrics.h"

namespace StageWidgetsTestValues
{
    const juce::String testName     = "StageWidgets";
    const juce::String testCategory = "Unit";

    const juce::String ledState      = "a rubber button's LED starts off and follows setLedOn";
    const juce::String styleKept     = "a rubber button keeps its style, LED colour and label";
    const juce::String capOnlyClicks = "only the cap of a key button takes clicks, not its label";
    const juce::String keyGeometry   = "a key button's cap is centred, LED left of it, label under it";
    const juce::String knobRange     = "a knob runs 0-127 in steps of 1 over -135..+135 degrees";
    const juce::String knobDrag      = "a knob turns with a vertical drag";

    const juce::String sectionLabel  = "Intro 1";
    const juce::String twoLineLabel  = "Sync\nStart";
    const juce::String knobLabel     = "Volume";

    constexpr float ownerScale   = 1.6f;
    constexpr int   oneLine      = 1;
    constexpr int   twoLines     = 2;
    constexpr double knobMinimum = 0.0;
    constexpr double knobMaximum = 127.0;
    constexpr double knobStep    = 1.0;
    constexpr float  degreesFromTop = 135.0f;
    constexpr float  tolerance   = 0.001f;
    constexpr float  halves      = 2.0f;
    constexpr float  topEdge     = 0.0f;
}

class StageWidgetsTest : public juce::UnitTest
{
public:
    StageWidgetsTest() : juce::UnitTest (StageWidgetsTestValues::testName, StageWidgetsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StageWidgetsTestValues;

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
            button.setSize (button.getPreferredWidth(), button.getPreferredHeight());

            const auto cap = button.getCapBounds();
            expectWithinAbsoluteError (cap.getWidth(),  StageMetrics::buttonWidth  * ownerScale, tolerance);
            expectWithinAbsoluteError (cap.getHeight(), StageMetrics::buttonHeight * ownerScale, tolerance);
            expectWithinAbsoluteError (cap.getCentreX(), button.getWidth() / halves, tolerance);
            expectWithinAbsoluteError (cap.getY(), topEdge, tolerance);

            const auto led = button.getLedBounds();
            expectWithinAbsoluteError (led.getWidth(), StageMetrics::ledDiameter * ownerScale, tolerance);
            expectWithinAbsoluteError (cap.getX() - led.getRight(), StageMetrics::ledToButtonGap * ownerScale, tolerance);
            expectWithinAbsoluteError (led.getCentreY(), cap.getCentreY(), tolerance);

            const auto label = button.getLabelBounds();
            expectWithinAbsoluteError (label.getY() - cap.getBottom(), StageMetrics::buttonToLabelGap * ownerScale, tolerance);
            expectWithinAbsoluteError (label.getCentreX(), cap.getCentreX(), tolerance);
            expectWithinAbsoluteError (label.getBottom(), static_cast<float> (button.getHeight()), tolerance);
        }

        beginTest (capOnlyClicks);
        {
            RubberButton button { sectionLabel, RubberButton::Style::key, RubberButton::LedColour::red };
            button.setScale (ownerScale);
            button.setSize (button.getPreferredWidth(), button.getPreferredHeight());

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

        beginTest (knobDrag);
        {
            StageKnob knob { knobLabel, StageKnob::Size::effects };
            expect (knob.getSliderStyle() == juce::Slider::RotaryVerticalDrag);
            expect (knob.getTextBoxPosition() == juce::Slider::NoTextBox);
            expect (knob.getSize() == StageKnob::Size::effects);
        }
    }
};
static StageWidgetsTest stageWidgetsTest;
