> Part 2 of 2 of [2026-06-05-arranger-engine-phase1](2026-06-05-arranger-engine-phase1.md). Previous: [part 1](2026-06-05-arranger-engine-phase1.md).

## Task 5: ArrangerPatternBuilder — TrackEntry → ArrangerStyle

**Files:**
- Create: `Source/Arranger/ArrangerPatternBuilder.h`, `Source/Arranger/ArrangerPatternBuilder.cpp`
- Create: `tests/unit/test_arranger_pattern_builder.cpp`
- Modify: `Project Synth2.jucer`

**Context:** `TrackEntry` (see `Source/TrackEntry.h`) holds `sequence` (a `juce::MidiMessageSequence` whose event timestamps are in **seconds**), `originalBPM`, and `type` (`TrackType::Melodic|Percussion`). The classic player maps percussion → channel 10, melodic → channels 2,3,4,… in order. We mirror that.

- [ ] **Step 1: Write the failing tests**

Create `tests/unit/test_arranger_pattern_builder.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerPatternBuilder.h"
#include "TrackEntry.h"

class ArrangerPatternBuilderTest : public juce::UnitTest
{
public:
    ArrangerPatternBuilderTest() : juce::UnitTest ("ArrangerPatternBuilder", "Arranger") {}

    void runTest() override
    {
        beginTest ("seconds convert to beats at the reference BPM");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0.0);
            seq.addEvent (juce::MidiMessage::noteOff (1, 60), 1.0); // 1.0s -> 2 beats @120

            auto evts = ArrangerPatternBuilder::buildBeatEvents (seq, 120.0, 5);
            expectEquals ((int) evts.size(), 2);
            expectWithinAbsoluteError (evts[0].beats, 0.0, 1e-9);
            expectWithinAbsoluteError (evts[1].beats, 2.0, 1e-9);
            // channel is overridden to the supplied value
            expectEquals (evts[0].message.getChannel(), 5);
            expectEquals (evts[1].message.getChannel(), 5);
        }

        beginTest ("non-note events are skipped");
        {
            juce::MidiMessageSequence seq;
            seq.addEvent (juce::MidiMessage::controllerEvent (1, 7, 100), 0.0);
            seq.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0.0);
            auto evts = ArrangerPatternBuilder::buildBeatEvents (seq, 120.0, 2);
            expectEquals ((int) evts.size(), 1);
            expect (evts[0].message.isNoteOn());
        }

        beginTest ("single-section style: channel mapping and loop length");
        {
            // melodic track: a note ending at 2.0s -> 4 beats @120 -> exactly 1 bar (4/4).
            TrackEntry melodic;
            melodic.type = TrackType::Melodic;
            melodic.originalBPM = 120.0;
            melodic.sequence.addEvent (juce::MidiMessage::noteOn  (1, 60, (juce::uint8) 100), 0.0);
            melodic.sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 2.0);

            // percussion track: a hit ending at 3.0s -> 6 beats -> rounds up to 2 bars.
            TrackEntry perc;
            perc.type = TrackType::Percussion;
            perc.originalBPM = 120.0;
            perc.sequence.addEvent (juce::MidiMessage::noteOn  (1, 36, (juce::uint8) 100), 0.0);
            perc.sequence.addEvent (juce::MidiMessage::noteOff (1, 36), 3.0);

            std::vector<TrackEntry> tracks { melodic, perc };
            ArrangerStyle style = ArrangerPatternBuilder::buildSingleSectionStyle (tracks, 4, 4);

            expectEquals ((int) style.sections.size(), 1);
            auto& sec = style.sections[0];
            expectEquals ((int) sec.tracks.size(), 2);

            // melodic -> channel 2; percussion -> channel 10
            expectEquals (sec.tracks[0].channel, 2);
            expect (sec.tracks[0].partType != ArrangerPartType::Drum);
            expectEquals (sec.tracks[1].channel, 10);
            expect (sec.tracks[1].partType == ArrangerPartType::Drum);

            // loop length = max(1 bar, 2 bars) = 2 bars = 8 beats
            expectEquals (sec.lengthBars, 2);
        }

        beginTest ("originalBPM <= 0 falls back to 120");
        {
            TrackEntry t;
            t.type = TrackType::Melodic;
            t.originalBPM = 0.0;
            t.sequence.addEvent (juce::MidiMessage::noteOn  (1, 60, (juce::uint8) 100), 0.0);
            t.sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 2.0); // 2s -> 4 beats @120
            std::vector<TrackEntry> tracks { t };
            ArrangerStyle style = ArrangerPatternBuilder::buildSingleSectionStyle (tracks, 4, 4);
            expectEquals (style.sections[0].lengthBars, 1);
        }
    }
};

static ArrangerPatternBuilderTest arrangerPatternBuilderTest;
```

