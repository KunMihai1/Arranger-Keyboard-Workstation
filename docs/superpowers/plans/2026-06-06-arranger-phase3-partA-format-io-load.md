# Arranger Phase 3 — Part A: Self-Contained Style Format, IO & Load Path — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a self-contained `.style` file format (events + bar-window sections), its JSON save/load, a generalized builder that slices windows into the engine's per-section loops, and the load path that plays a hand-authored style — all headless and unit-tested.

**Architecture:** New on-disk file model (`ArrangerStyleFile`) kept separate from the runtime `ArrangerModel`. Enum + MIDI-hex string codecs. A `ArrangerStyleIOHelper` (JUCE `DynamicObject`/JSON). The slicing helper currently buried inside `buildDemoMultiSectionStyle` is extracted and reused by a new `buildStyleFromWindows`. A channel allocator enforces the locked channel map. No runtime engine changes.

**Tech Stack:** C++17, JUCE (`juce_core`, `juce_audio_basics`), JUCE `UnitTest` framework (category "Arranger").

**Spec:** `docs/superpowers/specs/2026-06-06-arranger-phase3-design.md`

---

## Test / build workflow (read once)

Tests are JUCE `UnitTest` subclasses registered as static instances. They run from `Source/Main.cpp` via `juce::UnitTestRunner`, which is **normally commented out**. To run tests for a task:

1. In `Source/Main.cpp`, enable the test runner block (uncomment) so it calls `runner.runTestsInCategory ("Arranger")`.
2. Build the VS2022 target under `Builds/VisualStudio2022/` (if you re-saved the `.jucer`, expect a double-build; close any running instance first to avoid the PDB lock).
3. Run the app once; results are written to `test-results.txt` in the app data folder.
4. Confirm the new `beginTest(...)` lines report pass, then re-comment the runner before normal use.

All new unit tests in Part A use category **"Arranger"** so `runTestsInCategory ("Arranger")` picks them up. Each task's "run" step means: build with the runner on, run, and read `test-results.txt`.

---

## File Structure

**Create:**
- `Source/Arranger/ArrangerEnums.h` — `toString`/`fromString` for `ArrangerSectionType`, `ArrangerPartType`, `ArrangerAfterComplete`.
- `Source/Arranger/ArrangerMidiHex.h` — `midiToHex` / `midiFromHex` for note messages.
- `Source/Arranger/ArrangerStyleFile.h` — file-model structs (`SourceTrackFile`, `SectionWindow`, `ArrangerStyleFile`).
- `Source/Arranger/ArrangerStyleIOHelper.h` / `.cpp` — JSON save/load of `ArrangerStyleFile`.
- `Source/Arranger/ArrangerChannels.h` / `.cpp` — channel allocator.
- `tests/unit/test_arranger_enums.cpp`
- `tests/unit/test_arranger_midi_hex.cpp`
- `tests/unit/test_arranger_style_io.cpp`
- `tests/unit/test_arranger_channels.cpp`

**Modify:**
- `Source/Arranger/ArrangerPatternBuilder.h` / `.cpp` — extract `sliceSection`, add `buildStyleFromWindows`, add `buildStyleFromFile`.
- `tests/unit/test_arranger_pattern_builder.cpp` — tests for the new builders.
- `Project Synth2.jucer` (+ regenerated VS project) — add the new `Source/Arranger/*` files to the build.

> After adding any new `Source/...` file, add it to the Projucer project (`Project Synth2.jucer`) and re-save so it compiles. Do this as part of each task's implementation step.

---

## Task 1: Enum string codecs

**Files:**
- Create: `Source/Arranger/ArrangerEnums.h`
- Test: `tests/unit/test_arranger_enums.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/test_arranger_enums.cpp
#include <juce_core/juce_core.h>
#include "Arranger/ArrangerEnums.h"

class ArrangerEnumsTest : public juce::UnitTest
{
public:
    ArrangerEnumsTest() : juce::UnitTest ("ArrangerEnums", "Arranger") {}

    void runTest() override
    {
        beginTest ("section type round-trips through string");
        {
            for (auto t : { ArrangerSectionType::Intro, ArrangerSectionType::Variation,
                            ArrangerSectionType::Fill, ArrangerSectionType::Break,
                            ArrangerSectionType::Ending, ArrangerSectionType::CountIn })
                expect (ArrangerEnums::sectionTypeFromString (ArrangerEnums::toString (t)) == t);
        }

        beginTest ("afterComplete round-trips through string");
        {
            for (auto a : { ArrangerAfterComplete::Loop, ArrangerAfterComplete::FallThrough,
                            ArrangerAfterComplete::Stop })
                expect (ArrangerEnums::afterFromString (ArrangerEnums::toString (a)) == a);
        }

        beginTest ("partType round-trips through string");
        {
            for (auto p : { ArrangerPartType::Drum, ArrangerPartType::Perc,
                            ArrangerPartType::Bass, ArrangerPartType::Acc })
                expect (ArrangerEnums::partFromString (ArrangerEnums::toString (p)) == p);
        }

        beginTest ("unknown strings fall back to safe defaults");
        {
            expect (ArrangerEnums::sectionTypeFromString ("nope") == ArrangerSectionType::Variation);
            expect (ArrangerEnums::afterFromString ("nope")       == ArrangerAfterComplete::Loop);
            expect (ArrangerEnums::partFromString ("nope")        == ArrangerPartType::Acc);
        }
    }
};
static ArrangerEnumsTest arrangerEnumsTest;
```

