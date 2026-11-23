#include <JuceHeader.h>
#include "UI/Stage/StageScreen.h"
#include "Common/AppInfo.h"

namespace StageScreenTestValues
{
    const juce::String testName     = "StageScreen";
    const juce::String testCategory = "Unit";

    const juce::String topLineDefaults = "the top line says STYLE, or the app name while asleep";
    const juce::String messageShown    = "a posted message replaces the top line, upper case";
    const juce::String wheelOnTempo    = "the wheel over the tempo changes it, elsewhere it does nothing";
    const juce::String rightClickReset = "right-click on the tempo goes back to the style tempo";
    const juce::String lockedTempo     = "a locked tempo does not change and says why";
    const juce::String typedTempo      = "clicking the tempo opens an editor; Enter applies, Esc cancels";
    const juce::String tempoButtons    = "the Tempo -/+ buttons step the tempo through the screen";

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