- [ ] **Step 2: Implement the header**

Create `Source/Arranger/ArrangerPatternBuilder.h`:
```cpp
#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include "TrackEntry.h"
#include <vector>

/** Builds Phase-1 arranger content (beat-based patterns, single-section style) from TrackEntry data. */
namespace ArrangerPatternBuilder
{
    /** Convert a seconds-timestamped sequence to note-on/off beat events on `channel`. */
    std::vector<TimedBeatEvent> buildBeatEvents (const juce::MidiMessageSequence& seq,
                                                 double referenceBpm, int channel);

    /** Wrap the selected tracks as one looped section. Channels: perc->10, melodic->2,3,4,…
        Loop length = longest track rounded up to whole bars. originalTempo from the first
        track's originalBPM (fallback 120). */
    ArrangerStyle buildSingleSectionStyle (const std::vector<TrackEntry>& tracks,
                                           int timeSigNum, int timeSigDenom);
}
```

- [ ] **Step 3: Implement the source**

Create `Source/Arranger/ArrangerPatternBuilder.cpp`:
```cpp
#include "ArrangerPatternBuilder.h"
#include "ArrangerTime.h"
#include <algorithm>

namespace ArrangerPatternBuilder
{
    std::vector<TimedBeatEvent> buildBeatEvents (const juce::MidiMessageSequence& seq,
                                                 double referenceBpm, int channel)
    {
        std::vector<TimedBeatEvent> out;
        for (int i = 0; i < seq.getNumEvents(); ++i)
        {
            const auto& msg = seq.getEventPointer (i)->message;
            if (! (msg.isNoteOn() || msg.isNoteOff()))
                continue;

            juce::MidiMessage m = msg;
            m.setChannel (channel);
            out.push_back ({ ArrangerTime::secondsToBeats (msg.getTimeStamp(), referenceBpm), m });
        }
        std::stable_sort (out.begin(), out.end(),
                          [] (const TimedBeatEvent& a, const TimedBeatEvent& b) { return a.beats < b.beats; });
        return out;
    }

    ArrangerStyle buildSingleSectionStyle (const std::vector<TrackEntry>& tracks,
                                           int timeSigNum, int timeSigDenom)
    {
        ArrangerStyle style;
        style.timeSigNum = timeSigNum;
        style.timeSigDenom = timeSigDenom;
        if (! tracks.empty())
            style.originalTempo = tracks.front().originalBPM > 0.0 ? tracks.front().originalBPM : 120.0;

        const double bpb = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);

        ArrangerSection section;
        section.type = ArrangerSectionType::Variation;
        section.name = "Variation 1";
        section.id = "var_1";

        int melodicChannel = 2;
        int maxBars = 0;

        for (const auto& te : tracks)
        {
            const double refBpm = te.originalBPM > 0.0 ? te.originalBPM : 120.0;
            const bool isPerc = (te.type == TrackType::Percussion);
            const int channel = isPerc ? 10 : melodicChannel++;

            ArrangerTrack at;
            at.id = te.getUniqueID();
            at.name = te.getDisplayName();
            at.partType = isPerc ? ArrangerPartType::Drum : ArrangerPartType::Acc;
            at.instrument = te.instrumentAssociated;
            at.channel = channel;
            at.volume = te.volumeAssociated;
            at.pattern = buildBeatEvents (te.sequence, refBpm, channel);

            double lastBeat = at.pattern.empty() ? 0.0 : at.pattern.back().beats;
            maxBars = std::max (maxBars, ArrangerTime::barsForBeats (lastBeat, bpb));

            section.tracks.push_back (std::move (at));
        }

        section.lengthBars = std::max (1, maxBars);
        style.sections.push_back (std::move (section));
        return style;
    }
}
```

