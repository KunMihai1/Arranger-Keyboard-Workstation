# Arranger Engine — Phase 1 Implementation Plan (Looping Pattern Engine + Data Model)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an "Arranger" playback mode where pressing play makes the current style's tracks loop cleanly and in time to the existing MIDI-out/SFZ path, behind a Classic/Arranger toggle, with the existing linear playback untouched.

**Architecture:** A new, self-contained `ArrangerEngine` (parallel to `MultipleTrackPlayer`) drives a pure, unit-tested `ArrangerScheduler` that emits a style section's MIDI events on a seamless beat-based loop, closing any hanging notes at the loop seam. A `juce::HighResolutionTimer` advances a monotonic beat playhead and dispatches events to the same `weak_ptr<MidiOutput>` + `onMidiMessage` SFZ-inject outputs the classic player already uses. A persisted `ArrangerModeEnabled` setting routes the play/stop controls to the right engine.

**Tech Stack:** C++17, JUCE (Projucer project), `juce::UnitTest` tests compiled into the app, MSBuild (VS2022 exporter), Projucer CLI for project regeneration.

---

## Project-specific reference (read once before starting)

**Tool paths (verified on this machine):**
- MSBuild: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`
- Projucer: `C:\Users\Oricum\OneDrive\Desktop\Latest Juce\JUCE\Projucer.exe`
- Solution: `Builds\VisualStudio2022\Project Synth2.sln`
- Built exe: `Builds\VisualStudio2022\x64\Debug\App\Project Synth2.exe`
- Project file: `Project Synth2.jucer` (NOTE: this file and `docs/`, `Builds/`, `JuceLibraryCode/` are **git-ignored** — see `.gitignore`. Commits below therefore do not include the `.jucer`; that is expected and fine.)

**Commit policy:** This project's memory says *always ask the user before `git commit`*. At every "Commit" step, pause and ask for approval rather than committing unprompted.

**Standard commands (PowerShell), referenced as `[RESAVE]`, `[BUILD]`, `[TEST]` below:**

- `[RESAVE]` — regenerate VS project after adding/removing files in the `.jucer` (only needed when files are added/removed):
  ```powershell
  & "C:\Users\Oricum\OneDrive\Desktop\Latest Juce\JUCE\Projucer.exe" --resave "Project Synth2.jucer"
  ```
- `[BUILD]` — compile Debug x64:
  ```powershell
  & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "Builds\VisualStudio2022\Project Synth2.sln" /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
  ```
  Expected on success: ends with `Build succeeded.` / `0 Error(s)`. (Full rebuilds take a few minutes; incremental builds are faster. Skip `[RESAVE]` when no files were added/removed.)
- `[TEST]` — run only the Arranger-category unit tests and read results:
  ```powershell
  if (Test-Path test-results.txt) { Remove-Item test-results.txt }
  $p = Start-Process -FilePath "Builds\VisualStudio2022\x64\Debug\App\Project Synth2.exe" -ArgumentList '--run-tests','--arranger-tests' -Wait -PassThru
  "ExitCode (failure count) = $($p.ExitCode)"
  Get-Content test-results.txt
  ```
  Exit code is the number of failed assertions (0 = all pass). `test-results.txt` holds per-test detail.

**Test category convention:** Every test class in this plan is constructed as `juce::UnitTest("<name>", "Arranger")` so `--arranger-tests` runs them in isolation from the existing (GUI-heavy) Unit/Integration suites.

**File-registration convention (`.jucer`):** Source files go in the `Source` GROUP (`id="{77180095-A5AE-9575-2B89-DCC21B51B797}"`, near line 306). Test files go in the `Tests` → `Unit` GROUP (`id="{A1B2C3D4-1111-2222-3333-444455556666}"`, near line 7), appended before its closing `</GROUP>`. `.cpp` uses `compile="1"`; `.h` uses `compile="0"`. `id` must be any unique token. After editing, run `[RESAVE]`.

---

## File Structure

**New (all under `Source/Arranger/`):**
- `ArrangerModel.h` — header-only data model: `ArrangerPartType`, `ArrangerSectionType` enums; `ArrangerTrack`, `ArrangerSection`, `ArrangerStyle` structs; schema version constant.
- `ArrangerTime.h` / `ArrangerTime.cpp` — pure musical-time conversions (seconds↔beats, beats-per-bar, bars-for-beats).
- `ArrangerScheduler.h` / `ArrangerScheduler.cpp` — pure loop scheduler: `advance(from,to)` emits events with seam note-offs. The testable core.
- `ArrangerPatternBuilder.h` / `ArrangerPatternBuilder.cpp` — builds beat-event patterns and a single-section `ArrangerStyle` from `TrackEntry`s (seconds→beats, channel mapping, loop length).
- `ArrangerEngine.h` / `ArrangerEngine.cpp` — real-time wrapper: timer + output, drives the scheduler.

**New tests (under `tests/unit/`):**
- `test_arranger_time.cpp`, `test_arranger_scheduler.cpp`, `test_arranger_pattern_builder.cpp`, `test_arranger_engine.cpp`.

**Modified:**
- `Source/Main.cpp` — re-enable + harden the `--run-tests` runner; add `--arranger-tests`.
- `Source/CurrentStyleComponent.h` / `.cpp` — own an `ArrangerEngine`; wire outputs to both players; `setArrangerModeEnabled`; route start/stop.
- `Source/SettingsWindow.h` / `.cpp` (class `MIDIWindow`) — add Arranger-mode checkbox + `onArrangerModeChanged` callback.
- `Source/MainComponent.cpp` — persist `ArrangerModeEnabled`; propagate to the style component.
- `Source/Display.h` / `.cpp` — forward `setArrangerModeEnabled` to `CurrentStyleComponent`.
- `Project Synth2.jucer` — register new files.

---

## Task 1: Enable a reliable test runner

**Files:**
- Modify: `Source/Main.cpp:31-64` (the commented-out runner inside `initialise`)

- [ ] **Step 1: Replace the commented runner block with a hardened one**

In `Source/Main.cpp`, replace the entire commented block (lines 31-64, the `/* ... */` around the old `--run-tests` logic) with this active code, placed at the very start of `initialise` (before `mainWindow.reset(...)`):

```cpp
        if (commandLine.contains ("--run-tests"))
        {
            juce::UnitTestRunner runner;
            runner.setAssertOnFailure (false);

            if (commandLine.contains ("--arranger-tests"))
                runner.runTestsInCategory ("Arranger");
            else if (commandLine.contains ("--unit-tests"))
                runner.runTestsInCategory ("Unit");
            else if (commandLine.contains ("--integration-tests"))
                runner.runTestsInCategory ("Integration");
            else
                runner.runAllTests();

            int pass = 0, fail = 0;
            juce::StringArray lines;
            for (int i = 0; i < runner.getNumResults(); ++i)
            {
                auto* r = runner.getResult (i);
                pass += r->passes;
                fail += r->failures;
                lines.add (r->unitTestName + " | passes: " + juce::String (r->passes)
                           + " | failures: " + juce::String (r->failures));
                for (auto& m : r->messages)
                    if (m.isNotEmpty()) lines.add ("    " + m);
            }
            lines.add ("TOTAL | passes: " + juce::String (pass) + " | failures: " + juce::String (fail));

            juce::File::getCurrentWorkingDirectory()
                .getChildFile ("test-results.txt")
                .replaceWithText (lines.joinIntoString ("\n"));

            setApplicationReturnValue (fail);
            quit();
            return;
        }
