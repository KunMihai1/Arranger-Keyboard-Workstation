> Part 2 of 2 of [2026-06-06-arranger-phase3-partA-format-io-load](2026-06-06-arranger-phase3-partA-format-io-load.md). Previous: [part 1](2026-06-06-arranger-phase3-partA-format-io-load.md).

## Task 5: Channel allocator

**Files:**
- Create: `Source/Arranger/ArrangerChannels.h`, `Source/Arranger/ArrangerChannels.cpp`
- Test: `tests/unit/test_arranger_channels.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/test_arranger_channels.cpp
#include <juce_core/juce_core.h>
#include "Arranger/ArrangerChannels.h"

class ArrangerChannelsTest : public juce::UnitTest
{
public:
    ArrangerChannelsTest() : juce::UnitTest ("ArrangerChannels", "Arranger") {}

    void runTest() override
    {
        beginTest ("melodic channels are 2..9 then 11..13, skipping reserved");
        {
            const int expected[] = { 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13 };
            for (int i = 0; i < 11; ++i)
                expectEquals (ArrangerChannels::melodicChannelForIndex (i), expected[i]);
        }

        beginTest ("never returns a reserved channel");
        {
            for (int i = 0; i < 11; ++i)
            {
                const int ch = ArrangerChannels::melodicChannelForIndex (i);
                expect (ch != 1 && ch != 10 && ch != 14 && ch != 15 && ch != 16);
            }
        }

        beginTest ("overflow past 11 melodic tracks returns -1");
        {
            expectEquals (ArrangerChannels::melodicChannelForIndex (11), -1);
            expectEquals (ArrangerChannels::melodicChannelForIndex (50), -1);
        }
    }
};
static ArrangerChannelsTest arrangerChannelsTest;
```

- [ ] **Step 2: Run to verify it fails**

Build + run. Expected: compile error (`ArrangerChannels.h` not found).

- [ ] **Step 3: Implement**

```cpp
// Source/Arranger/ArrangerChannels.h
#pragma once

namespace ArrangerChannels
{
    constexpr int kDrumChannel = 10;

    /** Melodic accompaniment channel for a 0-based melodic-track index.
        Order: 2..9 then 11..13, skipping hands (1,16), drums (10), record (14,15).
        Returns -1 when more than 11 melodic tracks are requested. */
    int melodicChannelForIndex (int melodicIndex);
}
```

```cpp
// Source/Arranger/ArrangerChannels.cpp
#include "ArrangerChannels.h"

namespace ArrangerChannels
{
    int melodicChannelForIndex (int melodicIndex)
    {
        static const int channels[] = { 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13 };
        constexpr int count = (int) (sizeof (channels) / sizeof (channels[0]));
        if (melodicIndex < 0 || melodicIndex >= count)
            return -1;
        return channels[melodicIndex];
    }
}
```

Add both files + test to the `.jucer`, re-save.

- [ ] **Step 4: Run to verify it passes**

