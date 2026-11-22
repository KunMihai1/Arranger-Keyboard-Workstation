#include <JuceHeader.h>
#include "CurrentStyleComponent.h"
#include "CustomBeatBar.h"

namespace CurrentStyleStartTestValues
{
    const juce::String testName     = "CurrentStyleComponent (Start with nothing to play)";
    const juce::String testCategory = "Unit";

    const juce::String soloStartOnNoneTrack = "Solo Start on a \"None\" track plays nothing and does not crash";
    const juce::String noTracksStaysStopped = "Start with no tracks leaves the panel stopped (";
    const juce::String arrangerModeLabel    = "arranger mode";
    const juce::String trackPlayerModeLabel = "track-player mode";
    const juce::String closingBracket       = ")";

    const juce::String childrenNotFound    = "panel children not found";
    const juce::String soloStillPlaying    = "beat bar shows 'playing' after a Solo Start on a track with nothing to play";
    const juce::String sliderStartsEnabled = "BPM slider should start enabled";
    const juce::String beatStillPlaying    = "beat bar still shows 'playing' after a Start that had nothing to play";
    const juce::String sliderLocked        = "BPM slider locked after a Start that had nothing to play";

    const juce::String styleName   = "Style 1";
    const juce::String startButton = "Start";
    constexpr int   panelWidth   = 700;
    constexpr int   panelHeight  = 120;
    constexpr double minTempo    = 60.0;    // the BPM slider's range identifies it
    constexpr double maxTempo    = 200.0;
    constexpr float noOrientation = 0.0f;
    constexpr float noRotation    = 0.0f;
    constexpr float noTilt        = 0.0f;
    constexpr int   singleClick   = 1;
}

/*
    Start with nothing to play must leave the style panel stopped.

    Bug (2026-11-21): pressing Start on a style whose 8 tracks are all "None" showed
    "There are no tracks to play", but the beat light stayed red (the panel still believed it was
    playing) and, in arranger mode, the BPM slider stayed locked until Stop was pressed.

    The test drives the real component through its real children (Start button, beat bar, BPM
    slider) - no test-only hooks in production code.
*/
class CurrentStyleStartTest : public juce::UnitTest
{
public:
    CurrentStyleStartTest() : juce::UnitTest (CurrentStyleStartTestValues::testName, CurrentStyleStartTestValues::testCategory) {}

    void runTest() override
    {
        using namespace CurrentStyleStartTestValues;
        checkStartWithNoTracks (false);
        checkStartWithNoTracks (true);
        checkSoloStartOnNoneTrack();
    }

private:
    template <typename T, typename Pred>
    static T* findChild (juce::Component& parent, Pred pred)
    {
        for (auto* child : parent.getChildren())
            if (auto* typed = dynamic_cast<T*> (child))
                if (pred (*typed))
                    return typed;
        return nullptr;
    }

    template <typename T>
    static T* findDescendant (juce::Component& parent)
    {
        for (auto* child : parent.getChildren())
        {
            if (auto* typed = dynamic_cast<T*> (child))
                return typed;
            if (auto* found = findDescendant<T> (*child))
                return found;
        }
        return nullptr;
    }

    /*  Bug (2026-11-22, reproduced live as an access violation): Solo mode, click a "None" track,
        Start. Loading a style runs applyBPMchangeBeforePlayback(), whose map operator[] inserts a
        null entry for every "None" track; the Solo path then dereferenced that entry. It must
        behave like "All tracks" instead: nothing to play, panel stays stopped. */
    void checkSoloStartOnNoneTrack()
    {
        using namespace CurrentStyleStartTestValues;
        beginTest (soloStartOnNoneTrack);

        std::unordered_map<juce::Uuid, TrackEntry*> trackEntries;
        CurrentStyleComponent panel (styleName, trackEntries, {}, {});
        panel.setMidiInjectCallback ([] (const juce::MidiMessage&) {});
        panel.setSize (panelWidth, panelHeight);

        panel.applyBPMchangeBeforePlayback (ArrangerStyleDefaults::tempoBpm);   // what loading a style does

        auto* mode  = findChild<juce::ComboBox> (panel, [] (juce::ComboBox&) { return true; });
        auto* track = findDescendant<Track> (panel);
        auto* start = findChild<juce::TextButton> (panel, [] (juce::TextButton& button) { return button.getButtonText() == startButton; });
        auto* beat  = findChild<BeatBar> (panel, [] (BeatBar&) { return true; });

        expect (mode != nullptr && track != nullptr && start != nullptr && beat != nullptr, childrenNotFound);
        if (mode == nullptr || track == nullptr || start == nullptr || beat == nullptr)
            return;

        mode->setSelectedId (PlayModeIds::soloTrack, juce::sendNotificationSync);   // "Solo track (last selected)"

        // A click on the track selects it (the panel listens to its children's mouse-downs).
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent click (juce::Desktop::getInstance().getMainMouseSource(), {}, juce::ModifierKeys::leftButtonModifier,
                                      juce::MouseInputSource::defaultPressure, noOrientation, noRotation, noTilt, noTilt,
                                      track, track, now, {}, now, singleClick, false);
        static_cast<juce::Component&> (panel).mouseDown (click);   // the override is private; dispatch as JUCE does

        start->onClick();

        expect (! (beat->isPlayingCheck && beat->isPlayingCheck()),
                soloStillPlaying);

        closeAlerts();
    }

    static void closeAlerts()
    {
        // The "no tracks" path opens an async AlertWindow; deleting a modal component is safe
        // (ModalComponentManager cancels its entry), and keeps the runner from leaving windows up.
        while (auto* modal = juce::Component::getCurrentlyModalComponent())
            delete modal;
    }

    void checkStartWithNoTracks (bool arrangerMode)
    {
        using namespace CurrentStyleStartTestValues;
        beginTest (noTracksStaysStopped + (arrangerMode ? arrangerModeLabel : trackPlayerModeLabel) + closingBracket);

        std::unordered_map<juce::Uuid, TrackEntry*> noTrackEntries;
        CurrentStyleComponent panel (styleName, noTrackEntries, {}, {});
        panel.setMidiInjectCallback ([] (const juce::MidiMessage&) {});   // audio output present, as in the app
        panel.setArrangerModeEnabled (arrangerMode);
        panel.setSize (panelWidth, panelHeight);                                          // resized() labels the buttons

        auto* start = findChild<juce::TextButton> (panel, [] (juce::TextButton& button) { return button.getButtonText() == startButton; });
        auto* beat  = findChild<BeatBar> (panel, [] (BeatBar&) { return true; });
        auto* tempo = findChild<juce::Slider> (panel, [] (juce::Slider& slider) { return slider.getMinimum() == minTempo && slider.getMaximum() == maxTempo; });

        expect (start != nullptr && beat != nullptr && tempo != nullptr, childrenNotFound);
        if (start == nullptr || beat == nullptr || tempo == nullptr)
            return;

        expect (tempo->isEnabled(), sliderStartsEnabled);

        start->onClick();   // what a click runs; triggerClick() would only post it

        expect (! (beat->isPlayingCheck && beat->isPlayingCheck()),
                beatStillPlaying);
        expect (tempo->isEnabled(),
                sliderLocked);

        closeAlerts();
    }
};

static CurrentStyleStartTest currentStyleStartTest;
