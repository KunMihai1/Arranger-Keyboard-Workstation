#include <JuceHeader.h>
#include "CurrentStyleComponent.h"
#include "CustomBeatBar.h"

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
    CurrentStyleStartTest() : juce::UnitTest ("CurrentStyleComponent (Start with nothing to play)", "Unit") {}

    void runTest() override
    {
        checkStartWithNoTracks (false);
        checkStartWithNoTracks (true);
        checkSoloStartOnNoneTrack();
    }

private:
    template <typename T, typename Pred>
    static T* findChild (juce::Component& parent, Pred pred)
    {
        for (auto* c : parent.getChildren())
            if (auto* t = dynamic_cast<T*> (c))
                if (pred (*t))
                    return t;
        return nullptr;
    }

    template <typename T>
    static T* findDescendant (juce::Component& parent)
    {
        for (auto* c : parent.getChildren())
        {
            if (auto* t = dynamic_cast<T*> (c))
                return t;
            if (auto* t = findDescendant<T> (*c))
                return t;
        }
        return nullptr;
    }

    /*  Bug (2026-11-22, reproduced live as an access violation): Solo mode, click a "None" track,
        Start. Loading a style runs applyBPMchangeBeforePlayback(), whose map operator[] inserts a
        null entry for every "None" track; the Solo path then dereferenced that entry. It must
        behave like "All tracks" instead: nothing to play, panel stays stopped. */
    void checkSoloStartOnNoneTrack()
    {
        beginTest ("Solo Start on a \"None\" track plays nothing and does not crash");

        std::unordered_map<juce::Uuid, TrackEntry*> trackEntries;
        CurrentStyleComponent panel ("Style 1", trackEntries, {}, {});
        panel.setMidiInjectCallback ([] (const juce::MidiMessage&) {});
        panel.setSize (700, 120);

        panel.applyBPMchangeBeforePlayback (120.0);   // what loading a style does

        auto* mode  = findChild<juce::ComboBox> (panel, [] (juce::ComboBox&) { return true; });
        auto* track = findDescendant<Track> (panel);
        auto* start = findChild<juce::TextButton> (panel, [] (juce::TextButton& b) { return b.getButtonText() == "Start"; });
        auto* beat  = findChild<BeatBar> (panel, [] (BeatBar&) { return true; });

        expect (mode != nullptr && track != nullptr && start != nullptr && beat != nullptr, "panel children not found");
        if (mode == nullptr || track == nullptr || start == nullptr || beat == nullptr)
            return;

        mode->setSelectedId (2, juce::sendNotificationSync);   // "Solo track (last selected)"

        // A click on the track selects it (the panel listens to its children's mouse-downs).
        const auto now = juce::Time::getCurrentTime();
        const juce::MouseEvent click (juce::Desktop::getInstance().getMainMouseSource(), {}, juce::ModifierKeys::leftButtonModifier,
                                      juce::MouseInputSource::defaultPressure, 0.0f, 0.0f, 0.0f, 0.0f,
                                      track, track, now, {}, now, 1, false);
        static_cast<juce::Component&> (panel).mouseDown (click);   // the override is private; dispatch as JUCE does

        start->onClick();

        expect (! (beat->isPlayingCheck && beat->isPlayingCheck()),
                "beat bar shows 'playing' after a Solo Start on a track with nothing to play");

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
        beginTest (juce::String ("Start with no tracks leaves the panel stopped (")
                   + (arrangerMode ? "arranger mode" : "track-player mode") + ")");

        std::unordered_map<juce::Uuid, TrackEntry*> noTrackEntries;
        CurrentStyleComponent panel ("Style 1", noTrackEntries, {}, {});
        panel.setMidiInjectCallback ([] (const juce::MidiMessage&) {});   // audio output present, as in the app
        panel.setArrangerModeEnabled (arrangerMode);
        panel.setSize (700, 120);                                          // resized() labels the buttons

        auto* start = findChild<juce::TextButton> (panel, [] (juce::TextButton& b) { return b.getButtonText() == "Start"; });
        auto* beat  = findChild<BeatBar> (panel, [] (BeatBar&) { return true; });
        auto* tempo = findChild<juce::Slider> (panel, [] (juce::Slider& s) { return s.getMinimum() == 60.0 && s.getMaximum() == 200.0; });

        expect (start != nullptr && beat != nullptr && tempo != nullptr, "panel children not found");
        if (start == nullptr || beat == nullptr || tempo == nullptr)
            return;

        expect (tempo->isEnabled(), "BPM slider should start enabled");

        start->onClick();   // what a click runs; triggerClick() would only post it

        expect (! (beat->isPlayingCheck && beat->isPlayingCheck()),
                "beat bar still shows 'playing' after a Start that had nothing to play");
        expect (tempo->isEnabled(),
                "BPM slider locked after a Start that had nothing to play");

        closeAlerts();
    }
};

static CurrentStyleStartTest currentStyleStartTest;
