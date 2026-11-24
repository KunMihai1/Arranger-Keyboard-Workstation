#include <JuceHeader.h>
#include "UI/Stage/StageModels.h"

namespace StageModelsTestValues
{
    const juce::String styleViewsBack  = "Style page views: Exit / Esc from a sub-page goes back to the Style view";
    const juce::String styleViewsReset = "Style page views: closing the page forgets the sub-page";
    const juce::String testName     = "StageModels";
    const juce::String testCategory = "Unit";

    const juce::String tempoSteps      = "tempo steps by one and stops at the range ends";
    const juce::String tempoTyped      = "a typed tempo is clamped; empty or non-digits change nothing";
    const juce::String tempoWheel      = "the wheel moves by one, Shift by five";
    const juce::String tempoLocked     = "a locked tempo refuses every change";
    const juce::String tempoReset      = "right-click goes back to the style tempo";
    const juce::String tempoHoldDelays = "holding a tempo button repeats faster, down to a floor";
    const juce::String lampsFollowBeat = "the lamp of the current beat is lit, red on the downbeat";
    const juce::String lampsCount      = "the lamp count follows the time signature numerator";
    const juce::String lampsDark       = "lamps are dark when stopped or asleep";
    const juce::String messagesShow    = "a message shows upper case for 2.5 s";
    const juce::String messagesReplace = "a newer message replaces the older and gets its full time";
    const juce::String pagesToggle     = "a page button toggles its page and switches between pages";

    constexpr double styleTempo      = 120.0;
    constexpr double stepUpTempo     = 121.0;
    constexpr double fastestTempo    = 200.0;
    constexpr double slowestTempo    = 60.0;
    constexpr double typedTempo      = 140.0;
    constexpr double coarseWheelUp   = 125.0;
    constexpr double twoNotchesDown  = 118.0;
    constexpr double startTempo      = 100.0;
    constexpr int    stepUp          = 1;
    constexpr int    stepDown        = -1;
    constexpr int    oneNotch        = 1;
    constexpr int    twoNotchesBack  = -2;
    constexpr bool   coarse          = true;
    constexpr bool   fine            = false;

    const juce::String typedInRange = "140";
    const juce::String typedTooFast = "999";
    const juce::String typedTooSlow = "40";
    const juce::String typedEmpty   = "";
    const juce::String typedLetters = "abc";
    const juce::String typedMixed   = "12a";
    const juce::String typedPadded  = " 140 ";
    const juce::String typedHuge    = "99999999999999999999999";

    constexpr int firstHoldStep    = 0;
    constexpr int secondHoldStep   = 1;
    constexpr int sixthHoldStep    = 5;
    constexpr int twentiethHoldStep = 20;
    constexpr int firstHoldDelay   = 400;
    constexpr int secondHoldDelay  = 120;
    constexpr int sixthHoldDelay   = 80;
    constexpr int floorHoldDelay   = 30;

    constexpr int    fourBeats      = 4;
    constexpr int    threeBeats     = 3;
    constexpr int    sixBeats       = 6;
    constexpr double barStart       = 0.0;
    constexpr double thirdBeat      = 2.3;
    constexpr double nextBarSecond  = 5.5;
    constexpr int    thirdBeatIndex = 2;
    constexpr int    secondBeatIndex = 1;
    constexpr bool   playing        = true;
    constexpr bool   stopped        = false;
    constexpr bool   asleep         = true;
    constexpr bool   awake          = false;

    const juce::String recordingMessage      = "Recording started!";
    const juce::String recordingMessageUpper = "RECORDING STARTED!";
    const juce::String firstMessage          = "first";
    const juce::String secondMessage         = "second";
    const juce::String secondMessageUpper    = "SECOND";
    constexpr double postedAt        = 0.0;
    constexpr double shortlyAfter    = 100.0;
    constexpr double afterShowTime   = 2600.0;
    constexpr double secondPostedAt  = 2000.0;
    constexpr double afterFirstWouldEnd = 4000.0;
    constexpr double afterSecondEnds = 4600.0;

    constexpr double tolerance = 0.0001;

