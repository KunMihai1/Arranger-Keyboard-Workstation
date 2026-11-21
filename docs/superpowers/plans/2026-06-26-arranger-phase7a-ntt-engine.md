# Arranger Phase 7a — Real NTT Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the single root-shift + third-flip in `ChordTransposer` with a per-track, type-dispatched NTT engine (NoTranspose / Parallel / Chord / Fixed) that remaps every chord-tone role onto the held chord and snaps passing tones to the held chord's scale, with a curated minor-scale selector and a master on/off bypass.

**Architecture:** `ChordTransposer` stays a pure, allocation-free, lock-free per-note function (read on the engine timer thread only). A new data table (`NttScales.h`) supplies the snap scale per active-chord quality. Each track carries an `NttType`; it is threaded file → model → scheduler → emit exactly the way `PartKind` already is. Settings (minor scale, master enable) are set ahead of time off the audio path.

**Tech Stack:** JUCE C++17, Projucer/MSBuild Debug x64, `juce::UnitTest` (tests compiled into the exe, run via `Project Synth2.exe --run-tests --arranger-tests | --unit-tests`).

## Global Constraints

- **Commits:** Never auto-commit — ask the user for approval before every `git commit`. Never add `Co-Authored-By: Claude` or any Claude mention to a commit message.
- **Branch:** Work on `arranger-phase7a-ntt-engine` (already created from `main`).
- **Audio-thread latency:** The per-note `transpose(...)` path must not allocate, lock, or branch heavily. All table/scale/enabled/chord selection is set ahead of time and read by value. Settings setters (`setMinorScale`, `setNttEnabled`) run on the message thread, never inside the per-note call.
- **Naming:** The smart NTT type is named **`Chord`** (`NttType::Chord`) — Korg/Yamaha's own term. Qualified names disambiguate it from the recognition-layer `ChordMode`.
- **Backward compatibility:** Any `.style` loaded without an `nttType` field must receive the part-derived default — no existing style may break.
- **Scale snap tie-break:** When a passing tone is equidistant between two scale tones, resolve **downward** (toward the lower pitch).
- **Tests:** `ChordTransposer` / scale / model / IO tests live in the existing `juce::UnitTest` files and run under `--arranger-tests` (or `--unit-tests` for the model/io files — match each existing file's current suite). Each is registered by a `static` instance at file scope.

**Build/run reference:**
- Close the running app first (it locks the PDB), then build: `Get-Process -Name "Project Synth2" -ErrorAction SilentlyContinue | Stop-Process -Force` then MSBuild Debug x64.
- Run tests: `"Builds/VisualStudio2022/x64/Debug/App/Project Synth2.exe" --run-tests --arranger-tests`; results in `Builds/VisualStudio2022/test-results.txt` (`TOTAL | passes | failures`).

---

## File Structure

**New files:**
- `Source/Arranger/NttScales.h` — pure data table: `MinorScaleChoice`-aware scale-degree lookup per `ChordQuality`. One responsibility: "what scale do I snap passing tones to for this active chord?"

**Modified files:**
- `Source/Arranger/Chord.h` — add `enum class NttType`, `enum class MinorScaleChoice`.
- `Source/Arranger/ArrangerEnums.h` — `toString(NttType)` / `nttFromString` + `nttDefaultForPart(ArrangerPartType)`.
- `Source/Arranger/ArrangerModel.h` — `ArrangerTrack.nttType`.
- `Source/Arranger/ArrangerStyleFile.h` — `SourceTrackFile.nttType`; bump `schemaVersion` 4 → 5.
- `Source/Arranger/ArrangerStyleIOHelper.cpp` — serialize/deserialize `nttType` with part-derived migration.
- `Source/Arranger/ArrangerSourceBuilder.cpp` — set part-derived `nttType` when building fresh source tracks.
- `Source/Arranger/ArrangerPatternBuilder.cpp` — thread `st.nttType` → `at.nttType`.
- `Source/Arranger/ChordTransposer.h/.cpp` — new 3-arg `transpose`, `minorScale`/`nttEnabled` members + setters, scale-snap + wrap-around.
- `Source/Arranger/ArrangerScheduler.h/.cpp` — carry `NttType` parallel to `PartKind` (third `setLoop` arg + `EmittedEvent.ntt` + `activeNoteNtt`).
- `Source/Arranger/ArrangerEngine.h/.cpp` — thread per-track `nttType` into schedulers and into `transpose`; `setMinorScale` / `setNttEnabled` forwarders.
- `Source/Arranger/ArrangerStyleEditor.h/.cpp` — per-track NTT-type selector in the track rows.
- `Source/UI/SettingsWindow.h/.cpp` — master "Chord Transposition" toggle + 3-way minor-scale selector.
- `Source/UI/MainComponent.cpp` — wire the two settings to the engine + startup restore.
- `Source/Styles/CurrentStyleComponent.h/.cpp` + `Source/UI/Display.h/.cpp` — pass-through + re-apply-on-Start (mirror Bass Inversion).
- Tests: `tests/unit/test_chord_transposer.cpp`, `tests/unit/test_arranger_style_io.cpp`, plus a new `tests/unit/test_ntt_scales.cpp`.

---

## Task 1: Enums, per-track field, schema bump + part-derived migration

**Files:**
- Modify: `Source/Arranger/Chord.h` (after the existing enums, ~line 15)
- Modify: `Source/Arranger/ArrangerEnums.h`
- Modify: `Source/Arranger/ArrangerModel.h:27-37` (`struct ArrangerTrack`)
- Modify: `Source/Arranger/ArrangerStyleFile.h:7-39`
- Modify: `Source/Arranger/ArrangerStyleIOHelper.cpp:7-28` (`trackToVar`) and `:86-101` (load loop)
- Modify: `Source/Arranger/ArrangerSourceBuilder.cpp:34-40`
- Modify: `Source/Arranger/ArrangerPatternBuilder.cpp:167`
- Test: `tests/unit/test_arranger_style_io.cpp`

**Interfaces:**
- Produces: `enum class NttType { NoTranspose, Parallel, Chord, Fixed };`, `enum class MinorScaleChoice { Dorian, Aeolian, HarmonicMinor };` (both in `Chord.h`). `ArrangerEnums::toString(NttType)`, `ArrangerEnums::nttFromString(const juce::String&)`, `ArrangerEnums::nttDefaultForPart(ArrangerPartType)`. New field `NttType nttType` on both `ArrangerTrack` and `SourceTrackFile`.

- [ ] **Step 1: Write the failing test** — add to `tests/unit/test_arranger_style_io.cpp` inside the existing test class's `runTest()`:

```cpp
beginTest ("nttType round-trips and missing field migrates by part");
{
    ArrangerStyleFile in;
    in.id = "s"; in.name = "S";
    SourceTrackFile bass;  bass.id="b";  bass.name="Bass";  bass.partType=ArrangerPartType::Bass; bass.channel=2;
    bass.nttType = NttType::Parallel;
    SourceTrackFile acc;   acc.id="a";   acc.name="Piano"; acc.partType=ArrangerPartType::Acc;  acc.channel=3;
    acc.nttType  = NttType::Fixed;       // non-default, must survive the round-trip
    in.sourceTracks = { bass, acc };

    auto tmp = juce::File::createTempFile (".style");
    ArrangerStyleIOHelper::saveToFile (tmp, in);
    ArrangerStyleFile out; juce::String err;
    expect (ArrangerStyleIOHelper::loadFromFile (tmp, out, err));
    expectEquals ((int) out.sourceTracks.size(), 2);
    expect (out.sourceTracks[0].nttType == NttType::Parallel);
    expect (out.sourceTracks[1].nttType == NttType::Fixed);

    // a hand-written v4 JSON with NO nttType field -> part-derived defaults
    const juce::String legacy =
        "{ \"schemaVersion\":4, \"kind\":\"arrangerStyle\", \"sourceTracks\":["
        "  {\"id\":\"d\",\"name\":\"Drums\",\"partType\":\"Drum\",\"channel\":10},"
        "  {\"id\":\"b\",\"name\":\"Bass\",\"partType\":\"Bass\",\"channel\":2},"
        "  {\"id\":\"a\",\"name\":\"Pad\",\"partType\":\"Acc\",\"channel\":3} ], \"sections\":[] }";
    auto tmp2 = juce::File::createTempFile (".style"); tmp2.replaceWithText (legacy);
    ArrangerStyleFile mig; expect (ArrangerStyleIOHelper::loadFromFile (tmp2, mig, err));
    expect (mig.sourceTracks[0].nttType == NttType::NoTranspose);  // Drum
    expect (mig.sourceTracks[1].nttType == NttType::Parallel);     // Bass
    expect (mig.sourceTracks[2].nttType == NttType::Chord);        // Acc
    tmp.deleteFile(); tmp2.deleteFile();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: build, then `"...Project Synth2.exe" --run-tests --unit-tests`
Expected: FAIL to compile (`NttType` / `nttType` undefined).

- [ ] **Step 3: Add the enums** — in `Source/Arranger/Chord.h`, after `enum class ChordMode ...` (line 15):

```cpp
/** Per-track Note Transposition Table type (Phase 7a). NoTranspose = identity (drums);
    Parallel = uniform root-interval shift; Chord = role-remap chord tones + scale-snap passing
    tones (Korg's "Chord" NTT); Fixed = same as Chord but kept in the octave nearest the original
    note (no register drift). Unrelated to ChordMode despite the shared word. */
enum class NttType { NoTranspose, Parallel, Chord, Fixed };

/** Curated minor scale used to snap passing tones over Min/Min7 chords (Phase 7a). */
enum class MinorScaleChoice { Dorian, Aeolian, HarmonicMinor };
```

- [ ] **Step 4: Add string + default helpers** — in `Source/Arranger/ArrangerEnums.h`, inside `namespace ArrangerEnums`:

```cpp
inline juce::String toString (NttType n)
{
    switch (n)
    {
        case NttType::NoTranspose: return "NoTranspose";
        case NttType::Parallel:    return "Parallel";
        case NttType::Chord:       return "Chord";
        case NttType::Fixed:       return "Fixed";
    }
    return "Chord";
}

inline NttType nttFromString (const juce::String& s)
{
    if (s == "NoTranspose") return NttType::NoTranspose;
    if (s == "Parallel")    return NttType::Parallel;
    if (s == "Fixed")       return NttType::Fixed;
    return NttType::Chord;
}

/** Migration / fresh-track default derived from the part role. */
inline NttType nttDefaultForPart (ArrangerPartType p)
{
    switch (p)
    {
        case ArrangerPartType::Drum:
        case ArrangerPartType::Perc: return NttType::NoTranspose;
        case ArrangerPartType::Bass: return NttType::Parallel;
        case ArrangerPartType::Acc:  return NttType::Chord;
    }
    return NttType::Chord;
}
```

- [ ] **Step 5: Add the field to both track structs**

`Source/Arranger/ArrangerModel.h`, in `struct ArrangerTrack` (after `double volume = 100.0;`):
```cpp
    NttType nttType = NttType::Chord;   // Phase 7a: per-track transposition type
```
`Source/Arranger/ArrangerStyleFile.h`, in `struct SourceTrackFile` (after `double volume = 100.0;`):
```cpp
    NttType nttType = NttType::Chord;   // Phase 7a; default overridden by part-derived migration
```
Bump the version in the same file: `int schemaVersion = 5;` (was 4 — v5 adds per-track nttType).

- [ ] **Step 6: Serialize + deserialize with migration** — `Source/Arranger/ArrangerStyleIOHelper.cpp`.

In `trackToVar`, after the `volume` line:
```cpp
    o->setProperty ("nttType", ArrangerEnums::toString (t.nttType));
```
In the load loop (`if (auto* tracks = ...)`), after `t.partType = ...` is set and before `out.sourceTracks.push_back`:
```cpp
            const auto nttStr = tv.getProperty ("nttType", juce::var()).toString();
            t.nttType = nttStr.isEmpty() ? ArrangerEnums::nttDefaultForPart (t.partType)
                                         : ArrangerEnums::nttFromString (nttStr);
```

- [ ] **Step 7: Set the default when building fresh source tracks** — `Source/Arranger/ArrangerSourceBuilder.cpp`, in the loop that fills `SourceTrackFile st;` (after `st.partType = ...`):
```cpp
            st.nttType = ArrangerEnums::nttDefaultForPart (st.partType);
```
Ensure the file `#include "ArrangerEnums.h"` (add if absent).

- [ ] **Step 8: Thread file → model** — `Source/Arranger/ArrangerPatternBuilder.cpp:167`, where `at.partType = st.partType;` is set, add on the same statement group:
```cpp
            at.nttType = st.nttType;
```

- [ ] **Step 9: Run the test — expect PASS**

Run: build, `"...Project Synth2.exe" --run-tests --unit-tests`
Expected: PASS (round-trip + migration assertions green); all previously-green IO/model tests still pass.

- [ ] **Step 10: Commit** (ask the user first)

```bash
git add Source/Arranger/Chord.h Source/Arranger/ArrangerEnums.h Source/Arranger/ArrangerModel.h \
        Source/Arranger/ArrangerStyleFile.h Source/Arranger/ArrangerStyleIOHelper.cpp \
        Source/Arranger/ArrangerSourceBuilder.cpp Source/Arranger/ArrangerPatternBuilder.cpp \
        tests/unit/test_arranger_style_io.cpp
git commit -m "feat(arranger): add per-track NttType field, schema v5 + part-derived migration"
```

---

## Task 2: Data-driven scale table (`NttScales.h`)

**Files:**
- Create: `Source/Arranger/NttScales.h`
- Create: `tests/unit/test_ntt_scales.cpp`
- Modify: `Project Synth2.jucer` (add both files to the build — or confirm the test glob picks `tests/unit/*.cpp` up automatically; the existing test files are individually referenced, so add the new test there too).

**Interfaces:**
- Produces: `struct NttScale { int count; std::array<int,8> degrees; };` and `NttScale scaleForQuality (ChordQuality q, MinorScaleChoice minor);` and `NttScale minorScaleFor (MinorScaleChoice c);` — all `inline`, pure, allocation-free. `degrees[i]` are semitone offsets from the **active chord root**; only the first `count` entries are valid.

- [ ] **Step 1: Write the failing test** — create `tests/unit/test_ntt_scales.cpp`:

```cpp
#include <juce_core/juce_core.h>
#include "Arranger/NttScales.h"

class NttScalesTest : public juce::UnitTest
{
public:
    NttScalesTest() : juce::UnitTest ("NttScales", "Arranger") {}

    static bool hasDegree (const NttScale& s, int d)
    {
        for (int i = 0; i < s.count; ++i) if (s.degrees[i] == d) return true;
        return false;
    }

    void runTest() override
    {
        beginTest ("major qualities use Ionian");
        {
            auto s = scaleForQuality (ChordQuality::Maj, MinorScaleChoice::Dorian);
            expectEquals (s.count, 7);
            for (int d : { 0,2,4,5,7,9,11 }) expect (hasDegree (s, d));
            expect (! hasDegree (s, 6));   // no #4 in Ionian
        }

        beginTest ("dominant + sus use Mixolydian (flat 7)");
        {
            for (auto q : { ChordQuality::Dom7, ChordQuality::Sus2, ChordQuality::Sus4 })
            {
                auto s = scaleForQuality (q, MinorScaleChoice::Dorian);
                expect (hasDegree (s, 10));        // b7
                expect (! hasDegree (s, 11));      // not a major 7
            }
        }

        beginTest ("minor row swaps with the MinorScaleChoice");
        {
            auto dor = scaleForQuality (ChordQuality::Min, MinorScaleChoice::Dorian);
            auto aeo = scaleForQuality (ChordQuality::Min, MinorScaleChoice::Aeolian);
            auto har = scaleForQuality (ChordQuality::Min7, MinorScaleChoice::HarmonicMinor);
            expect (  hasDegree (dor, 9));   // Dorian natural-6
            expect (! hasDegree (dor, 8));
            expect (  hasDegree (aeo, 8));   // Aeolian b6
            expect (! hasDegree (aeo, 9));
            expect (  hasDegree (har, 11));  // harmonic raised-7
            expect (  hasDegree (har, 8));
        }

        beginTest ("aug is whole-tone, dim is whole-half");
        {
            auto aug = scaleForQuality (ChordQuality::Aug, MinorScaleChoice::Dorian);
            expectEquals (aug.count, 6);
            for (int d : { 0,2,4,6,8,10 }) expect (hasDegree (aug, d));
            auto dim = scaleForQuality (ChordQuality::Dim, MinorScaleChoice::Dorian);
            expectEquals (dim.count, 8);
            for (int d : { 0,2,3,5,6,8,9,11 }) expect (hasDegree (dim, d));
        }
    }
};
static NttScalesTest nttScalesTest;
```

- [ ] **Step 2: Run test to verify it fails**

Run: build → expect FAIL to compile (`NttScales.h` missing).

- [ ] **Step 3: Create `Source/Arranger/NttScales.h`**

```cpp
#pragma once
#include "Chord.h"
#include <array>

/** A snap scale: `count` valid semitone degrees (from the active chord root) in `degrees`. */
struct NttScale { int count = 7; std::array<int,8> degrees { { 0,2,4,5,7,9,11,0 } }; };

/** The curated minor scale for Min/Min7 (the one user-selectable row). */
inline NttScale minorScaleFor (MinorScaleChoice c)
{
    switch (c)
    {
        case MinorScaleChoice::Aeolian:       return { 7, { { 0,2,3,5,7,8,10,0 } } };
        case MinorScaleChoice::HarmonicMinor: return { 7, { { 0,2,3,5,7,8,11,0 } } };
        case MinorScaleChoice::Dorian:
        default:                              return { 7, { { 0,2,3,5,7,9,10,0 } } };
    }
}

/** Scale used to snap passing tones, chosen by the held (active) chord's quality. Data-only,
    allocation-free; read on the engine timer thread. */
inline NttScale scaleForQuality (ChordQuality q, MinorScaleChoice minor)
{
    switch (q)
    {
        case ChordQuality::Maj:
        case ChordQuality::Maj7:    return { 7, { { 0,2,4,5,7,9,11,0 } } };   // Ionian
        case ChordQuality::Dom7:
        case ChordQuality::Sus2:
        case ChordQuality::Sus4:    return { 7, { { 0,2,4,5,7,9,10,0 } } };   // Mixolydian
        case ChordQuality::Min:
        case ChordQuality::Min7:    return minorScaleFor (minor);
        case ChordQuality::Dim:     return { 8, { { 0,2,3,5,6,8,9,11 } } };   // whole-half diminished
        case ChordQuality::HalfDim: return { 7, { { 0,1,3,5,6,8,10,0 } } };   // Locrian
        case ChordQuality::Aug:     return { 6, { { 0,2,4,6,8,10,0,0 } } };   // whole-tone
        case ChordQuality::None:
        default:                    return { 7, { { 0,2,4,5,7,9,11,0 } } };
    }
}
```

- [ ] **Step 4: Register the test file in the build** — add `tests/unit/test_ntt_scales.cpp` to `Project Synth2.jucer` next to `test_chord_transposer.cpp` (same group/compile flags), then re-save with Projucer (or add the `<FILE>` entry by hand and let the resave double-build settle). Confirm it compiles into the exe.

- [ ] **Step 5: Run the test — expect PASS**

Run: `"...Project Synth2.exe" --run-tests --arranger-tests`
Expected: `NttScales` test group passes.

- [ ] **Step 6: Commit** (ask first)

```bash
git add Source/Arranger/NttScales.h tests/unit/test_ntt_scales.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): add data-driven NTT snap-scale table with curated minor selector"
```

---

## Task 3: `ChordTransposer` — four NTT types, role remap + scale snap

**Files:**
- Modify: `Source/Arranger/ChordTransposer.h`
- Modify: `Source/Arranger/ChordTransposer.cpp` (full rewrite of the dispatch)
- Test: `tests/unit/test_chord_transposer.cpp`

**Interfaces:**
- Consumes: `chordIntervals(ChordQuality)` (`Chord.h`), `scaleForQuality(...)` (`NttScales.h`).
- Produces: `int ChordTransposer::transpose (int note, PartKind part, NttType ntt) const;` plus a 2-arg overload `transpose(note, part)` delegating with `NttType::Chord` (keeps existing call sites + tests compiling). Setters `setMinorScale(MinorScaleChoice)`, `setNttEnabled(bool)`. Private helper `int snapToScale(int noteVal) const`.

- [ ] **Step 1: Write the failing tests** — append new `beginTest` blocks to `tests/unit/test_chord_transposer.cpp` (keep the existing ones; they must stay green):

```cpp
        beginTest ("Parallel shifts every note by the root interval");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 5, ChordQuality::Maj, 5 });   // F (+5)
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Parallel), 65);
            expectEquals (t.transpose (62, PartKind::Acc, NttType::Parallel), 67);
            expectEquals (t.transpose (64, PartKind::Acc, NttType::Parallel), 69);
        }

        beginTest ("Chord remaps 3rd/5th to the active chord's roles");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C E G
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A C E
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord) % 12, 9);  // C -> A
            expectEquals (t.transpose (64, PartKind::Acc, NttType::Chord) % 12, 0);  // E -> C (min 3rd)
            expectEquals (t.transpose (67, PartKind::Acc, NttType::Chord) % 12, 4);  // G -> E
        }

        beginTest ("Chord snaps a passing tone to the active scale");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C maj (passing tones not in {0,4,7})
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A min, Dorian default
            t.setMinorScale (MinorScaleChoice::Dorian);
            // G# (deg 8, passing) + interval 9 -> pc 5 (F); F not in A-Dorian {9,11,0,2,4,6,7}
            // nearest scale tones are 4 and 6 (tie) -> resolve DOWN -> 4 (E)
            expectEquals (t.transpose (68, PartKind::Acc, NttType::Chord) % 12, 4);
        }

        beginTest ("NoTranspose returns the input note");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 5, ChordQuality::Maj, 5 });
            expectEquals (t.transpose (61, PartKind::Acc, NttType::NoTranspose), 61);
        }

        beginTest ("Fixed keeps the octave nearest the original note (no drift)");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });   // C
            t.setActiveChord   ({ 9, ChordQuality::Maj, 9 });   // A (+9 up, but nearest is -3 down)
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord), 69);  // drifts up an octave-ish
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Fixed), 57);  // A nearest to C4 is A3
        }

        beginTest ("minor-scale selector changes the snap target");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 9, ChordQuality::Min, 9 });   // A min
            t.setMinorScale (MinorScaleChoice::Aeolian);        // has F natural (pc 5)
            expectEquals (t.transpose (68, PartKind::Acc, NttType::Chord) % 12, 5);  // stays on F
            t.setMinorScale (MinorScaleChoice::Dorian);         // F# instead of F
            expectEquals (t.transpose (68, PartKind::Acc, NttType::Chord) % 12, 4);  // snaps to E
        }
```

- [ ] **Step 2: Run to verify failure**

Run: build → expect FAIL to compile (3-arg `transpose`, `setMinorScale`, `setNttEnabled` missing).

- [ ] **Step 3: Update the header** — `Source/Arranger/ChordTransposer.h`, replace the public/private bodies:

```cpp
#pragma once
#include "Chord.h"
#include "NttScales.h"

/**
 * Note Transposition Table (NTT, Phase 7a): maps each accompaniment note from the recorded
 * (original) chord onto the played (active) chord per the track's NttType. Chord-tone roles are
 * remapped to the active chord; passing tones snap to the active chord's scale. Pure: no JUCE
 * GUI/threads, no allocation on the per-note path; read on the engine's timer thread.
 */
class ChordTransposer
{
public:
    void setOriginalChord (ArrangerChord recorded) { original = recorded; }
    void setActiveChord   (ArrangerChord played)   { active = played; }
    void setBassInversion (bool shouldInvert)      { bassInversion = shouldInvert; }
    bool isBassInversionOn() const                 { return bassInversion; }

    void setMinorScale (MinorScaleChoice c)        { minorScale = c; }
    void setNttEnabled (bool enabled)              { nttEnabled = enabled; }

    /** Map one note for a part using the given NTT type. Fixed parts and invalid chords return the
        note unchanged; when NTT is globally disabled every type behaves as NoTranspose. */
    int transpose (int noteNumber, PartKind part, NttType ntt) const;

    /** Back-compat overload (Chord type). */
    int transpose (int noteNumber, PartKind part) const { return transpose (noteNumber, part, NttType::Chord); }

private:
    int snapToScale (int noteVal) const;
    int wrapToOctave (int origin, int n) const;   // implemented in Task 4 (returns clamp until then)

    ArrangerChord    original   { 0, ChordQuality::Maj, 0 };   // default C major
    ArrangerChord    active;
    bool             bassInversion = false;
    MinorScaleChoice minorScale    = MinorScaleChoice::Dorian;
    bool             nttEnabled     = true;
};
```

- [ ] **Step 4: Rewrite `Source/Arranger/ChordTransposer.cpp`**

```cpp
#include "ChordTransposer.h"
#include <juce_core/juce_core.h>
#include <cstdlib>

namespace
{
    int mod12 (int x) { return ((x % 12) + 12) % 12; }
    int clampMidi (int n) { while (n < 0) n += 12; while (n > 127) n -= 12; return n; }

    int indexOfDegree (const std::vector<int>& tones, int deg)
    {
        for (int i = 0; i < (int) tones.size(); ++i) if (tones[i] == deg) return i;
        return -1;
    }
}

int ChordTransposer::snapToScale (int noteVal) const
{
    const NttScale sc = scaleForQuality (active.quality, minorScale);
    const int pc = mod12 (noteVal);
    int best = noteVal, bestDist = 1000;
    for (int i = 0; i < sc.count; ++i)
    {
        const int spc = mod12 (active.root + sc.degrees[i]);
        const int up  = noteVal + mod12 (spc - pc);   // nearest note >= noteVal with class spc
        for (int cand : { up, up - 12 })
        {
            const int d = std::abs (cand - noteVal);
            if (d < bestDist || (d == bestDist && cand < best)) { best = cand; bestDist = d; }
        }
    }
    return best;
}

int ChordTransposer::wrapToOctave (int origin, int n) const
{
    // Task 4 upgrades this; until then keep notes in MIDI range.
    return clampMidi (n);
}

int ChordTransposer::transpose (int noteNumber, PartKind part, NttType ntt) const
{
    if (part == PartKind::Fixed || ! active.isValid() || ! original.isValid())
        return noteNumber;

    if (! nttEnabled)
        ntt = NttType::NoTranspose;   // master bypass: play in the recorded home key

    // Bass inversion (slash chords) layers on top, independent of NTT type.
    if (part == PartKind::Bass && bassInversion && active.bassNote >= 0)
        return clampMidi (noteNumber + (active.bassNote - original.root));

    if (ntt == NttType::NoTranspose)
        return noteNumber;

    const int interval = active.root - original.root;

    if (ntt == NttType::Parallel)
        return wrapToOctave (noteNumber, noteNumber + interval);

    // Chord / Fixed: remap chord-tone roles; snap passing tones to the active scale.
    const int  deg  = mod12 (noteNumber - original.root);
    const auto orig = chordIntervals (original.quality);
    const auto act  = chordIntervals (active.quality);
    const int  idx  = indexOfDegree (orig, deg);

    int result;
    if (idx >= 0 && idx < (int) act.size())
        result = noteNumber + interval + (act[idx] - orig[idx]);   // same role of the active chord
    else
        result = snapToScale (noteNumber + interval);             // passing tone -> active scale

    if (ntt == NttType::Fixed)
    {
        // Place the resulting pitch class in the octave nearest the original note (no drift).
        const int pc = mod12 (result);
        const int up = noteNumber + mod12 (pc - mod12 (noteNumber));
        result = (up - noteNumber <= 6) ? up : up - 12;
        return clampMidi (result);
    }

    return wrapToOctave (noteNumber, result);
}
```

- [ ] **Step 5: Run the tests — expect PASS**

Run: `"...Project Synth2.exe" --run-tests --arranger-tests`
Expected: PASS — both the new Phase-7a blocks and all the pre-existing `ChordTransposer` blocks (verified by hand to coincide for those cases).

- [ ] **Step 6: Commit** (ask first)

```bash
git add Source/Arranger/ChordTransposer.h Source/Arranger/ChordTransposer.cpp tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): NTT engine — four types, role remap + scale-snap passing tones"
```

---

## Task 4: Wrap-around register guard

**Files:**
- Modify: `Source/Arranger/ChordTransposer.cpp` (`wrapToOctave`)
- Test: `tests/unit/test_chord_transposer.cpp`

**Interfaces:**
- Consumes: `wrapToOctave(int origin, int n)` declared in Task 3.
- Produces: same signature; now folds results that land more than one octave from `origin` back toward it, then MIDI-clamps.

- [ ] **Step 1: Write the failing test** — append to `tests/unit/test_chord_transposer.cpp`:

```cpp
        beginTest ("wrap-around folds a >1-octave result back toward the origin");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Dim, 0 });   // C dim {0,3,6}
            t.setActiveChord   ({ 11, ChordQuality::Aug, 11 }); // B aug {0,4,8}, interval +11
            // F# is the dim 5th (deg 6, idx 2); active aug 5th is deg 8 -> +2. result = 60+11+2 = 73,
            // which is 13 semitones above the origin 60 -> fold down an octave to 61 (same pitch class).
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord), 61);
            expectEquals (t.transpose (60, PartKind::Acc, NttType::Chord) % 12, 1);
        }

        beginTest ("result still clamps to 0..127 for Chord");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 7, ChordQuality::Dom7, 7 });
            for (int n = 0; n <= 127; ++n)
            {
                const int x = t.transpose (n, PartKind::Acc, NttType::Chord);
                expect (x >= 0 && x <= 127);
            }
        }
```

- [ ] **Step 2: Run to verify failure**

Run: build + `--arranger-tests` → expect the fold test to FAIL (current stub returns 73, not 61).

- [ ] **Step 3: Implement the guard** — replace the `wrapToOctave` body in `ChordTransposer.cpp`:

```cpp
int ChordTransposer::wrapToOctave (int origin, int n) const
{
    while (n - origin > 12) n -= 12;   // more than an octave above -> fold down
    while (origin - n > 12) n += 12;   // more than an octave below -> fold up
    return clampMidi (n);
}
```

- [ ] **Step 4: Run the tests — expect PASS**

Run: `--arranger-tests`
Expected: PASS, including the new fold test and the full-range clamp; all earlier blocks remain green (Parallel intervals are ≤ 11 semitones, so they never fold and stay truly parallel).

- [ ] **Step 5: Commit** (ask first)

```bash
git add Source/Arranger/ChordTransposer.cpp tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): NTT register guard — fold >1-octave results back toward origin"
```

---

## Task 5: Master "Chord Transposition" bypass — engine forwarder + Settings toggle

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.h/.cpp` (`setNttEnabled`)
- Modify: `Source/Styles/CurrentStyleComponent.h/.cpp` + `Source/UI/Display.h/.cpp` (pass-through + re-apply on Start — mirror `setArrangerBassInversion`)
- Modify: `Source/UI/SettingsWindow.h/.cpp` (toggle, property `"NttEnabled"`, default true)
- Modify: `Source/UI/MainComponent.cpp` (callback wiring + startup restore)
- Test: `tests/unit/test_chord_transposer.cpp`

**Interfaces:**
- Consumes: `ChordTransposer::setNttEnabled(bool)` (Task 3).
- Produces: `ArrangerEngine::setNttEnabled(bool)`; `CurrentStyleComponent::setArrangerNttEnabled(bool)` + remembered `arrangerNttEnabled`; `Display::setArrangerNttEnabled(bool)`; `SettingsWindow::onNttEnabledChanged` (`std::function<void(bool)>`) + `nttEnabledToggle`.

- [ ] **Step 1: Write the failing transposer test** — append to `tests/unit/test_chord_transposer.cpp`:

```cpp
        beginTest ("master bypass: disabled NTT returns input notes for all parts/types");
        {
            ChordTransposer t;
            t.setOriginalChord ({ 0, ChordQuality::Maj, 0 });
            t.setActiveChord   ({ 5, ChordQuality::Min, 5 });
            t.setNttEnabled (false);
            expectEquals (t.transpose (60, PartKind::Acc,  NttType::Chord),    60);
            expectEquals (t.transpose (64, PartKind::Acc,  NttType::Parallel), 64);
            expectEquals (t.transpose (43, PartKind::Bass, NttType::Parallel), 43);
            t.setNttEnabled (true);
            expect (t.transpose (60, PartKind::Acc, NttType::Chord) != 60);   // active again
        }
```

(This already passes if Task 3's `if (! nttEnabled) ntt = NoTranspose;` is in place — it locks the contract before the UI wiring. Run it to confirm green, then proceed to wire the UI, which has no unit test and is verified by build + manual smoke.)

- [ ] **Step 2: Run it — expect PASS** (`--arranger-tests`). If it fails, fix Task 3's bypass line.

- [ ] **Step 3: Engine forwarder** — `ArrangerEngine.h` next to `void setBassInversion (bool);` add:
```cpp
    void setNttEnabled (bool enabled);
```
`ArrangerEngine.cpp` next to `setBassInversion`:
```cpp
void ArrangerEngine::setNttEnabled (bool enabled)
{
    transposer.setNttEnabled (enabled);
}
```

- [ ] **Step 4: Pass-through + re-apply on Start** — mirror Bass Inversion exactly.

`CurrentStyleComponent.h`: add `void setArrangerNttEnabled (bool enabled);` and member `bool arrangerNttEnabled = true;`.
`CurrentStyleComponent.cpp`: add (modeled on `setArrangerBassInversion`, ~line 376):
```cpp
void CurrentStyleComponent::setArrangerNttEnabled (bool enabled)
{
    arrangerNttEnabled = enabled;
    if (arrangerEngine != nullptr)
        arrangerEngine->setNttEnabled (enabled);
}
```
In `startPlaying()` where `arrangerEngine->setBassInversion (arrangerBassInversion);` is re-applied (~line 36), add directly after:
```cpp
        arrangerEngine->setNttEnabled (arrangerNttEnabled);   // re-apply; setStyle/rebuild may reset
```
`Display.h`: add `void setArrangerNttEnabled (bool enabled);` and `bool arrangerNttEnabled = true;`.
`Display.cpp`: mirror `setArrangerBassInversion` (~line 456):
```cpp
void Display::setArrangerNttEnabled (bool enabled)
{
    arrangerNttEnabled = enabled;
    if (currentStyleComponent != nullptr)
        currentStyleComponent->setArrangerNttEnabled (enabled);
}
```
And where `currentStyleComponent->setArrangerBassInversion(arrangerBassInversion);` is applied on (re)creation (~line 246), add:
```cpp
        currentStyleComponent->setArrangerNttEnabled (arrangerNttEnabled);
```

- [ ] **Step 5: Settings toggle** — `SettingsWindow.h`: next to `onChordBassInversionChanged` add:
```cpp
    std::function<void(bool)> onNttEnabledChanged;       // Phase 7a: master Chord Transposition on/off
```
next to `chordBassInversionToggle` add:
```cpp
    juce::ToggleButton nttEnabledToggle { "Chord transposition (NTT)" };   /**< Phase 7a master bypass */
```
`SettingsWindow.cpp`: in the layout method, after `chordBassInversionToggle.setBounds(...)` (~line 339):
```cpp
    y += rowH + 6;
    nttEnabledToggle.setBounds (pad, y, 320, rowH);
```
In the setup method, after the `chordBassInversionToggle` block (~line 495):
```cpp
    settingsPanel.addAndMakeVisible (nttEnabledToggle);
    nttEnabledToggle.setToggleState (propertyFile != nullptr && propertyFile->getBoolValue ("NttEnabled", true),
                                     juce::dontSendNotification);
    nttEnabledToggle.onClick = [this]()
    {
        const bool on = nttEnabledToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue ("NttEnabled", on); propertyFile->saveIfNeeded(); }
        if (onNttEnabledChanged) onNttEnabledChanged (on);
    };
