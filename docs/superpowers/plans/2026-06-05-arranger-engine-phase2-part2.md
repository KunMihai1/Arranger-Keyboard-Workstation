> Part 2 of 2 of [2026-06-05-arranger-engine-phase2](2026-06-05-arranger-engine-phase2.md). Previous: [part 1](2026-06-05-arranger-engine-phase2.md).

## Task 4: ArrangerEngine — multi-section playback

**Context:** the Phase-1 engine holds a single `ArrangerScheduler scheduler` and renders it directly (`Source/Arranger/ArrangerEngine.cpp`). Phase 2 replaces it with one scheduler per section plus the sequencer. `renderRange` walks the sequencer's segments; on every section change it flushes the outgoing scheduler's hung notes and resets the incoming one (so each section enters clean at its bar 0). An Ending stop arriving on the timer thread must NOT call `stopTimer()` inline (that deadlocks `HighResolutionTimer`); it silences audio immediately and defers the timer stop to the message thread.

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.h`, `Source/Arranger/ArrangerEngine.cpp`
- Modify: `tests/unit/test_arranger_engine.cpp`

- [ ] **Step 1: Add the failing tests**

In `tests/unit/test_arranger_engine.cpp`, add a second style factory below `makeStyle()`:
```cpp
    // Two one-bar variations on channel 2. Variation 1 holds a note across the bar
    // (no note-off) so a section switch must flush it; Variation 2 plays a short note.
    static ArrangerStyle makeTwoVarStyle()
    {
        ArrangerTrack v1; v1.channel = 2; v1.partType = ArrangerPartType::Acc;
        v1.pattern = { { 0.0, juce::MidiMessage::noteOn (2, 60, (juce::uint8) 100) } }; // held
        ArrangerSection s1; s1.id = "var_1"; s1.name = "Variation 1";
        s1.type = ArrangerSectionType::Variation; s1.lengthBars = 1; s1.tracks.push_back (v1);

        ArrangerTrack v2; v2.channel = 2; v2.partType = ArrangerPartType::Acc;
        v2.pattern = { { 0.0, juce::MidiMessage::noteOn  (2, 67, (juce::uint8) 100) },
                       { 0.5, juce::MidiMessage::noteOff (2, 67) } };
        ArrangerSection s2; s2.id = "var_2"; s2.name = "Variation 2";
        s2.type = ArrangerSectionType::Variation; s2.lengthBars = 1; s2.tracks.push_back (v2);

        ArrangerStyle st; st.timeSigNum = 4; st.timeSigDenom = 4; st.originalTempo = 120.0;
        st.sections.push_back (s1); st.sections.push_back (s2);
        return st;
    }

    static ArrangerStyle makeEndingStyle()
    {
        ArrangerTrack v; v.channel = 2; v.partType = ArrangerPartType::Acc;
        v.pattern = { { 0.0, juce::MidiMessage::noteOn  (2, 60, (juce::uint8) 100) },
                      { 0.5, juce::MidiMessage::noteOff (2, 60) } };
        ArrangerSection var; var.id = "var_1"; var.name = "Variation 1";
        var.type = ArrangerSectionType::Variation; var.lengthBars = 1; var.tracks.push_back (v);

        ArrangerSection end; end.id = "ending_1"; end.name = "Ending 1";
        end.type = ArrangerSectionType::Ending; end.lengthBars = 1;
        end.afterComplete = ArrangerAfterComplete::Stop; end.tracks.push_back (v);

        ArrangerStyle st; st.timeSigNum = 4; st.timeSigDenom = 4; st.originalTempo = 120.0;
        st.sections.push_back (var); st.sections.push_back (end);
        return st;
    }
