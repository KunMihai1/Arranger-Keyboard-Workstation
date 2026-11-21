# Arranger Phase 3 — Part B: Visual Timeline UI, Editor & Mode Wiring — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the user author a self-contained arranger style by recording/loading a backing performance, marking section bar-ranges on a visual draggable timeline, previewing it live, and saving a `.style` file — then pick and play those styles in arranger mode.

**Architecture:** All non-trivial logic lives in **pure, unit-tested helpers** (`ArrangerTimelineGeometry` for bar↔pixel/snap/hit-test, `ArrangerSourceBuilder` for `TrackEntry`→`SourceTrackFile`, `ArrangerDefaults` for default windows). The JUCE components (`ArrangerTimelineComponent`, `ArrangerStyleEditor`) stay thin: paint + mouse + wiring that call those helpers. Builds entirely on Part A's format/builder/IO.

**Tech Stack:** C++17, JUCE (`juce_gui_basics`, `juce_audio_basics`, `juce_core`), JUCE `UnitTest` framework (category "Arranger").

**Prerequisite:** Part A merged (`ArrangerStyleFile`, `ArrangerStyleIOHelper`, `ArrangerChannels`, `buildStyleFromWindows`/`buildStyleFromFile`).

**Spec:** `docs/superpowers/specs/2026-06-06-arranger-phase3-design.md`

---

## Test / build workflow

Same as Part A: enable the `UnitTestRunner` in `Source/Main.cpp` (`runTestsInCategory ("Arranger")`), build the VS2022 target, run, read `test-results.txt`. UI tasks (4–7) are **manually verified by running the app** — each lists exact observable steps. Add every new `Source/...` file to `Project Synth2.jucer` and re-save.

---

## File Structure

**Create:**
- `Source/Arranger/ArrangerTimelineGeometry.h` / `.cpp` — pure bar↔pixel, snap, region/edge hit-test.
- `Source/Arranger/ArrangerSourceBuilder.h` / `.cpp` — `TrackEntry[]` → `SourceTrackFile[]` with channel allocation + beat conversion.
- `Source/Arranger/ArrangerDefaults.h` / `.cpp` — default section windows for a recording length.
- `Source/Arranger/ArrangerTimelineComponent.h` / `.cpp` — the visual draggable timeline.
- `Source/Arranger/ArrangerStyleEditor.h` / `.cpp` — authoring controller (recording → preview → save).
- `Source/Arranger/ArrangerStyleListComponent.h` / `.cpp` — lists `.style` files, fires load/edit.
- `tests/unit/test_arranger_timeline_geometry.cpp`
- `tests/unit/test_arranger_source_builder.cpp`
- `tests/unit/test_arranger_defaults.cpp`

**Modify:**
- `Source/UI/MainComponent.h` / `.cpp` — own the editor + style list, wire the "create/edit style" and "pick style" entry points into arranger mode.
- `Source/Common/IOHelper.cpp` (optional) — reuse `IOHelper::getFolder("ArrangerStyles")` for the styles directory (no code change needed if `getFolder` already creates on demand; confirm in Task 6).

---

## Task 1: Timeline geometry (pure, fully tested)

**Files:**
- Create: `Source/Arranger/ArrangerTimelineGeometry.h`, `.cpp`
- Test: `tests/unit/test_arranger_timeline_geometry.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/test_arranger_timeline_geometry.cpp
#include <juce_gui_basics/juce_gui_basics.h>
#include "Arranger/ArrangerTimelineGeometry.h"

class ArrangerTimelineGeometryTest : public juce::UnitTest
{
public:
    ArrangerTimelineGeometryTest() : juce::UnitTest ("ArrangerTimelineGeometry", "Arranger") {}

    void runTest() override
    {
        using namespace ArrangerTimelineGeometry;
        // 8 bars across a 820px-wide view with 10px left/right margins -> 800 usable -> 100px/bar.
        Layout L { 8, 820, 10 };

        beginTest ("bar 1 maps to the left margin, bar 9 (end) to the right edge");
        {
            expectWithinAbsoluteError (barToX (1.0, L), 10.0, 1e-6);
            expectWithinAbsoluteError (barToX (9.0, L), 810.0, 1e-6); // 8 bars later
            expectWithinAbsoluteError (barToX (5.0, L), 410.0, 1e-6); // 4 bars in
        }

        beginTest ("x snaps to the nearest bar line, clamped to [1, totalBars+1]");
        {
            expectEquals (xToSnappedBar (10.0,  L), 1);
            expectEquals (xToSnappedBar (59.0,  L), 1);  // <50px from bar1 line
            expectEquals (xToSnappedBar (61.0,  L), 2);  // >50px -> bar2
            expectEquals (xToSnappedBar (-100.0, L), 1); // clamp low
            expectEquals (xToSnappedBar (9999.0, L), 9); // clamp high (totalBars+1)
        }

        beginTest ("region bounds span its bars");
        {
            auto r = regionBounds (5, 2, L, 40); // startBar 5, len 2, height 40
            expectEquals (r.getX(), 410);
            expectEquals (r.getWidth(), 200); // 2 bars * 100px
            expectEquals (r.getHeight(), 40);
        }

        beginTest ("hit-test detects left edge, right edge, body, and miss");
        {
            auto r = regionBounds (5, 2, L, 40); // x 410..610
            expect (hitTest (412, r) == Hit::LeftEdge);
            expect (hitTest (608, r) == Hit::RightEdge);
            expect (hitTest (500, r) == Hit::Body);
            expect (hitTest (700, r) == Hit::None);
        }
    }
};
static ArrangerTimelineGeometryTest arrangerTimelineGeometryTest;
```

