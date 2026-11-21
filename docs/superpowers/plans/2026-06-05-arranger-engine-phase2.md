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