```
Then inside `runTest()` add:
```cpp
        beginTest ("queueSection switches active section only after the bar boundary");
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeTwoVarStyle());
            engine.queueSection (ArrangerSectionType::Variation, "Variation 2");

            expectEquals (engine.getActiveSectionIndex(), 0);
            engine.renderRange (0.0, 2.0);                    // no boundary yet
            expectEquals (engine.getActiveSectionIndex(), 0);
            engine.renderRange (2.0, 5.0);                    // crosses beat 4
            expectEquals (engine.getActiveSectionIndex(), 1);
        }

        beginTest ("a section swap flushes the outgoing section's held note (no hung notes)");
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& m) { captured.push_back (m); };

            engine.setStyle (makeTwoVarStyle());
            engine.queueSection (ArrangerSectionType::Variation, "Variation 2");
            engine.renderRange (0.0, 1.0);   // Variation 1 note 60 on (held)
            engine.renderRange (1.0, 5.0);   // boundary -> swap to Variation 2

            int offIdx = -1, onIdx = -1;
            for (int i = 0; i < (int) captured.size(); ++i)
            {
                if (captured[i].isNoteOff() && captured[i].getNoteNumber() == 60) offIdx = i;
                if (captured[i].isNoteOn()  && captured[i].getNoteNumber() == 67 && onIdx < 0) onIdx = i;
            }
            expect (offIdx >= 0);            // held note 60 was closed at the swap
            expect (onIdx  >= 0);            // Variation 2 note 67 started
            expect (offIdx < onIdx);         // closed before the new section sounded
        }

        beginTest ("an Ending completing silences output (all-notes-off)");
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            int allNotesOff = 0;
            engine.onMidiMessage = [&] (const juce::MidiMessage& m)
            {
                if (m.isAllNotesOff()) ++allNotesOff;
            };
            engine.setStyle (makeEndingStyle());
            engine.queueSection (ArrangerSectionType::Ending, "Ending 1");
            engine.renderRange (0.0, 5.0);   // enter ending at beat 4
            engine.renderRange (5.0, 9.0);   // ending completes at beat 8 -> stop
            expect (allNotesOff >= 1);
        }

        beginTest ("selectStartSection makes start begin on the chosen section");
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeTwoVarStyle());
            engine.selectStartSection (ArrangerSectionType::Variation, "Variation 2");
            // start() begins the real timer; instead exercise the selection deterministically:
            expectEquals (engine.peekPendingStartIndex(), 1);
        }
```

- [ ] **Step 2: Update the header**

In `Source/Arranger/ArrangerEngine.h`:

Add the include below `#include "ArrangerScheduler.h"`:
```cpp
#include "ArrangerSectionSequencer.h"
#include <vector>
```
In the `public:` section, after `void setBpm (double bpm);`, add:
```cpp
    /** Queue a section change (applied at the next bar boundary while playing). */
    void queueSection (ArrangerSectionType type, const juce::String& name);
    /** Choose which section a subsequent start() begins on (used while stopped). */
    void selectStartSection (ArrangerSectionType type, const juce::String& name);

    int getActiveSectionIndex() const;
    int peekPendingStartIndex() const { return pendingStartIndex; } // test seam
```
Replace the member declarations:
```cpp
    ArrangerScheduler scheduler;
```
with:
```cpp
    std::vector<ArrangerScheduler> schedulers;       // one per section
    ArrangerSectionSequencer       sequencer;
    int  currentSchedulerIndex = -1;                 // which scheduler is currently sounding
    int  pendingStartIndex     = -1;                 // section start() should begin on (-1 = default)
    bool timerShouldStop       = false;              // set on the timer thread, honoured on the msg thread
```
Replace the private method declaration `void rebuildScheduler();` with:
```cpp
    void rebuildFromStyle();
    void updateActiveLoopLength();
    void haltAudio();                 // silence + reset state, WITHOUT stopping the timer
    int  indexOfSection (ArrangerSectionType type, const juce::String& name) const;
```

- [ ] **Step 3: Rewrite the affected source**

In `Source/Arranger/ArrangerEngine.cpp`:

Replace the whole `rebuildScheduler()` method with:
```cpp
void ArrangerEngine::rebuildFromStyle()
{
    schedulers.clear();
    const double bpb = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);

    for (const auto& sec : style.sections)
    {
        std::vector<TimedBeatEvent> merged;
        for (const auto& tr : sec.tracks)
            for (const auto& ev : tr.pattern)
                merged.push_back (ev);

        const double bars = (double) juce::jmax (1, sec.lengthBars);
        ArrangerScheduler s;
        s.setLoop (std::move (merged), bars * bpb);
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
    int firstOfType = -1, nameMatch = -1;
    for (int i = 0; i < (int) style.sections.size(); ++i)
    {
        if (style.sections[i].type != type) continue;
        if (firstOfType < 0) firstOfType = i;
        if (style.sections[i].name == name) nameMatch = i;
    }
    return (nameMatch >= 0) ? nameMatch : firstOfType;
}

int ArrangerEngine::getActiveSectionIndex() const
{
    return sequencer.getActiveIndex();
}

void ArrangerEngine::queueSection (ArrangerSectionType type, const juce::String& name)
{
    sequencer.queue (type, name);
}

void ArrangerEngine::selectStartSection (ArrangerSectionType type, const juce::String& name)
{
    pendingStartIndex = indexOfSection (type, name);
}
```
Change `setStyle()` to call the new builder name:
```cpp
void ArrangerEngine::setStyle (ArrangerStyle newStyle)
{
    style = std::move (newStyle);
    if (style.originalTempo > 0.0) currentBpm = style.originalTempo;
    rebuildFromStyle();
}
```
Update `sendInstrumentSetup()` to cover every section's tracks (not just the first):
```cpp
void ArrangerEngine::sendInstrumentSetup()
{
    // Select each track's instrument + channel volume across all sections, so any section
    // we switch to already has the right sound. Drum channel (10) keeps its fixed kit.
    for (const auto& sec : style.sections)
        for (const auto& tr : sec.tracks)
        {
            if (tr.instrument >= 0 && tr.channel != 10)
                dispatch (juce::MidiMessage::programChange (tr.channel, tr.instrument));
            dispatch (juce::MidiMessage::controllerEvent (tr.channel, 7, juce::jlimit (0, 127, (int) tr.volume)));
        }
}
```
Replace `renderRange()` with the multi-section walk:
```cpp
void ArrangerEngine::renderRange (double fromBeats, double toBeats)
{
    if (schedulers.empty())
        return;

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
                    dispatch (e.message);
                schedulers[currentSchedulerIndex].reset();
            }
            currentSchedulerIndex = seg.sectionIndex;
            if (currentSchedulerIndex >= 0 && currentSchedulerIndex < (int) schedulers.size())
                schedulers[currentSchedulerIndex].reset();
        }

        if (seg.sectionIndex >= 0 && seg.sectionIndex < (int) schedulers.size())
            for (auto& e : schedulers[seg.sectionIndex].advance (seg.localFromBeats, seg.localToBeats))
                dispatch (e.message);
    }

    if (step.stopRequested)
    {
        haltAudio();            // silence + reset, but do not touch the timer here
        timerShouldStop = true; // the timer callback will stop the timer on the message thread
        return;
    }

    updateActiveLoopLength();
}
```
Replace `start()` with:
```cpp
void ArrangerEngine::start()
{
    if (schedulers.empty() || loopLengthBeats <= 0.0)
        return;

    for (auto& s : schedulers) s.reset();
    sequencer.reset();
    if (pendingStartIndex >= 0)
        sequencer.startAt (pendingStartIndex);
    pendingStartIndex = -1;
    currentSchedulerIndex = -1;
    timerShouldStop = false;

    sendInstrumentSetup();
    playheadBeats = 0.0;
    lastNowSeconds = (double) juce::Time::getHighResolutionTicks()
                     / (double) juce::Time::getHighResolutionTicksPerSecond();
    playing.store (true);
    startTimer (10);
}
```
Replace `stop()` with a thin wrapper over `haltAudio()` that also stops the timer (called only from the message thread):
```cpp
void ArrangerEngine::stop()
{
    stopTimer();
    haltAudio();
}

void ArrangerEngine::haltAudio()
{
    playing.store (false);
    sendAllNotesOff();
    for (auto& s : schedulers) s.reset();
    sequencer.reset();
    currentSchedulerIndex = -1;
    pendingStartIndex = -1;
    playheadBeats = 0.0;
    if (onElapsedBeats)
        juce::MessageManager::callAsync ([this] { if (onElapsedBeats) onElapsedBeats (0.0); });
}
```
Replace `hiResTimerCallback()` with a version that guards re-entry and defers the timer stop:
```cpp
void ArrangerEngine::hiResTimerCallback()
{
    if (! playing.load())
        return;

    const double now = (double) juce::Time::getHighResolutionTicks()
                       / (double) juce::Time::getHighResolutionTicksPerSecond();
    const double deltaSeconds = now - lastNowSeconds;
    lastNowSeconds = now;

    const double deltaBeats = ArrangerTime::secondsToBeats (deltaSeconds, currentBpm);
    const double from = playheadBeats;
    const double to   = playheadBeats + deltaBeats;

    renderRange (from, to);
    playheadBeats = to;

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
```