- [ ] **Step 2: Run to verify it fails**

Build + run with the runner on. Expected: compile error (`ArrangerTimelineGeometry.h` not found).

- [ ] **Step 3: Implement the header**

```cpp
// Source/Arranger/ArrangerTimelineGeometry.h
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ArrangerTimelineGeometry
{
    struct Layout
    {
        int totalBars = 1;
        int widthPx   = 0;
        int marginPx  = 0;   // equal left/right margin
    };

    enum class Hit { None, Body, LeftEdge, RightEdge };

    constexpr int kEdgeGrabPx = 6;

    double pixelsPerBar (const Layout& L);
    double barToX (double bar, const Layout& L);            // bar is 1-based, may be fractional
    int    xToSnappedBar (double x, const Layout& L);       // nearest bar line, clamp [1, totalBars+1]
    juce::Rectangle<int> regionBounds (int startBar, int lengthBars, const Layout& L, int heightPx);
    Hit    hitTest (int x, const juce::Rectangle<int>& region);
}
```

- [ ] **Step 4: Implement the .cpp**

```cpp
// Source/Arranger/ArrangerTimelineGeometry.cpp
#include "ArrangerTimelineGeometry.h"
#include <cmath>

namespace ArrangerTimelineGeometry
{
    double pixelsPerBar (const Layout& L)
    {
        const int usable = juce::jmax (1, L.widthPx - 2 * L.marginPx);
        return (double) usable / (double) juce::jmax (1, L.totalBars);
    }

    double barToX (double bar, const Layout& L)
    {
        return (double) L.marginPx + (bar - 1.0) * pixelsPerBar (L);
    }

    int xToSnappedBar (double x, const Layout& L)
    {
        const double bar = (x - (double) L.marginPx) / pixelsPerBar (L) + 1.0;
        const int snapped = (int) std::floor (bar + 0.5);
        return juce::jlimit (1, L.totalBars + 1, snapped);
    }

    juce::Rectangle<int> regionBounds (int startBar, int lengthBars, const Layout& L, int heightPx)
    {
        const int x  = (int) std::lround (barToX ((double) startBar, L));
        const int x2 = (int) std::lround (barToX ((double) (startBar + juce::jmax (1, lengthBars)), L));
        return juce::Rectangle<int> (x, 0, juce::jmax (1, x2 - x), heightPx);
    }

    Hit hitTest (int x, const juce::Rectangle<int>& region)
    {
        if (x < region.getX() - kEdgeGrabPx || x > region.getRight() + kEdgeGrabPx)
            return Hit::None;
        if (std::abs (x - region.getX())     <= kEdgeGrabPx) return Hit::LeftEdge;
        if (std::abs (x - region.getRight())  <= kEdgeGrabPx) return Hit::RightEdge;
        if (x >= region.getX() && x <= region.getRight())     return Hit::Body;
        return Hit::None;
    }
}
```

Add header, cpp, and test to the `.jucer`, re-save.

- [ ] **Step 5: Run to verify it passes**

Build + run. Expected: all four `ArrangerTimelineGeometry` tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerTimelineGeometry.h Source/Arranger/ArrangerTimelineGeometry.cpp tests/unit/test_arranger_timeline_geometry.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): pure timeline geometry (bar<->pixel, snap, hit-test)"
```

---

## Task 2: Recording → source tracks (pure, tested)

Converts the player's `TrackEntry` list into `SourceTrackFile`s, applying the channel allocator and beat conversion. Mirrors `buildSingleSectionStyle` but emits the **file model** and uses `ArrangerChannels`.

**Files:**
- Create: `Source/Arranger/ArrangerSourceBuilder.h`, `.cpp`
- Test: `tests/unit/test_arranger_source_builder.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/test_arranger_source_builder.cpp
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerSourceBuilder.h"

class ArrangerSourceBuilderTest : public juce::UnitTest
{
public:
    ArrangerSourceBuilderTest() : juce::UnitTest ("ArrangerSourceBuilder", "Arranger") {}

    void runTest() override
    {
        beginTest ("melodic tracks get 2,3,...; percussion gets 10; events convert to beats");
        {
            TrackEntry m1; m1.type = TrackType::Melodic; m1.displayName = "Keys";
            m1.instrumentAssociated = 4; m1.volumeAssociated = 90.0;
            m1.sequence.addEvent (juce::MidiMessage::noteOn  (1, 60, (juce::uint8) 100), 0.0);
            m1.sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 1.0); // 1s -> 2 beats @120

            TrackEntry m2; m2.type = TrackType::Melodic; m2.displayName = "Bass";
            m2.sequence.addEvent (juce::MidiMessage::noteOn  (1, 40, (juce::uint8) 100), 0.0);
            m2.sequence.addEvent (juce::MidiMessage::noteOff (1, 40), 1.0);

            TrackEntry d; d.type = TrackType::Percussion; d.displayName = "Drums";
            d.sequence.addEvent (juce::MidiMessage::noteOn  (1, 36, (juce::uint8) 100), 0.0);
            d.sequence.addEvent (juce::MidiMessage::noteOff (1, 36), 0.5);

            std::vector<TrackEntry> tracks { m1, d, m2 };
            auto src = ArrangerSourceBuilder::fromTrackEntries (tracks, 120.0);

            expectEquals ((int) src.size(), 3);
            // order preserved; channels allocated by type
            expectEquals (src[0].channel, 2);   // first melodic
            expect (src[0].partType != ArrangerPartType::Drum);
            expectEquals (src[1].channel, 10);  // percussion
            expect (src[1].partType == ArrangerPartType::Drum);
            expectEquals (src[2].channel, 3);   // second melodic

            // events converted + channel overridden
            expectEquals ((int) src[0].events.size(), 2);
            expectWithinAbsoluteError (src[0].events[1].beats, 2.0, 1e-9);
            expectEquals (src[0].events[0].message.getChannel(), 2);
            expectEquals (src[0].instrument, 4);
            expectWithinAbsoluteError (src[0].volume, 90.0, 1e-9);
        }

