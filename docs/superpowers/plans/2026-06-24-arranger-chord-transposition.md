# Arranger Chord Transposition (Phase 4) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the arranger accompaniment follow the chord the player fingers in the left-hand zone — chord-aware (NTT) transposition of pitched parts (Bass/Acc), drums/perc never moving.

**Architecture:** Two new pure units — `ChordDetector` (held notes → `Chord`) and `ChordTransposer` (note + original/active chord → transposed note) — wired into the existing real-time engine via a lock-guarded chord mailbox (mirroring `requestLock`/`queueSection`). Events are tagged transposable at merge time; the engine's timer thread applies transposition at emit, reusing per-sounding-note pitch maps so note-offs always match their note-ons. Original chord is style-level metadata, auto-detected at authoring, editable, and persisted (schema v3→v4).

**Tech Stack:** C++17, JUCE (Projucer/VS2022), `juce::UnitTest` test framework. Tests live in `tests/unit/`, compile into the app, run via `Project Synth2.exe --run-tests --arranger-tests`.

## Global Constraints

- **Build:** MSBuild Debug x64 via Projucer-generated solution. New `.cpp`/`.h` files must be registered in `Project Synth2.jucer` (Projucer `--resave`). `.jucer` is tracked-but-git-ignored; file-registration changes are NOT committed.
- **After a `--resave` that adds files, the FIRST MSBuild may not compile them — build twice** (or grep build output to confirm the new `.cpp` compiled).
- **Close the running app before building** (it locks `Project Synth2.pdb` → C1041).
- **Tests:** `juce::UnitTest` subclasses, statically registered at file scope. Category `"Arranger"`. Run: launch the exe with `--run-tests --arranger-tests` and `-WorkingDirectory` = project root; results land in `test-results.txt` in the working dir.
- **Commits:** NEVER auto-commit. Each task ends with a commit *command* the user runs/approves. NO `Co-Authored-By` / Claude attribution in messages. Do not `git add` the `.jucer`.
- **Pure units must not depend on JUCE GUI/threads** — only `juce_core` + `juce_audio_basics` (for `MidiMessage`), like the existing arranger units.
- **Threading invariant:** the engine timer thread is the sole mutator/reader of playback + transposer state. Other threads only write lock-guarded mailboxes.

---

## File Structure

**New files:**
- `Source/Arranger/Chord.h` — `Chord` struct, `ChordQuality` enum, enum↔string helpers. Header-only (like `ArrangerModel.h`).
- `Source/Arranger/ChordDetector.h` / `.cpp` — held-note tracking → `Chord` recognition.
- `Source/Arranger/ChordTransposer.h` / `.cpp` — note transposition (block shift → NTT).
- `tests/unit/test_chord.cpp` — enum↔string + `Chord` helpers.
- `tests/unit/test_chord_detector.cpp` — recognition incl. inversions.
- `tests/unit/test_chord_transposer.cpp` — transposition incl. drums/clamp/NTT.

**Modified files:**
- `Source/Arranger/ArrangerScheduler.h` / `.cpp` — `transposable` tag on stored + emitted events.
- `Source/Arranger/ArrangerEngine.h` / `.cpp` — chord mailbox, `setActiveChord`, `ChordTransposer` member, merge tagging, emit-time transposition with per-note pitch map, `setOriginalChord`.
- `Source/Arranger/ArrangerModel.h` — style-level `originalRoot`/`originalQuality`; retire per-track `originalChord`.
- `Source/Arranger/ArrangerStyleFile.h` — style-level fields; `schemaVersion` 3→4.
- `Source/Arranger/ArrangerStyleIOHelper.cpp` — (de)serialize new fields, v3 default.
- `Source/Midi/MidiHandler.h` / `.cpp` — `ChordScanArea`, owned `ChordDetector`, forward chord-zone notes, push `setActiveChord`, Chord Memory.
- `Source/Arranger/ArrangerStyleEditor.h` / `.cpp` — root+quality combos, auto-detect pre-fill, save.
- `Source/Styles/CurrentStyleComponent.h` / `.cpp` — auto-detect at build/Update-Tracks, Bass Inversion + scan-area toggles, pass original chord to engine.
- `tests/unit/test_arranger_scheduler.cpp`, `test_arranger_engine.cpp`, `test_arranger_style_io.cpp` — extend.
- `Project Synth2.jucer` — register new files (not committed).

---

# STAGE 0 — Pre-work: remove the latent `channel` data race (free)

Goal: `MidiHandler::channel` is a member overwritten by `setCorrectChannelBasedOnHand`, which runs on both the MIDI input thread (`handleIncomingMidiMessage`, under `midiMutex`) and the message thread (`noteOnKeyboard`/`noteOffKeyboard` via `KeyboardListener`, NOT under the lock). In normal use the two input sources are mutually exclusive, so it's effectively unreachable — but `channel` never needs to persist between notes, so we delete the shared state entirely (no lock, no cost).

---

### Task 0: Make `channel` a returned local, not a shared member

**Files:**
- Modify: `Source/Midi/MidiHandler.h` (replace `int channel` member with a method), `Source/Midi/MidiHandler.cpp`
- (No unit test — pure refactor; covered by existing MIDI tests + manual smoke. Behaviour is identical.)

**Interfaces:**
- Produces: `int MidiHandler::channelForHand (int note) const;` returning 16 for the right zone, 1 otherwise. Removes the `int channel` member and `void setCorrectChannelBasedOnHand(int)`.

- [ ] **Step 1: Add the pure helper** in `MidiHandler.cpp`, replacing `setCorrectChannelBasedOnHand`:

```cpp
int MidiHandler::channelForHand (int note) const
{
    return (rightHandBoundSetting != -1 && note >= rightHandBoundSetting) ? 16 : 1;
}
```
Declare `int channelForHand (int note) const;` in `MidiHandler.h`; delete the `int channel = 1;` member and the old `setCorrectChannelBasedOnHand` declaration.

- [ ] **Step 2: Update call sites** — in `handleIncomingMidiMessage` (note-on and note-off branches), `noteOnKeyboard`, and `noteOffKeyboard`, replace `setCorrectChannelBasedOnHand(note);` + uses of `channel` with a local:

```cpp
const int channel = channelForHand (note);
```
Place it where `setCorrectChannelBasedOnHand` was called; the rest of each handler already uses a `channel` name, so the local drops in.

- [ ] **Step 3: Grep for any other reader** — `Grep "this->channel|[^a-zA-Z]channel[^a-zA-Z=]"` in `MidiHandler.cpp`/`.h`. Confirm no code path relies on `channel` persisting between calls (expected: none). If a getter exists, point it at a sensible default or remove it.

- [ ] **Step 4: Build (twice if `.jucer` untouched here — no new files, so a single build), run existing MIDI tests** — `--run-tests --unit-tests`. Expected: PASS, behaviour unchanged.

- [ ] **Step 5: Commit**

```bash
git add Source/Midi/MidiHandler.h Source/Midi/MidiHandler.cpp
git commit -m "refactor(midi): make channel a per-note local, removing input/UI thread shared state"
```

---

# STAGE 1 — Walking skeleton (root-only, end-to-end)

Goal: finger a root in the left hand → whole pitched backing shifts; drums don't. Smarts stubbed.

---

### Task 1: `Chord` value type + enum↔string helpers

**Files:**
- Create: `Source/Arranger/Chord.h`
- Test: `tests/unit/test_chord.cpp`

**Interfaces:**
- Produces:
  - `enum class ChordQuality { None, Maj, Min, Dom7, Maj7, Min7, Dim, HalfDim, Aug, Sus2, Sus4 };`
  - `struct Chord { int root = -1; ChordQuality quality = ChordQuality::None; int bassNote = -1; bool isValid() const; };`
  - `juce::String toString (ChordQuality);` and `ChordQuality chordQualityFromString (const juce::String&);`

- [ ] **Step 1: Write the failing test** — `tests/unit/test_chord.cpp`