```
If the panel has a fixed `setSize(...)` height, bump it by `rowH + 6` so the new row fits (match how the chord-zone-mute row bumped it earlier).

- [ ] **Step 6: MainComponent wiring** — in the block that sets `midiWindow->onChordBassInversionChanged` (~line 999):
```cpp
            midiWindow->onNttEnabledChanged = [this](bool on)
            {
                if (display != nullptr) display->setArrangerNttEnabled (on);
            };
```
In the startup-restore block near `display->setArrangerBassInversion(propertiesFile->getBoolValue("ChordBassInversion", false));` (~line 2108):
```cpp
            display->setArrangerNttEnabled (propertiesFile->getBoolValue ("NttEnabled", true));
```

- [ ] **Step 7: Build + manual smoke**

Build clean; launch; toggle "Chord transposition (NTT)" off → accompaniment stays in the recorded home key while sections/fills/synchro and chord recognition still run; toggle on → it follows held chords again. Confirm the setting persists across a restart.

- [ ] **Step 8: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp \
        Source/Styles/CurrentStyleComponent.h Source/Styles/CurrentStyleComponent.cpp \
        Source/UI/Display.h Source/UI/Display.cpp Source/UI/SettingsWindow.h Source/UI/SettingsWindow.cpp \
        Source/UI/MainComponent.cpp tests/unit/test_chord_transposer.cpp
git commit -m "feat(arranger): master Chord-Transposition on/off bypass (Settings + engine)"
```