        beginTest ("more than 11 melodic tracks: overflow tracks are dropped, not crashed");
        {
            std::vector<TrackEntry> tracks;
            for (int i = 0; i < 13; ++i)
            {
                TrackEntry t; t.type = TrackType::Melodic; t.displayName = "M" + juce::String (i);
                t.sequence.addEvent (juce::MidiMessage::noteOn  (1, 60, (juce::uint8) 100), 0.0);
                t.sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 0.5);
                tracks.push_back (t);
            }
            auto src = ArrangerSourceBuilder::fromTrackEntries (tracks, 120.0);
            expectEquals ((int) src.size(), 11); // 11 melodic channels available
        }
    }
};
static ArrangerSourceBuilderTest arrangerSourceBuilderTest;
```

- [ ] **Step 2: Run to verify it fails**

Build + run. Expected: compile error (`ArrangerSourceBuilder.h` not found).

- [ ] **Step 3: Implement the header**

```cpp
// Source/Arranger/ArrangerSourceBuilder.h
#pragma once
#include <JuceHeader.h>
#include "ArrangerStyleFile.h"
#include "TrackEntry.h"
#include <vector>

namespace ArrangerSourceBuilder
{
    /** Convert recorded tracks into source tracks for a style: percussion -> channel 10,
        melodic -> 2..9 then 11..13 (ArrangerChannels). Tracks past the melodic budget are
        dropped. `referenceBpm` must be the tempo `TrackEntry::sequence` is scaled to. */
    std::vector<SourceTrackFile> fromTrackEntries (const std::vector<TrackEntry>& tracks,
                                                   double referenceBpm);
}
```

- [ ] **Step 4: Implement the .cpp**

```cpp
// Source/Arranger/ArrangerSourceBuilder.cpp
#include "ArrangerSourceBuilder.h"
#include "ArrangerChannels.h"
#include "ArrangerPatternBuilder.h"

namespace ArrangerSourceBuilder
{
    std::vector<SourceTrackFile> fromTrackEntries (const std::vector<TrackEntry>& tracks,
                                                   double referenceBpm)
    {
        if (referenceBpm <= 0.0) referenceBpm = 120.0;

        std::vector<SourceTrackFile> out;
        int melodicIndex = 0;

        for (const auto& te : tracks)
        {
            const bool isPerc = (te.type == TrackType::Percussion);
            int channel;
            if (isPerc)
            {
                channel = ArrangerChannels::kDrumChannel;
            }
            else
            {
                channel = ArrangerChannels::melodicChannelForIndex (melodicIndex++);
                if (channel < 0)
                    continue; // melodic budget exhausted; drop this track
            }

            SourceTrackFile st;
            st.id        = te.getUniqueID();
            st.name      = te.getDisplayName();
            st.partType  = isPerc ? ArrangerPartType::Drum : ArrangerPartType::Acc;
            st.channel   = channel;
            st.instrument = te.instrumentAssociated;
            st.volume    = te.volumeAssociated;
            st.events    = ArrangerPatternBuilder::buildBeatEvents (te.sequence, referenceBpm, channel);
            out.push_back (std::move (st));
        }

        return out;
    }
}
```

Add header, cpp, test to the `.jucer`, re-save.

- [ ] **Step 5: Run to verify it passes**

Build + run. Expected: both `ArrangerSourceBuilder` tests pass.

- [ ] **Step 6: Commit**

```bash
git add Source/Arranger/ArrangerSourceBuilder.h Source/Arranger/ArrangerSourceBuilder.cpp tests/unit/test_arranger_source_builder.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): convert recorded tracks into style source tracks"
```

---

## Task 3: Default section windows (pure, tested)

**Files:**
- Create: `Source/Arranger/ArrangerDefaults.h`, `.cpp`
- Test: `tests/unit/test_arranger_defaults.cpp`

- [ ] **Step 1: Write the failing test**

```cpp
// tests/unit/test_arranger_defaults.cpp
#include <juce_core/juce_core.h>
#include "Arranger/ArrangerDefaults.h"

class ArrangerDefaultsTest : public juce::UnitTest
{
public:
    ArrangerDefaultsTest() : juce::UnitTest ("ArrangerDefaults", "Arranger") {}

