#include <JuceHeader.h>
#include "CurrentStyleComponent.h"
#include "CustomBeatBar.h"
#include "CustomTableContainer.h"
#include "Arranger/ArrangerEngine.h"
#include "Arranger/ArrangerModel.h"

/*
    Pressing Start while already playing must leave the BPM slider locked while playback runs, and
    pressing Stop must unlock it - whatever the order and number of presses.

    Bugs (reproduced live 2026-11-22):
    - Track player: every start() and every counted stop() posted a "flip the BPM slider" notification.
      Start while playing stops uncounted and starts counted, so the flip went the wrong way: BPM
      unlocked while playing, locked after Stop. The notes table flipped its Change/Apply buttons the
      same way.
    - Arranger: every stop that was playing posts "stopped" to the message thread. Start while playing
      restarts at once, so that notification arrived after the restart and cleared the playing state
      of an engine that was playing again.

    Notifications are asynchronous and the shared test runner never dispatches its message queue, so
    each test delivers a notification by calling the listener exactly once per notification the code
    under test posts - which is what the message queue would do.
*/
namespace StartWhilePlayingTestValues
{
    const juce::String testName     = "Start pressed while already playing";
    const juce::String testCategory = "Unit";

    const juce::String styleName             = "Start while playing";
    const juce::String trackName             = "Held note";
    const juce::String startButtonText       = "Start";
    const juce::String stopButtonText        = "Stop";
    const juce::String tableChangeButtonText = "Change";
    const juce::String tableApplyButtonText  = "Apply";

    constexpr int panelWidth  = 700;
    constexpr int panelHeight = 120;

    constexpr int         midiChannel    = 2;
    constexpr int         heldNoteNumber = 60;   // middle C
    constexpr juce::uint8 noteVelocity   = 100;
    constexpr double      noteOnSeconds  = 0.0;
    constexpr double      noteOffSeconds = 600.0;   // far longer than the test, so playback never ends by itself
    constexpr double      trackTempoBpm  = 120.0;

    constexpr double sliderMinimumBpm = 60.0;   // identifies the BPM slider among the panel's sliders
    constexpr double sliderMaximumBpm = 200.0;

    constexpr int    styleBeatsPerBar   = 4;
    constexpr int    styleBeatUnit      = 4;
    constexpr int    sectionLengthBars  = 1;
    constexpr double patternNoteOnBeat  = 0.0;
    constexpr double patternNoteOffBeat = 0.5;

    constexpr bool tableOpenedWhilePlaying = false;
    constexpr int  noStoppedNotifications  = 0;
    constexpr int  oneStoppedNotification  = 1;
}

namespace StartWhilePlayingTestText
{
    const juce::String trackPlayerTitle = "Track player: BPM locked while playing and unlocked when stopped, after Start, Start, Stop, Stop";
    const juce::String panelChildrenMissing        = "panel children not found";
    const juce::String startDidNotStartPlayer      = "Start did not start the track player";
    const juce::String bpmUnlockedAfterStart       = "BPM slider unlocked after Start";
    const juce::String secondStartStoppedPlayer    = "second Start left the track player stopped";
    const juce::String bpmUnlockedAfterSecondStart = "BPM slider unlocked while playing, after Start pressed during playback";
    const juce::String stopDidNotStopPlayer        = "Stop did not stop the track player";
    const juce::String bpmLockedAfterStop          = "BPM slider locked after Stop";
    const juce::String bpmLockedAfterSecondStop    = "BPM slider locked after Stop pressed while stopped";

    const juce::String arrangerModeTitle = "Arranger mode: a track-player notification does not unlock the BPM slider";
    const juce::String arrangerDidNotLockBpm     = "BPM slider not locked after Start in arranger mode";
    const juce::String trackPlayerUnlockedArranger = "a track-player notification unlocked the BPM slider while the arranger plays";

    const juce::String notesTableTitle = "Notes table: Change and Apply stay disabled across repeated 'playing' notifications";
    const juce::String tableButtonsMissing           = "Change/Apply buttons not found";
    const juce::String changeEnabledWhilePlaying     = "Change button enabled while playing";
    const juce::String changeEnabledAfterSecondStart = "Change button enabled while playing, after Start pressed during playback";
    const juce::String applyEnabledAfterSecondStart  = "Apply button enabled while playing, after Start pressed during playback";
    const juce::String changeDisabledAfterStop       = "Change button disabled after Stop";
    const juce::String applyDisabledAfterStop        = "Apply button disabled after Stop";

    const juce::String arrangerRestartTitle = "Arranger: a 'stopped' notification arriving after a restart is ignored";
    const juce::String engineNotPlayingAfterRestart = "engine not playing after the restart";
    const juce::String staleStopReachedUi           = "a stale 'stopped' notification reached the UI after a restart";
    const juce::String realStopNotReported          = "a real stop must still be reported";
}

class StartWhilePlayingTest : public juce::UnitTest
{
public:
    StartWhilePlayingTest()
        : juce::UnitTest (StartWhilePlayingTestValues::testName, StartWhilePlayingTestValues::testCategory) {}

