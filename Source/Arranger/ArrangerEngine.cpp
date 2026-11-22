#include "ArrangerEngine.h"
#include "ArrangerTime.h"
#include "ArrangerCountIn.h"
#include "ArrangerChannels.h"

namespace
{
    // Note-offs sent when silencing go out immediately; their beat position is not used.
    constexpr double flushNotesAtBeats      = 0.0;
    constexpr int    timerIntervalMs        = 10;    // the engine's render tick
    constexpr int    channelVolumeCC        = 7;
    constexpr int    lowestControllerValue  = 0;
    constexpr int    highestControllerValue = 127;
    constexpr int    lowestProgram          = 0;
    constexpr int    firstSection           = 0;
    constexpr int    oneSection             = 1;
    constexpr int    minimumBeatsPerBar     = 1;
    constexpr double noTempo                = 0.0;
    constexpr double noBeats                = 0.0;
}

ArrangerEngine::ArrangerEngine (std::weak_ptr<juce::MidiOutput> out) : outputDevice (out) {}

ArrangerEngine::~ArrangerEngine()
{
    stopTimer();
}

void ArrangerEngine::setDeviceOutput (std::weak_ptr<juce::MidiOutput> out)
{
    outputDevice = out;
}

void ArrangerEngine::setBpm (double bpm)
{
    if (bpm > noTempo) currentBpm = bpm;
}

void ArrangerEngine::rebuildFromStyle()
{
    schedulers.clear();
    const double beatsPerBarValue = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);

    for (const auto& section : style.sections)
    {
        std::vector<TimedBeatEvent> merged;
        std::vector<PartKind>       parts;
        std::vector<NttType>        ntts;
        for (const auto& track : section.tracks)
        {
            // Drums/Perc never transpose; Bass honours bass inversion; Acc is plain harmony.
            const PartKind partKind = (track.partType == ArrangerPartType::Bass) ? PartKind::Bass
                                    : (track.partType == ArrangerPartType::Acc)  ? PartKind::Acc
                                                                                 : PartKind::Fixed;
            for (const auto& event : track.pattern)
            {
                merged.push_back (event);
                parts.push_back (partKind);
                ntts.push_back (track.nttType);   // Phase 7a: per-track NTT type
            }
        }

        const double bars = (double) juce::jmax (ArrangerStyleDefaults::minimumLengthBars, section.lengthBars);
        ArrangerScheduler scheduler;
        scheduler.setLoop (std::move (merged), std::move (parts), std::move (ntts), bars * beatsPerBarValue);
        schedulers.push_back (std::move (scheduler));
    }

    sequencer.setStyle (style);
    currentSchedulerIndex = ArrangerStyleDefaults::noIndex;
    updateActiveLoopLength();
}

void ArrangerEngine::updateActiveLoopLength()
{
    if (style.sections.empty())
    {
        loopLengthBeats = noBeats;
        return;
    }
    const double beatsPerBarValue = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);
    const int activeIndex = juce::jlimit (firstSection, (int) style.sections.size() - oneSection, sequencer.getActiveIndex());
    loopLengthBeats = (double) juce::jmax (ArrangerStyleDefaults::minimumLengthBars, style.sections[activeIndex].lengthBars) * beatsPerBarValue;
}

int ArrangerEngine::indexOfSection (ArrangerSectionType type, const juce::String& name) const
{
    // Match by type + trailing number (like the sequencer's findSection), so the live button labels
    // ("Var 2") resolve to the matching section ("Variation 2") even though the strings differ.
    const int wantedNumber = name.getTrailingIntValue();
    int firstOfType = ArrangerStyleDefaults::noIndex, numMatch = ArrangerStyleDefaults::noIndex;
    for (int sectionIndex {}; sectionIndex < (int) style.sections.size(); ++sectionIndex)
    {
        if (style.sections[sectionIndex].type != type) continue;
        if (firstOfType == ArrangerStyleDefaults::noIndex) firstOfType = sectionIndex;
        if (style.sections[sectionIndex].name.getTrailingIntValue() == wantedNumber) numMatch = sectionIndex;
    }
    return (numMatch != ArrangerStyleDefaults::noIndex) ? numMatch : firstOfType;
}