---

## Task 6: Minor-scale selector — engine forwarder + Settings 3-way

**Files:**
- Modify: `Source/Arranger/ArrangerEngine.h/.cpp` (`setMinorScale`)
- Modify: `Source/Styles/CurrentStyleComponent.h/.cpp` + `Source/UI/Display.h/.cpp` (pass-through + re-apply on Start)
- Modify: `Source/UI/SettingsWindow.h/.cpp` (3-way selector, property `"NttMinorScale"` int 0/1/2)
- Modify: `Source/UI/MainComponent.cpp` (callback + startup restore)

**Interfaces:**
- Consumes: `ChordTransposer::setMinorScale(MinorScaleChoice)` (Task 3).
- Produces: `ArrangerEngine::setMinorScale(MinorScaleChoice)`; `CurrentStyleComponent::setArrangerMinorScale(MinorScaleChoice)` + remembered member; `Display::setArrangerMinorScale(MinorScaleChoice)`; `SettingsWindow::onNttMinorScaleChanged` (`std::function<void(int)>`, 0=Dorian/1=Aeolian/2=HarmonicMinor).

This task has no new pure logic to unit-test (Task 3's "minor-scale selector changes the snap target" already covers the engine behavior). It is a wiring task verified by build + smoke + the existing minor-scale transposer test.

- [ ] **Step 1: Engine forwarder** — `ArrangerEngine.h`:
```cpp
    void setMinorScale (MinorScaleChoice choice);
```
`ArrangerEngine.cpp`:
```cpp
void ArrangerEngine::setMinorScale (MinorScaleChoice choice)
{
    transposer.setMinorScale (choice);
}
```

- [ ] **Step 2: Pass-through + re-apply on Start** — mirror Task 5.

`CurrentStyleComponent.h`: `void setArrangerMinorScale (MinorScaleChoice c);` + `MinorScaleChoice arrangerMinorScale = MinorScaleChoice::Dorian;`.
`CurrentStyleComponent.cpp`:
```cpp
void CurrentStyleComponent::setArrangerMinorScale (MinorScaleChoice c)
{
    arrangerMinorScale = c;
    if (arrangerEngine != nullptr)
        arrangerEngine->setMinorScale (c);
}
```
In `startPlaying()` after the `setNttEnabled` re-apply line:
```cpp
        arrangerEngine->setMinorScale (arrangerMinorScale);   // re-apply on Start
```
`Display.h`: `void setArrangerMinorScale (MinorScaleChoice c);` + `MinorScaleChoice arrangerMinorScale = MinorScaleChoice::Dorian;`.
`Display.cpp`:
```cpp
void Display::setArrangerMinorScale (MinorScaleChoice c)
{
    arrangerMinorScale = c;
    if (currentStyleComponent != nullptr)
        currentStyleComponent->setArrangerMinorScale (c);
}
```
And in the currentStyleComponent (re)creation block:
```cpp
        currentStyleComponent->setArrangerMinorScale (arrangerMinorScale);
```

- [ ] **Step 3: Settings 3-way selector** — model on the existing 3-way chord-mode button group in `SettingsWindow.cpp` (the Phase-5 selector with `accent1`-on / dark-off colours added this session). Add three `juce::TextButton` (or reuse the existing radio-group helper) labelled **Dorian / Aeolian / Harmonic**, mutually exclusive, default Dorian.

`SettingsWindow.h`:
```cpp
    std::function<void(int)> onNttMinorScaleChanged;     // 0=Dorian, 1=Aeolian, 2=HarmonicMinor
    juce::TextButton nttMinorDorianBtn   { "Dorian" };
    juce::TextButton nttMinorAeolianBtn  { "Aeolian" };
    juce::TextButton nttMinorHarmonicBtn { "Harmonic" };
```
`SettingsWindow.cpp` setup (mirror the chord-mode group's colours/radio behaviour exactly — same `setColour` calls so it matches the app theme):
```cpp
    auto initMinorBtn = [this](juce::TextButton& b, int idx)
    {
        settingsPanel.addAndMakeVisible (b);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (7701);
        b.setColour (juce::TextButton::buttonColourId,   cardBg);
        b.setColour (juce::TextButton::buttonOnColourId, accent1);
        b.setColour (juce::TextButton::textColourOffId,  subtitleText);
        b.setColour (juce::TextButton::textColourOnId,   background);
        b.onClick = [this, idx]()
        {
            if (propertyFile != nullptr) { propertyFile->setValue ("NttMinorScale", idx); propertyFile->saveIfNeeded(); }
            if (onNttMinorScaleChanged) onNttMinorScaleChanged (idx);
        };
    };
    initMinorBtn (nttMinorDorianBtn,   0);
    initMinorBtn (nttMinorAeolianBtn,  1);
    initMinorBtn (nttMinorHarmonicBtn, 2);
    const int savedMinor = propertyFile != nullptr ? propertyFile->getIntValue ("NttMinorScale", 0) : 0;
    (savedMinor == 1 ? nttMinorAeolianBtn : savedMinor == 2 ? nttMinorHarmonicBtn : nttMinorDorianBtn)
        .setToggleState (true, juce::dontSendNotification);
```
Layout: lay the three side by side on a new row below the `nttEnabledToggle` (match the chord-mode row's geometry). Bump the panel height again if it's fixed.

- [ ] **Step 4: MainComponent wiring** — near the other arranger settings callbacks:
```cpp
            midiWindow->onNttMinorScaleChanged = [this](int idx)
            {
                const auto c = idx == 1 ? MinorScaleChoice::Aeolian
                             : idx == 2 ? MinorScaleChoice::HarmonicMinor
                                        : MinorScaleChoice::Dorian;
                if (display != nullptr) display->setArrangerMinorScale (c);
            };
```
Startup restore (near the `NttEnabled` restore):
```cpp
            {
                const int m = propertiesFile->getIntValue ("NttMinorScale", 0);
                const auto c = m == 1 ? MinorScaleChoice::Aeolian
                             : m == 2 ? MinorScaleChoice::HarmonicMinor
                                      : MinorScaleChoice::Dorian;
                if (display != nullptr) display->setArrangerMinorScale (c);
            }
```

- [ ] **Step 5: Build + smoke** — build clean; play a minor chord with a style that has audible passing tones; switch Dorian ↔ Aeolian ↔ Harmonic and hear the passing-tone colour change; confirm persistence across restart.

- [ ] **Step 6: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerEngine.h Source/Arranger/ArrangerEngine.cpp \
        Source/Styles/CurrentStyleComponent.h Source/Styles/CurrentStyleComponent.cpp \
        Source/UI/Display.h Source/UI/Display.cpp Source/UI/SettingsWindow.h Source/UI/SettingsWindow.cpp \
        Source/UI/MainComponent.cpp
git commit -m "feat(arranger): curated minor-scale selector for NTT passing-tone snap"
```

---

## Task 7: Per-track NTT-type selector in the style editor

**Files:**
- Modify: `Source/Arranger/ArrangerStyleEditor.h/.cpp` (track rows)
- Test: manual (editor UI); persistence already covered by Task 1's IO round-trip.

**Interfaces:**
- Consumes: `SourceTrackFile.nttType` (Task 1), `ArrangerEnums::toString/nttFromString` (Task 1).
- Produces: a per-track 4-way control writing `sourceTracks[i].nttType`; the value flows out through the existing `toStyleFile()` → save path and into the live preview through `buildStyle()` (Task 8 consumes it at emit).

- [ ] **Step 1: Inspect the existing track-row rendering** — in `ArrangerStyleEditor.cpp`, find where each `sourceTracks[i]` row is drawn/laid out (the loop near `for (const auto& st : sourceTracks)` ~line 115, and wherever per-track controls like name/part are shown). Identify whether rows use child Components or are custom-painted; follow that pattern.

- [ ] **Step 2: Add a 4-way selector per track** — a `juce::ComboBox` per row (items: `No Transpose`, `Parallel`, `Chord`, `Fixed`; ids 1..4 mapping to `NttType` 0..3), themed to match the editor. Initialize each from `sourceTracks[i].nttType`. On change:
```cpp
    // idx0 = NttType order: NoTranspose, Parallel, Chord, Fixed
    sourceTracks[trackIndex].nttType = static_cast<NttType> (comboSelectedId - 1);
    // keep the live preview in sync, exactly like a region/key edit does:
    onSourceTracksChanged();   // or the existing method that calls engine.setStyle(buildStyle())
```
Use the editor's existing "something changed → rebuild preview" entry point (the same one region moves use, which calls `engine.setStyle(buildStyle())`). Do **not** introduce a new rebuild path.

- [ ] **Step 3: Persist** — no new code: `toStyleFile()` already copies `sourceTracks` wholesale (`f.sourceTracks = sourceTracks;`), and Task 1 serializes `nttType`. Saving the style now writes per-track NTT; reloading restores it.

- [ ] **Step 4: Build + smoke** — open the editor, change a track's type (e.g. set the bass to Parallel, a pad to Fixed), save, reload → the selectors restore; play and confirm the audible behavior differs per track. Confirm drum tracks default to / show **No Transpose**.

- [ ] **Step 5: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerStyleEditor.h Source/Arranger/ArrangerStyleEditor.cpp
git commit -m "feat(arranger): per-track NTT-type selector in the style editor"
```

---

## Task 8: Engine wiring — thread per-track `NttType` to emit time

**Files:**
- Modify: `Source/Arranger/ArrangerScheduler.h/.cpp` (carry `NttType` parallel to `PartKind`)
- Modify: `Source/Arranger/ArrangerEngine.cpp` (`rebuildFromStyle` builds the `NttType` vector; dispatch passes it to `transpose`)
- Test: `tests/unit/test_arranger_engine.cpp` (or `test_arranger_scheduler.cpp`)

**Interfaces:**
- Consumes: `ArrangerTrack.nttType` (Task 1), `ChordTransposer::transpose(note, part, ntt)` (Task 3), the existing `EmittedEvent.part` / `sortedParts` / `activeNoteParts` threading.
- Produces: `EmittedEvent.ntt`; `ArrangerScheduler::setLoop(events, parts, ntts, loopLengthBeats)` (3-vector overload); `sortedNtt` + `activeNoteNtt` mirroring the part vectors so note-offs carry the same type as their note-ons.

- [ ] **Step 1: Write the failing test** — in `tests/unit/test_arranger_engine.cpp` (or the scheduler test), assert a non-default per-track type reaches `transpose`. Concretely, build a one-track style whose single Acc track is set to `NttType::Parallel`, set original C / active F, advance the engine over the note, and assert the emitted note is the **parallel** shift (note+5), not the Chord remap. Use the test harness's existing engine-construction helper in that file. Sketch:

```cpp
beginTest ("per-track NttType reaches emit-time transposition");
{
    ArrangerStyle style = /* one section, one Acc track, single note C4 (60) at beat 0 */;
    style.originalRoot = 0; style.originalQuality = ChordQuality::Maj;
    style.sections[0].tracks[0].nttType = NttType::Parallel;

    ArrangerEngine engine (/* null/loopback out per the file's pattern */);
    engine.setStyle (style);
    engine.setActiveChord ({ 5, ChordQuality::Maj, 5 });   // F
    // capture emitted notes via the file's existing onMidiMessage hook
    // advance one tick covering beat 0
    // EXPECT a note-on for 65 (Parallel: 60 + 5), NOT 65-from-Chord-coincidence:
    //   choose a note where Parallel and Chord differ, e.g. original note 62 (D passing):
    //     Parallel -> 67 ; Chord over F maj would snap 67 (G) -> in F scale, also 67 — pick instead
    //   original note 61 (C#, passing): Parallel -> 66 ; Chord -> snaps 66 to F-Ionian (65 or 67).
    // Use note 61 so Parallel(66) != Chord(65/67).
    expect (capturedContains (66));   // proves Parallel type was used
}
```
Pick the seed note so Parallel and Chord give different results (e.g. a chromatic passing tone like MIDI 61 over C→F: Parallel→66, Chord snaps to 65 or 67). Match the file's actual engine/dispatch capture helpers.

- [ ] **Step 2: Run to verify failure** — expect FAIL (engine currently ignores `nttType`; emits the Chord-type result).

- [ ] **Step 3: Carry `NttType` through the scheduler** — `ArrangerScheduler.h`:
  - In `struct EmittedEvent`, after `PartKind part = PartKind::Fixed;` add:
    ```cpp
    NttType ntt = NttType::Chord;   // Phase 7a: how to transpose this note
    ```
  - Add a 3-vector overload:
    ```cpp
    void setLoop (std::vector<TimedBeatEvent> events, std::vector<PartKind> parts,
                  std::vector<NttType> ntts, double loopLengthBeats);
    ```
  - Add parallel storage mirroring `sortedParts` / `activeNoteParts`:
    ```cpp
    std::vector<NttType>                   sortedNtt;            // aligned with sortedEvents
    std::map<std::pair<int,int>, NttType>  activeNoteNtt;        // ntt of each sounding note (seam offs)
    ```
  - `trackActiveNote` signature gains the ntt:
    ```cpp
    void trackActiveNote (const juce::MidiMessage& m, PartKind part, NttType ntt);
    ```

  `ArrangerScheduler.cpp`:
  - Keep the existing 2-vector `setLoop` delegating; have it pass an empty ntt vector to the 3-vector form:
    ```cpp
    void ArrangerScheduler::setLoop (std::vector<TimedBeatEvent> events,
                                     std::vector<PartKind> parts, double loopLengthBeats)
    {
        setLoop (std::move (events), std::move (parts), {}, loopLengthBeats);
    }
    ```
  - In the 3-vector `setLoop`, fill `sortedNtt` exactly as `sortedParts` is filled, defaulting missing entries to `NttType::Chord`:
    ```cpp
        sortedNtt.push_back (i < ntts.size() ? ntts[i] : NttType::Chord);
    ```
    Clear `activeNoteNtt` everywhere `activeNoteParts` is cleared (reset paths + both `clear()` sites).
  - In `advance`, where it reads `const PartKind part = sortedParts[i];`, also read `const NttType ntt = sortedNtt[i];`, set `ev.ntt = ntt;` on the emitted note-on, and call `trackActiveNote (m, part, ntt);`.
  - In `trackActiveNote`, store `activeNoteNtt[key] = ntt;` on note-on and `erase(key)` on note-off (mirror the part map).
  - In `flushActiveNotes` and the loop-seam off path that looks up `activeNoteParts`, also look up `activeNoteNtt` (default `NttType::Chord`) and set `ev.ntt` on the synthesized note-off so the off transposes identically to its on.

- [ ] **Step 4: Build the `NttType` vector in `rebuildFromStyle`** — `ArrangerEngine.cpp:29-43`, alongside `parts`:
```cpp
        std::vector<TimedBeatEvent> merged;
        std::vector<PartKind>       parts;
        std::vector<NttType>        ntts;
        for (const auto& tr : sec.tracks)
        {
            const PartKind pk = (tr.partType == ArrangerPartType::Bass) ? PartKind::Bass
                              : (tr.partType == ArrangerPartType::Acc)  ? PartKind::Acc
                                                                        : PartKind::Fixed;
            for (const auto& ev : tr.pattern)
            {
                merged.push_back (ev);
                parts.push_back (pk);
                ntts.push_back (tr.nttType);
            }
        }
        ...
        s.setLoop (std::move (merged), std::move (parts), std::move (ntts), bars * bpb);
```

- [ ] **Step 5: Pass `ntt` into `transpose` at dispatch** — find `dispatchEmitted` (where it currently calls `transposer.transpose (note, ev.part)` / `(note, part)`), and change the call to:
```cpp
        const int outNote = transposer.transpose (note, ev.part, ev.ntt);
```
Apply at every transpose call site that handles emitted note-on/off (note-on and the seam/flush note-off must both use `ev.ntt`).

- [ ] **Step 6: Run the test — expect PASS**

Run: `--arranger-tests`
Expected: PASS — the Parallel-typed track emits the parallel shift; all other arranger-engine tests stay green (default `NttType::Chord` reproduces prior behavior for untyped paths).

- [ ] **Step 7: Full regression + smoke**

Run all suites: `--unit-tests`, `--arranger-tests`, `--integration-tests` → 0 failures. Then a manual smoke: load a real style, set per-track types in the editor, play held chords, toggle the master bypass and minor scale — confirm each track transposes per its type and drums never move.

- [ ] **Step 8: Commit** (ask first)

```bash
git add Source/Arranger/ArrangerScheduler.h Source/Arranger/ArrangerScheduler.cpp \
        Source/Arranger/ArrangerEngine.cpp tests/unit/test_arranger_engine.cpp
git commit -m "feat(arranger): thread per-track NttType through scheduler to emit-time transposition"
```

---

## Self-Review (against the spec)

**Spec coverage:**
- §4 four types → Task 3 (NoTranspose/Parallel/Chord/Fixed) + Task 4 (wrap). ✓
- §4.1 role remap → Task 3 (`act[idx]-orig[idx]`). ✓
- §4.2 passing-tone snap + downward tie → Task 3 `snapToScale`. ✓
- §5 data-driven scale table → Task 2 (`NttScales.h`). ✓
- §5.1 curated minor selector → Task 2 (table) + Task 6 (Settings). ✓
- §6 wrap-around → Task 4. ✓
- §7 master bypass → Task 5. ✓
- §8 enums + per-track field + schema bump + migration → Task 1. ✓
- §9 UI (per-track selector; minor + master in Settings) → Task 7 + Tasks 5/6. ✓
- §10 engine wiring + off-audio-path setters + purity → Task 8 + Tasks 5/6 forwarders. ✓
- §11 tests → Tasks 1–8 cover NoTranspose, Parallel, role, snap, minor-switch, Fixed, wrap, bypass, migration; Bass-Inversion regression preserved by keeping the existing inversion test green (Task 3 keeps the inversion branch ahead of type dispatch). ✓

**Type consistency:** `NttType` order `{NoTranspose, Parallel, Chord, Fixed}` is fixed in Task 1 and used as `static_cast<NttType>(id-1)` (Task 7) and combo ids 1..4; `MinorScaleChoice {Dorian, Aeolian, HarmonicMinor}` ↔ ints 0/1/2 consistently in Tasks 2/6. `setLoop` 3-vector signature defined in Task 8 header matches its `.cpp` and engine call. `transpose(note, part, ntt)` signature identical across Tasks 3/8.

**Placeholder scan:** none — every code step shows the code; the only deliberately deferred body is `wrapToOctave` (stubbed in Task 3, implemented in Task 4) with its own failing test gating it.

**Open confirmations for the implementer (resolve by reading the cited code, not by guessing):**
- Task 2: confirm how `Project Synth2.jucer` lists test files and add the new one the same way.
- Task 7: confirm the editor track-row construction pattern before choosing ComboBox vs. the editor's own control idiom; reuse the existing preview-rebuild entry point.
- Task 8: confirm the exact name of the dispatch method and the test file's engine-capture helper before writing the call/test.
