#include "ArrangerEngine.h"
#include "ArrangerTime.h"
#include "ArrangerCountIn.h"

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
    if (bpm > 0.0) currentBpm = bpm;
}

void ArrangerEngine::rebuildFromStyle()
{
    schedulers.clear();
    const double bpb = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);

    for (const auto& sec : style.sections)
    {
        std::vector<TimedBeatEvent> merged;
        std::vector<PartKind>       parts;
        std::vector<NttType>        ntts;
        for (const auto& tr : sec.tracks)
        {
            // Drums/Perc never transpose; Bass honours bass inversion; Acc is plain harmony.
            const PartKind pk = (tr.partType == ArrangerPartType::Bass) ? PartKind::Bass
                              : (tr.partType == ArrangerPartType::Acc)  ? PartKind::Acc
                                                                        : PartKind::Fixed;
            for (const auto& ev : tr.pattern)
            {
                merged.push_back (ev);
                parts.push_back (pk);
                ntts.push_back (tr.nttType);   // Phase 7a: per-track NTT type
            }
        }

        const double bars = (double) juce::jmax (1, sec.lengthBars);
        ArrangerScheduler s;
        s.setLoop (std::move (merged), std::move (parts), std::move (ntts), bars * bpb);
        schedulers.push_back (std::move (s));
    }

    sequencer.setStyle (style);
    currentSchedulerIndex = -1;
    updateActiveLoopLength();
}

void ArrangerEngine::updateActiveLoopLength()
{
    if (style.sections.empty())
    {
        loopLengthBeats = 0.0;
        return;
    }
    const double bpb = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);
    const int idx = juce::jlimit (0, (int) style.sections.size() - 1, sequencer.getActiveIndex());
    loopLengthBeats = (double) juce::jmax (1, style.sections[idx].lengthBars) * bpb;
}

int ArrangerEngine::indexOfSection (ArrangerSectionType type, const juce::String& name) const
{
    // Match by type + trailing number (like the sequencer's findSection), so the live button labels
    // ("Var 2") resolve to the matching section ("Variation 2") even though the strings differ.
    const int wantN = name.getTrailingIntValue();
    int firstOfType = -1, numMatch = -1;
    for (int i = 0; i < (int) style.sections.size(); ++i)
    {
        if (style.sections[i].type != type) continue;
        if (firstOfType < 0) firstOfType = i;
        if (style.sections[i].name.getTrailingIntValue() == wantN) numMatch = i;
    }
    return (numMatch >= 0) ? numMatch : firstOfType;
}

int ArrangerEngine::getActiveSectionIndex() const
{
    return sequencer.getActiveIndex();
}

void ArrangerEngine::queueSection (ArrangerSectionType type, const juce::String& name)
{
    // Called from the message thread. Hand the request to the timer thread instead of touching the
    // sequencer directly (it's advanced on the timer thread; a direct write here is a data race).
    const juce::ScopedLock sl (requestLock);
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
    if (style.originalTempo > 0.0) currentBpm = style.originalTempo;
    rebuildFromStyle();
    // The recorded chord the NTT maps out of (style-level). bassNote = root (no inversion baked in).
    transposer.setOriginalChord ({ style.originalRoot, style.originalQuality, style.originalRoot });

    // Start every Start/Play in the style's OWN home key (setStyle runs on each Start). Live chord
    // changes during playback persist; pressing Start again resets to home rather than keeping the
    // last chord, which is what a performer expects.
    transposer.setActiveChord ({});
    {
        const juce::ScopedLock sl (chordLock);
        pendingChord   = {};
        hasChordUpdate = false;
    }

    if (wasPlaying)
    {
        // rebuildFromStyle() resets the sequencer to the first section at activeStartAbs=0. Realign the
        // engine playhead to 0 as well -- otherwise the resumed groove picks up at (old playhead % loop
        // length), an arbitrary mid-loop phase that sounds like a glitchy jump. Zeroing it makes the
        // rebuilt preview resume cleanly from the section's downbeat.
        playheadBeats  = 0.0;
        // Don't let the rebuild's wall-clock duration count as elapsed musical time:
        lastNowSeconds = (double) juce::Time::getHighResolutionTicks()
                         / (double) juce::Time::getHighResolutionTicksPerSecond();
        startTimer (10);
    }
}

