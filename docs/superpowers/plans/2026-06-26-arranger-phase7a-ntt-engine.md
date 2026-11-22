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


> Continued in [part 2 of 2](2026-06-26-arranger-phase7a-ntt-engine-part2.md).