```

- [ ] **Step 2: Build**

Run `[BUILD]` (no `[RESAVE]` — no files added).
Expected: `Build succeeded.`

- [ ] **Step 3: Verify the runner works against existing tests**

Run:
```powershell
if (Test-Path test-results.txt) { Remove-Item test-results.txt }
$p = Start-Process -FilePath "Builds\VisualStudio2022\x64\Debug\App\Project Synth2.exe" -ArgumentList '--run-tests','--unit-tests' -Wait -PassThru
"ExitCode = $($p.ExitCode)"; Get-Content test-results.txt
```
Expected: `test-results.txt` exists and lists existing tests with a `TOTAL | passes: N | failures: M` line. (Pre-existing failures, if any, are not our concern — we only need proof the runner runs and writes results.)

- [ ] **Step 4: Add `test-results.txt` to .gitignore**

Append a line `test-results.txt` to `.gitignore`.

- [ ] **Step 5: Commit** (ask approval first)

```powershell
git add Source/Main.cpp .gitignore
git commit -m "test: enable headless unit-test runner with results file and Arranger category"
```

---

## Task 2: Arranger data model

**Files:**
- Create: `Source/Arranger/ArrangerModel.h`
- Create: `tests/unit/test_arranger_model.cpp`
- Modify: `Project Synth2.jucer`

- [ ] **Step 1: Write the failing test**

Create `tests/unit/test_arranger_model.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerModel.h"