    const juce::String tempoShown   = "a tempo set elsewhere is shown even while locked, clamped, without counting as a change";
    const juce::String chordNames   = "chords read like the board: root, quality in lower case, bass after a slash";
    const juce::String barNumbers   = "bars count from 1 and follow the elapsed beats";
    constexpr double shownTempo     = 133.0;
    constexpr double tooFastShown   = 250.0;
    constexpr int    rootC          = 0;
    constexpr int    rootFSharp     = 6;
    constexpr int    bassE          = 4;
    const juce::String cMajor       = "C maj";
    const juce::String fSharpMinor7 = "F# min7";
    const juce::String cMajorOverE  = "C maj/E";
    constexpr double firstBarBeat   = 0.5;
    constexpr double thirdBarBeat   = 9.0;
    constexpr double beforeStart    = -1.0;
    constexpr int    firstBar       = 1;
    constexpr int    thirdBar       = 3;

    const juce::String longMessages  = "a message longer than the screen holds, scrolls to its end, holds, and stays long enough";
    const juce::String customTime    = "a message can be given a longer time than 2.5 s";
    constexpr float  noOverflow      = 0.0f;
    constexpr float  overflowPx      = 300.0f;
    constexpr float  pixelsPerMs     = 0.06f;
    constexpr double startOfMessage  = 0.0;
    constexpr double justAfterHold   = 1000.0;
    constexpr double longAfter       = 60000.0;
    constexpr double longShowTime    = 9000.0;
    constexpr double beforeLongEnds  = 8000.0;
    constexpr double afterLongEnds   = 9100.0;
    constexpr float  pixelTolerance  = 0.001f;
}