int ArrangerEngine::getActiveSectionIndex() const
{
    return sequencer.getActiveIndex();
}

void ArrangerEngine::queueSection (ArrangerSectionType type, const juce::String& name)
{
    // Called from the message thread. Hand the request to the timer thread instead of touching the
    // sequencer directly (it's advanced on the timer thread; a direct write here is a data race).
    const juce::ScopedLock requestGuard (requestLock);
    requestedType    = type;
    requestedName    = name;
    hasQueuedRequest = true;
}

void ArrangerEngine::selectStartSection (ArrangerSectionType type, const juce::String& name)
{
    pendingStartIndex = indexOfSection (type, name);
}

void ArrangerEngine::setStyle (ArrangerStyle newStyle)
{
    // setStyle runs on the MESSAGE thread (every Start, and every editor preview rebuild -- moving a
    // region, changing the recorded key, etc.). The render runs on the TIMER thread. When we're playing,
    // mutating the schedulers from here would race that render (the old drone bug + a latent crash).
    //
    // Fix: if playing, stop the timer first. stopTimer() BLOCKS until any in-flight callback has
    // returned, so once it returns the timer thread is guaranteed idle and we can swap the schedulers
    // safely; then we resume. This locks nothing on the render path, and it blocks only this message
    // thread for a fraction of a tick -- it never touches the real-time audio thread (which pulls MIDI
    // from its own buffer independently). silenceArrangerNotes() first so the old notes get their
    // note-offs before rebuildFromStyle() replaces the schedulers (no stranded drone).
    const bool wasPlaying = playing.load();
    if (wasPlaying)
    {
        stopTimer();
        silenceArrangerNotes();
    }

    style = std::move (newStyle);
    if (style.originalTempo > noTempo) currentBpm = style.originalTempo;
    rebuildFromStyle();
    // The recorded chord the NTT maps out of (style-level). bassNote = root (no inversion baked in).
    transposer.setOriginalChord ({ style.originalRoot, style.originalQuality, style.originalRoot });

    // Start every Start/Play in the style's OWN home key (setStyle runs on each Start). Live chord
    // changes during playback persist; pressing Start again resets to home rather than keeping the
    // last chord, which is what a performer expects.
    transposer.setActiveChord ({});
    {
        const juce::ScopedLock chordGuard (chordLock);
        pendingChord   = {};
        hasChordUpdate = false;
    }

    if (wasPlaying)
    {
        // rebuildFromStyle() resets the sequencer to the first section at activeStartAbs=0. Realign the
        // engine playhead to 0 as well -- otherwise the resumed groove picks up at (old playhead % loop
        // length), an arbitrary mid-loop phase that sounds like a glitchy jump. Zeroing it makes the
        // rebuilt preview resume cleanly from the section's downbeat.
        playheadBeats  = noBeats;
        // Don't let the rebuild's wall-clock duration count as elapsed musical time:
        lastNowSeconds = (double) juce::Time::getHighResolutionTicks()
                         / (double) juce::Time::getHighResolutionTicksPerSecond();
        startTimer (timerIntervalMs);
    }
}

void ArrangerEngine::setActiveChord (ArrangerChord played)
{
    // Called from the MIDI input thread. Hand the chord to the timer thread via a guarded mailbox;
    // it is applied to the transposer at the top of renderRange (timer thread only).
    {
        const juce::ScopedLock chordGuard (chordLock);
        pendingChord   = played;
        hasChordUpdate = true;
    }

    // Phase 6: Synchro Start — the first recognised chord releases the gate so the groove begins.
    if (played.isValid() && synchroArmed.load())
        synchroArmed.store (false);
}

void ArrangerEngine::setOriginalChord (ArrangerChord recorded)
{
    transposer.setOriginalChord (recorded);
}

void ArrangerEngine::setBassInversion (bool shouldInvert)
{
    transposer.setBassInversion (shouldInvert);
}

void ArrangerEngine::setNttEnabled (bool enabled)
{
    transposer.setNttEnabled (enabled);
}

void ArrangerEngine::setMinorScale (MinorScaleChoice choice)
{
    transposer.setMinorScale (choice);
}