class ArrangerModelTest : public juce::UnitTest
{
public:
    ArrangerModelTest() : juce::UnitTest ("ArrangerModel", "Arranger") {}

    void runTest() override
    {
        beginTest ("defaults and assembly");
        {
            ArrangerStyle style;
            expect (style.schemaVersion == ARRANGER_SCHEMA_VERSION);
            expect (style.sections.empty());

            ArrangerTrack track;
            track.partType = ArrangerPartType::Bass;
            track.channel  = 2;
            track.pattern.push_back ({ 0.0, juce::MidiMessage::noteOn (2, 60, (juce::uint8) 100) });

            ArrangerSection section;
            section.type = ArrangerSectionType::Variation;
            section.lengthBars = 2;
            section.tracks.push_back (track);

            style.sections.push_back (section);

            expectEquals ((int) style.sections.size(), 1);
            expectEquals (style.sections[0].lengthBars, 2);
            expectEquals ((int) style.sections[0].tracks[0].pattern.size(), 1);
            expect (style.sections[0].tracks[0].partType == ArrangerPartType::Bass);
        }
    }
};

static ArrangerModelTest arrangerModelTest;
```

- [ ] **Step 2: Create the model header**

Create `Source/Arranger/ArrangerModel.h`:
```cpp
#pragma once
#include <JuceHeader.h>
#include <vector>

/** Schema version for arranger styles, bumped when the model changes. */
static constexpr int ARRANGER_SCHEMA_VERSION = 1;

/** Musical role of a track (drives transposition behaviour in later phases). */
enum class ArrangerPartType { Drum, Perc, Bass, Acc };

/** Style-element type (the section buttons). */
enum class ArrangerSectionType { Intro, Variation, Fill, Break, Ending, CountIn };

/** One timestamped MIDI event, positioned in beats within its section's loop. */
struct TimedBeatEvent
{
    double beats = 0.0;            // [0, loopLengthBeats)
    juce::MidiMessage message;
};

/** One accompaniment track within a section. */
struct ArrangerTrack
{
    juce::String id;
    juce::String name;
    ArrangerPartType partType = ArrangerPartType::Acc;
    int instrument = -1;
    int channel = 1;
    double volume = 100.0;

    std::vector<TimedBeatEvent> pattern;   // Phase 1: the section's loop for this track

    // --- reserved for later phases, inert in Phase 1 ---
    juce::String originalChord;            // Phase 4/5: recorded key/chord
};

/** One style element / section (e.g. Variation 1), an independent loop. */
struct ArrangerSection
{
    juce::String id;
    juce::String name;
    ArrangerSectionType type = ArrangerSectionType::Variation;
    int lengthBars = 1;                    // this section's own loop length
    std::vector<ArrangerTrack> tracks;
};