- [ ] **Step 4: Register in `.jucer`**

`Source` GROUP:
```xml
      <FILE id="arrPb1" name="ArrangerPatternBuilder.cpp" compile="1" resource="0" file="Source/Arranger/ArrangerPatternBuilder.cpp"/>
      <FILE id="arrPb2" name="ArrangerPatternBuilder.h" compile="0" resource="0" file="Source/Arranger/ArrangerPatternBuilder.h"/>
```
`Tests` → `Unit` GROUP:
```xml
        <FILE id="arrTs3" name="test_arranger_pattern_builder.cpp" compile="1" resource="0" file="tests/unit/test_arranger_pattern_builder.cpp"/>
```

- [ ] **Step 5: Regenerate, build, test**

Run `[RESAVE]`, `[BUILD]`, `[TEST]`.
Expected: `ArrangerPatternBuilder | ... | failures: 0`, `ExitCode = 0`.

- [ ] **Step 6: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerPatternBuilder.h Source/Arranger/ArrangerPatternBuilder.cpp tests/unit/test_arranger_pattern_builder.cpp
git commit -m "feat(arranger): build single-section style from TrackEntry data"
```

---

## Task 6: ArrangerEngine — real-time wrapper

**Files:**
- Create: `Source/Arranger/ArrangerEngine.h`, `Source/Arranger/ArrangerEngine.cpp`
- Create: `tests/unit/test_arranger_engine.cpp`
- Modify: `Project Synth2.jucer`

**Design:** the timer computes a monotonic beat playhead from elapsed wall-clock and the current BPM, then calls `renderRange(prev, curr)`, which asks the scheduler for events and dispatches each to `outputDevice` and/or `onMidiMessage`. Tests drive `renderRange` directly (no timer) and capture via `onMidiMessage`.

- [ ] **Step 1: Write the failing test**

Create `tests/unit/test_arranger_engine.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Arranger/ArrangerEngine.h"
#include "Arranger/ArrangerModel.h"

class ArrangerEngineTest : public juce::UnitTest
{
public:
    ArrangerEngineTest() : juce::UnitTest ("ArrangerEngine", "Arranger") {}

    static ArrangerStyle makeStyle()
    {
        ArrangerTrack t;
        t.channel = 2;
        t.partType = ArrangerPartType::Acc;
        t.pattern = {
            { 0.0, juce::MidiMessage::noteOn  (2, 60, (juce::uint8) 100) },
            { 0.5, juce::MidiMessage::noteOff (2, 60) },
        };
        ArrangerSection s; s.lengthBars = 1; s.tracks.push_back (t);
        ArrangerStyle style; style.timeSigNum = 4; style.timeSigDenom = 4; style.originalTempo = 120.0;
        style.sections.push_back (s);
        return style;
    }

    void runTest() override
    {
        beginTest ("renderRange dispatches scheduled events through onMidiMessage");
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            std::vector<juce::MidiMessage> captured;
            engine.onMidiMessage = [&] (const juce::MidiMessage& m) { captured.push_back (m); };

            engine.setStyle (makeStyle());
            engine.renderRange (0.0, 1.0);   // beats [0,1): note on then off

            expectEquals ((int) captured.size(), 2);
            expect (captured[0].isNoteOn());
            expectEquals (captured[0].getNoteNumber(), 60);
            expect (captured[1].isNoteOff());
        }

