#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include "ArrangerScheduler.h"
#include "ArrangerSectionSequencer.h"
#include "Chord.h"
#include "ChordTransposer.h"
#include <atomic>
#include <vector>
#include <map>

/**
 * Real-time arranger playback: loops the active section of a style to MIDI-out and/or
 * an inject callback (SFZ). Parallel to MultipleTrackPlayer; the classic player is untouched.
 */
class ArrangerEngine : private juce::HighResolutionTimer
{
public:
    std::function<void (const juce::MidiMessage&)> onMidiMessage; // SFZ inject
    std::function<void (double)> onElapsedBeats;                  // for the beat bar

    /** Fired (on the message thread) whenever the active section changes, so the live section
        buttons can highlight the one that's actually sounding. sectionIndex<0 means none. */
    std::function<void (int sectionIndex, ArrangerSectionType type, juce::String name)> onActiveSectionChanged;
    /** Fired (on the message thread) when the engine stops itself (an Ending completed), so the UI
        can drop its "playing" state (reset the beat bar, etc.). Not fired for a no-op stop. */
    std::function<void()> onStoppedItself;

    explicit ArrangerEngine (std::weak_ptr<juce::MidiOutput> out);
    ~ArrangerEngine() override;

    void setDeviceOutput (std::weak_ptr<juce::MidiOutput> out);
    void setStyle (ArrangerStyle newStyle);   // builds the scheduler from section 0
    void setBpm (double bpm);

    /** Queue a section change (applied at the next bar boundary while playing). */
    void queueSection (ArrangerSectionType type, const juce::String& name);
    /** Choose which section a subsequent start() begins on (used while stopped). */
    void selectStartSection (ArrangerSectionType type, const juce::String& name);

    /** Phase 4: set the live played chord that pitched parts transpose to. Thread-safe â€” callable
        from the MIDI input thread; applied on the timer thread before the next emit. */
    void setActiveChord (ArrangerChord played);
    /** Phase 4: set the recorded chord the style was authored in (the NTT "from" reference). */
    void setOriginalChord (ArrangerChord recorded);
    /** Phase 4: toggle Bass Inversion (lowest fingered note drives the bass; slash chords). */
    void setBassInversion (bool shouldInvert);
    /** Phase 7a: master NTT bypass — when false every track plays in the recorded home key. */
    void setNttEnabled (bool enabled);
    /** Phase 7a: curated minor scale used to snap passing tones over minor chords. */
    void setMinorScale (MinorScaleChoice choice);

    /** Enable/disable Auto Fill on variation switches (delegates to the sequencer). */
    void setAutoFillEnabled (bool enabled) { sequencer.setAutoFillEnabled (enabled); }

    int getActiveSectionIndex() const;
    int peekPendingStartIndex() const { return pendingStartIndex; } // test seam

    /** Beats elapsed within the currently-active section (monotonic playhead minus the section's
        start). For the editor's section-relative playhead arrow; read on the message thread, so a
        sub-tick-stale value is fine. */
    double getActiveSectionLocalBeats() const { return playheadBeats - sequencer.getActiveStartAbs(); }

    /** Begin playback. useTransportFeel=true honours Synchro Start / Count-In (live performance). The
        editor preview passes false so it ALWAYS starts immediately: it feeds the engine no chords, so an
        armed Synchro gate would wait forever (silent preview). */
    void start (bool useTransportFeel = true);
    void stop();
    bool isPlaying() const { return playing.load(); }

    /** Phase 6: Synchro Start. When enabled, start() arms the engine instead of playing immediately;
        the groove begins on the first recognised chord (a chord already held at Start fires it at
        once, via the Phase-5 held-chord seed). */
    void setSynchroStartEnabled (bool b) { synchroStartEnabled = b; }
    bool isSynchroArmed() const { return synchroArmed.load(); }   // test seam
    void armSynchro() { synchroArmed.store (true); }              // test seam (no timer)

    /** Phase 6b: Count-In. When enabled, start() plays one bar of metronome clicks before the groove
        (after the Synchro gate, if Synchro Start is also on). */
    void setCountInEnabled (bool b) { countInEnabled = b; }
    bool isCountingIn() const { return countingIn.load(); }       // test seam
    void armCountIn();                                            // test seam (no timer)
    bool renderCountIn (double deltaBeats);                       // dispatch clicks; true while counting

    double getLoopLengthBeats() const { return loopLengthBeats; }

    /** Render and dispatch events for the monotonic beat window [fromBeats, toBeats). Public for tests. */
    void renderRange (double fromBeats, double toBeats);

private:
    void hiResTimerCallback() override;
    void dispatch (const juce::MidiMessage& m);
    void dispatchEmitted (const EmittedEvent& e);   // transpose (by PartKind) then dispatch
    void silenceArrangerNotes();   // note-off ONLY the arranger's own sounding notes (not the player's)
    void sendInstrumentSetup();   // program-change + volume per channel, like the classic player
    void rebuildFromStyle();
    void updateActiveLoopLength();
    void haltAudio();             // silence + reset state, WITHOUT stopping the timer
    void notifyActiveSection (bool force);   // tell the UI which section is active (on change, or forced)
    int  indexOfSection (ArrangerSectionType type, const juce::String& name) const;

    std::weak_ptr<juce::MidiOutput> outputDevice;
    ArrangerStyle style;
    std::vector<ArrangerScheduler> schedulers;       // one per section
    ArrangerSectionSequencer       sequencer;
    int  currentSchedulerIndex = -1;                 // which scheduler is currently sounding
    int  pendingStartIndex     = -1;                 // section start() should begin on (-1 = default)
    int  lastReportedSectionIndex = -1;              // last index sent to onActiveSectionChanged
    bool timerShouldStop       = false;              // set on the timer thread, honoured on the msg thread

    // Section-switch request: set on the message thread (UI click/button), applied on the timer
    // thread at the top of each callback so all sequencer mutation stays single-threaded.
    juce::CriticalSection requestLock;
    ArrangerSectionType   requestedType = ArrangerSectionType::Variation;
    juce::String          requestedName;
    bool                  hasQueuedRequest = false;

    double currentBpm = 120.0;
    double loopLengthBeats = 0.0;
    double playheadBeats = 0.0;     // monotonic beat position since start()
    double lastNowSeconds = 0.0;    // wall-clock of previous tick (for delta accumulation)
    std::atomic<bool> playing { false };

    // Phase 6: Synchro Start. synchroStartEnabled is the user's setting (message thread); synchroArmed
    // is the live "waiting for the first chord" gate, set on start() and cleared on the first valid
    // chord (input thread) or on stop. Atomic so the timer thread can check it lock-free.
    bool              synchroStartEnabled = false;
    std::atomic<bool> synchroArmed { false };

    // Phase 6b: Count-In. countInEnabled is the user's setting; countingIn is the live pre-roll gate.
    bool              countInEnabled = false;
    std::atomic<bool> countingIn { false };
    double            countInPlayhead = 0.0;     // beats elapsed within the count-in bar
    double            countInLengthBeats = 0.0;  // = beats per bar

    // Phase 4: chord transposition. The transposer + activePlayedNote are timer-thread-only; the
    // mailbox (chordLock-guarded) carries the live chord from the MIDI input thread.
    ChordTransposer       transposer;
    juce::CriticalSection chordLock;
    ArrangerChord                 pendingChord;
    bool                  hasChordUpdate = false;
    std::map<std::pair<int,int>, int> activePlayedNote;  // (channel, originalNote) -> sounding note
};