    void runTest() override
    {
        beginTest ("default is a single Variation covering the whole recording");
        {
            auto w = ArrangerDefaults::defaultWindowsForBars (8);
            expectEquals ((int) w.size(), 1);
            expect (w[0].type == ArrangerSectionType::Variation);
            expectEquals (w[0].startBar, 1);
            expectEquals (w[0].lengthBars, 8);
            expect (w[0].afterComplete == ArrangerAfterComplete::Loop);
            expect (w[0].id.isNotEmpty());
        }

        beginTest ("zero or negative bars clamps to a 1-bar Variation");
        {
            auto w = ArrangerDefaults::defaultWindowsForBars (0);
            expectEquals ((int) w.size(), 1);
            expectEquals (w[0].lengthBars, 1);
        }
    }
};
static ArrangerDefaultsTest arrangerDefaultsTest;
```

- [ ] **Step 2: Run to verify it fails**

Build + run. Expected: compile error (`ArrangerDefaults.h` not found).

- [ ] **Step 3: Implement**

```cpp
// Source/Arranger/ArrangerDefaults.h
#pragma once
#include <JuceHeader.h>
#include "ArrangerStyleFile.h"
#include <vector>

namespace ArrangerDefaults
{
    /** Default windows for a freshly captured recording: one Variation across all bars.
        The user adds Intro/Fill/Ending by dragging on the timeline. */
    std::vector<SectionWindow> defaultWindowsForBars (int totalBars);
}
```

```cpp
// Source/Arranger/ArrangerDefaults.cpp
#include "ArrangerDefaults.h"

namespace ArrangerDefaults
{
    std::vector<SectionWindow> defaultWindowsForBars (int totalBars)
    {
        SectionWindow v;
        v.id            = juce::Uuid().toString();
        v.name          = "Variation 1";
        v.type          = ArrangerSectionType::Variation;
        v.startBar      = 1;
        v.lengthBars    = juce::jmax (1, totalBars);
        v.afterComplete = ArrangerAfterComplete::Loop;
        return { v };
    }
}
```

Add header, cpp, test to the `.jucer`, re-save.

- [ ] **Step 4: Run to verify it passes**

Build + run. Expected: both `ArrangerDefaults` tests pass.

- [ ] **Step 5: Commit**

```bash
git add Source/Arranger/ArrangerDefaults.h Source/Arranger/ArrangerDefaults.cpp tests/unit/test_arranger_defaults.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): default section windows for a new recording"
```

---

## Task 4: The visual timeline component

A thin `juce::Component` that renders the bar ruler + colored regions and edits them through drag/resize, delegating all math to `ArrangerTimelineGeometry`. Verified by running the app.

**Files:**
- Create: `Source/Arranger/ArrangerTimelineComponent.h`, `.cpp`

- [ ] **Step 1: Implement the header**

```cpp
// Source/Arranger/ArrangerTimelineComponent.h
#pragma once
#include <JuceHeader.h>
#include "ArrangerStyleFile.h"
#include "ArrangerTimelineGeometry.h"
#include <vector>
#include <functional>

/** Bar ruler with draggable/resizable colored section regions. Holds a copy of the
    windows; emits a callback whenever they change. Knows nothing about files or the engine. */
class ArrangerTimelineComponent : public juce::Component
{
public:
    std::function<void (const std::vector<SectionWindow>&)> onWindowsChanged;
    std::function<void (int sectionIndex)> onSectionSelected;

    void setTotalBars (int bars);
    void setWindows (const std::vector<SectionWindow>& windows);
    const std::vector<SectionWindow>& getWindows() const { return windows; }

    void setPlayheadBar (double bar);   // for the live preview cursor (1-based, fractional)
    int  getSelectedIndex() const { return selectedIndex; }

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

private:
    ArrangerTimelineGeometry::Layout layout() const;
    juce::Colour colourForType (ArrangerSectionType t) const;

    std::vector<SectionWindow> windows;
    int    totalBars   = 1;
    int    marginPx    = 10;
    double playheadBar = 1.0;

    int  selectedIndex = -1;
    ArrangerTimelineGeometry::Hit dragMode = ArrangerTimelineGeometry::Hit::None;
    int  dragStartBarValue = 1;
    int  dragStartLenValue = 1;
};
```

- [ ] **Step 2: Implement the .cpp**

```cpp
// Source/Arranger/ArrangerTimelineComponent.cpp
#include "ArrangerTimelineComponent.h"
#include "ArrangerEnums.h"
#include "AppColours.h"

using namespace ArrangerTimelineGeometry;

Layout ArrangerTimelineComponent::layout() const
{
    return Layout { juce::jmax (1, totalBars), getWidth(), marginPx };
}

juce::Colour ArrangerTimelineComponent::colourForType (ArrangerSectionType t) const
{
    switch (t)
    {
        case ArrangerSectionType::Intro:     return juce::Colour (0xff4fc1ff);
        case ArrangerSectionType::Variation: return juce::Colour (0xff6a9955);
        case ArrangerSectionType::Fill:      return juce::Colour (0xffdcdcaa);
        case ArrangerSectionType::Break:     return juce::Colour (0xffc586c0);
        case ArrangerSectionType::Ending:    return juce::Colour (0xffd16969);
        case ArrangerSectionType::CountIn:   return juce::Colour (0xff808080);
    }
    return juce::Colours::grey;
}

void ArrangerTimelineComponent::setTotalBars (int bars)     { totalBars = juce::jmax (1, bars); repaint(); }
void ArrangerTimelineComponent::setWindows (const std::vector<SectionWindow>& w) { windows = w; repaint(); }
void ArrangerTimelineComponent::setPlayheadBar (double bar) { playheadBar = bar; repaint(); }