```cpp
#include <juce_core/juce_core.h>
#include "Arranger/Chord.h"

class ChordTest : public juce::UnitTest
{
public:
    ChordTest() : juce::UnitTest ("Chord", "Arranger") {}

    void runTest() override
    {
        beginTest ("quality enum round-trips through string");
        for (auto q : { ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dom7,
                        ChordQuality::Maj7, ChordQuality::Min7, ChordQuality::Dim,
                        ChordQuality::HalfDim, ChordQuality::Aug, ChordQuality::Sus2,
                        ChordQuality::Sus4 })
            expect (chordQualityFromString (toString (q)) == q);

        beginTest ("unknown string maps to None");
        expect (chordQualityFromString ("not-a-quality") == ChordQuality::None);

        beginTest ("isValid requires a root and a real quality");
        expect (! Chord{}.isValid());
        expect (! (Chord{ 0, ChordQuality::None, -1 }).isValid());
        expect ((Chord{ 0, ChordQuality::Maj, -1 }).isValid());
    }
};
static ChordTest chordTest;
```

- [ ] **Step 2: Run test to verify it fails** — build, then `Project Synth2.exe --run-tests --arranger-tests`. Expected: compile error (`Chord.h` missing) or FAIL.

- [ ] **Step 3: Write `Source/Arranger/Chord.h`**

```cpp
#pragma once
#include <juce_core/juce_core.h>

/** Quality of a recognized chord (extensible; None = unrecognized). */
enum class ChordQuality { None, Maj, Min, Dom7, Maj7, Min7, Dim, HalfDim, Aug, Sus2, Sus4 };

/** A recognized chord: pitch-class root (0=C..11=B), quality, and the lowest
    fingered pitch class (for Bass Inversion). root=-1 / quality=None = "no chord". */
struct Chord
{
    int          root     = -1;   // 0..11, -1 = none
    ChordQuality quality  = ChordQuality::None;
    int          bassNote = -1;   // pitch class of lowest note; -1 = use root

    bool isValid() const { return root >= 0 && quality != ChordQuality::None; }
    bool operator== (const Chord& o) const
        { return root == o.root && quality == o.quality && bassNote == o.bassNote; }
};

inline juce::String toString (ChordQuality q)
{
    switch (q) {
        case ChordQuality::Maj:     return "Maj";
        case ChordQuality::Min:     return "Min";
        case ChordQuality::Dom7:    return "Dom7";
        case ChordQuality::Maj7:    return "Maj7";
        case ChordQuality::Min7:    return "Min7";
        case ChordQuality::Dim:     return "Dim";
        case ChordQuality::HalfDim: return "HalfDim";
        case ChordQuality::Aug:     return "Aug";
        case ChordQuality::Sus2:    return "Sus2";
        case ChordQuality::Sus4:    return "Sus4";
        case ChordQuality::None:    return "None";
    }
    return "None";
}

inline ChordQuality chordQualityFromString (const juce::String& s)
{
    if (s == "Maj")     return ChordQuality::Maj;
    if (s == "Min")     return ChordQuality::Min;
    if (s == "Dom7")    return ChordQuality::Dom7;
    if (s == "Maj7")    return ChordQuality::Maj7;
    if (s == "Min7")    return ChordQuality::Min7;
    if (s == "Dim")     return ChordQuality::Dim;
    if (s == "HalfDim") return ChordQuality::HalfDim;
    if (s == "Aug")     return ChordQuality::Aug;
    if (s == "Sus2")    return ChordQuality::Sus2;
    if (s == "Sus4")    return ChordQuality::Sus4;
    return ChordQuality::None;
}
```

- [ ] **Step 4: Register `tests/unit/test_chord.cpp` + `Chord.h` in `Project Synth2.jucer`** (Projucer → Add Existing Files → `--resave`). Build **twice** (new-file quirk).

- [ ] **Step 5: Run test to verify it passes** — `--run-tests --arranger-tests`. Expected: `Chord` tests PASS in `test-results.txt`.

- [ ] **Step 6: Commit** (do NOT add the `.jucer`)

```bash
git add Source/Arranger/Chord.h tests/unit/test_chord.cpp
git commit -m "feat(arranger): add Chord value type and quality string helpers"
```

---

### Task 2: `ChordDetector` (Stage-1 root-only)

**Files:**
- Create: `Source/Arranger/ChordDetector.h`, `Source/Arranger/ChordDetector.cpp`
- Test: `tests/unit/test_chord_detector.cpp`

**Interfaces:**
- Consumes: `Chord`, `ChordQuality` (Task 1).
- Produces:
  - `class ChordDetector { void noteOn(int); void noteOff(int); void reset(); Chord current() const; };`
  - Stage 1 contract: with ≥1 held note, `current().root` = pitch class of the **lowest** held note, `quality = Maj` (placeholder), `bassNote` = lowest held pitch class; with 0 held notes, `current()` = invalid `Chord`.

- [ ] **Step 1: Write the failing test** — `tests/unit/test_chord_detector.cpp`

```cpp
#include <juce_core/juce_core.h>
#include "Arranger/ChordDetector.h"

class ChordDetectorTest : public juce::UnitTest
{
public:
    ChordDetectorTest() : juce::UnitTest ("ChordDetector", "Arranger") {}

    void runTest() override
    {
        beginTest ("no held notes -> invalid chord");
        {
            ChordDetector d;
            expect (! d.current().isValid());
        }

        beginTest ("root is the pitch class of the lowest held note (stage 1)");
        {
            ChordDetector d;
            d.noteOn (60); d.noteOn (64); d.noteOn (67);   // C4 E4 G4
            expectEquals (d.current().root, 0);            // C
            expectEquals (d.current().bassNote, 0);        // lowest = C
        }

        beginTest ("noteOff removes notes; emptying returns invalid");
        {
            ChordDetector d;
            d.noteOn (62); d.noteOff (62);
            expect (! d.current().isValid());
        }

        beginTest ("lowest note defines root regardless of add order");
        {
            ChordDetector d;
            d.noteOn (67); d.noteOn (60); d.noteOn (64);   // added high-first
            expectEquals (d.current().root, 0);            // still C (lowest)
        }
    }
};
static ChordDetectorTest chordDetectorTest;
```

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error / FAIL.

- [ ] **Step 3: Write `ChordDetector.h`**

```cpp
#pragma once
#include <juce_core/juce_core.h>
#include "Chord.h"
#include <set>

/** Tracks held notes in the chord zone and recognizes a Chord.
    Stage 1: root = pitch class of the lowest held note, quality = Maj placeholder. */
class ChordDetector
{
public:
    void  noteOn  (int noteNumber);
    void  noteOff (int noteNumber);
    void  reset();
    Chord current() const;
private:
    std::set<int> heldNotes;   // absolute note numbers
};
```

- [ ] **Step 4: Write `ChordDetector.cpp`**

```cpp
#include "ChordDetector.h"

void ChordDetector::noteOn  (int n) { heldNotes.insert (n); }
void ChordDetector::noteOff (int n) { heldNotes.erase (n); }
void ChordDetector::reset()         { heldNotes.clear(); }

Chord ChordDetector::current() const
{
    if (heldNotes.empty())
        return {};

    const int lowest = *heldNotes.begin();   // std::set is ordered ascending
    Chord c;
    c.root     = lowest % 12;
    c.quality  = ChordQuality::Maj;          // stage-1 placeholder
    c.bassNote = lowest % 12;
    return c;
}
```

- [ ] **Step 5: Register both files in `.jucer`, `--resave`, build twice.**

- [ ] **Step 6: Run test to verify it passes** — Expected: `ChordDetector` PASS.

- [ ] **Step 7: Commit**

```bash
git add Source/Arranger/ChordDetector.h Source/Arranger/ChordDetector.cpp tests/unit/test_chord_detector.cpp
git commit -m "feat(arranger): add ChordDetector with stage-1 root-only recognition"
```

---

### Task 3: `ChordTransposer` (Stage-1 block shift)