- [ ] **Step 2: Run to verify it fails**

Enable the runner, build, run. Expected: compile error (`ArrangerEnums.h` not found) — that counts as failing.

- [ ] **Step 3: Implement**

```cpp
// Source/Arranger/ArrangerEnums.h
#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"

namespace ArrangerEnums
{
    inline juce::String toString (ArrangerSectionType t)
    {
        switch (t)
        {
            case ArrangerSectionType::Intro:     return "Intro";
            case ArrangerSectionType::Variation: return "Variation";
            case ArrangerSectionType::Fill:      return "Fill";
            case ArrangerSectionType::Break:     return "Break";
            case ArrangerSectionType::Ending:    return "Ending";
            case ArrangerSectionType::CountIn:   return "CountIn";
        }
        return "Variation";
    }

    inline ArrangerSectionType sectionTypeFromString (const juce::String& s)
    {
        if (s == "Intro")   return ArrangerSectionType::Intro;
        if (s == "Fill")    return ArrangerSectionType::Fill;
        if (s == "Break")   return ArrangerSectionType::Break;
        if (s == "Ending")  return ArrangerSectionType::Ending;
        if (s == "CountIn") return ArrangerSectionType::CountIn;
        return ArrangerSectionType::Variation;
    }

    inline juce::String toString (ArrangerAfterComplete a)
    {
        switch (a)
        {
            case ArrangerAfterComplete::Loop:        return "Loop";
            case ArrangerAfterComplete::FallThrough: return "FallThrough";
            case ArrangerAfterComplete::Stop:        return "Stop";
        }
        return "Loop";
    }

    inline ArrangerAfterComplete afterFromString (const juce::String& s)
    {
        if (s == "FallThrough") return ArrangerAfterComplete::FallThrough;
        if (s == "Stop")        return ArrangerAfterComplete::Stop;
        return ArrangerAfterComplete::Loop;
    }

    inline juce::String toString (ArrangerPartType p)
    {
        switch (p)
        {
            case ArrangerPartType::Drum: return "Drum";
            case ArrangerPartType::Perc: return "Perc";
            case ArrangerPartType::Bass: return "Bass";
            case ArrangerPartType::Acc:  return "Acc";
        }
        return "Acc";
    }

    inline ArrangerPartType partFromString (const juce::String& s)
    {
        if (s == "Drum") return ArrangerPartType::Drum;
        if (s == "Perc") return ArrangerPartType::Perc;
        if (s == "Bass") return ArrangerPartType::Bass;
        return ArrangerPartType::Acc;
    }
}
```

Add `ArrangerEnums.h` and `test_arranger_enums.cpp` to `Project Synth2.jucer`, re-save.

- [ ] **Step 4: Run to verify it passes**

Build + run. Expected: all four `ArrangerEnums` tests pass in `test-results.txt`.

- [ ] **Step 5: Commit**

```bash
git add Source/Arranger/ArrangerEnums.h tests/unit/test_arranger_enums.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): add enum string codecs for style serialization"
```

---

## Task 2: MIDI hex codec

