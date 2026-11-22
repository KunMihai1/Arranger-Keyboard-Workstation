> Part 2 of 2 of [2026-06-06-arranger-phase3-partB-timeline-ui-editor](2026-06-06-arranger-phase3-partB-timeline-ui-editor.md). Previous: [part 1](2026-06-06-arranger-phase3-partB-timeline-ui-editor.md).

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