void ArrangerEngine::dispatch (const juce::MidiMessage& message)
{
    if (auto device = outputDevice.lock())
        device->sendMessageNow (message);
    if (onMidiMessage)
        onMidiMessage (message);
}

void ArrangerEngine::dispatchEmitted (const EmittedEvent& emitted)
{
    juce::MidiMessage message = emitted.message;

    if (message.isNoteOn() && emitted.part != PartKind::Fixed)   // isNoteOn() excludes velocity 0
    {
        // Transpose the pitched note and remember the played pitch, keyed by its ORIGINAL note, so
        // the matching note-off (which carries the original pitch) closes the same sounding note even
        // if the chord changed in between.
        const int originalNote = message.getNoteNumber();
        const int soundingNote = transposer.transpose (originalNote, emitted.part, emitted.ntt);
        activePlayedNote[{ message.getChannel(), originalNote }].push_back (soundingNote);
        message = juce::MidiMessage::noteOn (message.getChannel(), soundingNote, message.getVelocity());
    }
    else if (message.isNoteOff())   // JUCE counts a velocity-0 note-on as a note-off
    {
        const auto started = activePlayedNote.find (std::make_pair (message.getChannel(), message.getNoteNumber()));
        if (started != activePlayedNote.end() && ! started->second.empty())
        {
            message = juce::MidiMessage::noteOff (message.getChannel(), started->second.front());
            started->second.pop_front();   // the oldest start of this pitch ends first
            if (started->second.empty())
                activePlayedNote.erase (started);
        }
    }

    dispatch (message);
}

void ArrangerEngine::silenceArrangerNotes()
{
    // Silence ONLY the notes the arranger itself turned on. A blanket all-notes-off across channels
    // 1-16 (the old behaviour) also killed the player's manually-held notes -- e.g. a chord held
    // while pressing Start went silent. Flushing each scheduler emits note-offs for its active notes
    // keyed by their ORIGINAL pitch; routing them through dispatchEmitted closes the matching SOUNDING
    // note for pitched parts (and clears activePlayedNote) while leaving drums/Fixed as-is. Any pitched
    // note still tracked (e.g. its scheduler was already reset) is then closed directly.
    for (auto& scheduler : schedulers)
        for (auto& emitted : scheduler.flushActiveNotes (flushNotesAtBeats))
            dispatchEmitted (emitted);

    for (const auto& [channelAndOriginalNote, soundingNotes] : activePlayedNote)
        for (const int soundingNote : soundingNotes)
            dispatch (juce::MidiMessage::noteOff (channelAndOriginalNote.first, soundingNote));
    activePlayedNote.clear();
}

void ArrangerEngine::sendInstrumentSetup()
{
    // Select each track's instrument + channel volume across all sections, so any section
    // we switch to already has the right sound. Drum channel (10) keeps its fixed kit. The
    // SFZ engine ignores program-change but honours CC7 volume; an external synth honours both.
    for (const auto& section : style.sections)
        for (const auto& track : section.tracks)
        {
            if (track.instrument >= lowestProgram && track.channel != ArrangerChannels::kDrumChannel)
                dispatch (juce::MidiMessage::programChange (track.channel, track.instrument));

            dispatch (juce::MidiMessage::controllerEvent (track.channel, channelVolumeCC,
                                                          juce::jlimit (lowestControllerValue, highestControllerValue, (int) track.volume)));
        }
}

void ArrangerEngine::armCountIn()
{
    countInPlayhead    = noBeats;
    countInLengthBeats = (double) juce::jmax (minimumBeatsPerBar, style.timeSigNum);   // one bar
    countingIn.store (true);
}

bool ArrangerEngine::renderCountIn (double deltaBeats)
{
    if (! countingIn.load())
        return false;

    const double from = countInPlayhead;
    const double to   = countInPlayhead + deltaBeats;
    for (const auto& click : ArrangerCountIn::clicksInWindow (from, to, countInLengthBeats))
        dispatch (juce::MidiMessage::noteOn (click.channel, click.note, (juce::uint8) click.velocity));

    countInPlayhead = to;
    if (countInPlayhead >= countInLengthBeats)
    {
        countingIn.store (false);
        return false;   // count-in finished this tick; the section starts next tick at beat 0
    }
    return true;
}