**Files:**
- Create: `Source/Arranger/ArrangerMidiHex.h`
- Test: `tests/unit/test_arranger_midi_hex.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/test_arranger_midi_hex.cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerMidiHex.h"

class ArrangerMidiHexTest : public juce::UnitTest
{
public:
    ArrangerMidiHexTest() : juce::UnitTest ("ArrangerMidiHex", "Arranger") {}

    void runTest() override
    {
        beginTest ("note-on round-trips through hex");
        {
            auto m = juce::MidiMessage::noteOn (10, 36, (juce::uint8) 100);
            auto back = ArrangerMidiHex::fromHex (ArrangerMidiHex::toHex (m));
            expect (back.isNoteOn());
            expectEquals (back.getChannel(), 10);
            expectEquals (back.getNoteNumber(), 36);
            expectEquals ((int) back.getVelocity(), 100);
        }

        beginTest ("note-off round-trips through hex");
        {
            auto m = juce::MidiMessage::noteOff (2, 60);
            auto back = ArrangerMidiHex::fromHex (ArrangerMidiHex::toHex (m));
            expect (back.isNoteOff());
            expectEquals (back.getChannel(), 2);
            expectEquals (back.getNoteNumber(), 60);
        }

        beginTest ("hex string is uppercase bytes with no separators");
        {
            auto m = juce::MidiMessage::noteOn (1, 0x24, (juce::uint8) 0x64); // 90 24 64
            expectEquals (ArrangerMidiHex::toHex (m), juce::String ("902464"));
        }
    }
};
static ArrangerMidiHexTest arrangerMidiHexTest;
```

- [ ] **Step 2: Run to verify it fails**

Build + run. Expected: compile error (`ArrangerMidiHex.h` not found).

- [ ] **Step 3: Implement**

```cpp
// Source/Arranger/ArrangerMidiHex.h
#pragma once
#include <JuceHeader.h>

namespace ArrangerMidiHex
{
    inline juce::String toHex (const juce::MidiMessage& m)
    {
        const juce::uint8* data = m.getRawData();
        const int size = m.getRawDataSize();
        juce::String out;
        for (int i = 0; i < size; ++i)
            out += juce::String::toHexString (data[i]).paddedLeft ('0', 2).toUpperCase();
        return out;
    }

    inline juce::MidiMessage fromHex (const juce::String& hex)
    {
        std::vector<juce::uint8> bytes;
        for (int i = 0; i + 1 < hex.length(); i += 2)
            bytes.push_back ((juce::uint8) hex.substring (i, i + 2).getHexValue32());
        if (bytes.empty())
            return juce::MidiMessage();
        return juce::MidiMessage (bytes.data(), (int) bytes.size());
    }
}
```

Add both files to the `.jucer`, re-save.

- [ ] **Step 4: Run to verify it passes**

Build + run. Expected: all three `ArrangerMidiHex` tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/Arranger/ArrangerMidiHex.h tests/unit/test_arranger_midi_hex.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): add MIDI<->hex codec for style serialization"
```

---

## Task 3: File-model structs

**Files:**
- Create: `Source/Arranger/ArrangerStyleFile.h`
- (No standalone test — exercised by Task 4 IO round-trip.)

- [ ] **Step 1: Implement the structs**

```cpp
// Source/Arranger/ArrangerStyleFile.h
#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"   // ArrangerPartType, ArrangerSectionType, ArrangerAfterComplete, TimedBeatEvent
#include <vector>

/** One source track in a saved style: events stored ONCE, beats measured from bar 1. */
struct SourceTrackFile
{
    juce::String id, name;
    ArrangerPartType partType = ArrangerPartType::Acc;
    int    channel    = 2;
    int    instrument = -1;
    double volume     = 100.0;
    std::vector<TimedBeatEvent> events;   // note-on/off only
    // effects slot intentionally omitted in Part A (reserved for Phase 4+)
};

/** One section as a bar-window into the source tracks. */
struct SectionWindow
{
    juce::String id, name;
    ArrangerSectionType   type          = ArrangerSectionType::Variation;
    int                   startBar       = 1;   // 1-based
    int                   lengthBars     = 1;
    ArrangerAfterComplete afterComplete  = ArrangerAfterComplete::Loop;
};

/** A whole self-contained arranger style as stored on disk. */
struct ArrangerStyleFile
{
    int schemaVersion = 3;
    juce::String id, name;
    double originalTempo = 120.0;
    int timeSigNum = 4, timeSigDenom = 4;
    std::vector<SourceTrackFile> sourceTracks;
    std::vector<SectionWindow>   sections;
};
```

Add to the `.jucer`, re-save.

- [ ] **Step 2: Commit**

```bash
git add Source/Arranger/ArrangerStyleFile.h "Project Synth2.jucer"
git commit -m "feat(arranger): add on-disk style file model structs"
```

---

## Task 4: Style JSON save/load (round-trip)

**Files:**
- Create: `Source/Arranger/ArrangerStyleIOHelper.h`, `Source/Arranger/ArrangerStyleIOHelper.cpp`
- Test: `tests/unit/test_arranger_style_io.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/test_arranger_style_io.cpp
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerStyleIOHelper.h"

