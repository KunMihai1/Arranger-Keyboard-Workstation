#pragma once
#include <JuceHeader.h>
#include "ArrangerTimelineComponent.h"
#include "ArrangerStyleFile.h"
#include "ArrangerEngine.h"
#include "TrackEntry.h"
#include "UI/Stage/StagePageWidgets.h"
#include "UI/Sections/SectionLaneHeaders.h"
#include "UI/Sections/SectionJumpPads.h"
#include "UI/Sections/SectionInspector.h"
#include <vector>
#include <functional>

/** Labels and button captions of the style editor (board PageSections). */
namespace StyleEditorText
{
    inline const juce::String nameHint      = "Configuration name";
    inline const juce::String addHeading    = "Add section";
    inline const juce::String addIntro      = "+ Intro";
    inline const juce::String addVariation  = "+ Var";
    inline const juce::String addFill       = "+ Fill";
    inline const juce::String addBreak      = "+ Break";
    inline const juce::String addEnding     = "+ Ending";
    inline const juce::String preview       = "Preview";
    inline const juce::String stop          = "Stop";
    inline const juce::String updateTracks  = "Update tracks";
    inline const juce::String save          = "Save";
    inline const juce::String recordedKey   = "Recorded key";
    inline const juce::String zoom          = "Zoom";
    inline const char* const  zoomOutUtf8   = "\xe2\x88\x92";   // "−"
    inline const juce::String zoomIn        = "+";
}

/** Authoring: turns a recording + section edits into a saved .style file, with live preview through a shared
    ArrangerEngine. The Sections page's editor (board PageSections): it fills the page's content area and
    leaves the configurations list's corner to the list; its name, key, zoom, preview, Update tracks and Save
    go on the page's title row (getTitleActions). */
