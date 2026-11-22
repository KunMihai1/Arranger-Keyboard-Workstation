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


> Continued in [part 2 of 2](2026-06-05-arranger-engine-phase1-part2.md).