static ArrangerStyleFile makeSample()
{
    ArrangerStyleFile s;
    s.name = "Sunset Groove";
    s.originalTempo = 120.0;
    s.timeSigNum = 4; s.timeSigDenom = 4;

    SourceTrackFile drums;
    drums.id = "t1"; drums.name = "Drums";
    drums.partType = ArrangerPartType::Drum;
    drums.channel = 10; drums.instrument = -1; drums.volume = 100.0;
    drums.events.push_back ({ 0.0, juce::MidiMessage::noteOn  (10, 36, (juce::uint8) 100) });
    drums.events.push_back ({ 0.5, juce::MidiMessage::noteOff (10, 36) });
    s.sourceTracks.push_back (drums);

    SectionWindow var;
    var.id = "var_1"; var.name = "Variation 1";
    var.type = ArrangerSectionType::Variation;
    var.startBar = 5; var.lengthBars = 8;
    var.afterComplete = ArrangerAfterComplete::Loop;
    s.sections.push_back (var);
    return s;
}

class ArrangerStyleIOTest : public juce::UnitTest
{
public:
    ArrangerStyleIOTest() : juce::UnitTest ("ArrangerStyleIO", "Arranger") {}

    void runTest() override
    {
        beginTest ("save then load reproduces the style");
        {
            auto file = juce::File::createTempFile (".style");
            auto original = makeSample();
            ArrangerStyleIOHelper::saveToFile (file, original);

            ArrangerStyleFile loaded; juce::String err;
            const bool ok = ArrangerStyleIOHelper::loadFromFile (file, loaded, err);
            expect (ok, err);

            expectEquals (loaded.schemaVersion, 3);
            expectEquals (loaded.name, juce::String ("Sunset Groove"));
            expectEquals (loaded.timeSigNum, 4);
            expectEquals ((int) loaded.sourceTracks.size(), 1);
            expectEquals ((int) loaded.sections.size(), 1);

            auto& tr = loaded.sourceTracks[0];
            expectEquals (tr.channel, 10);
            expect (tr.partType == ArrangerPartType::Drum);
            expectEquals ((int) tr.events.size(), 2);
            expect (tr.events[0].message.isNoteOn());
            expectEquals (tr.events[0].message.getNoteNumber(), 36);
            expectWithinAbsoluteError (tr.events[1].beats, 0.5, 1e-9);

            auto& sec = loaded.sections[0];
            expect (sec.type == ArrangerSectionType::Variation);
            expectEquals (sec.startBar, 5);
            expectEquals (sec.lengthBars, 8);
            expect (sec.afterComplete == ArrangerAfterComplete::Loop);

            file.deleteFile();
        }

        beginTest ("loading a missing file fails gracefully");
        {
            ArrangerStyleFile loaded; juce::String err;
            const bool ok = ArrangerStyleIOHelper::loadFromFile (
                juce::File ("does-not-exist.style"), loaded, err);
            expect (! ok);
            expect (err.isNotEmpty());
        }
    }
};
static ArrangerStyleIOTest arrangerStyleIOTest;
```

- [ ] **Step 2: Run to verify it fails**

Build + run. Expected: compile error (`ArrangerStyleIOHelper.h` not found).

- [ ] **Step 3: Implement the header**

```cpp
// Source/Arranger/ArrangerStyleIOHelper.h
#pragma once
#include <JuceHeader.h>
#include "ArrangerStyleFile.h"

class ArrangerStyleIOHelper
{
public:
    static void saveToFile (const juce::File& file, const ArrangerStyleFile& style);
    static bool loadFromFile (const juce::File& file, ArrangerStyleFile& out, juce::String& error);
};
```

- [ ] **Step 4: Implement the .cpp**

```cpp
// Source/Arranger/ArrangerStyleIOHelper.cpp
#include "ArrangerStyleIOHelper.h"
#include "ArrangerEnums.h"
#include "ArrangerMidiHex.h"

using juce::var;

static var trackToVar (const SourceTrackFile& t)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("id", t.id);
    o->setProperty ("name", t.name);
    o->setProperty ("partType", ArrangerEnums::toString (t.partType));
    o->setProperty ("channel", t.channel);
    o->setProperty ("instrument", t.instrument);
    o->setProperty ("volume", t.volume);
    o->setProperty ("effects", var()); // reserved: null

    juce::Array<var> events;
    for (const auto& e : t.events)
    {
        auto* eo = new juce::DynamicObject();
        eo->setProperty ("b", e.beats);
        eo->setProperty ("m", ArrangerMidiHex::toHex (e.message));
        events.add (var (eo));
    }
    o->setProperty ("events", events);
    return var (o);
}