/** A whole arranger style. */
struct ArrangerStyle
{
    int schemaVersion = ARRANGER_SCHEMA_VERSION;
    juce::String id;
    juce::String name;
    double originalTempo = 120.0;          // BPM patterns were authored at
    int timeSigNum = 4;
    int timeSigDenom = 4;
    std::vector<ArrangerSection> sections;
};
```

- [ ] **Step 3: Register both files in the `.jucer`**

In `Project Synth2.jucer`, inside the `Source` GROUP (`id="{77180095-A5AE-9575-2B89-DCC21B51B797}"`), add:
```xml
      <FILE id="arrMd1" name="ArrangerModel.h" compile="0" resource="0" file="Source/Arranger/ArrangerModel.h"/>
```
Inside the `Tests` → `Unit` GROUP (`id="{A1B2C3D4-1111-2222-3333-444455556666}"`), before its `</GROUP>`, add:
```xml
        <FILE id="arrTs0" name="test_arranger_model.cpp" compile="1" resource="0" file="tests/unit/test_arranger_model.cpp"/>
```

- [ ] **Step 4: Regenerate and build**

Run `[RESAVE]` then `[BUILD]`.
Expected: `Build succeeded.`

- [ ] **Step 5: Run the test**

Run `[TEST]`.
Expected: results include `ArrangerModel | passes: ... | failures: 0` and `ExitCode = 0`.

- [ ] **Step 6: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerModel.h tests/unit/test_arranger_model.cpp
git commit -m "feat(arranger): add Phase 1 data model (Style/Section/Track)"
```

---

## Task 3: Musical-time conversions

**Files:**
- Create: `Source/Arranger/ArrangerTime.h`, `Source/Arranger/ArrangerTime.cpp`
- Create: `tests/unit/test_arranger_time.cpp`
- Modify: `Project Synth2.jucer`

- [ ] **Step 1: Write the failing test**

Create `tests/unit/test_arranger_time.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include "Arranger/ArrangerTime.h"

class ArrangerTimeTest : public juce::UnitTest
{
public:
    ArrangerTimeTest() : juce::UnitTest ("ArrangerTime", "Arranger") {}

    void runTest() override
    {
        beginTest ("seconds <-> beats at 120 BPM");
        {
            expectWithinAbsoluteError (ArrangerTime::secondsToBeats (1.0, 120.0), 2.0, 1e-9);
            expectWithinAbsoluteError (ArrangerTime::beatsToSeconds (2.0, 120.0), 1.0, 1e-9);
        }

        beginTest ("beats per bar");
        {
            expectWithinAbsoluteError (ArrangerTime::beatsPerBar (4, 4), 4.0, 1e-9);
            expectWithinAbsoluteError (ArrangerTime::beatsPerBar (3, 4), 3.0, 1e-9);
            expectWithinAbsoluteError (ArrangerTime::beatsPerBar (6, 8), 3.0, 1e-9); // 6 * 4/8
        }

        beginTest ("bars for beats rounds up to whole bars");
        {
            expectEquals (ArrangerTime::barsForBeats (8.0, 4.0), 2);
            expectEquals (ArrangerTime::barsForBeats (8.0001, 4.0), 3);
            expectEquals (ArrangerTime::barsForBeats (0.0, 4.0), 0);
            expectEquals (ArrangerTime::barsForBeats (7.9999, 4.0), 2); // tolerance swallows fp noise
        }
    }
};

static ArrangerTimeTest arrangerTimeTest;
```

- [ ] **Step 2: Run to verify it fails (compile error / not found)**

Run `[RESAVE]` then `[BUILD]`. After registering the test file in Step 4 the build will fail because `ArrangerTime.h` does not exist yet — that is the expected failing state. (If you prefer, create the files first, then this step is the post-implementation build.)

- [ ] **Step 3: Implement**