void ArrangerEngine::renderRange (double fromBeats, double toBeats)
{
    if (schedulers.empty())
        return;

    // Apply any UI-queued section switch before advancing. Done here (not only in the timer callback)
    // so the request is honoured whether we're driven by the timer or by a direct renderRange call
    // (e.g. unit tests). The lock guards the request fields; sequencer mutation stays on this thread.
    {
        const juce::ScopedLock requestGuard (requestLock);
        if (hasQueuedRequest)
        {
            sequencer.queue (requestedType, requestedName);
            hasQueuedRequest = false;
        }
    }

    // Apply the latest played chord (from the MIDI input thread) before emitting this window's notes.
    {
        const juce::ScopedLock chordGuard (chordLock);
        if (hasChordUpdate)
        {
            transposer.setActiveChord (pendingChord);
            hasChordUpdate = false;
        }
    }

    SequencerStep step = sequencer.advance (fromBeats, toBeats);

    for (const auto& segment : step.segments)
    {
        if (segment.sectionIndex != currentSchedulerIndex)
        {
            // Flush the outgoing section's hung notes, then reset both sides so the
            // incoming section enters clean at its own bar 0.
            if (currentSchedulerIndex >= firstSection && currentSchedulerIndex < (int) schedulers.size())
            {
                for (auto& emitted : schedulers[currentSchedulerIndex].flushActiveNotes (flushNotesAtBeats))
                    dispatchEmitted (emitted);
                schedulers[currentSchedulerIndex].reset();
            }
            currentSchedulerIndex = segment.sectionIndex;
            if (currentSchedulerIndex >= firstSection && currentSchedulerIndex < (int) schedulers.size())
                schedulers[currentSchedulerIndex].reset();
        }

        if (segment.sectionIndex >= firstSection && segment.sectionIndex < (int) schedulers.size())
            for (auto& emitted : schedulers[segment.sectionIndex].advance (segment.localFromBeats, segment.localToBeats))
                dispatchEmitted (emitted);
    }

    if (step.stopRequested)
    {
        haltAudio();            // silence + reset, but do not touch the timer here
        timerShouldStop = true; // the timer callback will stop the timer on the message thread
        return;
    }

    updateActiveLoopLength();
}

void ArrangerEngine::start (bool useTransportFeel)
{
    if (schedulers.empty() || loopLengthBeats <= noBeats)
        return;

    // A restart while already playing (e.g. pressing Preview again) must close the previous run's
    // sounding notes first; the reset/clear below would otherwise strand them as an overlapping drone.
    if (playing.load())
        silenceArrangerNotes();

    for (auto& scheduler : schedulers) scheduler.reset();
    sequencer.reset();
    activePlayedNote.clear();
    if (pendingStartIndex != ArrangerStyleDefaults::noIndex)
        sequencer.startAt (pendingStartIndex);
    pendingStartIndex = ArrangerStyleDefaults::noIndex;
    currentSchedulerIndex = ArrangerStyleDefaults::noIndex;
    lastReportedSectionIndex = ArrangerStyleDefaults::noIndex;   // so the first tick reports the starting section
    timerShouldStop = false;

    sendInstrumentSetup();   // select instruments + volumes before the first notes play
    playheadBeats = noBeats;
    lastNowSeconds = (double) juce::Time::getHighResolutionTicks()
                     / (double) juce::Time::getHighResolutionTicksPerSecond();
    // Transport feel (Synchro Start / Count-In) applies only to live performance. The editor preview
    // passes useTransportFeel=false so it never arms the Synchro gate (which would wait for a chord the
    // preview never sends) or a count-in -- it just plays.
    synchroArmed.store (useTransportFeel && synchroStartEnabled);   // Phase 6: wait for the first chord
    if (useTransportFeel && countInEnabled) armCountIn();           // Phase 6b: pre-roll a count-in bar
    else                                    countingIn.store (false);
    playing.store (true);
    startTimer (timerIntervalMs);
}

void ArrangerEngine::stop()
{
    stopTimer();   // only safe off the timer thread; the Ending path defers this via callAsync
    haltAudio();
}