static var sectionToVar (const SectionWindow& s)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("id", s.id);
    o->setProperty ("name", s.name);
    o->setProperty ("type", ArrangerEnums::toString (s.type));
    o->setProperty ("startBar", s.startBar);
    o->setProperty ("lengthBars", s.lengthBars);
    o->setProperty ("afterComplete", ArrangerEnums::toString (s.afterComplete));
    return var (o);
}

void ArrangerStyleIOHelper::saveToFile (const juce::File& file, const ArrangerStyleFile& style)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("schemaVersion", style.schemaVersion);
    root->setProperty ("kind", "arrangerStyle");
    root->setProperty ("id", style.id);
    root->setProperty ("name", style.name);
    root->setProperty ("originalTempo", style.originalTempo);
    root->setProperty ("timeSigNum", style.timeSigNum);
    root->setProperty ("timeSigDenom", style.timeSigDenom);

    juce::Array<var> tracks;
    for (const auto& t : style.sourceTracks) tracks.add (trackToVar (t));
    root->setProperty ("sourceTracks", tracks);

    juce::Array<var> sections;
    for (const auto& s : style.sections) sections.add (sectionToVar (s));
    root->setProperty ("sections", sections);

    file.replaceWithText (juce::JSON::toString (var (root), true));
}

bool ArrangerStyleIOHelper::loadFromFile (const juce::File& file, ArrangerStyleFile& out, juce::String& error)
{
    if (! file.existsAsFile()) { error = "File does not exist: " + file.getFullPathName(); return false; }

    var root;
    auto result = juce::JSON::parse (file.loadFileAsString(), root);
    if (result.failed() || ! root.isObject()) { error = "Invalid JSON: " + result.getErrorMessage(); return false; }

    out = ArrangerStyleFile();
    out.schemaVersion = (int)    root.getProperty ("schemaVersion", 3);
    out.id            =          root.getProperty ("id", "").toString();
    out.name          =          root.getProperty ("name", "").toString();
    out.originalTempo = (double) root.getProperty ("originalTempo", 120.0);
    out.timeSigNum    = (int)    root.getProperty ("timeSigNum", 4);
    out.timeSigDenom  = (int)    root.getProperty ("timeSigDenom", 4);

    if (auto* tracks = root.getProperty ("sourceTracks", var()).getArray())
        for (const auto& tv : *tracks)
        {
            SourceTrackFile t;
            t.id         =          tv.getProperty ("id", "").toString();
            t.name       =          tv.getProperty ("name", "").toString();
            t.partType   = ArrangerEnums::partFromString (tv.getProperty ("partType", "Acc").toString());
            t.channel    = (int)    tv.getProperty ("channel", 2);
            t.instrument = (int)    tv.getProperty ("instrument", -1);
            t.volume     = (double) tv.getProperty ("volume", 100.0);
            if (auto* evs = tv.getProperty ("events", var()).getArray())
                for (const auto& ev : *evs)
                    t.events.push_back ({ (double) ev.getProperty ("b", 0.0),
                                          ArrangerMidiHex::fromHex (ev.getProperty ("m", "").toString()) });
            out.sourceTracks.push_back (std::move (t));
        }

    if (auto* sections = root.getProperty ("sections", var()).getArray())
        for (const auto& sv : *sections)
        {
            SectionWindow s;
            s.id            =          sv.getProperty ("id", "").toString();
            s.name          =          sv.getProperty ("name", "").toString();
            s.type          = ArrangerEnums::sectionTypeFromString (sv.getProperty ("type", "Variation").toString());
            s.startBar      = (int)    sv.getProperty ("startBar", 1);
            s.lengthBars    = (int)    sv.getProperty ("lengthBars", 1);
            s.afterComplete = ArrangerEnums::afterFromString (sv.getProperty ("afterComplete", "Loop").toString());
            out.sections.push_back (std::move (s));
        }

    return true;
}
```

Add `ArrangerStyleIOHelper.h/.cpp` and `test_arranger_style_io.cpp` to the `.jucer`, re-save.

- [ ] **Step 5: Run to verify it passes**

Build + run. Expected: both `ArrangerStyleIO` tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerStyleIOHelper.h Source/Arranger/ArrangerStyleIOHelper.cpp tests/unit/test_arranger_style_io.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): JSON save/load for self-contained style files"
```

---


> Continued in [part 2 of 2](2026-06-06-arranger-phase3-partA-format-io-load-part2.md).