Build + run. Expected: all three `ArrangerChannels` tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/Arranger/ArrangerChannels.h Source/Arranger/ArrangerChannels.cpp tests/unit/test_arranger_channels.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): channel allocator honouring the locked channel map"
```

---

## Task 6: Extract `sliceSection` and add `buildStyleFromWindows` + `buildStyleFromFile`

**Files:**
- Modify: `Source/Arranger/ArrangerPatternBuilder.h`, `Source/Arranger/ArrangerPatternBuilder.cpp`
- Test: `tests/unit/test_arranger_pattern_builder.cpp` (append)

- [ ] **Step 1: Write the failing tests (append to the existing file, before the static registration line)**

```cpp
        beginTest ("buildStyleFromWindows slices real windows and rebases to beat 0");
        {
            // One melodic source track, 4 bars long (4/4 @120 -> 1 bar = 2 beats? no: 1 bar = 4 beats).
            // Put a note at beat 0 (bar 1) and a note at beat 8 (bar 3).
            SourceTrackFile t;
            t.id = "t1"; t.name = "Keys"; t.partType = ArrangerPartType::Acc;
            t.channel = 2; t.instrument = 0; t.volume = 100.0;
            t.events.push_back ({ 0.0, juce::MidiMessage::noteOn  (2, 60, (juce::uint8) 100) });
            t.events.push_back ({ 0.5, juce::MidiMessage::noteOff (2, 60) });
            t.events.push_back ({ 8.0, juce::MidiMessage::noteOn  (2, 67, (juce::uint8) 100) }); // bar 3
            t.events.push_back ({ 8.5, juce::MidiMessage::noteOff (2, 67) });

            std::vector<SourceTrackFile> tracks { t };

            SectionWindow intro;
            intro.id = "intro_1"; intro.name = "Intro 1"; intro.type = ArrangerSectionType::Intro;
            intro.startBar = 1; intro.lengthBars = 1; intro.afterComplete = ArrangerAfterComplete::FallThrough;

            SectionWindow var;
            var.id = "var_1"; var.name = "Variation 1"; var.type = ArrangerSectionType::Variation;
            var.startBar = 3; var.lengthBars = 1; var.afterComplete = ArrangerAfterComplete::Loop;

            std::vector<SectionWindow> windows { intro, var };

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromWindows (tracks, windows, 4, 4, 120.0);

            expectEquals ((int) style.sections.size(), 2);
            expect (style.sections[0].type == ArrangerSectionType::Intro);
            expect (style.sections[1].type == ArrangerSectionType::Variation);

            // Intro = bar 1: the beat-0 note stays at beat 0.
            expectEquals ((int) style.sections[0].tracks.size(), 1);
            expect (! style.sections[0].tracks[0].pattern.empty());
            expectWithinAbsoluteError (style.sections[0].tracks[0].pattern.front().beats, 0.0, 1e-9);
            expectEquals (style.sections[0].lengthBars, 1);

            // Variation = bar 3 (starts at beat 8): the beat-8 note rebases to beat 0.
            expect (! style.sections[1].tracks[0].pattern.empty());
            expectWithinAbsoluteError (style.sections[1].tracks[0].pattern.front().beats, 0.0, 1e-9);
            expectEquals (style.sections[1].tracks[0].pattern.front().message.getNoteNumber(), 67);
        }

        beginTest ("buildStyleFromWindows preserves per-track channel and afterComplete");
        {
            SourceTrackFile drums;
            drums.id = "d"; drums.partType = ArrangerPartType::Drum; drums.channel = 10;
            drums.events.push_back ({ 0.0, juce::MidiMessage::noteOn  (10, 36, (juce::uint8) 100) });
            drums.events.push_back ({ 0.5, juce::MidiMessage::noteOff (10, 36) });

            SectionWindow ending;
            ending.id = "end"; ending.type = ArrangerSectionType::Ending;
            ending.startBar = 1; ending.lengthBars = 1; ending.afterComplete = ArrangerAfterComplete::Stop;

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromWindows ({ drums }, { ending }, 4, 4, 120.0);
            expectEquals (style.sections[0].tracks[0].channel, 10);
            expect (style.sections[0].afterComplete == ArrangerAfterComplete::Stop);
        }

        beginTest ("buildStyleFromFile copies metadata and builds sections");
        {
            ArrangerStyleFile f;
            f.name = "X"; f.originalTempo = 100.0; f.timeSigNum = 4; f.timeSigDenom = 4;
            SourceTrackFile t; t.channel = 2;
            t.events.push_back ({ 0.0, juce::MidiMessage::noteOn  (2, 60, (juce::uint8) 100) });
            t.events.push_back ({ 0.5, juce::MidiMessage::noteOff (2, 60) });
            f.sourceTracks.push_back (t);
            SectionWindow w; w.type = ArrangerSectionType::Variation; w.startBar = 1; w.lengthBars = 1;
            f.sections.push_back (w);

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromFile (f);
            expectWithinAbsoluteError (style.originalTempo, 100.0, 1e-9);
            expectEquals ((int) style.sections.size(), 1);
        }