void ArrangerTimelineComponent::paint (juce::Graphics& g)
{
    const auto L = layout();
    g.fillAll (juce::Colour (0xff1e1e1e));

    // bar grid + numbers
    g.setColour (juce::Colour (0xff3a3a3a));
    for (int bar = 1; bar <= totalBars + 1; ++bar)
    {
        const int x = (int) std::lround (barToX ((double) bar, L));
        g.drawVerticalLine (x, 0.0f, (float) getHeight());
        if (bar <= totalBars)
        {
            g.setColour (juce::Colour (0xff777777));
            g.drawText (juce::String (bar), x + 3, 0, 30, 14, juce::Justification::left);
            g.setColour (juce::Colour (0xff3a3a3a));
        }
    }

    // section regions
    const int regionTop = 18, regionH = getHeight() - 26;
    for (int i = 0; i < (int) windows.size(); ++i)
    {
        auto r = regionBounds (windows[i].startBar, windows[i].lengthBars, L, regionH)
                     .withY (regionTop);
        auto c = colourForType (windows[i].type);
        g.setColour (c.withAlpha (i == selectedIndex ? 0.85f : 0.55f));
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (i == selectedIndex ? juce::Colours::white : c);
        g.drawRoundedRectangle (r.toFloat(), 4.0f, i == selectedIndex ? 2.0f : 1.0f);
        g.setColour (juce::Colours::white);
        g.drawText (windows[i].name, r.reduced (6, 2), juce::Justification::centredLeft, true);
    }

    // playhead
    const int px = (int) std::lround (barToX (playheadBar, L));
    g.setColour (juce::Colours::orange);
    g.drawVerticalLine (px, (float) regionTop, (float) (regionTop + regionH));
}

void ArrangerTimelineComponent::mouseDown (const juce::MouseEvent& e)
{
    const auto L = layout();
    const int regionTop = 18, regionH = getHeight() - 26;
    selectedIndex = -1;
    dragMode = Hit::None;

    for (int i = (int) windows.size() - 1; i >= 0; --i)
    {
        auto r = regionBounds (windows[i].startBar, windows[i].lengthBars, L, regionH).withY (regionTop);
        if (e.y >= r.getY() && e.y <= r.getBottom())
        {
            const auto hit = hitTest (e.x, r);
            if (hit != Hit::None)
            {
                selectedIndex     = i;
                dragMode          = hit;
                dragStartBarValue = windows[i].startBar;
                dragStartLenValue = windows[i].lengthBars;
                break;
            }
        }
    }

    if (onSectionSelected) onSectionSelected (selectedIndex);
    repaint();
}

void ArrangerTimelineComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (selectedIndex < 0 || dragMode == Hit::None) return;
    const auto L = layout();
    auto& w = windows[selectedIndex];

    const int barUnderCursor = xToSnappedBar ((double) e.x, L);

    if (dragMode == Hit::Body)
    {
        const int deltaBars = xToSnappedBar ((double) e.x, L)
                            - xToSnappedBar ((double) e.getMouseDownX(), L);
        w.startBar = juce::jlimit (1, totalBars + 1 - dragStartLenValue, dragStartBarValue + deltaBars);
    }
    else if (dragMode == Hit::LeftEdge)
    {
        const int newStart = juce::jlimit (1, w.startBar + w.lengthBars - 1, barUnderCursor);
        w.lengthBars = (w.startBar + w.lengthBars) - newStart;
        w.startBar   = newStart;
    }
    else if (dragMode == Hit::RightEdge)
    {
        const int newEnd = juce::jlimit (w.startBar + 1, totalBars + 1, barUnderCursor);
        w.lengthBars = newEnd - w.startBar;
    }

    repaint();
}

void ArrangerTimelineComponent::mouseUp (const juce::MouseEvent&)
{
    if (dragMode != Hit::None && onWindowsChanged)
        onWindowsChanged (windows);
    dragMode = Hit::None;
}
```

Add header + cpp to the `.jucer`, re-save.

- [ ] **Step 3: Manual verification**

Temporarily host the component (e.g., add it to `MainComponent` behind a debug key, or a scratch window) with `setTotalBars(8)` and one default window. Build + run. Confirm:
- Eight numbered bar columns render; one green "Variation 1" region spans them.
- Dragging the region **body** moves it and snaps to bar lines; it can't run past bar 1 or the end.
- Dragging the **left/right edge** resizes it, snapping, min length 1 bar.
- Selecting a region outlines it white and fires `onSectionSelected`.
- Releasing the mouse fires `onWindowsChanged` once (log it to confirm).

- [ ] **Step 4: Commit**

```bash
git add Source/Arranger/ArrangerTimelineComponent.h Source/Arranger/ArrangerTimelineComponent.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): visual draggable section timeline component"
```

---

## Task 5: The style editor (recording → preview → save)

Owns the recording, the timeline, the section list, a preview hook to the engine, and Save. All math comes from the tested helpers.

**Files:**
- Create: `Source/Arranger/ArrangerStyleEditor.h`, `.cpp`

- [ ] **Step 1: Implement the header**

```cpp
// Source/Arranger/ArrangerStyleEditor.h
#pragma once
#include <JuceHeader.h>
#include "ArrangerTimelineComponent.h"
#include "ArrangerStyleFile.h"
#include "ArrangerEngine.h"
#include "TrackEntry.h"
#include <vector>
#include <functional>

/** Authoring screen: turns a recording + timeline edits into a saved .style file,
    with live preview through a shared ArrangerEngine. */
class ArrangerStyleEditor : public juce::Component
{
public:
    explicit ArrangerStyleEditor (ArrangerEngine& engineToPreviewWith);