- [ ] **Step 4: Build and test**

Run `[BUILD]` then `[TEST]` (no `[RESAVE]` — no files added).
Expected: `ArrangerEngine | ... | failures: 0`, and the Phase-1 `ArrangerEngine` assertions still pass, `ExitCode = 0`.

- [ ] **Step 5: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp tests/unit/test_arranger_engine.cpp
git commit -m "feat(arranger): multi-section engine with bar-synced switching and seam flush"
```

---

## Task 5: ArrangerPatternBuilder — demo multi-section style

**Context:** `buildSingleSectionStyle` (`Source/Arranger/ArrangerPatternBuilder.cpp:26`) already produces a correct "Variation 1" (channel mapping perc→10/melodic→2,3,…, loop length). We reuse it and derive Intro/Fill/Ending by cloning that section and changing only the `type`, `afterComplete`, and (for Fill) slicing to the last bar.

**Files:**
- Modify: `Source/Arranger/ArrangerPatternBuilder.h`, `Source/Arranger/ArrangerPatternBuilder.cpp`
- Modify: `tests/unit/test_arranger_pattern_builder.cpp`

- [ ] **Step 1: Add the failing test**

In `tests/unit/test_arranger_pattern_builder.cpp`, inside `runTest()` add:
```cpp
        beginTest ("demo multi-section style: four sections with the right transport rules");
        {
            // melodic note ends at 2.0s -> 4 beats @120 -> 1 bar; perc hit ends at 3.0s -> 2 bars.
            TrackEntry melodic;
            melodic.type = TrackType::Melodic;
            melodic.originalBPM = 120.0;
            melodic.sequence.addEvent (juce::MidiMessage::noteOn  (1, 60, (juce::uint8) 100), 0.0);
            melodic.sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 2.0);

            TrackEntry perc;
            perc.type = TrackType::Percussion;
            perc.originalBPM = 120.0;
            perc.sequence.addEvent (juce::MidiMessage::noteOn  (1, 36, (juce::uint8) 100), 5.0); // 10 beats -> bar 3 area
            perc.sequence.addEvent (juce::MidiMessage::noteOff (1, 36), 5.5);

            std::vector<TrackEntry> tracks { melodic, perc };
            ArrangerStyle style = ArrangerPatternBuilder::buildDemoMultiSectionStyle (tracks, 4, 4, 120.0);

            expectEquals (style.schemaVersion, 2);
            expectEquals ((int) style.sections.size(), 4);

            // order: Intro, Variation, Fill, Ending
            expect (style.sections[0].type == ArrangerSectionType::Intro);
            expect (style.sections[1].type == ArrangerSectionType::Variation);
            expect (style.sections[2].type == ArrangerSectionType::Fill);
            expect (style.sections[3].type == ArrangerSectionType::Ending);

            expect (style.sections[0].afterComplete == ArrangerAfterComplete::FallThrough); // intro
            expect (style.sections[1].afterComplete == ArrangerAfterComplete::Loop);        // variation
            expect (style.sections[2].afterComplete == ArrangerAfterComplete::FallThrough); // fill
            expect (style.sections[3].afterComplete == ArrangerAfterComplete::Stop);        // ending

            expectEquals (style.sections[2].lengthBars, 1); // fill is exactly one bar

            // Variation keeps the Phase-1 channel mapping: melodic -> 2, perc -> 10.
            expectEquals (style.sections[1].tracks[0].channel, 2);
            expectEquals (style.sections[1].tracks[1].channel, 10);
        }

        beginTest ("fill slices the loop's last bar and rebases it to beat 0");
        {
            // single 2-bar loop: a melodic hit in the last bar (beat ~6) must reappear in the fill near beat 2.
            TrackEntry t;
            t.type = TrackType::Melodic;
            t.originalBPM = 120.0;
            t.sequence.addEvent (juce::MidiMessage::noteOn  (1, 72, (juce::uint8) 100), 3.0); // 6 beats @120 -> bar 2
            t.sequence.addEvent (juce::MidiMessage::noteOff (1, 72), 3.5);

            std::vector<TrackEntry> tracks { t };
            ArrangerStyle style = ArrangerPatternBuilder::buildDemoMultiSectionStyle (tracks, 4, 4, 120.0);

            expectEquals (style.sections[1].lengthBars, 2);          // variation is 2 bars
            auto& fillTrack = style.sections[2].tracks[0];
            expect (! fillTrack.pattern.empty());
            // event at beat 6 in a 2-bar (8-beat) loop -> fill (last bar starts at beat 4) -> beat 2.
            expectWithinAbsoluteError (fillTrack.pattern.front().beats, 2.0, 1e-9);
        }