        beginTest ("loop length in beats reflects the section bars");
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            engine.setStyle (makeStyle());
            expectWithinAbsoluteError (engine.getLoopLengthBeats(), 4.0, 1e-9); // 1 bar of 4/4
        }

        beginTest ("stop sends all-notes-off and is reflected in isPlaying");
        {
            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            int noteOffCount = 0;
            engine.onMidiMessage = [&] (const juce::MidiMessage& m)
            {
                if (m.isAllNotesOff()) ++noteOffCount;
            };
            engine.setStyle (makeStyle());
            engine.stop();
            expect (! engine.isPlaying());
            expect (noteOffCount >= 1);
        }
    }
};

static ArrangerEngineTest arrangerEngineTest;
```

- [ ] **Step 2: Implement the header**

Create `Source/Arranger/ArrangerEngine.h`:
```cpp
#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include "ArrangerScheduler.h"
#include <atomic>

/**
 * Real-time arranger playback: loops the active section of a style to MIDI-out and/or
 * an inject callback (SFZ). Parallel to MultipleTrackPlayer; the classic player is untouched.
 */
class ArrangerEngine : private juce::HighResolutionTimer
{
public:
    std::function<void (const juce::MidiMessage&)> onMidiMessage; // SFZ inject
    std::function<void (double)> onElapsedBeats;                  // for the beat bar

    explicit ArrangerEngine (std::weak_ptr<juce::MidiOutput> out);
    ~ArrangerEngine() override;

    void setDeviceOutput (std::weak_ptr<juce::MidiOutput> out);
    void setStyle (ArrangerStyle newStyle);   // builds the scheduler from section 0
    void setBpm (double bpm);

    void start();
    void stop();
    bool isPlaying() const { return playing.load(); }

    double getLoopLengthBeats() const { return loopLengthBeats; }

    /** Render and dispatch events for the monotonic beat window [fromBeats, toBeats). Public for tests. */
    void renderRange (double fromBeats, double toBeats);

private:
    void hiResTimerCallback() override;
    void dispatch (const juce::MidiMessage& m);
    void sendAllNotesOff();
    void rebuildScheduler();

    std::weak_ptr<juce::MidiOutput> outputDevice;
    ArrangerStyle style;
    ArrangerScheduler scheduler;

    double currentBpm = 120.0;
    double loopLengthBeats = 0.0;
    double playheadBeats = 0.0;     // monotonic beat position since start()
    double lastNowSeconds = 0.0;    // wall-clock of previous tick (for delta accumulation)
    std::atomic<bool> playing { false };
};
```

- [ ] **Step 3: Implement the source**

Create `Source/Arranger/ArrangerEngine.cpp`:
```cpp
#include "ArrangerEngine.h"
#include "ArrangerTime.h"

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

void ArrangerEngine::rebuildScheduler()
{
    std::vector<TimedBeatEvent> merged;
    double bars = 1.0;

    if (! style.sections.empty())
    {
        const auto& sec = style.sections.front();
        bars = (double) juce::jmax (1, sec.lengthBars);
        for (const auto& tr : sec.tracks)
            for (const auto& ev : tr.pattern)
                merged.push_back (ev);
    }

    const double bpb = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);
    loopLengthBeats = bars * bpb;
    scheduler.setLoop (std::move (merged), loopLengthBeats);
}

void ArrangerEngine::setStyle (ArrangerStyle newStyle)
{
    style = std::move (newStyle);
    if (style.originalTempo > 0.0) currentBpm = style.originalTempo;
    rebuildScheduler();
}

void ArrangerEngine::dispatch (const juce::MidiMessage& m)
{
    if (auto out = outputDevice.lock())
        out->sendMessageNow (m);
    if (onMidiMessage)
        onMidiMessage (m);
}

void ArrangerEngine::sendAllNotesOff()
{
    for (int ch = 1; ch <= 16; ++ch)
        dispatch (juce::MidiMessage::allNotesOff (ch));
}

void ArrangerEngine::renderRange (double fromBeats, double toBeats)
{
    auto events = scheduler.advance (fromBeats, toBeats);
    for (auto& e : events)
        dispatch (e.message);
}