```

Add the include at the top of the test file:

```cpp
#include "Arranger/ArrangerStyleFile.h"
```

- [ ] **Step 2: Run to verify it fails**

Build + run. Expected: compile error (`buildStyleFromWindows` / `buildStyleFromFile` not declared).

- [ ] **Step 3: Declare the new functions in the header**

In `Source/Arranger/ArrangerPatternBuilder.h`, add the include and three declarations inside the `ArrangerPatternBuilder` namespace:

```cpp
#include "ArrangerStyleFile.h"
```

```cpp
    /** Slice whole bars out of a section and rebase events to beat 0. Public so both the
        demo builder and the window builder share one implementation. */
    ArrangerSection sliceSection (const ArrangerSection& src, double startBeats, int numBars, double beatsPerBar);

    /** Build a runtime ArrangerStyle from source tracks + explicit bar-window sections. */
    ArrangerStyle buildStyleFromWindows (const std::vector<SourceTrackFile>& sourceTracks,
                                         const std::vector<SectionWindow>& windows,
                                         int timeSigNum, int timeSigDenom,
                                         double referenceBpm);

    /** Convenience: build a runtime ArrangerStyle straight from a loaded style file. */
    ArrangerStyle buildStyleFromFile (const ArrangerStyleFile& file);
```

- [ ] **Step 4: Implement in the .cpp**

In `Source/Arranger/ArrangerPatternBuilder.cpp`, add `#include "ArrangerStyleFile.h"` at the top. Replace the local `sliceBars` lambda inside `buildDemoMultiSectionStyle` with a call to the new shared `sliceSection`, and add the new functions:

```cpp
ArrangerSection ArrangerPatternBuilder::sliceSection (const ArrangerSection& src,
                                                      double startBeats, int numBars, double beatsPerBar)
{
    ArrangerSection sec = src;
    const int    bars     = juce::jmax (1, numBars);
    const double endBeats = startBeats + (double) bars * beatsPerBar;
    for (auto& tr : sec.tracks)
    {
        std::vector<TimedBeatEvent> sliced;
        for (const auto& ev : tr.pattern)
            if (ev.beats >= startBeats - 1e-9 && ev.beats < endBeats - 1e-9)
                sliced.push_back ({ ev.beats - startBeats, ev.message });
        tr.pattern = std::move (sliced);
    }
    sec.lengthBars = bars;
    return sec;
}

ArrangerStyle ArrangerPatternBuilder::buildStyleFromWindows (const std::vector<SourceTrackFile>& sourceTracks,
                                                             const std::vector<SectionWindow>& windows,
                                                             int timeSigNum, int timeSigDenom,
                                                             double referenceBpm)
{
    if (referenceBpm <= 0.0) referenceBpm = 120.0;

    ArrangerStyle style;
    style.timeSigNum    = timeSigNum;
    style.timeSigDenom  = timeSigDenom;
    style.originalTempo = referenceBpm;

    const double bpb = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);

    // Master section = every source track as one full-length ArrangerTrack.
    ArrangerSection master;
    for (const auto& st : sourceTracks)
    {
        ArrangerTrack at;
        at.id = st.id; at.name = st.name; at.partType = st.partType;
        at.instrument = st.instrument; at.channel = st.channel; at.volume = st.volume;
        at.pattern = st.events;
        master.tracks.push_back (std::move (at));
    }

    for (const auto& w : windows)
    {
        const double startBeats = (double) (juce::jmax (1, w.startBar) - 1) * bpb;
        ArrangerSection sec = sliceSection (master, startBeats, juce::jmax (1, w.lengthBars), bpb);
        sec.id            = w.id;
        sec.name          = w.name;
        sec.type          = w.type;
        sec.afterComplete = w.afterComplete;
        style.sections.push_back (std::move (sec));
    }

    return style;
}

ArrangerStyle ArrangerPatternBuilder::buildStyleFromFile (const ArrangerStyleFile& file)
{
    ArrangerStyle style = buildStyleFromWindows (file.sourceTracks, file.sections,
                                                 file.timeSigNum, file.timeSigDenom, file.originalTempo);
    style.id   = file.id;
    style.name = file.name;
    return style;
}
```

