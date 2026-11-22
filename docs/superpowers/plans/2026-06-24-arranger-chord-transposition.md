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


> Continued in [part 2 of 2](2026-06-24-arranger-chord-transposition-part2.md).