**Files:**
- Create: `Source/Arranger/ChordTransposer.h`, `Source/Arranger/ChordTransposer.cpp`
- Test: `tests/unit/test_chord_transposer.cpp`

**Interfaces:**
- Consumes: `Chord` (Task 1).
- Produces:
  - `class ChordTransposer { void setOriginalChord(Chord); void setActiveChord(Chord); int transpose(int noteNumber, bool transposable) const; };`
  - Stage 1 contract: transposable note → `note + (active.root - original.root)`, octave-folded into 0..127; non-transposable → unchanged; invalid active chord → unchanged.

- [ ] **Step 1: Write the failing test** — `tests/unit/test_chord_transposer.cpp`

```cpp
#include <juce_core/juce_core.h>
#include "Arranger/ChordTransposer.h"

class ChordTransposerTest : public juce::UnitTest
{
public:
    ChordTransposerTest() : juce::UnitTest ("ChordTransposer", "Arranger") {}

    void runTest() override
    {
        beginTest ("block shift by root interval for transposable notes");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });  // C
            t.setActiveChord   ({ 2, ChordQuality::Maj, 2 });  // D  (+2)
            expectEquals (t.transpose (60, true), 62);
            expectEquals (t.transpose (67, true), 69);
        }

        beginTest ("non-transposable notes (drums) are unchanged");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 5, ChordQuality::Maj, 5 });  // F (+5)
            expectEquals (t.transpose (36, false), 36);        // kick stays
        }

        beginTest ("invalid active chord -> identity");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({});                            // none
            expectEquals (t.transpose (60, true), 60);
        }

        beginTest ("result stays in 0..127 (octave fold)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 11, ChordQuality::Maj, 11 }); // B (+11 or -1)
            const int out = t.transpose (1, true);
            expect (out >= 0 && out <= 127);
        }
    }
};
static ChordTransposerTest chordTransposerTest;
```

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error / FAIL.

- [ ] **Step 3: Write `ChordTransposer.h`**

```cpp
#pragma once
#include "Chord.h"

/** Maps accompaniment notes from the recorded (original) chord to the played (active)
    chord. Stage 1: block shift by root interval. Stage 3 swaps in real NTT. */
class ChordTransposer
{
public:
    void setOriginalChord (Chord recorded) { original = recorded; }
    void setActiveChord   (Chord played)   { active = played; }

    /** transposable=false (Drum/Perc) returns the note unchanged. */
    int transpose (int noteNumber, bool transposable) const;
private:
    Chord original { 0, ChordQuality::Maj, 0 };   // default C major
    Chord active;
};
```

- [ ] **Step 4: Write `ChordTransposer.cpp`**

```cpp
#include "ChordTransposer.h"
#include <juce_core/juce_core.h>

int ChordTransposer::transpose (int noteNumber, bool transposable) const
{
    if (! transposable || ! active.isValid() || ! original.isValid())
        return noteNumber;

    int shifted = noteNumber + (active.root - original.root);
    while (shifted < 0)   shifted += 12;
    while (shifted > 127) shifted -= 12;
    return shifted;
}
```

- [ ] **Step 5: Register both files in `.jucer`, `--resave`, build twice.**

- [ ] **Step 6: Run test to verify it passes** — Expected: PASS.

- [ ] **Step 7: Commit**

```bash
git add Source/Arranger/ChordTransposer.h Source/Arranger/ChordTransposer.cpp tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): add ChordTransposer with stage-1 block shift"
```

---

### Task 4: Scheduler carries a `transposable` tag

**Files:**
- Modify: `Source/Arranger/ArrangerScheduler.h`, `Source/Arranger/ArrangerScheduler.cpp`
- Test: `tests/unit/test_arranger_scheduler.cpp` (extend)

**Interfaces:**
- Produces:
  - `struct EmittedEvent { double beats; juce::MidiMessage message; bool transposable = false; };`
  - New overload `void setLoop (std::vector<TimedBeatEvent> events, std::vector<char> transposable, double loopLengthBeats);`
  - Existing `setLoop(events, len)` retained, delegating with all-`false` (back-compat: no transposition).
  - `advance()` and `flushActiveNotes()` set `EmittedEvent::transposable` from the per-event tag; seam/flush note-offs carry the **same** tag as the note they close.

- [ ] **Step 1: Write the failing test** — append to `tests/unit/test_arranger_scheduler.cpp` `runTest()`

```cpp
        beginTest ("transposable tag flows from setLoop to emitted on AND its seam off");
        {
            ArrangerScheduler s;
            // one held transposable note (acc, ch2, note 60) on at beat 0, no off in bar.
            std::vector<TimedBeatEvent> evs { { 0.0, juce::MidiMessage::noteOn (2, 60, (juce::uint8) 90) } };
            std::vector<char> tags { 1 };   // transposable = true
            s.setLoop (evs, tags, 4.0);

            auto on = s.advance (0.0, 1.0);
            expectEquals ((int) on.size(), 1);
            expect (on[0].message.isNoteOn());
            expect (on[0].transposable);

            auto seam = s.advance (3.5, 4.5);   // crosses seam -> synthetic note-off
            bool foundOff = false;
            for (auto& e : seam)
                if (e.message.isNoteOff() && e.message.getNoteNumber() == 60)
                    { foundOff = true; expect (e.transposable); }
            expect (foundOff);
        }

        beginTest ("legacy 2-arg setLoop marks events non-transposable");
        {
            ArrangerScheduler s;
            s.setLoop ({ { 0.0, juce::MidiMessage::noteOn (2, 60, (juce::uint8) 90) } }, 4.0);
            auto on = s.advance (0.0, 1.0);
            expectEquals ((int) on.size(), 1);
            expect (! on[0].transposable);
        }
```

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error (new overload / field missing).

- [ ] **Step 3: Edit `ArrangerScheduler.h`** — add field + overload + parallel tag store

```cpp
struct EmittedEvent
{
    double beats = 0.0;
    juce::MidiMessage message;
    bool transposable = false;   // Phase 4: should this note follow the played chord?
};
// ... in class ArrangerScheduler public:
    void setLoop (std::vector<TimedBeatEvent> events, double loopLengthBeats);   // legacy: all non-transposable
    void setLoop (std::vector<TimedBeatEvent> events, std::vector<char> transposable, double loopLengthBeats);
// ... private:
    std::vector<char> sortedTransposable;            // aligned with sortedEvents (1 = transposable)
    std::set<std::pair<int,int>> activeTransposed;   // (channel,note) currently sounding AND transposable
```

- [ ] **Step 4: Edit `ArrangerScheduler.cpp`** — populate + carry the tag

```cpp
void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events, double loopLengthBeats)
{
    setLoop (std::move (events), std::vector<char> (events.size(), 0), loopLengthBeats);
}

void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events,
                                 std::vector<char> transposable, double loopLengthBeats)
{
    // sort events and the tag array together by beats
    std::vector<size_t> idx (events.size());
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    std::stable_sort (idx.begin(), idx.end(),
        [&] (size_t a, size_t b) { return events[a].beats < events[b].beats; });

    sortedEvents.clear(); sortedTransposable.clear();
    sortedEvents.reserve (events.size()); sortedTransposable.reserve (events.size());
    for (size_t i : idx)
    {
        sortedEvents.push_back (events[i]);
        sortedTransposable.push_back (i < transposable.size() ? transposable[i] : (char) 0);
    }
    loopLen = loopLengthBeats;
    activeNotes.clear();
    activeTransposed.clear();
}
```
In `trackActiveNote`, when adding a note also record whether it is transposable so the seam off can be tagged. Replace the seam-close loops and `flushActiveNotes` to set `EmittedEvent::transposable` from `activeTransposed`. In `advance()`'s emit loop set `e.transposable = sortedTransposable[i]` and call a tag-aware tracker:

```cpp
// in advance(), emit loop — index the event so we can read its tag:
for (size_t i = 0; i < sortedEvents.size(); ++i)
{
    const auto& ev = sortedEvents[i];
    if (ev.beats >= phaseStart - 1e-12 && ev.beats < phaseEnd - 1e-12)
    {
        EmittedEvent e { iterationStartAbs + ev.beats, ev.message, (bool) sortedTransposable[i] };
        result.push_back (e);
        trackActiveNote (ev.message, (bool) sortedTransposable[i]);
    }
}
// seam close:
for (const auto& key : activeNotes)
{
    const bool tr = activeTransposed.count (key) > 0;
    result.push_back ({ nextWrapAbs, juce::MidiMessage::noteOff (key.first, key.second), tr });
}
activeNotes.clear(); activeTransposed.clear();
```
Update `trackActiveNote` signature to `(const juce::MidiMessage& m, bool transposable)`: on note-on insert into `activeNotes` and, if transposable, into `activeTransposed`; on note-off erase from both. Update `flushActiveNotes` to tag each off via `activeTransposed.count(key)` then clear both sets.

- [ ] **Step 5: Run tests to verify they pass** — Expected: new + all existing scheduler tests PASS (legacy 2-arg path unchanged behaviour).

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerScheduler.h Source/Arranger/ArrangerScheduler.cpp tests/unit/test_arranger_scheduler.cpp
git commit -m "feat(arranger): carry transposable tag through scheduler events and seams"
```

---

### Task 5: Engine — merge tagging, chord mailbox, emit-time transposition

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.h`, `Source/Arranger/ArrangerEngine.cpp`
- Test: `tests/unit/test_arranger_engine.cpp` (extend)

**Interfaces:**
- Consumes: `ChordTransposer`, `Chord` (Tasks 1,3), tagged scheduler (Task 4).
- Produces:
  - `void ArrangerEngine::setActiveChord (Chord);`  (thread-safe; callable from input thread)
  - `void ArrangerEngine::setOriginalChord (Chord);`
  - `rebuildFromStyle` tags each merged event `transposable = (partType==Bass||partType==Acc)`.
  - Emit path transposes each event via the transposer using a per-sounding-note pitch map keyed by `(channel, originalNote)` so note-offs reuse the note-on's played pitch.

- [ ] **Step 1: Write the failing test** — append to `tests/unit/test_arranger_engine.cpp`

```cpp
        beginTest ("active chord transposes Acc notes but not Drum notes at emit");
        {
            // Build a tiny style: one section, one Acc track (ch2, note 60) + one Drum track (ch10, note 36),
            // both at beat 0, 4-beat loop. (Use the file's existing style-building helper.)
            ArrangerStyle style = makeAccAndDrumStyle();   // helper defined in the test file (see note)

            std::vector<juce::MidiMessage> emitted;
            ArrangerEngine eng (std::weak_ptr<juce::MidiOutput>{});
            eng.onMidiMessage = [&] (const juce::MidiMessage& m) { emitted.push_back (m); };
            eng.setStyle (style);
            eng.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            eng.setActiveChord   ({ 2, ChordQuality::Maj, 2 });   // D (+2)
            eng.start();
            eng.renderRange (0.0, 1.0);

            bool accShifted = false, drumKept = false;
            for (auto& m : emitted)
            {
                if (m.isNoteOn() && m.getChannel() == 2  && m.getNoteNumber() == 62) accShifted = true;
                if (m.isNoteOn() && m.getChannel() == 10 && m.getNoteNumber() == 36) drumKept   = true;
            }
            expect (accShifted);   // 60 -> 62
            expect (drumKept);     // drum unchanged
            eng.stop();
        }

        beginTest ("note-off matches the transposed note-on pitch");
        {
            ArrangerStyle style = makeAccHeldNoteStyle();   // Acc ch2 note60 held across the seam
            std::vector<juce::MidiMessage> emitted;
            ArrangerEngine eng (std::weak_ptr<juce::MidiOutput>{});
            eng.onMidiMessage = [&] (const juce::MidiMessage& m) { emitted.push_back (m); };
            eng.setStyle (style);
            eng.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            eng.setActiveChord   ({ 2, ChordQuality::Maj, 2 });   // +2
            eng.start();
            eng.renderRange (0.0, 4.5);   // cross the seam -> synthetic off
            eng.stop();

            int ons = 0, offs = 0;
            for (auto& m : emitted)
            {
                if (m.isNoteOn()  && m.getChannel()==2 && m.getNoteNumber()==62) ++ons;
                if (m.isNoteOff() && m.getChannel()==2 && m.getNoteNumber()==62) ++offs;
            }
            expect (ons >= 1 && offs >= 1);   // both at the TRANSPOSED pitch 62, none at 60
        }
```
> **Note for implementer:** add two small static helpers in `test_arranger_engine.cpp` that build these styles using the existing `ArrangerStyle`/`ArrangerSection`/`ArrangerTrack` structs (set `partType` = `Acc`/`Drum`, channel 2/10, one `TimedBeatEvent` each). Follow the construction already used elsewhere in this test file.

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error (`setActiveChord`/`setOriginalChord` missing) / FAIL.

- [ ] **Step 3: Edit `ArrangerEngine.h`** — add API + state

```cpp
#include "Chord.h"
#include "ChordTransposer.h"
// ... public:
    void setActiveChord   (Chord played);     // thread-safe (input thread)
    void setOriginalChord (Chord recorded);   // style-level "from" chord
// ... private:
    ChordTransposer       transposer;
    juce::CriticalSection chordLock;
    Chord                 pendingChord;
    bool                  hasChordUpdate = false;
    std::map<std::pair<int,int>, int> activePlayedNote;  // (channel, originalNote) -> played note
```

- [ ] **Step 4: Edit `ArrangerEngine.cpp`** — mailbox, tagging, emit transposition

```cpp
void ArrangerEngine::setActiveChord (Chord played)
{
    const juce::ScopedLock sl (chordLock);
    pendingChord = played;
    hasChordUpdate = true;
}

void ArrangerEngine::setOriginalChord (Chord recorded)
{
    transposer.setOriginalChord (recorded);   // set while stopped / from setStyle (timer-thread-safe enough; see note)
}
```
In `rebuildFromStyle`, tag during the merge:
```cpp
std::vector<TimedBeatEvent> merged;
std::vector<char>           tags;
for (const auto& tr : sec.tracks)
{
    const char t = (tr.partType == ArrangerPartType::Bass
                 || tr.partType == ArrangerPartType::Acc) ? 1 : 0;
    for (const auto& ev : tr.pattern) { merged.push_back (ev); tags.push_back (t); }
}
ArrangerScheduler s;
s.setLoop (std::move (merged), std::move (tags), bars * bpb);
```
At the **top of `renderRange`** (next to the queued-section drain), drain the chord mailbox:
```cpp
{
    const juce::ScopedLock sl (chordLock);
    if (hasChordUpdate) { transposer.setActiveChord (pendingChord); hasChordUpdate = false; }
}
```
Where events are dispatched, route every emitted event through transposition with the per-note map (replace the direct `dispatch(e.message)`):
```cpp
void ArrangerEngine::dispatchEmitted (const EmittedEvent& e)
{
    juce::MidiMessage m = e.message;
    if (m.isNoteOn() && e.transposable)
    {
        const int orig   = m.getNoteNumber();
        const int played = transposer.transpose (orig, true);
        activePlayedNote[{ m.getChannel(), orig }] = played;
        m = juce::MidiMessage::noteOn (m.getChannel(), played, m.getVelocity());
    }
    else if (m.isNoteOff())
    {
        auto it = activePlayedNote.find ({ m.getChannel(), m.getNoteNumber() });
        if (it != activePlayedNote.end())
        {
            m = juce::MidiMessage::noteOff (m.getChannel(), it->second);
            activePlayedNote.erase (it);
        }
    }
    dispatch (m);
}
```
Call `dispatchEmitted(e)` for each emitted event from the schedulers in `renderRange`. Clear `activePlayedNote` in `start()`/`haltAudio()` alongside the other reset state.
> **Note:** `setOriginalChord` is called from the message thread (via `setStyle`/config). Since transposition only reads `original` on the timer thread and it changes only while configuring (not mid-emit), a plain assignment is acceptable here; if you prefer, route it through the same `chordLock`. Keep it simple unless a test shows a race.