Create `Source/Arranger/ArrangerTime.h`:
```cpp
#pragma once

/** Pure musical-time conversions. A "beat" is one quarter note. */
namespace ArrangerTime
{
    double secondsToBeats (double seconds, double bpm);
    double beatsToSeconds (double beats, double bpm);

    /** Quarter-note beats per bar for a time signature, e.g. 6/8 -> 3.0. */
    double beatsPerBar (int timeSigNum, int timeSigDenom);

    /** Whole bars needed to contain `beats`, rounded up (with fp tolerance). 0 -> 0. */
    int barsForBeats (double beats, double beatsPerBar);
}
```
Create `Source/Arranger/ArrangerTime.cpp`:
```cpp
#include "ArrangerTime.h"
#include <cmath>

namespace ArrangerTime
{
    double secondsToBeats (double seconds, double bpm)
    {
        if (bpm <= 0.0) bpm = 120.0;
        return seconds * (bpm / 60.0);
    }

    double beatsToSeconds (double beats, double bpm)
    {
        if (bpm <= 0.0) bpm = 120.0;
        return beats * (60.0 / bpm);
    }

    double beatsPerBar (int timeSigNum, int timeSigDenom)
    {
        if (timeSigNum <= 0)   timeSigNum = 4;
        if (timeSigDenom <= 0) timeSigDenom = 4;
        return (double) timeSigNum * (4.0 / (double) timeSigDenom);
    }

    int barsForBeats (double beats, double beatsPerBarValue)
    {
        if (beatsPerBarValue <= 0.0 || beats <= 0.0) return 0;
        const double bars = beats / beatsPerBarValue;
        return (int) std::ceil (bars - 1e-6);
    }
}
```

- [ ] **Step 4: Register in `.jucer`**

In the `Source` GROUP add:
```xml
      <FILE id="arrTm1" name="ArrangerTime.cpp" compile="1" resource="0" file="Source/Arranger/ArrangerTime.cpp"/>
      <FILE id="arrTm2" name="ArrangerTime.h" compile="0" resource="0" file="Source/Arranger/ArrangerTime.h"/>
```
In the `Tests` → `Unit` GROUP add:
```xml
        <FILE id="arrTs1" name="test_arranger_time.cpp" compile="1" resource="0" file="tests/unit/test_arranger_time.cpp"/>
```

- [ ] **Step 5: Regenerate, build, test**

Run `[RESAVE]`, `[BUILD]`, `[TEST]`.
Expected: `ArrangerTime | ... | failures: 0`, `ExitCode = 0`.

- [ ] **Step 6: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerTime.h Source/Arranger/ArrangerTime.cpp tests/unit/test_arranger_time.cpp
git commit -m "feat(arranger): add musical-time conversions"
```

---

## Task 4: ArrangerScheduler — the looping core

**Files:**
- Create: `Source/Arranger/ArrangerScheduler.h`, `Source/Arranger/ArrangerScheduler.cpp`
- Create: `tests/unit/test_arranger_scheduler.cpp`
- Modify: `Project Synth2.jucer`

- [ ] **Step 1: Write the failing tests**

Create `tests/unit/test_arranger_scheduler.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerScheduler.h"

class ArrangerSchedulerTest : public juce::UnitTest
{
public:
    ArrangerSchedulerTest() : juce::UnitTest ("ArrangerScheduler", "Arranger") {}

    static std::vector<TimedBeatEvent> oneBarKickAndHeld()
    {
        // loop length 4 beats. Kick on beat 0 (off at 0.5). A held note on at beat 3 with NO note-off
        // before the loop end (must be closed at the seam).
        return {
            { 0.0, juce::MidiMessage::noteOn  (10, 36, (juce::uint8) 100) },
            { 0.5, juce::MidiMessage::noteOff (10, 36) },
            { 3.0, juce::MidiMessage::noteOn  (2, 60, (juce::uint8) 90) },
        };
    }