```

- [ ] **Step 2: Declare the builder**

In `Source/Arranger/ArrangerPatternBuilder.h`, after the `buildSingleSectionStyle` declaration, add:
```cpp
    /** Phase-2 demo: derive Intro/Variation/Fill/Ending from the single loop produced by
        buildSingleSectionStyle. Transport rules are real (one-shot vs loop); distinct
        per-section audio is Phase 3. Fill = the loop's last bar. Order: Intro, Variation,
        Fill, Ending (the engine defaults to the first Variation). */
    ArrangerStyle buildDemoMultiSectionStyle (const std::vector<TrackEntry>& tracks,
                                              int timeSigNum, int timeSigDenom,
                                              double referenceBpm);
```

- [ ] **Step 3: Implement it**

In `Source/Arranger/ArrangerPatternBuilder.cpp`, add at the end of the namespace block (after `buildSingleSectionStyle`):
```cpp
    ArrangerStyle buildDemoMultiSectionStyle (const std::vector<TrackEntry>& tracks,
                                              int timeSigNum, int timeSigDenom,
                                              double referenceBpm)
    {
        ArrangerStyle base = buildSingleSectionStyle (tracks, timeSigNum, timeSigDenom, referenceBpm);

        ArrangerStyle style;
        style.timeSigNum    = timeSigNum;
        style.timeSigDenom  = timeSigDenom;
        style.originalTempo = base.originalTempo;

        if (base.sections.empty())
            return style;

        const double bpb     = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);
        ArrangerSection var  = base.sections.front();     // the full loop, correct channels/length
        const int loopBars   = juce::jmax (1, var.lengthBars);

        var.id = "var_1"; var.name = "Variation 1";
        var.type = ArrangerSectionType::Variation;
        var.afterComplete = ArrangerAfterComplete::Loop;

        ArrangerSection intro = var;
        intro.id = "intro_1"; intro.name = "Intro 1";
        intro.type = ArrangerSectionType::Intro;
        intro.afterComplete = ArrangerAfterComplete::FallThrough;

        ArrangerSection ending = var;
        ending.id = "ending_1"; ending.name = "Ending 1";
        ending.type = ArrangerSectionType::Ending;
        ending.afterComplete = ArrangerAfterComplete::Stop;

        ArrangerSection fill = var;
        fill.id = "fill_1"; fill.name = "Fill 1";
        fill.type = ArrangerSectionType::Fill;
        fill.afterComplete = ArrangerAfterComplete::FallThrough;
        fill.lengthBars = 1;
        {
            const double lastBarStart = (double) (loopBars - 1) * bpb;
            for (auto& tr : fill.tracks)
            {
                std::vector<TimedBeatEvent> sliced;
                for (const auto& ev : tr.pattern)
                    if (ev.beats >= lastBarStart - 1e-9)
                        sliced.push_back ({ ev.beats - lastBarStart, ev.message });
                tr.pattern = std::move (sliced);
            }
        }

        style.sections.push_back (std::move (intro));
        style.sections.push_back (std::move (var));
        style.sections.push_back (std::move (fill));
        style.sections.push_back (std::move (ending));
        return style;
    }