- [ ] **Step 5: Run tests to verify they pass** — Expected: new engine tests + all existing engine tests PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp tests/unit/test_arranger_engine.cpp
git commit -m "feat(arranger): emit-time chord transposition with mailbox and note-off pitch matching"
```

---

### Task 6: Wire live MIDI input → detector → engine (scan-area seam + Chord Memory)

**Files:**
- Modify: `Source/Midi/MidiHandler.h`, `Source/Midi/MidiHandler.cpp`
- (No new unit test — `handleIncomingMidiMessage` is hardware/threaded; verified via the manual smoke below. The detector/transposer/engine logic is already unit-covered.)

**Interfaces:**
- Consumes: `ChordDetector` (Task 2), `ArrangerEngine::setActiveChord` (Task 5).
- Produces: `enum class ChordScanArea { Split, Full };` + `void MidiHandler::setChordScanArea(ChordScanArea);` and a pointer/reference to the active `ArrangerEngine` so the handler can push chords.

- [ ] **Step 1: Edit `MidiHandler.h`** — add scan area, detector, engine hook

```cpp
#include "Arranger/ChordDetector.h"
// forward declare or include ArrangerEngine
enum class ChordScanArea { Split, Full };
// ... in class:
    void setChordScanArea (ChordScanArea a) { chordScanArea = a; }
    void setArrangerEngine (ArrangerEngine* e) { arrangerEngine = e; }
private:
    std::atomic<ChordScanArea> chordScanArea { ChordScanArea::Split };  // UI writes, input thread reads
    ChordDetector  chordDetector;     // input-thread only — never shared
    ArrangerEngine* arrangerEngine = nullptr;  // set once at init
    bool inChordZone (int note) const;       // Split: note < rightHandBoundSetting; Full: always true
    void updateChordFromInput (const juce::MidiMessage& m);
```

- [ ] **Step 2: Edit `MidiHandler.cpp`** — feed the detector from `handleIncomingMidiMessage`

```cpp
bool MidiHandler::inChordZone (int note) const
{
    if (chordScanArea == ChordScanArea::Full) return true;
    return rightHandBoundSetting == -1 ? true : note < rightHandBoundSetting;  // left zone
}

void MidiHandler::updateChordFromInput (const juce::MidiMessage& m)
{
    if (arrangerEngine == nullptr) return;
    const int note = m.getNoteNumber();
    if (! inChordZone (note)) return;

    if (m.isNoteOn() && m.getVelocity() > 0)      chordDetector.noteOn  (note);
    else if (m.isNoteOff() || m.isNoteOn())       chordDetector.noteOff (note);  // note-on vel 0 = off

    Chord c = chordDetector.current();
    if (c.isValid())
        arrangerEngine->setActiveChord (c);   // Chord Memory: only push valid chords;
                                              // emptying the zone keeps the last chord (no push of none)
}
```
Call `updateChordFromInput(message)` near the top of `handleIncomingMidiMessage` (after note-number extraction), for note-on and note-off, **without** altering the existing right-hand pass-through behaviour.

- [ ] **Step 3: Wire the engine pointer** — in the owner that constructs both `MidiHandler` and the `ArrangerEngine` (search for where `ArrangerEngine` is created — `CurrentStyleComponent`/`MainComponent`), call `midiHandler.setArrangerEngine (&engine);` once during setup. Add the include.

- [ ] **Step 4: Build (twice if `.jucer` touched), run all arranger tests** — Expected: still all PASS (no test change; this is wiring).

- [ ] **Step 5: Manual smoke (Stage 1 acceptance)** — launch the app, arranger mode, load/Start a style, hold a left-hand note: the pitched backing shifts by the interval; drums don't; right-hand melody unaffected; lifting the hand keeps the last chord.

- [ ] **Step 6: Commit**

```bash
git add Source/Midi/MidiHandler.h Source/Midi/MidiHandler.cpp Source/Styles/CurrentStyleComponent.cpp Source/Styles/CurrentStyleComponent.h
git commit -m "feat(arranger): drive accompaniment transposition from left-hand chord input"
```

**Stage 1 complete: end-to-end root transposition is audible.**

---

# STAGE 2 — Fingered chord recognition

Goal: `ChordDetector` returns true quality + extensions, inversion-correct.

---

### Task 7: Interval-template chord recognition

**Files:**
- Modify: `Source/Arranger/ChordDetector.cpp` (and `.h` if adding helpers)
- Test: `tests/unit/test_chord_detector.cpp` (extend)

**Interfaces:**
- Produces: `current()` now returns the real `ChordQuality` by matching the held pitch-class set against root-relative interval templates; `root` = template root (not lowest note); `bassNote` = lowest held pitch class.

- [ ] **Step 1: Write the failing test** — append cases

```cpp
        beginTest ("recognizes common triads and sevenths");
        {
            auto detect = [] (std::initializer_list<int> notes) {
                ChordDetector d; for (int n : notes) d.noteOn (n); return d.current();
            };
            expect (detect({60,64,67}).quality == ChordQuality::Maj);   // C E G
            expect (detect({60,63,67}).quality == ChordQuality::Min);   // C Eb G
            expect (detect({60,64,67,70}).quality == ChordQuality::Dom7);// C E G Bb
            expect (detect({60,63,67,70}).quality == ChordQuality::Min7);// C Eb G Bb
            expect (detect({60,63,66}).quality == ChordQuality::Dim);   // C Eb Gb
            expect (detect({60,65,67}).quality == ChordQuality::Sus4);  // C F G
            expectEquals (detect({60,64,67}).root, 0);                  // C
        }

        beginTest ("inversions resolve to the same chord and root");
        {
            auto detect = [] (std::initializer_list<int> notes) {
                ChordDetector d; for (int n : notes) d.noteOn (n); return d.current();
            };
            // E G C and G C E are both C major, root C
            expect (detect({64,67,72}).quality == ChordQuality::Maj);
            expectEquals (detect({64,67,72}).root, 0);
            expectEquals (detect({64,67,72}).bassNote, 4);   // lowest = E
            expectEquals (detect({67,72,76}).root, 0);
        }

        beginTest ("ambiguous/sparse set -> None");
        {
            ChordDetector d; d.noteOn (60); d.noteOn (61);   // C + C#: not a known chord
            expect (d.current().quality == ChordQuality::None || ! d.current().isValid());
        }
```

- [ ] **Step 2: Run test to verify it fails** — Expected: FAIL (stage-1 returns Maj for everything).

- [ ] **Step 3: Implement template matching in `ChordDetector.cpp`**

```cpp
namespace
{
    struct Template { ChordQuality q; std::vector<int> intervals; };  // root-relative semitones, sorted, incl 0
    // Order matters: richer (more-note) templates first so 4-note chords win over their 3-note subsets.
    const std::vector<Template>& templates()
    {
        static const std::vector<Template> t = {
            { ChordQuality::Maj7,    {0,4,7,11} },
            { ChordQuality::Dom7,    {0,4,7,10} },
            { ChordQuality::Min7,    {0,3,7,10} },
            { ChordQuality::HalfDim, {0,3,6,10} },
            { ChordQuality::Maj,     {0,4,7}    },
            { ChordQuality::Min,     {0,3,7}    },
            { ChordQuality::Dim,     {0,3,6}    },
            { ChordQuality::Aug,     {0,4,8}    },
            { ChordQuality::Sus2,    {0,2,7}    },
            { ChordQuality::Sus4,    {0,5,7}    },
        };
        return t;
    }

    // Does the pitch-class set, rooted at `root`, equal this interval template?
    bool matches (const std::set<int>& pcs, int root, const std::vector<int>& intervals)
    {
        if ((int) pcs.size() != (int) intervals.size()) return false;
        for (int iv : intervals)
            if (pcs.find ((root + iv) % 12) == pcs.end()) return false;
        return true;
    }
}