class StageModelsTest : public juce::UnitTest
{
public:
    StageModelsTest() : juce::UnitTest (StageModelsTestValues::testName, StageModelsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StageModelsTestValues;

        beginTest (tempoSteps);
        {
            TempoModel tempo { styleTempo };
            expect (tempo.step (stepUp));
            expectWithinAbsoluteError (tempo.value(), stepUpTempo, tolerance);

            TempoModel fastest { fastestTempo };
            expect (! fastest.step (stepUp));
            expectWithinAbsoluteError (fastest.value(), fastestTempo, tolerance);

            TempoModel slowest { slowestTempo };
            expect (! slowest.step (stepDown));
            expectWithinAbsoluteError (slowest.value(), slowestTempo, tolerance);
        }

        beginTest (tempoTyped);
        {
            TempoModel tempo { styleTempo };
            expect (tempo.applyTyped (typedInRange));
            expectWithinAbsoluteError (tempo.value(), typedTempo, tolerance);
            expect (tempo.applyTyped (typedTooFast));
            expectWithinAbsoluteError (tempo.value(), fastestTempo, tolerance);
            expect (tempo.applyTyped (typedTooSlow));
            expectWithinAbsoluteError (tempo.value(), slowestTempo, tolerance);
            expect (tempo.applyTyped (typedHuge));
            expectWithinAbsoluteError (tempo.value(), fastestTempo, tolerance);

            for (const auto& rejected : { typedEmpty, typedLetters, typedMixed })
            {
                TempoModel untouched { styleTempo };
                expect (! untouched.applyTyped (rejected), rejected);
                expectWithinAbsoluteError (untouched.value(), styleTempo, tolerance);
            }

            TempoModel padded { styleTempo };
            expect (padded.applyTyped (typedPadded));
            expectWithinAbsoluteError (padded.value(), typedTempo, tolerance);
        }

        beginTest (tempoWheel);
        {
            TempoModel fineWheel { styleTempo };
            expect (fineWheel.wheel (oneNotch, fine));
            expectWithinAbsoluteError (fineWheel.value(), stepUpTempo, tolerance);

            TempoModel coarseWheel { styleTempo };
            expect (coarseWheel.wheel (oneNotch, coarse));
            expectWithinAbsoluteError (coarseWheel.value(), coarseWheelUp, tolerance);

            TempoModel backWheel { styleTempo };
            expect (backWheel.wheel (twoNotchesBack, fine));
            expectWithinAbsoluteError (backWheel.value(), twoNotchesDown, tolerance);
        }

        beginTest (tempoLocked);
        {
            TempoModel tempo { styleTempo };
            tempo.setLocked (true);
            expect (tempo.isLocked());
            expect (! tempo.step (stepUp));
            expect (! tempo.applyTyped (typedInRange));
            expect (! tempo.wheel (oneNotch, coarse));
            expect (! tempo.resetToStyle());
            expectWithinAbsoluteError (tempo.value(), styleTempo, tolerance);

            tempo.setLocked (false);
            expect (! tempo.isLocked());
            expect (tempo.step (stepUp));
        }

        beginTest (tempoReset);
        {
            TempoModel tempo { styleTempo };
            expect (tempo.applyTyped (typedInRange));
            expect (tempo.resetToStyle());
            expectWithinAbsoluteError (tempo.value(), styleTempo, tolerance);
            expect (! tempo.resetToStyle());   // already there: nothing changes

            tempo.setStyleTempo (startTempo);
            expect (tempo.resetToStyle());
            expectWithinAbsoluteError (tempo.value(), startTempo, tolerance);
        }

        beginTest (tempoHoldDelays);
        {
            expectEquals (TempoModel::holdDelayMs (firstHoldStep), firstHoldDelay);
            expectEquals (TempoModel::holdDelayMs (secondHoldStep), secondHoldDelay);
            expectEquals (TempoModel::holdDelayMs (sixthHoldStep), sixthHoldDelay);
            expectEquals (TempoModel::holdDelayMs (twentiethHoldStep), floorHoldDelay);
        }

        beginTest (lampsFollowBeat);
        {
            const auto downbeat = BeatLampModel::lamps (fourBeats, barStart, playing, awake);
            expect (downbeat == std::vector<LampState> { LampState::downbeat, LampState::off, LampState::off, LampState::off });

            const auto third = BeatLampModel::lamps (fourBeats, thirdBeat, playing, awake);
            for (int lampIndex {}; lampIndex < fourBeats; ++lampIndex)
                expect (third[static_cast<size_t> (lampIndex)] == (lampIndex == thirdBeatIndex ? LampState::beat : LampState::off));

            const auto nextBar = BeatLampModel::lamps (fourBeats, nextBarSecond, playing, awake);
            expect (nextBar[static_cast<size_t> (secondBeatIndex)] == LampState::beat);
        }

        beginTest (lampsCount);
        {
            expectEquals (static_cast<int> (BeatLampModel::lamps (threeBeats, barStart, playing, awake).size()), threeBeats);
            expectEquals (static_cast<int> (BeatLampModel::lamps (sixBeats, barStart, playing, awake).size()), sixBeats);
            expect (BeatLampModel::lamps (int {}, barStart, playing, awake).empty());
        }

        beginTest (lampsDark);
        {
            for (const auto lamp : BeatLampModel::lamps (fourBeats, thirdBeat, stopped, awake))
                expect (lamp == LampState::off);
            for (const auto lamp : BeatLampModel::lamps (fourBeats, thirdBeat, playing, asleep))
                expect (lamp == LampState::off);
        }

        beginTest (messagesShow);
        {
            ScreenMessages messages;
            expect (messages.current (postedAt).isEmpty());
            messages.post (recordingMessage, postedAt);
            expectEquals (messages.current (shortlyAfter), recordingMessageUpper);
            expect (messages.current (afterShowTime).isEmpty());
        }

        beginTest (messagesReplace);
        {
            ScreenMessages messages;
            messages.post (firstMessage, postedAt);
            messages.post (secondMessage, secondPostedAt);
            expectEquals (messages.current (afterFirstWouldEnd), secondMessageUpper);
            expect (messages.current (afterSecondEnds).isEmpty());
        }

        beginTest (tempoShown);
        {
            TempoModel tempo { styleTempo };
            tempo.setLocked (true);
            tempo.showValue (shownTempo);
            expectWithinAbsoluteError (tempo.value(), shownTempo, tolerance);
            tempo.showValue (tooFastShown);
            expectWithinAbsoluteError (tempo.value(), fastestTempo, tolerance);
        }

        beginTest (chordNames);
        {
            ArrangerChord chord;
            expect (StageText::chordName (chord).isEmpty());

            chord.root = rootC;
            chord.quality = ChordQuality::Maj;
            expectEquals (StageText::chordName (chord), cMajor);

            chord.bassNote = bassE;
            expectEquals (StageText::chordName (chord), cMajorOverE);

            ArrangerChord minorSeventh;
            minorSeventh.root = rootFSharp;
            minorSeventh.quality = ChordQuality::Min7;
            minorSeventh.bassNote = rootFSharp;   // bass = root: no slash
            expectEquals (StageText::chordName (minorSeventh), fSharpMinor7);
        }

        beginTest (barNumbers);
        {
            expectEquals (StageText::barNumber (firstBarBeat, fourBeats), firstBar);
            expectEquals (StageText::barNumber (thirdBarBeat, fourBeats), thirdBar);
            expectEquals (StageText::barNumber (beforeStart, fourBeats), firstBar);
            expectEquals (StageText::barNumber (thirdBarBeat, int {}), firstBar);
        }

        beginTest (longMessages);
        {
            expectWithinAbsoluteError (StageText::marqueeDurationMs (noOverflow, pixelsPerMs), ScreenMessages::showTimeMs, tolerance);
            const auto duration = StageText::marqueeDurationMs (overflowPx, pixelsPerMs);
            expectWithinAbsoluteError (duration, StageText::marqueeHoldMs + overflowPx / pixelsPerMs + StageText::marqueeHoldMs, tolerance);
            expectGreaterThan (duration, ScreenMessages::showTimeMs);
            expectWithinAbsoluteError (StageText::marqueeOffset (overflowPx, pixelsPerMs, startOfMessage), noOverflow, pixelTolerance);
            expectWithinAbsoluteError (StageText::marqueeOffset (overflowPx, pixelsPerMs, StageText::marqueeHoldMs), noOverflow, pixelTolerance);
            expectWithinAbsoluteError (StageText::marqueeOffset (overflowPx, pixelsPerMs, justAfterHold),
                                       static_cast<float> ((justAfterHold - StageText::marqueeHoldMs) * pixelsPerMs), pixelTolerance);
            expectWithinAbsoluteError (StageText::marqueeOffset (overflowPx, pixelsPerMs, longAfter), overflowPx, pixelTolerance);
            expectWithinAbsoluteError (StageText::marqueeOffset (noOverflow, pixelsPerMs, longAfter), noOverflow, pixelTolerance);
        }

        beginTest (customTime);
        {
            ScreenMessages messages;
            messages.post (recordingMessage, postedAt, longShowTime);
            expectEquals (messages.current (beforeLongEnds), recordingMessageUpper);
            expect (messages.current (afterLongEnds).isEmpty());
            expectWithinAbsoluteError (messages.elapsedMs (beforeLongEnds), beforeLongEnds, tolerance);
        }

        beginTest (pagesToggle);
        {
            PageRouter router;
            expect (router.open() == StagePage::none);
            expect (router.toggle (StagePage::effects) == StagePage::effects);
            expect (router.toggle (StagePage::effects) == StagePage::none);
            expect (router.toggle (StagePage::effects) == StagePage::effects);
            expect (router.toggle (StagePage::settings) == StagePage::settings);
            expect (router.open() == StagePage::settings);
            expect (router.close() == StagePage::none);
            expect (router.open() == StagePage::none);
            expect (router.toggle (StagePage::none) == StagePage::none);
        }

        beginTest (styleViewsBack);
        {
            StyleViewRouter views;
            expect (views.current() == StyleView::style);
            expect (views.show (StyleView::tracks) == StyleView::tracks);
            expect (views.back() == StyleView::style);
            expect (views.back() == StyleView::style);
            views.show (StyleView::notes);
            expect (views.show (StyleView::settings) == StyleView::settings);
            expect (views.back() == StyleView::style);
        }

        beginTest (styleViewsReset);
        {
            StyleViewRouter views;
            views.show (StyleView::notes);
            views.reset();
            expect (views.current() == StyleView::style);
        }
    }
};
static StageModelsTest stageModelsTest;