    /** Seed the editor from the current recorded tracks at the given playback tempo. */
    void loadRecording (const std::vector<TrackEntry>& tracks, double referenceBpm,
                        int timeSigNum, int timeSigDenom, const juce::String& styleName);

    /** Build the current ArrangerStyleFile from editor state (source tracks + windows). */
    ArrangerStyleFile toStyleFile() const;

    /** Save to the ArrangerStyles folder; returns the written file (or an invalid File on failure). */
    juce::File save (juce::String& error);

    void resized() override;

private:
    void rebuildPreview();          // build style from current state and engine.setStyle
    void addSectionOfType (ArrangerSectionType type);

    ArrangerEngine& engine;
    ArrangerTimelineComponent timeline;

    juce::TextButton addIntroBtn { "Add Intro" }, addFillBtn { "Add Fill" },
                     addEndingBtn { "Add Ending" }, previewBtn { "Preview" },
                     saveBtn { "Save" };

    std::vector<SourceTrackFile> sourceTracks;
    std::vector<SectionWindow>   windows;
    juce::String name;
    double referenceBpm = 120.0;
    int    timeSigNum = 4, timeSigDenom = 4;
    int    totalBars  = 1;
    juce::String styleId = juce::Uuid().toString();
};
```

- [ ] **Step 2: Implement the .cpp**

```cpp
// Source/Arranger/ArrangerStyleEditor.cpp
#include "ArrangerStyleEditor.h"
#include "ArrangerSourceBuilder.h"
#include "ArrangerDefaults.h"
#include "ArrangerPatternBuilder.h"
#include "ArrangerStyleIOHelper.h"
#include "ArrangerTime.h"
#include "IOHelper.h"

ArrangerStyleEditor::ArrangerStyleEditor (ArrangerEngine& e) : engine (e)
{
    addAndMakeVisible (timeline);
    for (auto* b : { &addIntroBtn, &addFillBtn, &addEndingBtn, &previewBtn, &saveBtn })
        addAndMakeVisible (b);

    timeline.onWindowsChanged = [this] (const std::vector<SectionWindow>& w)
    {
        windows = w;
        rebuildPreview();
    };

    addIntroBtn.onClick  = [this] { addSectionOfType (ArrangerSectionType::Intro); };
    addFillBtn.onClick   = [this] { addSectionOfType (ArrangerSectionType::Fill); };
    addEndingBtn.onClick = [this] { addSectionOfType (ArrangerSectionType::Ending); };
    previewBtn.onClick   = [this] { rebuildPreview(); engine.start(); };
    saveBtn.onClick      = [this] { juce::String err; save (err); };

    engine.onElapsedBeats = [this] (double beats)
    {
        const double bpb = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);
        timeline.setPlayheadBar (1.0 + beats / juce::jmax (1.0, bpb));
    };
}

void ArrangerStyleEditor::loadRecording (const std::vector<TrackEntry>& tracks, double refBpm,
                                         int tsNum, int tsDenom, const juce::String& styleName)
{
    referenceBpm = (refBpm > 0.0 ? refBpm : 120.0);
    timeSigNum = tsNum; timeSigDenom = tsDenom;
    name = styleName.isNotEmpty() ? styleName : "New Style";

    sourceTracks = ArrangerSourceBuilder::fromTrackEntries (tracks, referenceBpm);

    // total bars = longest source track rounded up to whole bars
    const double bpb = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);
    double lastBeat = 0.0;
    for (const auto& st : sourceTracks)
        if (! st.events.empty())
            lastBeat = juce::jmax (lastBeat, st.events.back().beats);
    totalBars = juce::jmax (1, ArrangerTime::barsForBeats (lastBeat, bpb));

    windows = ArrangerDefaults::defaultWindowsForBars (totalBars);

    timeline.setTotalBars (totalBars);
    timeline.setWindows (windows);
    rebuildPreview();
}

ArrangerStyleFile ArrangerStyleEditor::toStyleFile() const
{
    ArrangerStyleFile f;
    f.id = styleId; f.name = name;
    f.originalTempo = referenceBpm;
    f.timeSigNum = timeSigNum; f.timeSigDenom = timeSigDenom;
    f.sourceTracks = sourceTracks;
    f.sections = windows;
    return f;
}

void ArrangerStyleEditor::rebuildPreview()
{
    auto style = ArrangerPatternBuilder::buildStyleFromWindows (
        sourceTracks, windows, timeSigNum, timeSigDenom, referenceBpm);
    engine.setStyle (style);
}

void ArrangerStyleEditor::addSectionOfType (ArrangerSectionType type)
{
    SectionWindow w;
    w.id = juce::Uuid().toString();
    w.type = type;
    w.startBar = 1;
    w.lengthBars = juce::jmin (type == ArrangerSectionType::Fill ? 1 : 2, totalBars);
    w.afterComplete = (type == ArrangerSectionType::Ending) ? ArrangerAfterComplete::Stop
                                                            : ArrangerAfterComplete::FallThrough;
    w.name = ArrangerEnums::toString (type) + " 1";
    windows.push_back (w);
    timeline.setWindows (windows);
    rebuildPreview();
}

juce::File ArrangerStyleEditor::save (juce::String& error)
{
    auto folder = IOHelper::getFolder ("ArrangerStyles");
    auto file = folder.getChildFile (juce::File::createLegalFileName (name) + ".style");
    ArrangerStyleIOHelper::saveToFile (file, toStyleFile());
    if (! file.existsAsFile()) { error = "Failed to write " + file.getFullPathName(); return {}; }
    return file;
}