    void runTest() override
    {
        beginTest ("emits events whose phase is inside the swept window");
        {
            ArrangerScheduler s;
            s.setLoop (oneBarKickAndHeld(), 4.0);
            auto out = s.advance (0.0, 1.0);   // sweeps beats [0,1)
            expectEquals ((int) out.size(), 2); // kick on + kick off
            expectWithinAbsoluteError (out[0].beats, 0.0, 1e-9);
            expect (out[0].message.isNoteOn());
            expectWithinAbsoluteError (out[1].beats, 0.5, 1e-9);
            expect (out[1].message.isNoteOff());
        }

        beginTest ("does not emit events outside the window");
        {
            ArrangerScheduler s;
            s.setLoop (oneBarKickAndHeld(), 4.0);
            auto out = s.advance (1.0, 2.0);
            expectEquals ((int) out.size(), 0);
        }

        beginTest ("closes a held note at the loop seam and retriggers next loop");
        {
            ArrangerScheduler s;
            s.setLoop (oneBarKickAndHeld(), 4.0);
            s.advance (2.5, 3.5);            // arm: turns the held note (2,60) on at beat 3.0
            // Now sweep across the seam: from beat 3.5 to beat 4.5.
            auto out = s.advance (3.5, 4.5);
            // Expected, in order:
            //   synthetic noteOff for the held (2,60) at beat 4.0 (seam),
            //   kick noteOn at beat 4.0, kick noteOff at beat 4.5? (4.5 is exclusive end -> not included)
            // beat 4.0 == loop start: kick-on (phase 0). Held note-on is at phase 3 -> not in [0,0.5).
            expect (out.size() >= 2);
            expectWithinAbsoluteError (out[0].beats, 4.0, 1e-9);
            expect (out[0].message.isNoteOff());
            expectEquals (out[0].message.getNoteNumber(), 60);
            expectEquals (out[0].message.getChannel(), 2);
            expect (out[1].message.isNoteOn());
            expectEquals (out[1].message.getNoteNumber(), 36); // kick retriggers at seam
        }

        beginTest ("reset clears active-note state (no spurious seam note-off)");
        {
            ArrangerScheduler s;
            s.setLoop (oneBarKickAndHeld(), 4.0);
            s.advance (3.0, 3.5);   // turns the held note (2,60) on
            s.reset();              // forget active notes
            auto out = s.advance (3.5, 4.5); // crossing seam, but nothing is active to close
            // No synthetic note-off for (2,60); only the kick retrigger at 4.0.
            for (auto& e : out)
                expect (! (e.message.isNoteOff() && e.message.getNoteNumber() == 60));
        }

        beginTest ("handles a window longer than the loop (multiple wraps)");
        {
            ArrangerScheduler s;
            s.setLoop (oneBarKickAndHeld(), 4.0);
            auto out = s.advance (0.0, 8.0); // two full loops
            int kickOns = 0;
            for (auto& e : out) if (e.message.isNoteOn() && e.message.getNoteNumber() == 36) ++kickOns;
            expectEquals (kickOns, 2);
        }
    }
};

static ArrangerSchedulerTest arrangerSchedulerTest;
```

- [ ] **Step 2: Implement the header**

Create `Source/Arranger/ArrangerScheduler.h`:
```cpp
#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include <vector>
#include <set>
#include <utility>

/** One event to emit, positioned at a monotonic beat within the advance window. */
struct EmittedEvent
{
    double beats = 0.0;
    juce::MidiMessage message;
};

/**
 * Pure, deterministic loop scheduler. Holds a section's events (in beats within
 * [0, loopLengthBeats)) and, on each advance(from, to), returns the events to emit,
 * wrapping the loop and closing any still-sounding notes at every seam so nothing hangs.
 * No threading, no I/O — fully unit-testable.
 */
class ArrangerScheduler
{
public:
    void setLoop (std::vector<TimedBeatEvent> events, double loopLengthBeats);
    void reset();   // forget which notes are currently sounding

    /** Advance the monotonic playhead from `fromBeats` to `toBeats` (from <= to). */
    std::vector<EmittedEvent> advance (double fromBeats, double toBeats);

private:
    void trackActiveNote (const juce::MidiMessage& m);