void ArrangerEngine::start()
{
    if (loopLengthBeats <= 0.0)
        return;

    scheduler.reset();
    playheadBeats = 0.0;
    lastNowSeconds = (double) juce::Time::getHighResolutionTicks()
                     / (double) juce::Time::getHighResolutionTicksPerSecond();
    playing.store (true);
    startTimer (10);
}

void ArrangerEngine::stop()
{
    stopTimer();
    playing.store (false);
    sendAllNotesOff();
    scheduler.reset();
}

void ArrangerEngine::hiResTimerCallback()
{
    // Accumulate beats from the per-tick wall-clock delta so that a BPM change mid-playback
    // changes the rate going forward without jumping the musical position (spec §5.5).
    const double now = (double) juce::Time::getHighResolutionTicks()
                       / (double) juce::Time::getHighResolutionTicksPerSecond();
    const double deltaSeconds = now - lastNowSeconds;
    lastNowSeconds = now;

    const double deltaBeats = ArrangerTime::secondsToBeats (deltaSeconds, currentBpm);
    const double from = playheadBeats;
    const double to   = playheadBeats + deltaBeats;

    renderRange (from, to);
    playheadBeats = to;

    if (onElapsedBeats)
    {
        const double beats = playheadBeats;
        juce::MessageManager::callAsync ([this, beats] { if (onElapsedBeats) onElapsedBeats (beats); });
    }
}
```

- [ ] **Step 4: Register in `.jucer`**

`Source` GROUP:
```xml
      <FILE id="arrEn1" name="ArrangerEngine.cpp" compile="1" resource="0" file="Source/Arranger/ArrangerEngine.cpp"/>
      <FILE id="arrEn2" name="ArrangerEngine.h" compile="0" resource="0" file="Source/Arranger/ArrangerEngine.h"/>
```
`Tests` → `Unit` GROUP:
```xml
        <FILE id="arrTs4" name="test_arranger_engine.cpp" compile="1" resource="0" file="tests/unit/test_arranger_engine.cpp"/>
```

- [ ] **Step 5: Regenerate, build, test**

Run `[RESAVE]`, `[BUILD]`, `[TEST]`.
Expected: `ArrangerEngine | ... | failures: 0`, `ExitCode = 0`.

- [ ] **Step 6: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp tests/unit/test_arranger_engine.cpp
git commit -m "feat(arranger): add real-time looping ArrangerEngine"
```

---

## Task 7: Route CurrentStyleComponent to the ArrangerEngine

**Files:**
- Modify: `Source/CurrentStyleComponent.h`, `Source/CurrentStyleComponent.cpp`

**Context (verified):** the component constructs `trackPlayer = std::make_unique<MultipleTrackPlayer>(outputDevice)` (CurrentStyleComponent.cpp:82), wires `onElapsedUpdate` to the beat bar, and start/stop buttons call `startPlaying()` / `stopPlaying()`. `startPlaying()` (CurrentStyleComponent.cpp:4) gathers `std::vector<TrackEntry> selectedTracks` then calls `trackPlayer->setTracks/syncPlaybackSettings/start`. Output is wired via `setDeviceOutputCurrentStyle` and `setMidiInjectCallback` (sets `trackPlayer->onMidiMessage`). This task adds a parallel `ArrangerEngine` fed by the same outputs and a `bool arrangerModeEnabled` switch. This is a GUI/integration task — verified by build + the manual procedure in Task 9 (no unit test).

- [ ] **Step 1: Add members and API to the header**

In `Source/CurrentStyleComponent.h`, add the include near the other engine includes (after `#include "TrackPlayer.h"`):
```cpp
#include "Arranger/ArrangerEngine.h"
```
In the `public:` section (near `getTrackPlayer`), add:
```cpp
    /** Enable/disable Arranger mode (vs classic linear playback). */
    void setArrangerModeEnabled (bool shouldEnable);
    bool isArrangerModeEnabled() const { return arrangerModeEnabled; }
```
In the `private:` member area (after `std::unique_ptr<MultipleTrackPlayer> trackPlayer = nullptr;`), add:
```cpp
    std::unique_ptr<ArrangerEngine> arrangerEngine = nullptr;
    bool arrangerModeEnabled = false;
```