void ArrangerStyleEditor::resized()
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop (32);
    for (auto* b : { &addIntroBtn, &addFillBtn, &addEndingBtn, &previewBtn, &saveBtn })
        b->setBounds (top.removeFromLeft (90).reduced (2));
    timeline.setBounds (area.reduced (4));
}
```

> `ArrangerStyleEditor::toStyleFile` is small and pure enough to unit-test if you want, but its parts (`fromTrackEntries`, `buildStyleFromWindows`, `defaultWindowsForBars`) are already covered, so a manual check is sufficient here.

Add header + cpp to the `.jucer`, re-save.

- [ ] **Step 3: Manual verification**

Open the editor (wired in Task 6) with a real recording loaded. Build + run. Confirm:
- The timeline shows the recording's bar count with a full-width Variation.
- "Add Intro/Fill/Ending" add colored regions you can drag/resize.
- "Preview" plays; the orange playhead tracks the bar position; tapping section buttons switches sections audibly (distinct content per region).
- "Save" writes `<name>.style` into the `ArrangerStyles` app-data folder (verify the file exists and re-loads via Part A's loader).

- [ ] **Step 4: Commit**

```bash
git add Source/Arranger/ArrangerStyleEditor.h Source/Arranger/ArrangerStyleEditor.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): style editor - recording to preview to saved .style"
```

---

## Task 6: Style list + mode wiring in MainComponent

Lets the user pick a saved arranger style to play, and open the editor to create/edit one. Plugs into the existing arranger mode (section buttons already drive the engine).

**Files:**
- Create: `Source/Arranger/ArrangerStyleListComponent.h`, `.cpp`
- Modify: `Source/UI/MainComponent.h`, `Source/UI/MainComponent.cpp`

- [ ] **Step 1: Implement the style list header**

```cpp
// Source/Arranger/ArrangerStyleListComponent.h
#pragma once
#include <JuceHeader.h>
#include <functional>

/** Lists *.style files from the ArrangerStyles folder; fires callbacks to load or edit. */
class ArrangerStyleListComponent : public juce::Component,
                                   private juce::ListBoxModel
{
public:
    std::function<void (const juce::File&)> onLoadStyle;
    std::function<void (const juce::File&)> onEditStyle;
    std::function<void()> onCreateNew;

    ArrangerStyleListComponent();
    void refresh();                 // rescan the folder
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;

private:
    juce::ListBox list { "styles", this };
    juce::TextButton newBtn { "New Style" }, editBtn { "Edit" };
    juce::Array<juce::File> files;
};
```

- [ ] **Step 2: Implement the style list .cpp**

```cpp
// Source/Arranger/ArrangerStyleListComponent.cpp
#include "ArrangerStyleListComponent.h"
#include "IOHelper.h"

ArrangerStyleListComponent::ArrangerStyleListComponent()
{
    addAndMakeVisible (list);
    addAndMakeVisible (newBtn);
    addAndMakeVisible (editBtn);
    newBtn.onClick  = [this] { if (onCreateNew) onCreateNew(); };
    editBtn.onClick = [this]
    {
        const int r = list.getSelectedRow();
        if (r >= 0 && r < files.size() && onEditStyle) onEditStyle (files[r]);
    };
    refresh();
}

void ArrangerStyleListComponent::refresh()
{
    files.clear();
    auto folder = IOHelper::getFolder ("ArrangerStyles");
    for (auto& f : folder.findChildFiles (juce::File::findFiles, false, "*.style"))
        files.add (f);
    list.updateContent();
    repaint();
}

int ArrangerStyleListComponent::getNumRows() { return files.size(); }

void ArrangerStyleListComponent::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= files.size()) return;
    if (selected) g.fillAll (juce::Colour (0xff3a3a3a));
    g.setColour (juce::Colours::white);
    g.drawText (files[row].getFileNameWithoutExtension(), 6, 0, w - 12, h, juce::Justification::centredLeft);
}

void ArrangerStyleListComponent::listBoxItemDoubleClicked (int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < files.size() && onLoadStyle) onLoadStyle (files[row]);
}

void ArrangerStyleListComponent::resized()
{
    auto area = getLocalBounds();
    auto bottom = area.removeFromBottom (30);
    newBtn.setBounds (bottom.removeFromLeft (area.getWidth() / 2).reduced (2));
    editBtn.setBounds (bottom.reduced (2));
    list.setBounds (area);
}
```

- [ ] **Step 3: Wire into MainComponent**

In `Source/UI/MainComponent.h`, add members (near the other arranger members):

```cpp
#include "Arranger/ArrangerStyleListComponent.h"
#include "Arranger/ArrangerStyleEditor.h"
// ...
ArrangerStyleListComponent arrangerStyleList;
std::unique_ptr<ArrangerStyleEditor> arrangerStyleEditor;
```

In `Source/UI/MainComponent.cpp` constructor, wire the callbacks (place near where the arranger engine and section buttons are already set up):

```cpp
addChildComponent (arrangerStyleList); // shown when in arranger mode

arrangerStyleList.onLoadStyle = [this] (const juce::File& f)
{
    ArrangerStyleFile sf; juce::String err;
    if (ArrangerStyleIOHelper::loadFromFile (f, sf, err))
        arrangerEngine->setStyle (ArrangerPatternBuilder::buildStyleFromFile (sf));
    else
        DBG ("Style load failed: " << err);
};