void ArrangerEngine::setActiveChord (ArrangerChord played)
{
    // Called from the MIDI input thread. Hand the chord to the timer thread via a guarded mailbox;
    // it is applied to the transposer at the top of renderRange (timer thread only).
    {
        const juce::ScopedLock sl (chordLock);
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

void ArrangerEngine::dispatch (const juce::MidiMessage& m)
{
    if (auto out = outputDevice.lock())
        out->sendMessageNow (m);
    if (onMidiMessage)
        onMidiMessage (m);
}

void ArrangerEngine::dispatchEmitted (const EmittedEvent& e)
{
    juce::MidiMessage m = e.message;

    if (m.isNoteOn() && m.getVelocity() > 0 && e.part != PartKind::Fixed)
    {
        // Transpose the pitched note and remember the played pitch, keyed by its ORIGINAL note, so
        // the matching note-off (which carries the original pitch) closes the same sounding note even
        // if the chord changed in between.
        const int orig   = m.getNoteNumber();
        const int played = transposer.transpose (orig, e.part, e.ntt);
        activePlayedNote[{ m.getChannel(), orig }] = played;
        m = juce::MidiMessage::noteOn (m.getChannel(), played, m.getVelocity());
    }
    else if (m.isNoteOff() || (m.isNoteOn() && m.getVelocity() == 0))
    {
        const auto key = std::make_pair (m.getChannel(), m.getNoteNumber());
        const auto it  = activePlayedNote.find (key);
        if (it != activePlayedNote.end())
        {
            m = juce::MidiMessage::noteOff (m.getChannel(), it->second);
            activePlayedNote.erase (it);
        }
    }

    dispatch (m);
}

void ArrangerEngine::silenceArrangerNotes()
{
    // Silence ONLY the notes the arranger itself turned on. A blanket all-notes-off across channels
    // 1-16 (the old behaviour) also killed the player's manually-held notes -- e.g. a chord held
    // while pressing Start went silent. Flushing each scheduler emits note-offs for its active notes
    // keyed by their ORIGINAL pitch; routing them through dispatchEmitted closes the matching SOUNDING
    // note for pitched parts (and clears activePlayedNote) while leaving drums/Fixed as-is. Any pitched
    // note still tracked (e.g. its scheduler was already reset) is then closed directly.
    for (auto& s : schedulers)
        for (auto& e : s.flushActiveNotes (0.0))
            dispatchEmitted (e);

    for (const auto& entry : activePlayedNote)
        dispatch (juce::MidiMessage::noteOff (entry.first.first, entry.second));
    activePlayedNote.clear();
}

void ArrangerEngine::sendInstrumentSetup()
{
    // Select each track's instrument + channel volume across all sections, so any section
    // we switch to already has the right sound. Drum channel (10) keeps its fixed kit. The
    // SFZ engine ignores program-change but honours CC7 volume; an external synth honours both.
    for (const auto& sec : style.sections)
        for (const auto& tr : sec.tracks)
        {
            if (tr.instrument >= 0 && tr.channel != 10)
                dispatch (juce::MidiMessage::programChange (tr.channel, tr.instrument));

            dispatch (juce::MidiMessage::controllerEvent (tr.channel, 7, juce::jlimit (0, 127, (int) tr.volume)));
        }
}

void ArrangerEngine::armCountIn()
{
    countInPlayhead    = 0.0;
    countInLengthBeats = (double) juce::jmax (1, style.timeSigNum);   // one bar
    countingIn.store (true);
}

bool ArrangerEngine::renderCountIn (double deltaBeats)
{
    if (! countingIn.load())
        return false;

    const double from = countInPlayhead;
    const double to   = countInPlayhead + deltaBeats;
    for (const auto& c : ArrangerCountIn::clicksInWindow (from, to, countInLengthBeats))
        dispatch (juce::MidiMessage::noteOn (c.channel, c.note, (juce::uint8) c.velocity));

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
        const juce::ScopedLock sl (requestLock);
        if (hasQueuedRequest)
        {
            sequencer.queue (requestedType, requestedName);
            hasQueuedRequest = false;
        }
    }

    // Apply the latest played chord (from the MIDI input thread) before emitting this window's notes.
    {
        const juce::ScopedLock sl (chordLock);
        if (hasChordUpdate)
        {
            transposer.setActiveChord (pendingChord);
            hasChordUpdate = false;
        }
    }

    SequencerStep step = sequencer.advance (fromBeats, toBeats);

    for (const auto& seg : step.segments)
    {
        if (seg.sectionIndex != currentSchedulerIndex)
        {
            // Flush the outgoing section's hung notes, then reset both sides so the
            // incoming section enters clean at its own bar 0.
            if (currentSchedulerIndex >= 0 && currentSchedulerIndex < (int) schedulers.size())
            {
                for (auto& e : schedulers[currentSchedulerIndex].flushActiveNotes (0.0))
                    dispatchEmitted (e);
                schedulers[currentSchedulerIndex].reset();
            }
            currentSchedulerIndex = seg.sectionIndex;
            if (currentSchedulerIndex >= 0 && currentSchedulerIndex < (int) schedulers.size())
                schedulers[currentSchedulerIndex].reset();
        }

        if (seg.sectionIndex >= 0 && seg.sectionIndex < (int) schedulers.size())
            for (auto& e : schedulers[seg.sectionIndex].advance (seg.localFromBeats, seg.localToBeats))
                dispatchEmitted (e);
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
    if (schedulers.empty() || loopLengthBeats <= 0.0)
        return;

    // A restart while already playing (e.g. pressing Preview again) must close the previous run's
    // sounding notes first; the reset/clear below would otherwise strand them as an overlapping drone.
    if (playing.load())
        silenceArrangerNotes();

    for (auto& s : schedulers) s.reset();
    sequencer.reset();
    activePlayedNote.clear();
    if (pendingStartIndex >= 0)
        sequencer.startAt (pendingStartIndex);
    pendingStartIndex = -1;
    currentSchedulerIndex = -1;
    lastReportedSectionIndex = -1;   // so the first tick reports the starting section
    timerShouldStop = false;

    sendInstrumentSetup();   // select instruments + volumes before the first notes play
    playheadBeats = 0.0;
    lastNowSeconds = (double) juce::Time::getHighResolutionTicks()
                     / (double) juce::Time::getHighResolutionTicksPerSecond();
    // Transport feel (Synchro Start / Count-In) applies only to live performance. The editor preview
    // passes useTransportFeel=false so it never arms the Synchro gate (which would wait for a chord the
    // preview never sends) or a count-in -- it just plays.
    synchroArmed.store (useTransportFeel && synchroStartEnabled);   // Phase 6: wait for the first chord
    if (useTransportFeel && countInEnabled) armCountIn();           // Phase 6b: pre-roll a count-in bar
    else                                    countingIn.store (false);
    playing.store (true);
    startTimer (10);
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
    for (auto& s : schedulers) s.reset();
    sequencer.reset();
    activePlayedNote.clear();
    currentSchedulerIndex = -1;
    pendingStartIndex = -1;
    playheadBeats = 0.0;
    lastReportedSectionIndex = -1;
    if (onElapsedBeats)   // reset the beat bar to the downbeat, like the classic player does
        juce::MessageManager::callAsync ([this] { if (onElapsedBeats) onElapsedBeats (0.0); });

    // Only when we were actually playing: re-arm the first variation (sequencer.reset() landed on it)
    // and tell the UI we stopped, so the buttons clear the Ending and the beat bar leaves its play state.
    if (wasPlaying)
    {
        notifyActiveSection (true);
        if (onStoppedItself)
            juce::MessageManager::callAsync ([this] { if (onStoppedItself) onStoppedItself(); });
    }
}

void ArrangerEngine::notifyActiveSection (bool force)
{
    const int idx = sequencer.getActiveIndex();
    if (! force && idx == lastReportedSectionIndex)
        return;
    lastReportedSectionIndex = idx;

    ArrangerSectionType type = ArrangerSectionType::Variation;
    juce::String name;
    if (idx >= 0 && idx < (int) style.sections.size())
    {
        type = style.sections[(size_t) idx].type;
        name = style.sections[(size_t) idx].name;
    }
    if (onActiveSectionChanged)
        juce::MessageManager::callAsync ([this, idx, type, name]
            { if (onActiveSectionChanged) onActiveSectionChanged (idx, type, name); });
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