- [ ] **Step 2: Construct and wire the engine**

In `Source/CurrentStyleComponent.cpp`, immediately after the `trackPlayer = std::make_unique<MultipleTrackPlayer>(outputDevice);` line (around line 82) and its existing `onElapsedUpdate` lambda, add:
```cpp
    arrangerEngine = std::make_unique<ArrangerEngine>(outputDevice);
    arrangerEngine->onElapsedBeats = [this](double elapsedBeats)
    {
        customBeatBar.setCurrentBeatsElapsed(elapsedBeats);
    };
```

- [ ] **Step 3: Keep both engines' outputs in sync**

In `setDeviceOutputCurrentStyle` (search for it in the .cpp), after the existing `trackPlayer->setDeviceOutputTrackPlayer(newOutput);`, add:
```cpp
    if (arrangerEngine)
        arrangerEngine->setDeviceOutput(newOutput);
```
In `setMidiInjectCallback`, change the body so both engines get the callback:
```cpp
void CurrentStyleComponent::setMidiInjectCallback(std::function<void(const juce::MidiMessage&)> cb)
{
    if (trackPlayer)
        trackPlayer->onMidiMessage = cb;
    if (arrangerEngine)
        arrangerEngine->onMidiMessage = cb;
}
```

- [ ] **Step 4: Implement the mode switch**

Add this method to `Source/CurrentStyleComponent.cpp` (e.g. after `stopPlaying`):
```cpp
void CurrentStyleComponent::setArrangerModeEnabled(bool shouldEnable)
{
    if (arrangerModeEnabled == shouldEnable)
        return;

    // stop whatever is currently playing before switching engines
    stopPlaying(false);
    if (arrangerEngine)
        arrangerEngine->stop();

    arrangerModeEnabled = shouldEnable;
    isPlaying = false;
    customBeatBar.repaint();
}
```

- [ ] **Step 5: Branch start/stop on the mode**

In `startPlaying()` (CurrentStyleComponent.cpp:4), after `selectedTracks` is populated and the existing early-out checks pass but **before** `trackPlayer->setTracks(selectedTracks);`, insert:
```cpp
    if (arrangerModeEnabled)
    {
        // Phase 1: fixed 4/4; real time-signature wiring is Phase 2.
        ArrangerStyle style = ArrangerPatternBuilder::buildSingleSectionStyle(selectedTracks, 4, 4);
        arrangerEngine->setBpm(currentTempo);
        arrangerEngine->setStyle(style);
        arrangerEngine->start();
        return;
    }
```
Add the include at the top of the .cpp (near the other includes):
```cpp
#include "Arranger/ArrangerPatternBuilder.h"
```
In `stopPlaying(bool shouldModify)`, branch so the right engine stops:
```cpp
void CurrentStyleComponent::stopPlaying(bool shouldModify)
{
    if (arrangerModeEnabled)
    {
        if (arrangerEngine)
            arrangerEngine->stop();
        return;
    }

    if (trackPlayer)
        trackPlayer->stop(shouldModify);
}
```

- [ ] **Step 6: Build**

Run `[BUILD]` (no `[RESAVE]` — no files added).
Expected: `Build succeeded.`

- [ ] **Step 7: Commit** (ask approval first)
```powershell
git add Source/CurrentStyleComponent.h Source/CurrentStyleComponent.cpp
git commit -m "feat(arranger): route play/stop to ArrangerEngine when arranger mode is on"
```

---

## Task 8: Persisted Classic/Arranger toggle in settings

**Files:**
- Modify: `Source/SettingsWindow.h`, `Source/SettingsWindow.cpp` (class `MIDIWindow`)
- Modify: `Source/MainComponent.cpp`
- Modify: `Source/Display.h`, `Source/Display.cpp`