> When replacing the `sliceBars` lambda in `buildDemoMultiSectionStyle`, change each `sliceBars (var, X, N)` call to `sliceSection (var, X, N, bpb)` and delete the lambda definition. The demo tests from the existing file must still pass unchanged.

- [ ] **Step 5: Run to verify it passes**

Build + run. Expected: the three new `buildStyleFromWindows`/`buildStyleFromFile` tests pass **and** all pre-existing `ArrangerPatternBuilder` tests (including the demo ones) still pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerPatternBuilder.h Source/Arranger/ArrangerPatternBuilder.cpp tests/unit/test_arranger_pattern_builder.cpp
git commit -m "feat(arranger): build runtime style from source tracks + bar windows"
```

---

## Task 7: End-to-end load+play smoke test (headless)

Proves a hand-authored `.style` file loads and produces engine-ready events, with no UI.

**Files:**
- Test: `tests/unit/test_arranger_style_io.cpp` (append a new `beginTest`)

- [ ] **Step 1: Append the failing test**

```cpp
        beginTest ("file -> buildStyleFromFile -> engine renders the first section");
        {
            // Build a 1-bar variation with a single note, save, reload, build, render one bar.
            ArrangerStyleFile f;
            f.name = "Smoke"; f.originalTempo = 120.0; f.timeSigNum = 4; f.timeSigDenom = 4;
            SourceTrackFile t; t.id = "t"; t.partType = ArrangerPartType::Acc; t.channel = 2;
            t.events.push_back ({ 0.0, juce::MidiMessage::noteOn  (2, 60, (juce::uint8) 100) });
            t.events.push_back ({ 1.0, juce::MidiMessage::noteOff (2, 60) });
            f.sourceTracks.push_back (t);
            SectionWindow w; w.id = "v"; w.type = ArrangerSectionType::Variation;
            w.startBar = 1; w.lengthBars = 1; w.afterComplete = ArrangerAfterComplete::Loop;
            f.sections.push_back (w);

            auto file = juce::File::createTempFile (".style");
            ArrangerStyleIOHelper::saveToFile (file, f);
            ArrangerStyleFile loaded; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (file, loaded, err), err);

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromFile (loaded);

            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            int noteOns = 0;
            engine.onMidiMessage = [&] (const juce::MidiMessage& m) { if (m.isNoteOn()) ++noteOns; };
            engine.setStyle (style);
            engine.renderRange (0.0, 4.0); // one 4/4 bar

            expect (noteOns >= 1);
            file.deleteFile();
        }
```

Add includes at the top of the test file:

```cpp
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerEngine.h"
```

- [ ] **Step 2: Run to verify it fails (then passes)**

Build + run. If Tasks 4 and 6 are in place this should pass immediately; if it fails, the failure pinpoints the integration gap. Expected final state: PASS (`noteOns >= 1`).

- [ ] **Step 3: Commit**

```bash
git add tests/unit/test_arranger_style_io.cpp
git commit -m "test(arranger): headless load->build->render smoke test for style files"
```

---

## Self-review notes (already applied)

- **Spec coverage:** §5 schema → Tasks 1–4; §3 channel map → Task 5; §6.2 builder/§7 load → Tasks 6–7. UI (§6.3–6.5) is **Part B** (separate plan). Effects slot serialized as `null` (Task 4) per D6.
- **Type consistency:** `buildStyleFromWindows`, `buildStyleFromFile`, `sliceSection`, `ArrangerChannels::melodicChannelForIndex`, `ArrangerMidiHex::toHex/fromHex`, `ArrangerEnums::*` are named identically across declaration, definition, and tests.
- **No placeholders:** every step shows real code or an exact build/run/commit action.

## What Part A delivers

A headless, tested pipeline: a `.style` JSON ⇄ `ArrangerStyleFile` ⇄ runtime `ArrangerStyle` that the existing engine plays — with the locked channel map and full round-trip coverage. You can author a style by hand-writing JSON and hear it. **Part B** adds the visual timeline, the `ArrangerStyleEditor`, the style list, and mode wiring on top of this core.
