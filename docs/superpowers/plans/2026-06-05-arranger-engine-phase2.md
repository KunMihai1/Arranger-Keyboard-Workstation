# Arranger Engine — Phase 2 Implementation Plan (Section State Machine + Transport)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Intro/Variation/Fill/Ending/Break section buttons drive the arranger like a Korg/Yamaha workstation — bar-synced switching, Intro-plays-once-then-falls-into-a-Variation, Ending-plays-once-then-stops, Fill-plays-once-then-returns — with no hung notes at any seam, while classic linear playback stays byte-for-byte unchanged.

**Architecture:** A new pure, unit-tested `ArrangerSectionSequencer` decides *which* section is active across a monotonic beat window, applying queued switches and one-shot completions only at bar boundaries. `ArrangerEngine` grows from a single scheduler to **one `ArrangerScheduler` per section**, walks the sequencer's segments each tick, and flushes the outgoing section's notes at every section swap (reusing the scheduler's tested seam machinery). A new `buildDemoMultiSectionStyle` derives Intro/Variation/Fill/Ending from the single Phase-1 loop (transport flags are real; distinct per-section *audio* is Phase 3). The existing section-button handlers gain an arranger-mode branch that calls `queueSection` / `selectStartSection`.

**Tech Stack:** C++17, JUCE (Projucer project), `juce::UnitTest` tests compiled into the app, MSBuild (VS2022 exporter), Projucer CLI for project regeneration.

**Design spec:** `docs/superpowers/specs/2026-06-05-arranger-engine-phase2-design.md`.

---

## Project-specific reference (read once before starting)

**Tool paths (verified on this machine):**
- MSBuild: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`
- Projucer: `C:\Users\Oricum\OneDrive\Desktop\Latest Juce\JUCE\Projucer.exe`
- Solution: `Builds\VisualStudio2022\Project Synth2.sln`
- Built exe: `Builds\VisualStudio2022\x64\Debug\App\Project Synth2.exe`
- Project file: `Project Synth2.jucer` (NOTE: this file and `docs/`, `Builds/`, `JuceLibraryCode/` are **git-ignored** — commits below therefore do not include the `.jucer` or the design/plan docs; that is expected and fine.)

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
  Expected on success: ends with `Build succeeded.` / `0 Error(s)`. (Skip `[RESAVE]` when no files were added/removed.)
- `[TEST]` — run only the Arranger-category unit tests and read results:
  ```powershell
  if (Test-Path test-results.txt) { Remove-Item test-results.txt }
  $p = Start-Process -FilePath "Builds\VisualStudio2022\x64\Debug\App\Project Synth2.exe" -ArgumentList '--run-tests','--arranger-tests' -Wait -PassThru
  "ExitCode (failure count) = $($p.ExitCode)"
  Get-Content test-results.txt
  ```
  Exit code is the number of failed assertions (0 = all pass). `test-results.txt` holds per-test detail.

**Test category convention:** Every test class is `juce::UnitTest("<name>", "Arranger")` so `--arranger-tests` runs them in isolation.

**File-registration convention (`.jucer`):** Source files go in the `Source` GROUP (`id="{77180095-A5AE-9575-2B89-DCC21B51B797}"`). Test files go in the `Tests` → `Unit` GROUP (`id="{A1B2C3D4-1111-2222-3333-444455556666}"`), appended before its closing `</GROUP>`. `.cpp` uses `compile="1"`; `.h` uses `compile="0"`. `id` must be any unique token. After editing, run `[RESAVE]`.

---

## File Structure

**New:**
- `Source/Arranger/ArrangerSectionSequencer.h` / `.cpp` — the pure section state machine. Owns section metadata (type/length/after-rule), the active section, the return target, and one pending request. `advance(from,to)` returns ordered `SectionSegment`s + a `stopRequested` flag. No MIDI, no threads.
- `tests/unit/test_arranger_section_sequencer.cpp` — unit tests for the state machine.

**Modified:**
- `Source/Arranger/ArrangerModel.h` — add `ArrangerAfterComplete { Loop, FallThrough, Stop }`; add `afterComplete` field to `ArrangerSection`; bump `ARRANGER_SCHEMA_VERSION` to 2.
- `Source/Arranger/ArrangerScheduler.h` / `.cpp` — add `flushActiveNotes(atBeats)` (emit note-offs for currently-sounding notes at a section swap).
- `Source/Arranger/ArrangerEngine.h` / `.cpp` — multi-section playback (`std::vector<ArrangerScheduler>` + `ArrangerSectionSequencer`), `queueSection`, `selectStartSection`, `getActiveSectionIndex`; safe stop from the timer thread.
- `Source/Arranger/ArrangerPatternBuilder.h` / `.cpp` — add `buildDemoMultiSectionStyle(...)`.
- `tests/unit/test_arranger_model.cpp`, `tests/unit/test_arranger_scheduler.cpp`, `tests/unit/test_arranger_engine.cpp`, `tests/unit/test_arranger_pattern_builder.cpp` — extend.
- `Source/CurrentStyleComponent.cpp` — arranger-mode branch in the five section handlers + switch `startPlaying()` to `buildDemoMultiSectionStyle`.
- `Project Synth2.jucer` — register the new sequencer files.

---

## Task 1: Model — one-shot rule + schema bump

**Files:**
- Modify: `Source/Arranger/ArrangerModel.h`
- Modify: `tests/unit/test_arranger_model.cpp`

- [ ] **Step 1: Add the failing test**

In `tests/unit/test_arranger_model.cpp`, inside `runTest()` (after the existing `beginTest` block), add:
```cpp
        beginTest ("schema v2 and after-complete defaults");
        {
            expectEquals (ARRANGER_SCHEMA_VERSION, 2);

            ArrangerSection s;
            expect (s.afterComplete == ArrangerAfterComplete::Loop); // sections loop unless told otherwise

            s.afterComplete = ArrangerAfterComplete::Stop;
            expect (s.afterComplete == ArrangerAfterComplete::Stop);
        }
```

- [ ] **Step 2: Update the model header**

In `Source/Arranger/ArrangerModel.h`:

Change the schema constant:
```cpp
static constexpr int ARRANGER_SCHEMA_VERSION = 2;
```
Add the enum just below `ArrangerSectionType` (near line 12):
```cpp
/** What a section does when its own length elapses: loop forever, fall through to
    the return variation (Intro/Fill), or stop playback (Ending). */
enum class ArrangerAfterComplete { Loop, FallThrough, Stop };
```
Add the field to `ArrangerSection` (after `int lengthBars = 1;`):
```cpp
    ArrangerAfterComplete afterComplete = ArrangerAfterComplete::Loop; // Phase 2 transport rule
```

- [ ] **Step 3: Build**

Run `[BUILD]` (no `[RESAVE]` — no files added).
Expected: `Build succeeded.`

- [ ] **Step 4: Run the tests**

Run `[TEST]`.
Expected: `ArrangerModel | ... | failures: 0`, and the existing `ArrangerEngine`/`ArrangerScheduler`/etc. still `failures: 0`, `ExitCode = 0`.

- [ ] **Step 5: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerModel.h tests/unit/test_arranger_model.cpp
git commit -m "feat(arranger): add section after-complete rule and bump schema to v2"
```

---

## Task 2: Scheduler — flush hung notes at a section swap

**Context:** `ArrangerScheduler` already closes notes at a *loop seam*. A *section switch* leaves the outgoing section mid-loop, so its held notes would never be closed. We add an explicit flush that emits note-offs for whatever is currently sounding and clears the active-note set. (See `Source/Arranger/ArrangerScheduler.cpp:585` for the existing `activeNotes` handling.)

**Files:**
- Modify: `Source/Arranger/ArrangerScheduler.h`, `Source/Arranger/ArrangerScheduler.cpp`
- Modify: `tests/unit/test_arranger_scheduler.cpp`

- [ ] **Step 1: Add the failing test**

In `tests/unit/test_arranger_scheduler.cpp`, inside `runTest()` add:
```cpp
        beginTest ("flushActiveNotes closes sounding notes and then nothing");
        {
            ArrangerScheduler s;
            // A held note on at beat 0 with no note-off in the bar.
            s.setLoop ({ { 0.0, juce::MidiMessage::noteOn (3, 64, (juce::uint8) 100) } }, 4.0);
            s.advance (0.0, 1.0);                 // turns (3,64) on

            auto flushed = s.flushActiveNotes (1.0);
            expectEquals ((int) flushed.size(), 1);
            expect (flushed[0].message.isNoteOff());
            expectEquals (flushed[0].message.getChannel(), 3);
            expectEquals (flushed[0].message.getNoteNumber(), 64);

            auto again = s.flushActiveNotes (1.0); // nothing left to close
            expectEquals ((int) again.size(), 0);
        }
```

- [ ] **Step 2: Declare it in the header**

In `Source/Arranger/ArrangerScheduler.h`, in the `public:` section after `reset();`, add:
```cpp
    /** Emit note-offs for every currently-sounding note (used when switching sections
        mid-loop, where the normal loop-seam close would not fire). Clears active-note state. */
    std::vector<EmittedEvent> flushActiveNotes (double atBeats);
```

- [ ] **Step 3: Implement it**

In `Source/Arranger/ArrangerScheduler.cpp`, after `reset()`'s definition, add:
```cpp
std::vector<EmittedEvent> ArrangerScheduler::flushActiveNotes (double atBeats)
{
    std::vector<EmittedEvent> result;
    for (const auto& key : activeNotes)
        result.push_back ({ atBeats, juce::MidiMessage::noteOff (key.first, key.second) });
    activeNotes.clear();
    return result;
}
```

- [ ] **Step 4: Build and test**

Run `[BUILD]` then `[TEST]`.
Expected: `ArrangerScheduler | ... | failures: 0`, `ExitCode = 0`.

- [ ] **Step 5: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerScheduler.h Source/Arranger/ArrangerScheduler.cpp tests/unit/test_arranger_scheduler.cpp
git commit -m "feat(arranger): add scheduler flushActiveNotes for section swaps"
```

---

## Task 3: ArrangerSectionSequencer — the pure state machine

**Files:**
- Create: `Source/Arranger/ArrangerSectionSequencer.h`, `Source/Arranger/ArrangerSectionSequencer.cpp`
- Create: `tests/unit/test_arranger_section_sequencer.cpp`
- Modify: `Project Synth2.jucer`

- [ ] **Step 1: Write the failing tests**

Create `tests/unit/test_arranger_section_sequencer.cpp`:
```cpp
#include <juce_core/juce_core.h>
#include "Arranger/ArrangerSectionSequencer.h"

class ArrangerSectionSequencerTest : public juce::UnitTest
{
public:
    ArrangerSectionSequencerTest() : juce::UnitTest ("ArrangerSectionSequencer", "Arranger") {}

    static ArrangerSection makeSection (const juce::String& id, const juce::String& name,
                                        ArrangerSectionType type, int bars,
                                        ArrangerAfterComplete after)
    {
        ArrangerSection s;
        s.id = id; s.name = name; s.type = type; s.lengthBars = bars; s.afterComplete = after;
        return s;
    }

    static ArrangerStyle twoVariations()
    {
        ArrangerStyle st; st.timeSigNum = 4; st.timeSigDenom = 4;
        st.sections.push_back (makeSection ("var_1", "Variation 1", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));
        st.sections.push_back (makeSection ("var_2", "Variation 2", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));
        return st;
    }

    void runTest() override
    {
        beginTest ("starts on the first variation");
        {
            ArrangerSectionSequencer seq;
            seq.setStyle (twoVariations());
            expectEquals (seq.getActiveIndex(), 0);
        }

        beginTest ("queued switch applies at the next bar boundary, not mid-bar");
        {
            ArrangerSectionSequencer seq;
            seq.setStyle (twoVariations());
            expect (seq.queue (ArrangerSectionType::Variation, "Variation 2"));

            auto a = seq.advance (0.0, 2.0);   // still inside bar 1
            expectEquals (seq.getActiveIndex(), 0);
            for (auto& s : a.segments) expectEquals (s.sectionIndex, 0);

            auto b = seq.advance (2.0, 5.0);   // crosses the bar line at beat 4
            expectEquals (seq.getActiveIndex(), 1);
            // last segment is the new section, flagged as a change
            expectEquals (b.segments.back().sectionIndex, 1);
            expect (b.segments.back().sectionChanged);
            expectWithinAbsoluteError (b.segments.back().localFromBeats, 0.0, 1e-9); // new section enters at its bar 0
        }

        beginTest ("double-press before the boundary keeps only the last request");
        {
            ArrangerSectionSequencer seq;
            ArrangerStyle st = twoVariations();
            st.sections.push_back (makeSection ("var_3", "Variation 3", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));
            seq.setStyle (st);
            seq.queue (ArrangerSectionType::Variation, "Variation 2");
            seq.queue (ArrangerSectionType::Variation, "Variation 3"); // replaces the pending one
            seq.advance (0.0, 5.0);
            expectEquals (seq.getActiveIndex(), 2);
        }

        beginTest ("a Loop section never auto-transitions");
        {
            ArrangerSectionSequencer seq;
            seq.setStyle (twoVariations());
            auto out = seq.advance (0.0, 100.0); // 25 bars
            expectEquals (seq.getActiveIndex(), 0);
            expect (! out.stopRequested);
        }

        beginTest ("Intro plays once then falls into the variation");
        {
            ArrangerStyle st; st.timeSigNum = 4; st.timeSigDenom = 4;
            st.sections.push_back (makeSection ("intro_1", "Intro 1", ArrangerSectionType::Intro, 1, ArrangerAfterComplete::FallThrough));
            st.sections.push_back (makeSection ("var_1", "Variation 1", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.startAt (0);                 // begin on the intro
            expectEquals (seq.getActiveIndex(), 0);

            auto out = seq.advance (0.0, 4.0); // exactly the intro's length
            expectEquals (seq.getActiveIndex(), 1); // fell through to the variation
            for (auto& s : out.segments) expectEquals (s.sectionIndex, 0); // intro played the whole bar
        }

        beginTest ("Fill plays once then returns to the variation it was triggered from");
        {
            ArrangerStyle st; st.timeSigNum = 4; st.timeSigDenom = 4;
            st.sections.push_back (makeSection ("var_1", "Variation 1", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));
            st.sections.push_back (makeSection ("var_2", "Variation 2", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));
            st.sections.push_back (makeSection ("fill_1", "Fill 1", ArrangerSectionType::Fill, 1, ArrangerAfterComplete::FallThrough));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.queue (ArrangerSectionType::Variation, "Variation 2");
            seq.advance (0.0, 5.0);          // now on Variation 2
            expectEquals (seq.getActiveIndex(), 1);

            seq.queue (ArrangerSectionType::Fill, "Fill 1");
            seq.advance (5.0, 9.0);          // boundary at 8 -> enter fill
            expectEquals (seq.getActiveIndex(), 2);

            seq.advance (9.0, 13.0);         // boundary at 12 -> fill done -> back to Variation 2
            expectEquals (seq.getActiveIndex(), 1);
        }

        beginTest ("Ending plays once then requests stop");
        {
            ArrangerStyle st; st.timeSigNum = 4; st.timeSigDenom = 4;
            st.sections.push_back (makeSection ("var_1", "Variation 1", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));
            st.sections.push_back (makeSection ("ending_1", "Ending 1", ArrangerSectionType::Ending, 1, ArrangerAfterComplete::Stop));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.queue (ArrangerSectionType::Ending, "Ending 1");
            seq.advance (0.0, 5.0);          // boundary at 4 -> enter ending
            expectEquals (seq.getActiveIndex(), 1);

            auto out = seq.advance (5.0, 9.0); // boundary at 8 -> ending complete
            expect (out.stopRequested);
            auto none = seq.advance (9.0, 13.0); // stopped: no further segments
            expect (none.segments.empty());
        }

        beginTest ("a multi-bar window resolves transitions in order");
        {
            ArrangerStyle st; st.timeSigNum = 4; st.timeSigDenom = 4;
            st.sections.push_back (makeSection ("intro_1", "Intro 1", ArrangerSectionType::Intro, 1, ArrangerAfterComplete::FallThrough));
            st.sections.push_back (makeSection ("var_1", "Variation 1", ArrangerSectionType::Variation, 1, ArrangerAfterComplete::Loop));

            ArrangerSectionSequencer seq;
            seq.setStyle (st);
            seq.startAt (0);
            auto out = seq.advance (0.0, 12.0);  // intro bar, then 2 variation bars, all in one call
            expect (out.segments.size() >= 2);
            expectEquals (out.segments.front().sectionIndex, 0); // starts on intro
            expectEquals (out.segments.back().sectionIndex, 1);  // ends on variation
            expectEquals (seq.getActiveIndex(), 1);
        }
    }
};

static ArrangerSectionSequencerTest arrangerSectionSequencerTest;
```

- [ ] **Step 2: Implement the header**

Create `Source/Arranger/ArrangerSectionSequencer.h`:
```cpp
#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include <vector>

/** One contiguous slice of an advance window during which a single section is active.
    `localFromBeats`/`localToBeats` are section-relative (feed the section's scheduler). */
struct SectionSegment
{
    int    sectionIndex   = 0;
    double localFromBeats = 0.0;
    double localToBeats   = 0.0;
    bool   sectionChanged = false; // true when a switch happened at this segment's start
};

/** Result of advancing the sequencer. */
struct SequencerStep
{
    std::vector<SectionSegment> segments;
    bool                        stopRequested = false; // an Ending finished
};

/**
 * Pure section state machine. Decides which section is active across a monotonic beat
 * window, applying queued switches and one-shot completions only at bar boundaries.
 * Emits no MIDI; the engine maps segments onto per-section schedulers. Fully unit-testable.
 */
class ArrangerSectionSequencer
{
public:
    void setStyle (const ArrangerStyle& style);   // records metadata, picks initial section
    void reset();                                 // back to the initial section at beat 0
    void startAt (int sectionIndex);              // begin on a specific section (e.g. Intro)

    /** Queue a switch to the section matching type (+name as tiebreaker), applied at the
        next bar boundary. Last call before the boundary wins. False if no such section. */
    bool queue (ArrangerSectionType type, const juce::String& name);

    /** Advance the monotonic playhead from `fromBeats` to `toBeats` (from <= to). */
    SequencerStep advance (double fromBeats, double toBeats);

    int    getActiveIndex() const { return activeIndex; }
    double getBeatsPerBar() const { return beatsPerBar; }

private:
    struct Info
    {
        juce::String          id, name;
        ArrangerSectionType   type  = ArrangerSectionType::Variation;
        int                   lengthBars = 1;
        ArrangerAfterComplete after = ArrangerAfterComplete::Loop;
    };

    int  firstVariationIndex() const;
    int  findSection (ArrangerSectionType type, const juce::String& name) const;
    void applyBoundary (double boundaryAbs, SequencerStep& step);

    std::vector<Info> sections;
    double beatsPerBar    = 4.0;

    int    activeIndex    = 0;
    int    returnIndex    = 0;     // variation to fall back to after Intro/Fill
    double activeStartAbs = 0.0;   // absolute beat where the active section began
    int    pendingIndex   = -1;    // queued target, -1 = none
    bool   stopped        = false;
};
```

- [ ] **Step 3: Implement the source**

Create `Source/Arranger/ArrangerSectionSequencer.cpp`:
```cpp
#include "ArrangerSectionSequencer.h"
#include "ArrangerTime.h"
#include <algorithm>
#include <cmath>

void ArrangerSectionSequencer::setStyle (const ArrangerStyle& style)
{
    sections.clear();
    for (const auto& s : style.sections)
        sections.push_back ({ s.id, s.name, s.type, juce::jmax (1, s.lengthBars), s.afterComplete });

    beatsPerBar = ArrangerTime::beatsPerBar (style.timeSigNum, style.timeSigDenom);
    reset();
}

int ArrangerSectionSequencer::firstVariationIndex() const
{
    for (int i = 0; i < (int) sections.size(); ++i)
        if (sections[i].type == ArrangerSectionType::Variation)
            return i;
    return sections.empty() ? -1 : 0;
}

int ArrangerSectionSequencer::findSection (ArrangerSectionType type, const juce::String& name) const
{
    int firstOfType = -1, nameMatch = -1;
    for (int i = 0; i < (int) sections.size(); ++i)
    {
        if (sections[i].type != type) continue;
        if (firstOfType < 0) firstOfType = i;
        if (sections[i].name == name) nameMatch = i;
    }
    return (nameMatch >= 0) ? nameMatch : firstOfType;
}

void ArrangerSectionSequencer::reset()
{
    activeIndex    = juce::jmax (0, firstVariationIndex());
    returnIndex    = activeIndex;
    activeStartAbs = 0.0;
    pendingIndex   = -1;
    stopped        = false;
}

void ArrangerSectionSequencer::startAt (int sectionIndex)
{
    if (sections.empty()) { reset(); return; }
    activeIndex    = juce::jlimit (0, (int) sections.size() - 1, sectionIndex);
    returnIndex    = juce::jmax (0, firstVariationIndex());
    activeStartAbs = 0.0;
    pendingIndex   = -1;
    stopped        = false;
}

bool ArrangerSectionSequencer::queue (ArrangerSectionType type, const juce::String& name)
{
    const int target = findSection (type, name);
    if (target < 0) return false;
    pendingIndex = target;
    return true;
}

void ArrangerSectionSequencer::applyBoundary (double boundaryAbs, SequencerStep& step)
{
    // 1) A queued user switch always wins.
    if (pendingIndex >= 0)
    {
        const int from = activeIndex;
        if (sections[pendingIndex].type == ArrangerSectionType::Fill)
        {
            // Remember the variation/break to return to after the fill.
            if (sections[from].type == ArrangerSectionType::Variation
                || sections[from].type == ArrangerSectionType::Break)
                returnIndex = from;
        }
        activeIndex    = pendingIndex;
        pendingIndex   = -1;
        activeStartAbs = boundaryAbs;
        return;
    }

    // 2) One-shot completion (Intro/Fill/Ending) when the section's own length has elapsed.
    const auto& a = sections[activeIndex];
    if (a.after == ArrangerAfterComplete::Loop)
        return;

    const double sectionLenBeats = (double) a.lengthBars * beatsPerBar;
    if (boundaryAbs - activeStartAbs < sectionLenBeats - 1e-6)
        return; // multi-bar one-shot not finished yet

    if (a.after == ArrangerAfterComplete::Stop)
    {
        stopped = true;
        step.stopRequested = true;
        return;
    }

    // FallThrough: Intro/Fill -> the saved return variation.
    activeIndex    = juce::jlimit (0, (int) sections.size() - 1, returnIndex);
    activeStartAbs = boundaryAbs;
}

SequencerStep ArrangerSectionSequencer::advance (double fromBeats, double toBeats)
{
    SequencerStep step;
    if (sections.empty() || stopped || toBeats <= fromBeats || beatsPerBar <= 0.0)
        return step;

    double pos = fromBeats;
    bool   startChanged = false;

    while (pos < toBeats - 1e-12)
    {
        // Next global bar line strictly after pos.
        double nextBoundary = (std::floor (pos / beatsPerBar + 1e-9) + 1.0) * beatsPerBar;
        if (nextBoundary <= pos + 1e-9)
            nextBoundary += beatsPerBar;

        const double segEnd = std::min (toBeats, nextBoundary);

        SectionSegment seg;
        seg.sectionIndex   = activeIndex;
        seg.localFromBeats = pos - activeStartAbs;
        seg.localToBeats   = segEnd - activeStartAbs;
        seg.sectionChanged = startChanged;
        step.segments.push_back (seg);
        startChanged = false;

        pos = segEnd;

        // Reached a bar boundary inside the window: apply transitions there.
        if (std::abs (segEnd - nextBoundary) < 1e-9 && segEnd <= toBeats + 1e-12)
        {
            const int before = activeIndex;
            applyBoundary (nextBoundary, step);
            if (step.stopRequested)
                break;
            if (activeIndex != before)
                startChanged = true;
        }
    }

    return step;
}
```

- [ ] **Step 4: Register in the `.jucer`**

In the `Source` GROUP add:
```xml
      <FILE id="arrSq1" name="ArrangerSectionSequencer.cpp" compile="1" resource="0" file="Source/Arranger/ArrangerSectionSequencer.cpp"/>
      <FILE id="arrSq2" name="ArrangerSectionSequencer.h" compile="0" resource="0" file="Source/Arranger/ArrangerSectionSequencer.h"/>
```
In the `Tests` → `Unit` GROUP add:
```xml
        <FILE id="arrTs5" name="test_arranger_section_sequencer.cpp" compile="1" resource="0" file="tests/unit/test_arranger_section_sequencer.cpp"/>
```

- [ ] **Step 5: Regenerate, build, test**

Run `[RESAVE]`, `[BUILD]`, `[TEST]`.
Expected: `ArrangerSectionSequencer | ... | failures: 0`, `ExitCode = 0`. If a boundary test fails, the usual culprit is the `nextBoundary` math or the one-shot-completion epsilon — fix before moving on.

- [ ] **Step 6: Commit** (ask approval first)
```powershell
git add Source/Arranger/ArrangerSectionSequencer.h Source/Arranger/ArrangerSectionSequencer.cpp tests/unit/test_arranger_section_sequencer.cpp
git commit -m "feat(arranger): add pure section state machine (sequencer)"
```

---


> Continued in [part 2 of 2](2026-06-05-arranger-engine-phase2-part2.md).
