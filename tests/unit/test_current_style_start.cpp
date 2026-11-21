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