    std::vector<TimedBeatEvent> sortedEvents;          // sorted by beats
    double loopLen = 0.0;
    std::set<std::pair<int,int>> activeNotes;          // (channel, noteNumber) currently sounding
};
```

- [ ] **Step 3: Implement the source**

Create `Source/Arranger/ArrangerScheduler.cpp`:
```cpp
#include "ArrangerScheduler.h"
#include <algorithm>
#include <cmath>

void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events, double loopLengthBeats)
{
    sortedEvents = std::move (events);
    std::stable_sort (sortedEvents.begin(), sortedEvents.end(),
                      [] (const TimedBeatEvent& a, const TimedBeatEvent& b) { return a.beats < b.beats; });
    loopLen = loopLengthBeats;
    activeNotes.clear();
}

void ArrangerScheduler::reset()
{
    activeNotes.clear();
}

void ArrangerScheduler::trackActiveNote (const juce::MidiMessage& m)
{
    const auto key = std::make_pair (m.getChannel(), m.getNoteNumber());
    if (m.isNoteOn() && m.getVelocity() > 0)
        activeNotes.insert (key);
    else if (m.isNoteOff() || (m.isNoteOn() && m.getVelocity() == 0))
        activeNotes.erase (key);
}

std::vector<EmittedEvent> ArrangerScheduler::advance (double fromBeats, double toBeats)
{
    std::vector<EmittedEvent> result;
    if (loopLen <= 0.0 || toBeats <= fromBeats)
        return result;

    double pos = fromBeats;
    while (pos < toBeats - 1e-12)
    {
        const double iterationIndex    = std::floor (pos / loopLen);
        const double iterationStartAbs = iterationIndex * loopLen;
        const double nextWrapAbs       = iterationStartAbs + loopLen;
        const double phaseStart        = pos - iterationStartAbs;
        const double segmentEndAbs     = std::min (toBeats, nextWrapAbs);
        const double phaseEnd          = segmentEndAbs - iterationStartAbs;

        for (const auto& ev : sortedEvents)
        {
            if (ev.beats >= phaseStart - 1e-12 && ev.beats < phaseEnd - 1e-12)
            {
                EmittedEvent e { iterationStartAbs + ev.beats, ev.message };
                result.push_back (e);
                trackActiveNote (ev.message);
            }
        }

        // If we actually reached a seam within the window, close hanging notes there.
        if (std::abs (segmentEndAbs - nextWrapAbs) < 1e-9 && nextWrapAbs <= toBeats + 1e-12)
        {
            for (const auto& key : activeNotes)
                result.push_back ({ nextWrapAbs, juce::MidiMessage::noteOff (key.first, key.second) });
            activeNotes.clear();
        }

        pos = segmentEndAbs;
    }

    return result;
}
```

- [ ] **Step 4: Register in `.jucer`**

`Source` GROUP:
```xml
      <FILE id="arrSc1" name="ArrangerScheduler.cpp" compile="1" resource="0" file="Source/Arranger/ArrangerScheduler.cpp"/>
      <FILE id="arrSc2" name="ArrangerScheduler.h" compile="0" resource="0" file="Source/Arranger/ArrangerScheduler.h"/>
```
`Tests` → `Unit` GROUP:
```xml
        <FILE id="arrTs2" name="test_arranger_scheduler.cpp" compile="1" resource="0" file="tests/unit/test_arranger_scheduler.cpp"/>
```

- [ ] **Step 5: Regenerate, build, test**

Run `[RESAVE]`, `[BUILD]`, `[TEST]`.
Expected: `ArrangerScheduler | ... | failures: 0`, `ExitCode = 0`. If any seam test fails, fix the boundary math in `advance` (the half-open interval and the seam-detection epsilon are the usual culprits) before moving on.

- [ ] **Step 6: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerScheduler.h Source/Arranger/ArrangerScheduler.cpp tests/unit/test_arranger_scheduler.cpp
git commit -m "feat(arranger): add looping scheduler with seam note-off handling"
```

---

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