    void runTest() override
    {
        trackPlayerKeepsBpmLockedOnlyWhilePlaying();
        trackPlayerNotificationLeavesArrangerBpmLockAlone();
        notesTableKeepsEditButtonsDisabledWhilePlaying();
        arrangerIgnoresAStopNotificationThatArrivesAfterARestart();
    }

private:
    struct PanelControls
    {
        Track*            firstSlot   = nullptr;
        juce::TextButton* startButton = nullptr;
        juce::TextButton* stopButton  = nullptr;
        juce::Slider*     bpmSlider   = nullptr;

        bool allFound() const { return firstSlot != nullptr && startButton != nullptr && stopButton != nullptr && bpmSlider != nullptr; }
    };

    template <typename ComponentType, typename Predicate>
    static ComponentType* findChild (juce::Component& parent, Predicate matches)
    {
        for (auto* child : parent.getChildren())
            if (auto* typed = dynamic_cast<ComponentType*> (child))
                if (matches (*typed))
                    return typed;
        return nullptr;
    }

    template <typename ComponentType>
    static ComponentType* findDescendant (juce::Component& parent)
    {
        for (auto* child : parent.getChildren())
        {
            if (auto* typed = dynamic_cast<ComponentType*> (child))
                return typed;
            if (auto* nested = findDescendant<ComponentType> (*child))
                return nested;
        }
        return nullptr;
    }

    static juce::TextButton* findButton (juce::Component& parent, const juce::String& buttonText)
    {
        return findChild<juce::TextButton> (parent, [&] (juce::TextButton& button) { return button.getButtonText() == buttonText; });
    }

    static PanelControls findPanelControls (CurrentStyleComponent& panel)
    {
        using namespace StartWhilePlayingTestValues;
        PanelControls controls;
        controls.firstSlot   = findDescendant<Track> (panel);
        controls.startButton = findButton (panel, startButtonText);
        controls.stopButton  = findButton (panel, stopButtonText);
        controls.bpmSlider   = findChild<juce::Slider> (panel, [] (juce::Slider& slider)
                                   { return slider.getMinimum() == sliderMinimumBpm && slider.getMaximum() == sliderMaximumBpm; });
        return controls;
    }

    static juce::MidiMessageSequence makeHeldNoteSequence()
    {
        using namespace StartWhilePlayingTestValues;
        juce::MidiMessageSequence sequence;
        sequence.addEvent (juce::MidiMessage::noteOn (midiChannel, heldNoteNumber, noteVelocity), noteOnSeconds);
        sequence.addEvent (juce::MidiMessage::noteOff (midiChannel, heldNoteNumber), noteOffSeconds);
        sequence.updateMatchedPairs();
        return sequence;
    }

    static TrackEntry makeHeldNoteTrack()
    {
        using namespace StartWhilePlayingTestValues;
        TrackEntry heldNoteTrack;
        heldNoteTrack.uuid        = juce::Uuid();
        heldNoteTrack.displayName = trackName;
        heldNoteTrack.sequence    = makeHeldNoteSequence();
        heldNoteTrack.originalBPM = trackTempoBpm;
        heldNoteTrack.type        = TrackType::Melodic;
        return heldNoteTrack;
    }

    void trackPlayerKeepsBpmLockedOnlyWhilePlaying()
    {
        using namespace StartWhilePlayingTestValues;
        using namespace StartWhilePlayingTestText;
        beginTest (trackPlayerTitle);

        auto heldNoteTrack = makeHeldNoteTrack();
        std::unordered_map<juce::Uuid, TrackEntry*> trackEntries { { heldNoteTrack.uuid, &heldNoteTrack } };
        CurrentStyleComponent panel (styleName, trackEntries, {}, {});
        panel.setMidiInjectCallback ([] (const juce::MidiMessage&) {});
        panel.setSize (panelWidth, panelHeight);

        const auto controls = findPanelControls (panel);
        auto* player = panel.getTrackPlayer();
        expect (controls.allFound() && player != nullptr, panelChildrenMissing);
        if (! controls.allFound() || player == nullptr)
            return;

        controls.firstSlot->setUUID (heldNoteTrack.uuid);

        // Each press below posts exactly one play-state notification (start() or a counted stop()).
        controls.startButton->onClick();
        panel.updateObjects();
        expect (player->isPlaying(), startDidNotStartPlayer);
        expect (! controls.bpmSlider->isEnabled(), bpmUnlockedAfterStart);

        controls.startButton->onClick();   // while already playing
        panel.updateObjects();
        expect (player->isPlaying(), secondStartStoppedPlayer);
        expect (! controls.bpmSlider->isEnabled(), bpmUnlockedAfterSecondStart);

        controls.stopButton->onClick();
        panel.updateObjects();
        expect (! player->isPlaying(), stopDidNotStopPlayer);
        expect (controls.bpmSlider->isEnabled(), bpmLockedAfterStop);

        controls.stopButton->onClick();    // while already stopped
        panel.updateObjects();
        expect (controls.bpmSlider->isEnabled(), bpmLockedAfterSecondStop);
    }