Chord ChordDetector::current() const
{
    if (heldNotes.empty()) return {};

    std::set<int> pcs;
    for (int n : heldNotes) pcs.insert (n % 12);
    const int lowestPc = (*heldNotes.begin()) % 12;

    for (const auto& tpl : templates())
        for (int root = 0; root < 12; ++root)
            if (matches (pcs, root, tpl.intervals))
                return { root, tpl.q, lowestPc };

    return { -1, ChordQuality::None, lowestPc };   // unrecognized
}
```

- [ ] **Step 4: Run tests to verify they pass** — Expected: new + Stage-1 detector tests PASS. (Stage-1 "root = lowest" test from Task 2 now only holds for actual chords; if that earlier single-note test conflicts, update it to assert `isValid()==false` for a lone note, matching the new contract — a lone note matches no template.)
> **Implementer note:** the Task-2 test "root is the pitch class of the lowest held note (stage 1)" used a full C-major triad, so it still passes (root C). The single-note expectations remain `!isValid()`.

- [ ] **Step 5: Commit**

```bash
git add Source/Arranger/ChordDetector.cpp Source/Arranger/ChordDetector.h tests/unit/test_chord_detector.cpp
git commit -m "feat(arranger): fingered chord recognition via interval templates (quality + inversions)"
```

---

# STAGE 3 — NTT mapping (real Option 3)

Goal: replace block shift with per-note role → chord-tone mapping.

---

### Task 8: NTT transposition in `ChordTransposer`

**Files:**
- Modify: `Source/Arranger/ChordTransposer.h`, `Source/Arranger/ChordTransposer.cpp`
- Test: `tests/unit/test_chord_transposer.cpp` (extend)

**Interfaces:**
- Produces: `transpose(note, true)` now maps the note by its scale-degree role in `original` to the corresponding tone of `active` (chord-tone snapping), preserving octave register; non-transposable + invalid paths unchanged from Stage 1.

**Design of the mapping (implement exactly):**
1. Build the **chord-tone pitch-class set** for a `Chord` from its quality's intervals (reuse the same interval lists as the detector — extract them to a shared helper `chordIntervals(ChordQuality)` in `Chord.h` so detector and transposer agree).
2. For an incoming note: `pc = note % 12`, `octave = note / 12`. Compute `degreeOffset = (pc - original.root + 12) % 12`.
3. Find the **chord tone of `original`** nearest to `pc` (by pitch-class distance). Its index `k` in the original chord-tone list is the note's "role".
4. Map to `active`'s chord tone at the **same role index** `k` (clamp/wrap index to the active chord's tone count). Target pc = that active chord tone.
5. Reattach the original octave, then shift by whole octaves to land nearest the original `note` (preserve register); clamp 0..127.
6. **Bass Inversion (Stage 5 will toggle):** if the note's role is the root (k==0) and the part is bass and inversion is enabled, target `active.bassNote` instead of `active.root`. (Stage 3 ignores the bass flag; add the hook but default off.)

- [ ] **Step 1: Write the failing test** — append

```cpp
        beginTest ("NTT maps the 3rd differently from root/5th (C maj -> A min)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C E G
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A C E
            // root C(60)->A, 3rd E(64)->C (minor 3rd), 5th G(67)->E
            expectEquals (t.transpose (60, true) % 12, 9);   // A
            expectEquals (t.transpose (64, true) % 12, 0);   // C  (the minor 3rd, NOT C#)
            expectEquals (t.transpose (67, true) % 12, 4);   // E
        }

        beginTest ("identity when active == original");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 0, ChordQuality::Maj, 0 });
            expectEquals (t.transpose (64, true), 64);
        }

        beginTest ("drums still untouched, result in range");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 7, ChordQuality::Dom7, 7 });
            expectEquals (t.transpose (36, false), 36);
            const int x = t.transpose (64, true);
            expect (x >= 0 && x <= 127);
        }
```

- [ ] **Step 2: Run test to verify it fails** — Expected: FAIL (block shift gives C#, not C, for the 3rd).

- [ ] **Step 3: Add `chordIntervals` helper to `Chord.h`**

```cpp
inline std::vector<int> chordIntervals (ChordQuality q)
{
    switch (q) {
        case ChordQuality::Maj:     return {0,4,7};
        case ChordQuality::Min:     return {0,3,7};
        case ChordQuality::Dom7:    return {0,4,7,10};
        case ChordQuality::Maj7:    return {0,4,7,11};
        case ChordQuality::Min7:    return {0,3,7,10};
        case ChordQuality::Dim:     return {0,3,6};
        case ChordQuality::HalfDim: return {0,3,6,10};
        case ChordQuality::Aug:     return {0,4,8};
        case ChordQuality::Sus2:    return {0,2,7};
        case ChordQuality::Sus4:    return {0,5,7};
        case ChordQuality::None:    return {0,4,7};   // safe default
    }
    return {0,4,7};
}
```
(Then refactor `ChordDetector`'s templates to call `chordIntervals` so the two never drift — optional but recommended; keep template ordering for match priority.)

- [ ] **Step 4: Implement NTT in `ChordTransposer.cpp`** (replace the block-shift body)

```cpp
#include <vector>
#include <cmath>

static int nearestChordToneIndex (int pc, int root, const std::vector<int>& iv)
{
    int best = 0, bestDist = 99;
    for (int k = 0; k < (int) iv.size(); ++k)
    {
        const int tonePc = (root + iv[k]) % 12;
        int d = std::abs (tonePc - pc); d = std::min (d, 12 - d);
        if (d < bestDist) { bestDist = d; best = k; }
    }
    return best;
}

int ChordTransposer::transpose (int noteNumber, bool transposable) const
{
    if (! transposable || ! active.isValid() || ! original.isValid())
        return noteNumber;

    const auto origIv = chordIntervals (original.quality);
    const auto actIv  = chordIntervals (active.quality);
    const int pc = noteNumber % 12;

    const int role     = nearestChordToneIndex (pc, original.root, origIv);
    const int actIndex = juce::jmin (role, (int) actIv.size() - 1);
    const int targetPc = (active.root + actIv[actIndex]) % 12;

    // place targetPc nearest to the original note's register
    int candidate = (noteNumber / 12) * 12 + targetPc;
    while (candidate - noteNumber >  6) candidate -= 12;
    while (noteNumber - candidate >  6) candidate += 12;
    while (candidate < 0)   candidate += 12;
    while (candidate > 127) candidate -= 12;
    return candidate;
}
```

- [ ] **Step 5: Run tests to verify they pass** — Expected: new NTT tests + Stage-1 transposer tests PASS (re-check the Stage-1 block-shift tests: the `C->D Maj` cases still map root→root etc. — for same-quality chords NTT reduces to a shift, so they remain green; if a Stage-1 case asserted a non-chord-tone note, adjust it to a chord tone).

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordTransposer.h Source/Arranger/ChordTransposer.cpp Source/Arranger/Chord.h tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): NTT note-role to chord-tone transposition"
```

**Stage 3 complete: chord-aware transposition is live.**

---

# STAGE 4 — Original chord: persistence + auto-detect + editor

---

### Task 9: Style-level original chord in model + schema v3→v4 persistence

**Files:**
- Modify: `Source/Arranger/ArrangerModel.h`, `Source/Arranger/ArrangerStyleFile.h`, `Source/Arranger/ArrangerStyleIOHelper.cpp`
- Test: `tests/unit/test_arranger_style_io.cpp` (extend)

**Interfaces:**
- Produces: `ArrangerStyle` and `ArrangerStyleFile` gain `int originalRoot = 0; ChordQuality originalQuality = ChordQuality::Maj;`. `ArrangerStyleFile::schemaVersion` default 4. IO writes both; reading a file without them defaults to C major.

- [ ] **Step 1: Write the failing test** — append to `test_arranger_style_io.cpp`

