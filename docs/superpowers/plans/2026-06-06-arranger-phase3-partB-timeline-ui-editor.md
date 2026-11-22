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


> Continued in [part 2 of 2](2026-06-06-arranger-phase3-partB-timeline-ui-editor-part2.md).