class ArrangerStyleEditor : public juce::Component,
                            private juce::ScrollBar::Listener,
                            private juce::Timer
{
public:
    /** Fired after a successful save, with the written file AND the freshly-built engine style, so the
        host can refresh the active config WITHOUT re-parsing the file (which would re-freeze the UI). */
    std::function<void (const juce::File&, const ArrangerStyle&)> onSaved;
    /** Supplies the app's current recording as raw track entries for the "Update Tracks" action. Set
        by the host; if unset, Update Tracks is a no-op. Returned raw so the heavy event-building can
        happen on the editor's background thread. */
    std::function<std::vector<TrackEntry>()> onRequestCurrentTracks;
    /** The title-row controls changed size (Preview ↔ Stop): the page lays its title row out again. */
    std::function<void()> onTitleActionsChanged;
    /** A save or Update tracks started (true) or finished: the host gates what lies outside the editor. */
    std::function<void (bool busy, const juce::String& text)> onBusyChanged;   // the host covers the app meanwhile
    bool isBusy() const { return busyNow; }

    explicit ArrangerStyleEditor (ArrangerEngine& engineToPreviewWith);
    ~ArrangerStyleEditor() override;

    /** Seed the editor from the current recorded tracks at the given playback tempo. */
    void loadRecording (const std::vector<TrackEntry>& tracks, double referenceBpm,
                        int timeSigNum, int timeSigDenom, const juce::String& styleName);

    /** Seed the editor from an already-saved style file (for editing). */
    void loadFromFile (const ArrangerStyleFile& styleFile);

    /** Remember the on-disk file this editor was opened from, so Save renames it (rather
        than leaving a duplicate) when the name changes. Leave unset for a brand-new config. */
    void setSourceFile (const juce::File& sourceFile) { loadedFile = sourceFile; updateTracksBtn.setEnabled (sourceFile.existsAsFile()); }

    /** Build the current ArrangerStyleFile from editor state (source tracks + windows). */
    ArrangerStyleFile toStyleFile() const;
    /** Build the engine-ready style from current editor state (same build rebuildPreview uses). */
    ArrangerStyle buildStyle() const;

    /** The page's title-row controls, left to right (name, recorded key, zoom, Preview, Update tracks, Save). */
    std::vector<juce::Component*> getTitleActions();
    void setScale (float newScale);

    void resized() override;
    void paint (juce::Graphics& g) override;

private:
    void requestSave();             // validate name, confirm on collision, then finishSave
    void finishSave (const juce::File& target);   // serialize + write off-thread, then refresh on the message thread
    void setBusy (bool busy, const juce::String& text = {});  // show/hide the in-editor "working" overlay + gate buttons
    void rebuildPreview();          // build style from current state and engine.setStyle
    void addSectionOfType (ArrangerSectionType type);
    void updateTracksFromRecording();  // replace source tracks with the app's current recording, keep sections, overwrite file
    void removeSelectedSection();   // remove the section currently selected on the timeline
    void renumberSectionsByType();  // assign unique per-type names (Intro 1, Intro 2, ...)
    void recomputeTotalBars();      // longest source track / furthest window, in whole bars
    int  firstSectionStartBar() const;  // earliest section's start bar (1 if none) — where the idle arrow rests
    void restIdlePlayhead();        // park the playhead at the first section when not previewing
    void layoutTimeline();          // size the (scrollable) timeline to totalBars × the bar width
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;  // detect manual scroll

    // The page (ArrangerStyleEditorPage.cpp)
    void setUpPageControls();
    void refreshLanes();               // lane headers + the lanes' notes from sourceTracks
    void refreshSelection();           // inspector + jump pads from the windows and the selection
    void windowsEdited();              // an inspector edit: what a drag does (rebuild, rest the playhead)
    void selectSection (int sectionIndex);   // a click on a window or a pad: select; while previewing, switch there
    void startPreview();
    void togglePreview();
    void playSelected();
    void stepZoom (int direction);
    void timerCallback() override;     // Preview's label and LED, the playing pad
    void syncPreviewState();

    ArrangerEngine& engine;
    juce::Viewport  timelineViewport;          // horizontal scroll for many bars
    ArrangerTimelineComponent timeline;
    SectionLaneHeaders laneHeaders;            // track names + transpose mode (was the NTT strip)
    SectionJumpPads    jumpPads;
    SectionInspector   inspector;

    // Title row
    StageLcdInput nameEditor { StyleEditorText::nameHint };
    SilkLabel     keyLabel { StyleEditorText::recordedKey, SilkLabel::Tone::dim };
    StageField    keyRootBox;
    StageField    keyQualityBox;
    SilkLabel     zoomLabel { StyleEditorText::zoom, SilkLabel::Tone::dim };
    SegButton     zoomOutBtn { juce::String::fromUTF8 (StyleEditorText::zoomOutUtf8), SegButton::Style::seg },
                  zoomInBtn { StyleEditorText::zoomIn, SegButton::Style::seg },
                  previewBtn { StyleEditorText::preview, SegButton::Style::seg, true, RubberButton::LedColour::green },
                  updateTracksBtn { StyleEditorText::updateTracks, SegButton::Style::seg },
                  saveBtn { StyleEditorText::save, SegButton::Style::lit };
    // Left column, under the configurations list
    SilkLabel     addHeading { StyleEditorText::addHeading, SilkLabel::Tone::bright };
    SegButton     addIntroBtn { StyleEditorText::addIntro, SegButton::Style::seg }, addVariationBtn { StyleEditorText::addVariation, SegButton::Style::seg },
                  addFillBtn { StyleEditorText::addFill, SegButton::Style::seg }, addBreakBtn { StyleEditorText::addBreak, SegButton::Style::seg },
                  addEndingBtn { StyleEditorText::addEnding, SegButton::Style::seg };
    void populateKeyControls();        // fill the combos and select the current originalRoot/quality

    bool busyNow = false;
    float scale { StageMetrics::referenceScale };
    bool previewShown = false;  // what Preview shows (Preview / Stop), to re-lay the title row on a change

    std::vector<SourceTrackFile> sourceTracks;
    std::vector<SectionWindow>   windows;
    juce::File   loadedFile;        // file this editor was opened from ({} for a new config)
    juce::String name;
    double referenceBpm = ArrangerStyleDefaults::tempoBpm;
    int    timeSigNum = ArrangerStyleDefaults::beatsPerBar, timeSigDenom = ArrangerStyleDefaults::beatUnit;
    int    totalBars  = ArrangerStyleDefaults::minimumLengthBars;
    juce::String styleId = juce::Uuid().toString();

    // Phase 4: the chord this config was recorded in (auto-detected from the source tracks on a new
    // recording, loaded from file, persisted on save; editable in a future UI).
    int          originalRoot    = PitchClasses::lowest;
    ChordQuality originalQuality = ChordQuality::Maj;
    void autoDetectOriginalChord();   // sets the two from the pitched source tracks
    bool   followPlayhead     = true;   // auto-scroll the viewport to keep the playhead in view
    bool   programmaticScroll = false;  // true while WE move the viewport (so it isn't seen as manual)
};