```cpp
        beginTest ("v4 round-trips original chord");
        {
            ArrangerStyleFile f = makeMinimalStyleFile();   // existing helper in this test file
            f.schemaVersion = 4;
            f.originalRoot = 9; f.originalQuality = ChordQuality::Min;   // A minor
            auto json = ArrangerStyleIOHelper::toJson (f);
            ArrangerStyleFile back;
            expect (ArrangerStyleIOHelper::fromJson (json, back));
            expectEquals (back.originalRoot, 9);
            expect (back.originalQuality == ChordQuality::Min);
        }

        beginTest ("v3 file (no original chord) loads as C major");
        {
            ArrangerStyleFile f = makeMinimalStyleFile();
            auto json = ArrangerStyleIOHelper::toJson (f);
            // simulate an old file: remove the new keys
            auto* obj = json.getDynamicObject();
            obj->removeProperty ("originalRoot");
            obj->removeProperty ("originalQuality");
            obj->setProperty ("schemaVersion", 3);
            ArrangerStyleFile back;
            expect (ArrangerStyleIOHelper::fromJson (json, back));
            expectEquals (back.originalRoot, 0);
            expect (back.originalQuality == ChordQuality::Maj);
        }
```
> **Implementer note:** match the actual IO entry points in `ArrangerStyleIOHelper` (the existing tests already call them — reuse those exact names; `toJson`/`fromJson` above are illustrative).

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error (fields missing) / FAIL.

- [ ] **Step 3: Add fields** — `ArrangerModel.h` `ArrangerStyle` and `ArrangerStyleFile.h` (`#include "Chord.h"`), both:

```cpp
    int          originalRoot    = 0;                  // 0=C..11=B
    ChordQuality originalQuality = ChordQuality::Maj;  // recorded key, default C major
```
Bump `ArrangerStyleFile::schemaVersion = 4;`. Remove the now-dead `ArrangerTrack::originalChord` string (and any reference).

- [ ] **Step 4: Edit `ArrangerStyleIOHelper.cpp`** — serialize + default

```cpp
// writing:
obj->setProperty ("originalRoot", f.originalRoot);
obj->setProperty ("originalQuality", toString (f.originalQuality));
// reading (with v3 default):
out.originalRoot    = (int) obj->getProperty ("originalRoot");          // absent -> 0
out.originalQuality = chordQualityFromString (obj->getProperty ("originalQuality").toString());
if (out.originalQuality == ChordQuality::None) out.originalQuality = ChordQuality::Maj;  // v3/default
```
Map the file fields into the runtime `ArrangerStyle` wherever the helper converts `ArrangerStyleFile` → `ArrangerStyle` (carry `originalRoot`/`originalQuality` across).

- [ ] **Step 5: Run tests to verify they pass** — Expected: new + existing IO tests PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerModel.h Source/Arranger/ArrangerStyleFile.h Source/Arranger/ArrangerStyleIOHelper.cpp tests/unit/test_arranger_style_io.cpp
git commit -m "feat(arranger): persist style-level original chord (schema v4, v3 defaults to C major)"
```

---

### Task 10: Engine reads style original chord; auto-detect helper

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.cpp` (`setStyle`), `Source/Arranger/ChordDetector.h`/`.cpp` (add a static key-finder) OR a small free function in `Chord.h`
- Test: `tests/unit/test_chord_detector.cpp` (extend for the key-finder)

**Interfaces:**
- Produces:
  - In `ArrangerEngine::setStyle`: `transposer.setOriginalChord ({ style.originalRoot, style.originalQuality, style.originalRoot });`
  - `Chord detectKeyFromEvents (const std::vector<TimedBeatEvent>& pitchedEvents);` — pitch-class histogram → best root+quality; empty/ambiguous → C major. Used at authoring time (Task 11).

- [ ] **Step 1: Write the failing test** — append to `test_chord_detector.cpp`

```cpp
        beginTest ("key finder picks C major from a C-major-heavy pattern");
        {
            std::vector<TimedBeatEvent> evs;
            for (int n : { 60,64,67, 60,64,67, 60, 67 })
                evs.push_back ({ 0.0, juce::MidiMessage::noteOn (2, n, (juce::uint8)100) });
            Chord k = detectKeyFromEvents (evs);
            expectEquals (k.root, 0);
            expect (k.quality == ChordQuality::Maj);
        }

        beginTest ("key finder falls back to C major on empty input");
        {
            Chord k = detectKeyFromEvents ({});
            expectEquals (k.root, 0);
            expect (k.quality == ChordQuality::Maj);
        }
```

- [ ] **Step 2: Run test to verify it fails** — Expected: compile error / FAIL.

- [ ] **Step 3: Implement `detectKeyFromEvents`** (declare in `ChordDetector.h`, define in `.cpp`)

```cpp
Chord detectKeyFromEvents (const std::vector<TimedBeatEvent>& events)
{
    int hist[12] = {0};
    for (const auto& e : events)
        if (e.message.isNoteOn() && e.message.getVelocity() > 0)
            hist[e.message.getNoteNumber() % 12]++;

    int total = 0, root = 0, best = -1;
    for (int i = 0; i < 12; ++i) { total += hist[i]; if (hist[i] > best) { best = hist[i]; root = i; } }
    if (total == 0) return { 0, ChordQuality::Maj, 0 };   // fallback C major

    // major vs minor: compare the third above the root
    const int majThird = hist[(root + 4) % 12];
    const int minThird = hist[(root + 3) % 12];
    const ChordQuality q = (minThird > majThird) ? ChordQuality::Min : ChordQuality::Maj;
    return { root, q, root };
}
```

- [ ] **Step 4: Edit `ArrangerEngine::setStyle`** — after building, set the original chord:

```cpp
transposer.setOriginalChord ({ style.originalRoot, style.originalQuality, style.originalRoot });
```

- [ ] **Step 5: Run tests to verify they pass** — Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordDetector.h Source/Arranger/ChordDetector.cpp Source/Arranger/ArrangerEngine.cpp tests/unit/test_chord_detector.cpp
git commit -m "feat(arranger): engine reads style original chord; add key-finder for auto-detect"
```

---

### Task 11: Editor — original-chord combos, auto-detect pre-fill, persistence

**Files:**
- Modify: `Source/Arranger/ArrangerStyleEditor.h`/`.cpp`, `Source/Styles/CurrentStyleComponent.cpp`
- (No new unit test — UI; covered by manual smoke. Logic — key finder — is unit-tested in Task 10.)

**Interfaces:**
- Consumes: `detectKeyFromEvents` (Task 10), style-level fields (Task 9).
- Produces: editor exposes root + quality combos bound to the working style's `originalRoot`/`originalQuality`, pre-filled by auto-detect on `loadRecording`/Update Tracks, saved through the existing save path.

- [ ] **Step 1: Add combos to `ArrangerStyleEditor.h`** — `juce::ComboBox originalRootBox, originalQualityBox;` add to the `addAndMakeVisible` toolbar list; lay out in `resized()` next to `nameEditor` (drop to a second toolbar row if width is tight).

- [ ] **Step 2: Populate + bind** — fill `originalRootBox` with C..B (ids 1..12), `originalQualityBox` with maj/min (+ the other qualities, ids matching `ChordQuality`). On change, write into the working style's `originalRoot`/`originalQuality`.

- [ ] **Step 3: Pre-fill from auto-detect** — in `loadRecording` and after Update Tracks, call `detectKeyFromEvents` over the pitched (non-drum) source-track events, set the combos (fallback C major), and store into the working style.

- [ ] **Step 4: Persist** — ensure the working style's `originalRoot`/`originalQuality` are written when building the `ArrangerStyleFile` for save (Task 9 added the fields; confirm the editor's save path carries them).

- [ ] **Step 5: Build (twice if `.jucer` touched), run all arranger tests** — Expected: PASS (unchanged).

- [ ] **Step 6: Manual smoke** — author a config in a non-C recording → combos pre-fill to the detected key; edit them; Save; reload → key persists; Start → transposition uses the saved original chord.

- [ ] **Step 7: Commit**

```bash
git add Source/Arranger/ArrangerStyleEditor.h Source/Arranger/ArrangerStyleEditor.cpp Source/Styles/CurrentStyleComponent.cpp
git commit -m "feat(arranger): editor original-chord combos with auto-detect and persistence"
```

---

# STAGE 5 — Extras (Chord Memory toggle, Bass Inversion)

---

### Task 12: Bass Inversion (engine + transposer hook + toggle)

**Files:**
- Modify: `Source/Arranger/ChordTransposer.h`/`.cpp` (bass-aware target), `Source/Arranger/ArrangerScheduler.*` + `ArrangerEngine.*` (carry an `isBass` distinction), `Source/Styles/CurrentStyleComponent.*` (toggle)
- Test: `tests/unit/test_chord_transposer.cpp` (extend)

**Interfaces:**
- Produces: `ChordTransposer::setBassInversion(bool)`; when on, a **bass-part root-role** note targets `active.bassNote` instead of `active.root`. Requires distinguishing Bass from Acc at emit — extend the scheduler tag from `char transposable` (0/1) to a small code: `0=fixed, 1=acc, 2=bass`, and pass the part kind into `transpose`.

- [ ] **Step 1: Write the failing test**

```cpp
        beginTest ("bass inversion targets the played bass note for the bass root");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 0, ChordQuality::Maj, 4 });   // C major, bass = E (C/E)
            t.setBassInversion (true);
            // a bass root note C should now sound E
            expectEquals (t.transposeBass (36, /*isBassRoot…*/ true) % 12, 4);  // see note
        }