void ArrangerEngine::haltAudio()
{
    const bool wasPlaying = playing.exchange (false);
    synchroArmed.store (false);   // Phase 6: never leave the Synchro gate armed across a stop
    countingIn.store (false);     // Phase 6b: cancel any in-progress count-in
    silenceArrangerNotes();
    for (auto& scheduler : schedulers) scheduler.reset();
    sequencer.reset();
    activePlayedNote.clear();
    currentSchedulerIndex = ArrangerStyleDefaults::noIndex;
    pendingStartIndex = ArrangerStyleDefaults::noIndex;
    playheadBeats = noBeats;
    lastReportedSectionIndex = ArrangerStyleDefaults::noIndex;
    if (onElapsedBeats)   // reset the beat bar to the downbeat, like the classic player does
        juce::MessageManager::callAsync ([this] { if (onElapsedBeats) onElapsedBeats (noBeats); });

    // Only when we were actually playing: re-arm the first variation (sequencer.reset() landed on it)
    // and tell the UI we stopped, so the buttons clear the Ending and the beat bar leaves its play state.
    if (wasPlaying)
    {
        notifyActiveSection (true);
        if (onStoppedItself)
            juce::MessageManager::callAsync ([this] { notifyStoppedUnlessRestarted(); });
    }
}

void ArrangerEngine::notifyStoppedUnlessRestarted()
{
    // Start pressed while playing stops and restarts at once; this notification then arrives after the
    // restart and would mark a playing engine as stopped (BPM unlocked, beat bar reset).
    if (playing.load())
        return;

    if (onStoppedItself)
        onStoppedItself();
}

void ArrangerEngine::notifyActiveSection (bool force)
{
    const int activeIndex = sequencer.getActiveIndex();
    if (! force && activeIndex == lastReportedSectionIndex)
        return;
    lastReportedSectionIndex = activeIndex;

    ArrangerSectionType type = ArrangerSectionType::Variation;
    juce::String name;
    if (activeIndex >= firstSection && activeIndex < (int) style.sections.size())
    {
        type = style.sections[(size_t) activeIndex].type;
        name = style.sections[(size_t) activeIndex].name;
    }
    if (onActiveSectionChanged)
        juce::MessageManager::callAsync ([this, activeIndex, type, name]
            { if (onActiveSectionChanged) onActiveSectionChanged (activeIndex, type, name); });
}

void ArrangerEngine::hiResTimerCallback()
{
    if (! playing.load())
        return;

    const double now = (double) juce::Time::getHighResolutionTicks()
                       / (double) juce::Time::getHighResolutionTicksPerSecond();

    // Phase 6: Synchro Start — hold at the downbeat until the first chord clears the gate. Keep the
    // clock fresh so the first tick after release has a normal ~10ms delta (the groove starts clean).
    if (synchroArmed.load())
    {
        lastNowSeconds = now;
        return;
    }

    // Accumulate beats from the per-tick wall-clock delta so that a BPM change mid-playback
    // changes the rate going forward without jumping the musical position.
    const double deltaSeconds = now - lastNowSeconds;
    lastNowSeconds = now;

    const double deltaBeats = ArrangerTime::secondsToBeats (deltaSeconds, currentBpm);

    // Phase 6b: count-in pre-roll — play one bar of metronome clicks before the section advances.
    if (countingIn.load())
    {
        renderCountIn (deltaBeats);
        return;   // hold the section at beat 0 until the count-in bar elapses
    }

    // (A UI-queued section switch is drained inside renderRange, below.)
    const double from = playheadBeats;
    const double to   = playheadBeats + deltaBeats;

    renderRange (from, to);
    playheadBeats = to;

    notifyActiveSection (false);   // highlight the live button for the section now sounding

    if (timerShouldStop)
    {
        timerShouldStop = false;
        juce::MessageManager::callAsync ([this] { stop(); }); // stopTimer() safely off the timer thread
        return;
    }

    if (onElapsedBeats)
    {
        const double beats = playheadBeats;
        juce::MessageManager::callAsync ([this, beats] { if (onElapsedBeats) onElapsedBeats (beats); });
    }
}
