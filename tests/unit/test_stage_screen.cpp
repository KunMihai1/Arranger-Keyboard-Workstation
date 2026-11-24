#include <JuceHeader.h>
#include "UI/Stage/StageScreen.h"
#include "Common/AppInfo.h"

namespace StageScreenTestValues
{
    const juce::String chordAtRowStart = "with no variation shown, the chord starts at the row's left edge (owner: it was indented)";
    const juce::String chordText       = "C maj";
    const juce::String testName     = "StageScreen";
    const juce::String testCategory = "Unit";

    const juce::String topLineDefaults = "the top line says STYLE, or the app name while asleep";
    const juce::String messageShown    = "a posted message replaces the top line, upper case";
    const juce::String wheelOnTempo    = "the wheel over the tempo changes it, elsewhere it does nothing";
    const juce::String rightClickReset = "right-click on the tempo goes back to the style tempo";
    const juce::String lockedTempo     = "a locked tempo does not change and says why";
    const juce::String typedTempo      = "clicking the tempo opens an editor; Enter applies, Esc cancels";
    const juce::String tempoButtons    = "the Tempo -/+ buttons step the tempo through the screen";
    const juce::String tempoFromStyle  = "a tempo from the open style is shown, and becomes the right-click tempo";
    const juce::String longMessageStays = "a message too long for the screen stays until it has scrolled through";
    const juce::String dragReportsOnce = "a tempo drag shows every step but reports once, on release (review: every step re-scaled all tracks)";
    constexpr int    dragSteps        = 5;
    constexpr double draggedTempo     = 125.0;
    const juce::String messageBesideMode = "a message that fits replaces only STYLE; PLAY and the time signature stay (board); a long one takes the row";
    const juce::String playMode         = "PLAY";
    const juce::String fourFour         = "4/4";
    const juce::String editorLooksLikeBoard = "the tempo editor is light text on a rounded dark field, as on the board (was: an inverted light block)";
    constexpr float darkLuminance    = 0.35f;   // below this a pixel counts as the dark field
    constexpr float mostlyDark       = 0.6f;    // share of the field that must stay dark around the digits
    const juce::String longMessage = "No SFZ loaded for Grand Piano (right hand), Warm Strings (left hand) and 3 more in style Style 1";
    constexpr double messageStart   = 0.0;
    constexpr double afterShortTime = 3000.0;
    constexpr double styleSliderTempo  = 133.0;
    constexpr double otherStyleTempo   = 110.0;

    const juce::String styleTopLine    = "STYLE";
    const juce::String recordingMessage = "Recording started!";
    const juce::String recordingUpper  = "RECORDING STARTED!";
    const juce::String backToStyle     = "TEMPO BACK TO STYLE: 120";
    const juce::String lockedMessage   = "TEMPO LOCKED WHILE PLAYING";
    const juce::String typedText       = "140";
    const juce::String styleName       = "Style 1";

    constexpr double styleTempo  = 120.0;
    constexpr double fasterTempo = 121.0;
    constexpr double slowerTempo = 119.0;
    constexpr double typedValue  = 140.0;
    constexpr float  ownerScale  = 1.6f;
    constexpr float  wheelUp     = 0.25f;
    constexpr float  noPressure  = 0.0f;
    constexpr float  noAngle     = 0.0f;
    constexpr int    singleClick = 1;
    constexpr int    stepUp      = 1;
    constexpr int    stepDown    = -1;
    constexpr double tolerance   = 0.0001;
    constexpr double noReport    = -1.0;
    constexpr float  halfStep    = 1.0f;   // px past the last step, clear of rounding
}

class StageScreenTest : public juce::UnitTest
{
    static juce::MouseEvent eventAt (StageScreen& screen, juce::Point<float> position, juce::ModifierKeys modifiers)
    {
        using namespace StageScreenTestValues;
        const auto now = juce::Time::getCurrentTime();
        return { juce::Desktop::getInstance().getMainMouseSource(), position, modifiers, noPressure, noAngle, noAngle,
                 noAngle, noAngle, &screen, &screen, now, position, now, singleClick, false };
    }

    static juce::MouseEvent dragEvent (StageScreen& screen, juce::Point<float> from, juce::Point<float> to)
    {
        using namespace StageScreenTestValues;
        const auto now = juce::Time::getCurrentTime();
        return { juce::Desktop::getInstance().getMainMouseSource(), to, juce::ModifierKeys::leftButtonModifier, noPressure, noAngle, noAngle,
                 noAngle, noAngle, &screen, &screen, now, from, now, singleClick, true };
    }

    static juce::MouseWheelDetails wheelDetails (float deltaY)
    {
        juce::MouseWheelDetails details {};
        details.deltaY = deltaY;
        return details;
    }

    static StageScreen::Content awakeContent()
    {
        StageScreen::Content content;
        content.styleName = StageScreenTestValues::styleName;
        return content;
    }