```
> **Implementer note:** choose the exact API — either add a `PartKind` param to `transpose(note, PartKind)` (`Fixed/Acc/Bass`) and branch internally, or a dedicated `transposeBass`. Keep one path; update Task-5/8 call sites and the scheduler tag type accordingly. Update earlier transposer tests to pass `PartKind::Acc` where they passed `true`.

- [ ] **Step 2: Run test to verify it fails.**

- [ ] **Step 3: Implement** — thread a `PartKind` from `rebuildFromStyle` (Bass→bass, Acc→acc, Drum/Perc→fixed) through the scheduler tag and `EmittedEvent`, into `ChordTransposer`. When bass inversion is on and the note's role is the root of a bass part, set target pc = `active.bassNote` (fallback to `active.root` when `bassNote < 0`).

- [ ] **Step 4: Add the UI toggle** — a "Bass Inversion" switch in the arranger controls calling `engine.setBassInversion(bool)` (off by default). Persist the preference like `ArrangerAutoFillEnabled` if desired.

- [ ] **Step 5: Run tests + manual smoke** — play an inversion with bass inversion on → slash-chord bass; off → root bass.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordTransposer.h Source/Arranger/ChordTransposer.cpp Source/Arranger/ArrangerScheduler.h Source/Arranger/ArrangerScheduler.cpp Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp Source/Styles/CurrentStyleComponent.cpp Source/Styles/CurrentStyleComponent.h tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): bass inversion (slash chords) with UI toggle"
```

---

### Task 13: Chord Memory toggle (off = revert to original chord on release)

**Files:**
- Modify: `Source/Midi/MidiHandler.h`/`.cpp`, `Source/Styles/CurrentStyleComponent.*`
- (No unit test — input/threaded; manual smoke.)

**Interfaces:**
- Produces: `MidiHandler::setChordMemory(bool)` (default true). When OFF and the chord zone empties, push the style's original chord (identity) instead of holding the last chord.

- [ ] **Step 1: Implement** — in `updateChordFromInput`, when the zone becomes empty: if memory on → push nothing (hold); if memory off → `arrangerEngine->setActiveChord(originalChordOfActiveStyle)` (expose the style's original chord to the handler, or send an "identity" sentinel the engine treats as original).

- [ ] **Step 2: Add the toggle UI** — a "Chord Memory" switch (default on), wired to `setChordMemory`.

- [ ] **Step 3: Build, run all arranger tests** — Expected: PASS (unchanged).

- [ ] **Step 4: Manual smoke** — memory on: lift hand → backing holds the chord; memory off: lift hand → backing returns to the recorded key.

- [ ] **Step 5: Commit**

```bash
git add Source/Midi/MidiHandler.h Source/Midi/MidiHandler.cpp Source/Styles/CurrentStyleComponent.cpp Source/Styles/CurrentStyleComponent.h
git commit -m "feat(arranger): chord memory toggle (hold vs revert on release)"
```

---

# STAGE 6 — Full-Keyboard mode (chord/melody inference, tuned by ear)

---

### Task 14: Full-Keyboard inference layer

**Files:**
- Modify: `Source/Arranger/ChordDetector.h`/`.cpp` (or a new `FullKeyboardChordFilter`), `Source/Midi/MidiHandler.cpp`
- Test: `tests/unit/test_chord_detector.cpp` (extend for the deterministic parts)

**Interfaces:**
- Produces: when `ChordScanArea::Full`, an inference step decides which held notes are chord vs melody before recognition: minimum chord size (≥3 notes forming a known chord), held-duration/recency weighting, and hysteresis (don't change the recognized chord unless evidence is strong).

- [ ] **Step 1: Write the failing test (deterministic rules only)**

```cpp
        beginTest ("full-keyboard: a lone melody note does not change the chord");
        {
            // Hold a C major chord, then add one high passing note: chord stays C major.
            ChordDetector d;
            d.setFullKeyboardMode (true);
            for (int n : {48,52,55}) d.noteOn (n);     // C major low
            Chord before = d.current();
            d.noteOn (84);                              // high melody note
            expect (d.current() == before);            // unchanged
        }

        beginTest ("full-keyboard: requires >=3 notes forming a known chord");
        {
            ChordDetector d; d.setFullKeyboardMode (true);
            d.noteOn (60); d.noteOn (84);
            expect (! d.current().isValid());          // not enough chord evidence
        }
```

- [ ] **Step 2: Run test to verify it fails.**

- [ ] **Step 3: Implement the inference** — in Full mode, before template matching: require ≥3 notes; try recognizing the **lowest 3–4 sustained notes** first; keep the previously recognized chord (hysteresis) unless a new full chord is clearly present. Add `setFullKeyboardMode(bool)` and the held-note metadata (timestamps via a monotonic counter incremented per `noteOn`).

- [ ] **Step 4: Wire the mode** — `MidiHandler::setChordScanArea(Full)` forwards **all** notes and calls `chordDetector.setFullKeyboardMode(true)`.

- [ ] **Step 5: Run tests + extended manual tuning** — play melody+chord together; adjust thresholds (min notes, hysteresis strength) by ear until the accompaniment doesn't lurch on melody notes but tracks real chord changes.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ChordDetector.h Source/Arranger/ChordDetector.cpp Source/Midi/MidiHandler.cpp tests/unit/test_chord_detector.cpp
git commit -m "feat(arranger): full-keyboard chord/melody inference mode"
```

**Phase 4 complete.** Final step: update the wiki (`Arranger Phase Roadmap` → Phase 4 done, Phase 5/6 next) and `hot.md`/`log.md` per the wiki sync process — separate from this plan, on user request.

---

## Self-review notes (coverage map)

- Spec §4.1 Chord → Task 1; §4.2 ChordDetector → Tasks 2,7,14; §4.3 ChordTransposer → Tasks 3,8,12; §4.4 tagging → Tasks 4,5; §4.5 mailbox + emit + note-off matching → Task 5; §4.6 input wiring + scan area + memory → Tasks 6,13,14; §4.7 persistence v3→v4 → Task 9; §4.8 editor + auto-detect → Tasks 10,11; §5 threading → Task 5 (mailbox) + Task 6 (input thread only writes); §7 testing → per-task tests; §8 risk (note-off pitch) → Task 5 test "note-off matches transposed pitch"; §10 stages → Stage headers 1–6.
- All six stages have at least one independently testable deliverable. UI-only tasks (6,11,13) are explicitly marked manual-smoke with their underlying logic unit-tested elsewhere.