    void trackPlayerNotificationLeavesArrangerBpmLockAlone()
    {
        using namespace StartWhilePlayingTestValues;
        using namespace StartWhilePlayingTestText;
        beginTest (arrangerModeTitle);

        auto heldNoteTrack = makeHeldNoteTrack();
        std::unordered_map<juce::Uuid, TrackEntry*> trackEntries { { heldNoteTrack.uuid, &heldNoteTrack } };
        CurrentStyleComponent panel (styleName, trackEntries, {}, {});
        panel.setMidiInjectCallback ([] (const juce::MidiMessage&) {});
        panel.setArrangerModeEnabled (true);
        panel.setSize (panelWidth, panelHeight);

        const auto controls = findPanelControls (panel);
        expect (controls.allFound(), panelChildrenMissing);
        if (! controls.allFound())
            return;

        controls.firstSlot->setUUID (heldNoteTrack.uuid);

        controls.startButton->onClick();   // the arranger plays and locks the BPM slider
        expect (! controls.bpmSlider->isEnabled(), arrangerDidNotLockBpm);

        // A track-player notification still queued from before the switch to arranger mode
        // (e.g. a track that ended just before) arrives now.
        panel.updateObjects();
        expect (! controls.bpmSlider->isEnabled(), trackPlayerUnlockedArranger);

        controls.stopButton->onClick();
    }

    void notesTableKeepsEditButtonsDisabledWhilePlaying()
    {
        using namespace StartWhilePlayingTestValues;
        using namespace StartWhilePlayingTestText;
        beginTest (notesTableTitle);

        auto sequence = makeHeldNoteSequence();
        std::unordered_map<int, MidiChangeInfo> noteChanges;
        TableContainer notesTable (sequence, trackName, midiChannel, noteChanges, tableOpenedWhilePlaying);

        auto* changeButton = findButton (notesTable, tableChangeButtonText);
        auto* applyButton  = findButton (notesTable, tableApplyButtonText);
        expect (changeButton != nullptr && applyButton != nullptr, tableButtonsMissing);
        if (changeButton == nullptr || applyButton == nullptr)
            return;

        bool playerRunning = false;
        notesTable.isPlayerRunning = [&] { return playerRunning; };

        playerRunning = true;          // Start
        notesTable.updateObjects();
        expect (! changeButton->isEnabled(), changeEnabledWhilePlaying);

        notesTable.updateObjects();    // Start pressed again while playing
        expect (! changeButton->isEnabled(), changeEnabledAfterSecondStart);
        expect (! applyButton->isEnabled(),  applyEnabledAfterSecondStart);

        playerRunning = false;         // Stop
        notesTable.updateObjects();
        expect (changeButton->isEnabled(), changeDisabledAfterStop);
        expect (applyButton->isEnabled(),  applyDisabledAfterStop);
    }

    static ArrangerStyle makeOneBarStyle()
    {
        using namespace StartWhilePlayingTestValues;
        ArrangerTrack accompaniment;
        accompaniment.channel  = midiChannel;
        accompaniment.partType = ArrangerPartType::Acc;
        accompaniment.pattern  = {
            { patternNoteOnBeat,  juce::MidiMessage::noteOn  (midiChannel, heldNoteNumber, noteVelocity) },
            { patternNoteOffBeat, juce::MidiMessage::noteOff (midiChannel, heldNoteNumber) },
        };
        ArrangerSection section;
        section.lengthBars = sectionLengthBars;
        section.tracks.push_back (accompaniment);

        ArrangerStyle style;
        style.timeSigNum    = styleBeatsPerBar;
        style.timeSigDenom  = styleBeatUnit;
        style.originalTempo = trackTempoBpm;
        style.sections.push_back (section);
        return style;
    }

    void arrangerIgnoresAStopNotificationThatArrivesAfterARestart()
    {
        using namespace StartWhilePlayingTestValues;
        using namespace StartWhilePlayingTestText;
        beginTest (arrangerRestartTitle);

        ArrangerEngine engine (std::weak_ptr<juce::MidiOutput> {});
        engine.setStyle (makeOneBarStyle());

        int stoppedNotifications = noStoppedNotifications;
        engine.onStoppedItself = [&] { ++stoppedNotifications; };

        engine.start();
        engine.stop();                          // posts "stopped"...
        engine.start();                         // ...but Start restarts before it is delivered
        engine.notifyStoppedUnlessRestarted();  // the queued notification arrives now
        expect (engine.isPlaying(), engineNotPlayingAfterRestart);
        expect (stoppedNotifications == noStoppedNotifications, staleStopReachedUi);

        engine.stop();
        engine.notifyStoppedUnlessRestarted();
        expect (stoppedNotifications == oneStoppedNotification, realStopNotReported);
    }
};

static StartWhilePlayingTest startWhilePlayingTest;