    struct ScreenUnderTest
    {
        StageScreen screen { StageScreenTestValues::styleTempo };
        std::vector<double> reported;

        ScreenUnderTest()
        {
            screen.setScale (StageScreenTestValues::ownerScale);
            screen.setSize (screen.getPreferredWidth(), screen.getPreferredHeight());
            screen.setContent (awakeContent());
            screen.onTempoChanged = [this] (double tempo) { reported.push_back (tempo); };
        }

        juce::Point<float> tempoCentre() const { return screen.getTempoBounds().getCentre(); }
        double lastReported() const { return reported.empty() ? StageScreenTestValues::noReport : reported.back(); }
    };

public:
    StageScreenTest() : juce::UnitTest (StageScreenTestValues::testName, StageScreenTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StageScreenTestValues;

        beginTest (topLineDefaults);
        {
            ScreenUnderTest under;
            expectEquals (under.screen.getTopLineText(), styleTopLine);

            auto asleep = awakeContent();
            asleep.asleep = true;
            under.screen.setContent (asleep);
            expectEquals (under.screen.getTopLineText(), juce::String (AppInfo::appName).toUpperCase());
        }

        beginTest (messageShown);
        {
            ScreenUnderTest under;
            under.screen.postMessage (recordingMessage);
            expectEquals (under.screen.getTopLineText(), recordingUpper);
        }

        beginTest (wheelOnTempo);
        {
            ScreenUnderTest under;
            expect (! under.screen.getTempoBounds().isEmpty());
            under.screen.mouseWheelMove (eventAt (under.screen, under.tempoCentre(), {}), wheelDetails (wheelUp));
            expectEquals (static_cast<int> (under.reported.size()), singleClick);
            expectWithinAbsoluteError (under.lastReported(), fasterTempo, tolerance);

            under.screen.mouseWheelMove (eventAt (under.screen, {}, {}), wheelDetails (wheelUp));
            expectEquals (static_cast<int> (under.reported.size()), singleClick);
        }

        beginTest (rightClickReset);
        {
            ScreenUnderTest under;
            under.screen.mouseWheelMove (eventAt (under.screen, under.tempoCentre(), {}), wheelDetails (wheelUp));
            under.screen.mouseDown (eventAt (under.screen, under.tempoCentre(), juce::ModifierKeys::rightButtonModifier));
            expectWithinAbsoluteError (under.lastReported(), styleTempo, tolerance);
            expectEquals (under.screen.getTopLineText(), backToStyle);
        }

        beginTest (lockedTempo);
        {
            ScreenUnderTest under;
            under.screen.setTempoLocked (true);
            under.screen.mouseWheelMove (eventAt (under.screen, under.tempoCentre(), {}), wheelDetails (wheelUp));
            expect (under.reported.empty());
            expectEquals (under.screen.getTopLineText(), lockedMessage);

            under.screen.mouseDown (eventAt (under.screen, under.tempoCentre(), juce::ModifierKeys::leftButtonModifier));
            under.screen.mouseUp (eventAt (under.screen, under.tempoCentre(), juce::ModifierKeys::leftButtonModifier));
            expect (! under.screen.isEditingTempo());
        }

        beginTest (typedTempo);
        {
            ScreenUnderTest under;
            const auto click = eventAt (under.screen, under.tempoCentre(), juce::ModifierKeys::leftButtonModifier);
            under.screen.mouseDown (click);
            under.screen.mouseUp (click);
            expect (under.screen.isEditingTempo());

            auto& editor = under.screen.getTempoEditor();
            editor.setText (typedText);
            if (editor.onReturnKey != nullptr)
                editor.onReturnKey();
            expect (! under.screen.isEditingTempo());
            expectWithinAbsoluteError (under.lastReported(), typedValue, tolerance);

            under.screen.mouseDown (click);
            under.screen.mouseUp (click);
            editor.setText (styleName);   // Esc must drop whatever was typed
            if (editor.onEscapeKey != nullptr)
                editor.onEscapeKey();
            expect (! under.screen.isEditingTempo());
            expectEquals (static_cast<int> (under.reported.size()), singleClick);
            expectWithinAbsoluteError (under.screen.tempo().value(), typedValue, tolerance);
        }

        beginTest (chordAtRowStart);
        {
            // The same text as the chord (no variation) and as the variation (no chord) lands on the same pixels.
            auto render = [] (const juce::String& variation, const juce::String& chord)
            {
                ScreenUnderTest under;
                auto content = awakeContent();
                content.variation = variation;
                content.chord = chord;
                under.screen.setContent (content);
                juce::Image canvas (juce::Image::ARGB, under.screen.getWidth(), under.screen.getHeight(), true, juce::SoftwareImageType());
                juce::Graphics g (canvas);
                under.screen.paintEntireComponent (g, false);
                return canvas;
            };
            const auto asChord     = render ({}, chordText);
            const auto asVariation = render (chordText, {});
            int differentPixels {};
            for (int row {}; row < asChord.getHeight(); ++row)
                for (int column {}; column < asChord.getWidth(); ++column)
                    if (asChord.getPixelAt (column, row) != asVariation.getPixelAt (column, row))
                        ++differentPixels;
            expectEquals (differentPixels, int {});
        }

        beginTest (editorLooksLikeBoard);
        {
            ScreenUnderTest under;
            const auto click = eventAt (under.screen, under.tempoCentre(), juce::ModifierKeys::leftButtonModifier);
            under.screen.mouseDown (click);
            under.screen.mouseUp (click);
            auto& editor = under.screen.getTempoEditor();
            editor.selectAll();   // as when it opens: typing replaces the tempo

            juce::Image canvas (juce::Image::ARGB, under.screen.getWidth(), under.screen.getHeight(), true, juce::SoftwareImageType());
            {
                juce::Graphics g (canvas);
                under.screen.paintEntireComponent (g, false);
            }
            const auto field = editor.getBounds();
            int darkPixels {};
            for (int row = field.getY(); row < field.getBottom(); ++row)
                for (int column = field.getX(); column < field.getRight(); ++column)
                    if (canvas.getPixelAt (column, row).getPerceivedBrightness() < darkLuminance)
                        ++darkPixels;
            const auto darkShare = static_cast<float> (darkPixels) / static_cast<float> (field.getWidth() * field.getHeight());
            expect (darkShare > mostlyDark, juce::String (darkShare));

            const auto corner = canvas.getPixelAt (field.getX(), field.getY());   // rounded: the LCD shows in the corner
            expect (corner.getPerceivedBrightness() > darkLuminance, corner.toDisplayString (true));
        }

        beginTest (dragReportsOnce);
        {
            ScreenUnderTest under;
            const auto start = under.tempoCentre();
            const auto up    = start.translated (float {}, -StageMetrics::tempoDragStep * ownerScale * dragSteps - halfStep);
            under.screen.mouseDown (eventAt (under.screen, start, juce::ModifierKeys::leftButtonModifier));
            under.screen.mouseDrag (dragEvent (under.screen, start, up));
            expectWithinAbsoluteError (under.screen.tempo().value(), draggedTempo, tolerance);
            expect (under.reported.empty());
            under.screen.showTempo (styleTempo, styleTempo);   // the app's refresh, before the style has heard
            expectWithinAbsoluteError (under.screen.tempo().value(), draggedTempo, tolerance);
            under.screen.mouseUp (dragEvent (under.screen, start, up));
            expectEquals (static_cast<int> (under.reported.size()), singleClick);
            expectWithinAbsoluteError (under.lastReported(), draggedTempo, tolerance);
            expect (! under.screen.isEditingTempo());
        }

        beginTest (messageBesideMode);
        {
            ScreenUnderTest under;
            auto content = awakeContent();
            content.mode = playMode;
            content.timeSignature = fourFour;
            under.screen.setContent (content);

            under.screen.postMessage (recordingMessage);
            expect (! under.screen.isMessageShownAlone());

            under.screen.postMessage (longMessage);
            expect (under.screen.isMessageShownAlone());
        }

        beginTest (longMessageStays);
        {
            ScreenUnderTest under;
            double clockMs = messageStart;
            under.screen.setClock ([&clockMs] { return clockMs; });
            under.screen.postMessage (longMessage);
            clockMs = afterShortTime;
            expectEquals (under.screen.getTopLineText(), longMessage.toUpperCase());

            clockMs = messageStart;
            under.screen.postMessage (recordingMessage);
            clockMs = afterShortTime;
            expectEquals (under.screen.getTopLineText(), styleTopLine);
        }

        beginTest (tempoFromStyle);
        {
            ScreenUnderTest under;
            under.screen.setTempoLocked (true);
            under.screen.showTempo (styleSliderTempo, otherStyleTempo);
            expectWithinAbsoluteError (under.screen.tempo().value(), styleSliderTempo, tolerance);
            expect (under.reported.empty());   // shown, not a user change
            under.screen.setTempoLocked (false);
            under.screen.mouseDown (eventAt (under.screen, under.tempoCentre(), juce::ModifierKeys::rightButtonModifier));
            expectWithinAbsoluteError (under.lastReported(), otherStyleTempo, tolerance);
        }

        beginTest (tempoButtons);
        {
            ScreenUnderTest under;
            expect (under.screen.stepTempo (stepUp));
            expectWithinAbsoluteError (under.lastReported(), fasterTempo, tolerance);
            expect (under.screen.stepTempo (stepDown));
            expect (under.screen.stepTempo (stepDown));
            expectWithinAbsoluteError (under.lastReported(), slowerTempo, tolerance);
        }
    }
};
static StageScreenTest stageScreenTest;