```

- [ ] **Step 4: Build and test**

Run `[BUILD]` then `[TEST]` (no `[RESAVE]`).
Expected: `ArrangerPatternBuilder | ... | failures: 0`, `ExitCode = 0`.

- [ ] **Step 5: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerPatternBuilder.h Source/Arranger/ArrangerPatternBuilder.cpp tests/unit/test_arranger_pattern_builder.cpp
git commit -m "feat(arranger): derive Intro/Variation/Fill/Ending demo sections from the loop"
```

---

## Task 6: Wire the section buttons to the engine

**Context (verified):** `startPlaying()` already branches on `arrangerModeEnabled` and builds via `buildSingleSectionStyle` (`Source/CurrentStyleComponent.cpp:55-63`). The five section handlers (`handleIntro/Ending/Var/Fill/BreakCurrentStyle`, `CurrentStyleComponent.cpp:649-677`) currently only call `trackPlayer->setLastSectionUsed(...)`. `arrangerEngine` and `arrangerModeEnabled` are existing members (`CurrentStyleComponent.h:254-255`). `ArrangerSectionType` comes from `ArrangerModel.h`, already pulled in via `Arranger/ArrangerPatternBuilder.h` (included at `CurrentStyleComponent.cpp:3`). This is a GUI/integration task — verified by build + the manual procedure in Task 7.

- [ ] **Step 1: Use the multi-section builder in `startPlaying()`**

In `Source/CurrentStyleComponent.cpp`, in the `if (arrangerModeEnabled)` block (around line 55-63), replace the `buildSingleSectionStyle` line:
```cpp
        ArrangerStyle style = ArrangerPatternBuilder::buildSingleSectionStyle(selectedTracks, 4, 4, currentTempo);
```
with:
```cpp
        ArrangerStyle style = ArrangerPatternBuilder::buildDemoMultiSectionStyle(selectedTracks, 4, 4, currentTempo);
```

- [ ] **Step 2: Add a small routing helper**

In `Source/CurrentStyleComponent.cpp`, just above `handleIntroCurrentStyle` (line 649), add a file-local helper that queues while playing and selects-for-start while stopped:
```cpp
namespace
{
    void routeSectionToArranger (ArrangerEngine* engine, ArrangerSectionType type, const juce::String& name)
    {
        if (engine == nullptr)
            return;
        if (engine->isPlaying())
            engine->queueSection (type, name);
        else
            engine->selectStartSection (type, name);
    }
}
```

- [ ] **Step 3: Branch each handler on arranger mode**

In `Source/CurrentStyleComponent.cpp`, add an arranger-mode branch at the **top** of each handler (before the existing `trackPlayer->setLastSectionUsed(...)` line). Use the matching `ArrangerSectionType` per handler:

`handleIntroCurrentStyle`:
```cpp
    if (arrangerModeEnabled)
    {
        routeSectionToArranger (arrangerEngine.get(), ArrangerSectionType::Intro, name);
        return;
    }
```
`handleEndingCurrentStyle`:
```cpp
    if (arrangerModeEnabled)
    {
        routeSectionToArranger (arrangerEngine.get(), ArrangerSectionType::Ending, name);
        return;
    }
```
`handleVarCurrentStyle`:
```cpp
    if (arrangerModeEnabled)
    {
        routeSectionToArranger (arrangerEngine.get(), ArrangerSectionType::Variation, name);
        return;
    }
```
`handleFillCurrentStyle`:
```cpp
    if (arrangerModeEnabled)
    {
        routeSectionToArranger (arrangerEngine.get(), ArrangerSectionType::Fill, name);
        return;
    }
```
`handleBreakCurrentStyle`:
```cpp
    if (arrangerModeEnabled)
    {
        routeSectionToArranger (arrangerEngine.get(), ArrangerSectionType::Break, name);
        return;
    }
```