arrangerStyleList.onCreateNew = [this]
{
    arrangerStyleEditor = std::make_unique<ArrangerStyleEditor> (*arrangerEngine);
    addAndMakeVisible (*arrangerStyleEditor);
    arrangerStyleEditor->setBounds (getLocalBounds().reduced (40));
    // current recorded tracks + live tempo/timesig feed the editor:
    arrangerStyleEditor->loadRecording (currentArrangerSourceTracks(), currentPlaybackBpm(), 4, 4, "New Style");
};

arrangerStyleList.onEditStyle = [this] (const juce::File& f)
{
    ArrangerStyleFile sf; juce::String err;
    if (! ArrangerStyleIOHelper::loadFromFile (f, sf, err)) { DBG (err); return; }
    arrangerStyleEditor = std::make_unique<ArrangerStyleEditor> (*arrangerEngine);
    addAndMakeVisible (*arrangerStyleEditor);
    arrangerStyleEditor->setBounds (getLocalBounds().reduced (40));
    arrangerStyleEditor->loadRecording (sf.sourceTracks /* via overload below */, sf.originalTempo,
                                        sf.timeSigNum, sf.timeSigDenom, sf.name);
};
```

> **Note on `onEditStyle`:** `loadRecording` takes `TrackEntry`s, but an existing style stores `SourceTrackFile`s. Add a second `ArrangerStyleEditor::loadFromFile (const ArrangerStyleFile&)` overload that seeds `sourceTracks`/`windows`/`name`/tempo directly (no conversion) and calls `timeline.setTotalBars/​setWindows` + `rebuildPreview`. Implement it alongside Task 5 if you do editing of existing styles; for first delivery you may wire only New + Load and leave Edit hidden.

> **`currentArrangerSourceTracks()` / `currentPlaybackBpm()`** are small helpers you add to `MainComponent` that return the tracks currently loaded in the classic player (`MultipleTrackPlayer::getCurrentTracks()`) and the live BPM. If those accessors already exist under different names, use them; the point is to feed the editor the same tracks the user just recorded/loaded.

Show `arrangerStyleList` when entering arranger mode and hide it in classic mode, alongside the existing mode-switch logic that re-routes engine output.

Add the new files to the `.jucer`, re-save.

- [ ] **Step 4: Manual verification**

Build + run. Confirm:
- Entering arranger mode shows the style list (empty first run).
- "New Style" opens the editor seeded with the current tracks; Save writes a `.style`; the list refreshes and shows it.
- Double-clicking a listed style loads it; pressing Start + section buttons plays its distinct sections.
- Classic mode is unchanged and its styles still work.

- [ ] **Step 5: Commit**

```bash
git add Source/Arranger/ArrangerStyleListComponent.h Source/Arranger/ArrangerStyleListComponent.cpp Source/UI/MainComponent.h Source/UI/MainComponent.cpp "Project Synth2.jucer"
git commit -m "feat(arranger): style list + create/edit/load wiring in arranger mode"
```

---

## Task 7: End-to-end integration pass

- [ ] **Step 1: Full manual run-through**

Build + run. Walk the whole pipeline:
1. Record or load a backing performance in the classic flow.
2. Switch to arranger mode → New Style.
3. On the timeline: drag the Variation, Add Intro/Fill/Ending, resize each to real bars.
4. Preview: Start, watch the playhead, tap Intro→Variation→Fill→Ending and hear distinct, bar-synced sections; Ending stops.
5. Save. Confirm the `.style` file in the `ArrangerStyles` folder.
6. Restart the app, arranger mode, load the saved style, play again — identical.

- [ ] **Step 2: Regression check**

Enable the test runner, build, run `runTestsInCategory ("Arranger")`. Expected: all Part A + Part B unit tests pass. Re-comment the runner.

- [ ] **Step 3: Commit (docs/notes if any)**

```bash
git add -A
git commit -m "test(arranger): phase 3 end-to-end verified (manual + unit)"
```

---

## Self-review notes (already applied)

- **Spec coverage:** §6.3 timeline → Tasks 1,4; §6.4 editor → Tasks 2,3,5; §6.5 list/mode → Task 6; preview playhead via `onElapsedBeats` → Task 5; channel map reused via `ArrangerSourceBuilder` → Task 2. Effects deferred (no UI), consistent with D6.
- **Type consistency:** `ArrangerTimelineGeometry::{Layout,Hit,barToX,xToSnappedBar,regionBounds,hitTest}`, `ArrangerSourceBuilder::fromTrackEntries`, `ArrangerDefaults::defaultWindowsForBars`, `ArrangerStyleEditor::{loadRecording,toStyleFile,save}`, `ArrangerStyleListComponent::{onLoadStyle,onEditStyle,onCreateNew}` are used consistently across declarations, definitions, and call sites.
- **Placeholders:** none. The two integration hooks that depend on existing `MainComponent` internals (`currentArrangerSourceTracks()`, the mode-switch show/hide) are called out explicitly with what to substitute, not left vague.
- **Known follow-ups (out of scope, noted not hidden):** an `ArrangerStyleEditor::loadFromFile` overload for *editing* existing styles (Task 6 note); section rename/delete UI; per-section `afterComplete` override control. First delivery supports New + Load fully.

## What Part B delivers

End-to-end authoring: record → mark sections on a draggable, color-coded timeline → preview live with a tracking playhead → save a self-contained `.style` → reload and play. Built on Part A's tested core, with all geometry/conversion/default logic unit-tested and the JUCE components kept thin.