**Context (verified):** settings persist through a shared `juce::PropertiesFile* prop` (created in `MainComponent::initalizeSaveFileForUser`, e.g. `propertiesFile->getIntValue("EngineOptionIndex", 1)`). `MIDIWindow` already exposes engine-style callbacks like `onOutputEngineChanged`. `MainComponent` reaches the style component through `display` (e.g. `display->setMidiInjectCallback(...)`). We add a parallel `ArrangerModeEnabled` bool property, a checkbox, and a propagation path `MainComponent → Display → CurrentStyleComponent::setArrangerModeEnabled`.

- [ ] **Step 1: Add the callback + checkbox to `MIDIWindow`**

Controls in this window live inside a child `settingsPanel` and are laid out in `MIDIWindow::resized()` using the row variables `pad`, `y`, `cbX`, `cbW`, `rowH`, `labelW`. The output-engine combo is created in `outputEngineCBinit()` and laid out at `comboBoxOutputEngineType.setBounds(cbX, y, 180, rowH)` (SettingsWindow.cpp:312). Follow that idiom exactly.

In `Source/SettingsWindow.h`, next to `onOutputEngineChanged`, add:
```cpp
    std::function<void(bool)> onArrangerModeChanged; // fired when the Classic/Arranger toggle changes
```
In the private members of `MIDIWindow`, add:
```cpp
    juce::ToggleButton arrangerModeToggle { "Arranger mode (loop accompaniment)" };
```
In `Source/SettingsWindow.cpp`, in the constructor immediately after the `outputEngineCBinit();` call (SettingsWindow.cpp:459), add (the constructor parameter is named `prop`):
```cpp
    settingsPanel.addAndMakeVisible(arrangerModeToggle);
    arrangerModeToggle.setToggleState(prop != nullptr && prop->getBoolValue("ArrangerModeEnabled", false),
                                      juce::dontSendNotification);
    arrangerModeToggle.onClick = [this, prop]()
    {
        const bool on = arrangerModeToggle.getToggleState();
        if (prop != nullptr) { prop->setValue("ArrangerModeEnabled", on); prop->saveIfNeeded(); }
        if (onArrangerModeChanged) onArrangerModeChanged(on);
    };
```
In `MIDIWindow::resized()`, after the last control row's `setBounds(...)` and its `y` advance (the method advances `y` by a fixed row gap between rows — use the same gap), add a new row for the toggle using the real layout variables:
```cpp
    arrangerModeToggle.setBounds(pad, y, cbW, rowH);
```
*(If the final laid-out row is `sfzLibraryButton` at SettingsWindow.cpp:334, place this after it using that row's `y` plus the same inter-row increment already used above it.)*

- [ ] **Step 2: Add propagation through `Display`**

**Context:** `Display` owns a single `std::unique_ptr<CurrentStyleComponent> currentStyleComponent` that is created lazily (`std::make_unique<CurrentStyleComponent>(...)` at Display.cpp:187) and can be reset (Display.cpp:88). So `Display` must remember the desired mode and (re)apply it whenever the style component is created.

In `Source/Display.h`, in the public API of `Display` (near `setMidiInjectCallback`), add:
```cpp
    void setArrangerModeEnabled(bool shouldEnable);
```
and in its private members add:
```cpp
    bool arrangerModeEnabled = false;
```
In `Source/Display.cpp`, implement the setter:
```cpp
void Display::setArrangerModeEnabled(bool shouldEnable)
{
    arrangerModeEnabled = shouldEnable;
    if (currentStyleComponent)
        currentStyleComponent->setArrangerModeEnabled(shouldEnable);
}
```
And immediately after the `currentStyleComponent = std::make_unique<CurrentStyleComponent>(name, mapUuidToTrack, outputDevice, weakSectionsMap);` line (Display.cpp:187), apply the remembered flag:
```cpp
        currentStyleComponent->setArrangerModeEnabled(arrangerModeEnabled);
```

- [ ] **Step 3: Wire it in `MainComponent`**

In `Source/MainComponent.cpp`, where `midiWindow->onOutputEngineChanged = ...` is assigned (around line 935), add an adjacent assignment:
```cpp
            midiWindow->onArrangerModeChanged = [this](bool enabled)
            {
                if (display != nullptr)
                    display->setArrangerModeEnabled(enabled);
            };
```
And where the app applies persisted settings at startup (near the `EngineOptionIndex` read, around line 2342, or wherever `display` is first fully wired), apply the saved arranger mode once:
```cpp
            if (display != nullptr && propertiesFile != nullptr)
                display->setArrangerModeEnabled(propertiesFile->getBoolValue("ArrangerModeEnabled", false));
```

- [ ] **Step 4: Build**

Run `[BUILD]`.
Expected: `Build succeeded.` Fix any name mismatches against the real `Display`/`MainComponent` members (the property name `"ArrangerModeEnabled"` and method `setArrangerModeEnabled` must match Task 7).

- [ ] **Step 5: Commit** (ask approval first)
```powershell
git add Source/SettingsWindow.h Source/SettingsWindow.cpp Source/Display.h Source/Display.cpp Source/MainComponent.cpp
git commit -m "feat(arranger): persisted Classic/Arranger mode toggle in settings"
```

---

## Task 9: Manual verification & docs

**Files:**
- Modify: `docs/superpowers/specs/2026-06-05-arranger-engine-design.md` (mark Phase 1 status)

- [ ] **Step 1: Full build**

Run `[RESAVE]` then `[BUILD]`. Expected: `Build succeeded.`

- [ ] **Step 2: Full Arranger test sweep**

Run `[TEST]`. Expected: every `Arranger`-category test shows `failures: 0`, `ExitCode = 0`.

- [ ] **Step 3: Manual smoke test (the real proof)**

Launch the app normally:
```powershell
& "Builds\VisualStudio2022\x64\Debug\App\Project Synth2.exe"
```
Verify, in order:
1. With Arranger mode **off** (default): load/select a style, press Start — classic playback behaves exactly as before (plays through once, stops). ✅ no regression.
2. Open settings, tick **Arranger mode**, close settings.
3. Press Start — the style's tracks now **loop seamlessly** and stay in time; the beat bar advances. Let it loop several times: **no stuck/hung notes** at the loop point.
4. Press Stop — playback stops, all notes off (silence).
5. Re-open the app — the Arranger-mode tick is **remembered** (persisted).
6. Untick Arranger mode → Start → classic one-shot playback returns.

Record the result (pass/fail per item) in the task notes.

- [ ] **Step 4: Update the spec status**

In `docs/superpowers/specs/2026-06-05-arranger-engine-design.md`, change the Phase 1 line in the roadmap to mark it delivered (e.g. prefix the row with ✅) and set the doc `Status:` to `Phase 1 implemented`.

- [ ] **Step 5: Commit** (ask approval first)
```powershell
git add docs/superpowers/specs/2026-06-05-arranger-engine-design.md
git commit -m "docs(arranger): mark Phase 1 implemented"
```

---

## Notes for the executor

- **`StyleSection` scaffolding stays for now.** Phase 1 delivers the real model (`ArrangerModel.h`) and a working engine; the old placeholder `StyleSection` / `Display::initializeSectionsForStyle` and the section buttons are left untouched and harmless. They get wired/replaced in **Phase 2** (section state machine), per the spec (§5.6).
- **Builds are the slow step.** Only run `[RESAVE]` when you added/removed files in the `.jucer`; otherwise a plain `[BUILD]` incremental compile is much faster. Keep the test loop tight by editing impl/test `.cpp`s that are already registered.
- **Hung notes are the one thing to obsess over.** If the manual loop test ever leaves a note ringing, the bug is in `ArrangerScheduler::advance` seam handling or in `ArrangerEngine::stop` — re-check Task 4's seam tests first.
- **Commit policy:** pause for user approval at each commit step (project memory).
```