- [ ] **Step 4: Build**

Run `[BUILD]` (no `[RESAVE]`).
Expected: `Build succeeded.` Fix any name mismatch against the real handler signatures if the build complains.

- [ ] **Step 5: Commit** (ask approval first)
```powershell
git add Source/CurrentStyleComponent.cpp
git commit -m "feat(arranger): wire section buttons to the arranger engine in arranger mode"
```

---

## Task 7: Manual verification & docs

**Files:**
- Modify: `docs/superpowers/specs/2026-06-05-arranger-engine-phase2-design.md` (status line)
- Modify: `docs/superpowers/specs/2026-06-05-arranger-engine-design.md` (roadmap: mark Phase 2 delivered)

- [ ] **Step 1: Full build**

Run `[RESAVE]` then `[BUILD]`. Expected: `Build succeeded.`

- [ ] **Step 2: Full Arranger test sweep**

Run `[TEST]`. Expected: every `Arranger`-category test (`ArrangerModel`, `ArrangerTime`, `ArrangerScheduler`, `ArrangerSectionSequencer`, `ArrangerPatternBuilder`, `ArrangerEngine`) shows `failures: 0`, `ExitCode = 0`.

- [ ] **Step 3: Manual smoke test (the real proof)**

Launch the app:
```powershell
& "Builds\VisualStudio2022\x64\Debug\App\Project Synth2.exe"
```
With **Arranger mode ON** (Settings → tick it), select a style, then verify in order:
1. Press **Start** → Variation 1 loops seamlessly; beat bar advances; no hung notes after several loops.
2. Press **Fill** while playing → at the next bar a one-bar fill plays, then it returns to Variation 1. No stuck notes at either seam.
3. Press **Stop**. Press **Intro**, then **Start** → the intro plays once, then drops into Variation 1 and keeps looping.
4. Press **Ending** while playing → the ending plays once, then playback **stops on its own** (silence, beat bar resets).
5. Toggle Arranger mode **OFF** → Start → classic one-shot playback behaves exactly as before. ✅ no regression.

Record pass/fail per item in the task notes.

- [ ] **Step 4: Update doc statuses**

In `docs/superpowers/specs/2026-06-05-arranger-engine-phase2-design.md`, set `**Status:**` to `Phase 2 implemented`.
In `docs/superpowers/specs/2026-06-05-arranger-engine-design.md`, mark roadmap item 2 ("Section state machine + transport") as delivered (prefix ✅) and update the top `**Status:**` line accordingly.

- [ ] **Step 5: Commit** (ask approval first — note `docs/` is git-ignored, so this commit may be empty; that is expected)
```powershell
git add docs/superpowers/specs/2026-06-05-arranger-engine-phase2-design.md docs/superpowers/specs/2026-06-05-arranger-engine-design.md
git commit -m "docs(arranger): mark Phase 2 implemented"
```

---

## Notes for the executor

- **Hung notes are the one thing to obsess over.** Two seams now matter: the loop wrap (Phase 1, still handled by `ArrangerScheduler::advance`) and the **section swap** (Phase 2, handled by `flushActiveNotes` + reset in `ArrangerEngine::renderRange`). If the manual test ever leaves a note ringing on a section change, re-check Task 4's seam-flush block and Task 2's `flushActiveNotes`.
- **Never call `stopTimer()` from the timer thread.** The Ending→stop path silences audio inline (`haltAudio`) and defers `stopTimer()` to the message thread via `callAsync`. Don't "simplify" this back into an inline `stop()` — it deadlocks `juce::HighResolutionTimer`.
- **Builds are the slow step.** Only run `[RESAVE]` when files were added/removed (Task 3 only). Otherwise a plain `[BUILD]` incremental compile is faster.
- **Demo content is the only throwaway part.** `buildDemoMultiSectionStyle` is replaced wholesale in Phase 3; the sequencer and engine are permanent. Keep section-switching logic out of the builder.
- **Commit policy:** pause for user approval at each commit step (project memory).
```
