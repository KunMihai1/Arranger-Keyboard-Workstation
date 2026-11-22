#include "ArrangerStyleEditor.h"
#include "ArrangerSourceBuilder.h"
#include "ArrangerDefaults.h"
#include "ArrangerPatternBuilder.h"
#include "ArrangerStyleIOHelper.h"
#include "ArrangerEnums.h"
#include "ArrangerTime.h"
#include "ChordDetector.h"
#include "IOHelper.h"
#include "AppColours.h"
#include <cmath>

namespace
{
    namespace Text
    {
        const juce::String defaultConfigName = "New configuration";
        const juce::String defaultStyleName  = "Style";
        const juce::String trackPrefix       = "Track ";
        const juce::String nameSeparator     = " ";   // "<type> <number>"
        const juce::String styleExtension    = ".style";

        // Per-track NTT choices shown in the editor (display text, not the persisted names).
        const juce::String nttNoTranspose = "No Transpose";
        const juce::String nttParallel    = "Parallel";
        const juce::String nttChord       = "Chord";
        const juce::String nttFixed       = "Fixed";

        const juce::StringArray pitchClassNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        const juce::String updateTracksTitle   = "Update tracks";
        const juce::String saveFirst           = "Save this configuration first, then Update Tracks can replace its notes.";
        const juce::String replaceTracksStart  = "Replace the tracks of \"";
        const juce::String replaceTracksEnd    = "\" with the current recording and overwrite the saved "
                                                 "configuration?\nYour sections (intros/variations/fills) stay as they are.";
        const juce::String updateButton        = "Update";
        const juce::String cancelButton        = "Cancel";
        const juce::String noTracksToImport    = "No recorded tracks to import.";
        const juce::String updatingTracks      = "Updating tracks...";
        const juce::String failedToWrite       = "Failed to write ";
        const juce::String tracksUpdated       = "Tracks updated: ";

        const juce::String removeSectionTitle  = "Remove section";
        const juce::String selectSectionFirst  = "Click a section on the timeline to select it first.";

        const juce::String saveTitle           = "Save configuration";
        const juce::String overwriteStart      = "A configuration named \"";
        const juce::String overwriteEnd        = "\" already exists.\nOverwrite it?";
        const juce::String overwriteButton     = "Overwrite";
        const juce::String savingConfiguration = "Saving configuration...";
        const juce::String renamedTo           = "Renamed to: ";
        const juce::String savedAs             = "Saved: ";
    }

    constexpr int    dialogConfirmed       = 1;      // showOkCancelBox: 1 = OK button, 0 = Cancel
    constexpr int    comboIdOffset         = 1;      // ComboBox ids must be != 0: id = value + 1
    constexpr int    noItems               = 0;
    constexpr int    highestPitchClass     = 11;     // B
    constexpr int    numberStep            = 1;      // next per-type number = max existing + 1
    constexpr int    lastBarOffset         = 1;      // a window of N bars ends N - 1 bars after its start
    constexpr int    noBar                 = -1;
    constexpr int    fillLengthBars        = 1;      // a new Fill is one bar; other new sections two
    constexpr int    otherSectionLengthBars = 2;
    constexpr double noBeats               = 0.0;
    constexpr double minimumBeatsPerBar    = 1.0;
    constexpr bool   includeSeparator      = false;  // fromLastOccurrenceOf flags
    constexpr bool   ignoreCase            = false;
    constexpr double noTempo               = 0.0;

    // Look
    constexpr int   timelineScrollBarThickness = 12;
    constexpr int   nttScrollBarThickness      = 10;
    constexpr float busyFontHeight             = 24.0f;
    constexpr float busyDimAlpha               = 0.7f;
    constexpr float minimumNameScale           = 0.7f;
    constexpr int   borderThickness            = 2;

    // Auto-follow: scroll when the playhead comes this close to a viewport edge.
    constexpr int followEdgeMargin = 48;
    constexpr int halfViewDivisor  = 2;
    constexpr int viewTop          = 0;
    constexpr int noScroll         = 0;

    // Per-track NTT cells (pixels)
    constexpr int nttCellWidth        = 150;
    constexpr int nttCellGap          = 8;
    constexpr int nttNameHeight       = 18;
    constexpr int nttComboHeight      = 24;
    constexpr int nttCellPadding      = 4;
    constexpr int nttScrollBarRoom    = 12;

    // Editor layout (pixels)
    constexpr int editorMargin        = 8;
    constexpr int topRowHeight        = 30;
    constexpr int nameLabelWidth      = 50;
    constexpr int nameEditorWidth     = 180;
    constexpr int noInsetX            = 0;
    constexpr int fieldInsetY         = 2;
    constexpr int afterNameGap        = 8;
    constexpr int buttonWidth         = 84;
    constexpr int buttonInset         = 2;
    constexpr int rowGap              = 6;
    constexpr int keyRowHeight        = 26;
    constexpr int keyLabelWidth       = 90;
    constexpr int keyRootWidth        = 60;
    constexpr int keyQualityWidth     = 90;
    constexpr int nttStripHeight      = 78;
    constexpr int nttHeaderHeight     = 20;
    constexpr int minimumTimelineHeight = 40;
    constexpr int timelineScrollBarRoom = 14;

    std::vector<TimedBeatEvent> pitchedEvents (const std::vector<SourceTrackFile>& tracks)
    {
        // Drums/perc carry no harmony: only Bass/Acc tracks tell the recorded key.
        std::vector<TimedBeatEvent> pitched;
        for (const auto& track : tracks)
            if (track.partType == ArrangerPartType::Bass || track.partType == ArrangerPartType::Acc)
                for (const auto& event : track.events)
                    pitched.push_back (event);
        return pitched;
    }
}

ArrangerStyleEditor::ArrangerStyleEditor (ArrangerEngine& engineToPreviewWith) : engine (engineToPreviewWith)
{
    timelineViewport.setViewedComponent (&timeline, false);
    timelineViewport.setScrollBarsShown (false, true);   // horizontal only
    timelineViewport.setScrollBarThickness (timelineScrollBarThickness);
    timelineViewport.getHorizontalScrollBar().addListener (this);  // detect manual scroll
    addAndMakeVisible (timelineViewport);
    addAndMakeVisible (nameLabel);
    addAndMakeVisible (nameEditor);
    nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    nameEditor.setText (Text::defaultConfigName, juce::dontSendNotification);
    for (auto* button : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                          &removeBtn, &previewBtn, &stopBtn, &updateTracksBtn, &saveBtn, &closeBtn })
        addAndMakeVisible (button);

    updateTracksBtn.setEnabled (false);   // only meaningful when editing an existing saved config

    addAndMakeVisible (keyLabel);
    addAndMakeVisible (keyRootBox);
    addAndMakeVisible (keyQualityBox);
    keyLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    populateKeyControls();
    keyRootBox.onChange    = [this] { originalRoot = keyRootBox.getSelectedId() - comboIdOffset; rebuildPreview(); };
    keyQualityBox.onChange = [this] { originalQuality = (ChordQuality) keyQualityBox.getSelectedId(); rebuildPreview(); };

    // Phase 7a: per-track NTT selector strip (scrolls horizontally if there are many tracks).
    nttHeaderLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (nttHeaderLabel);
    nttTrackViewport.setViewedComponent (&nttTrackContent, false);
    nttTrackViewport.setScrollBarsShown (false, true);   // horizontal only
    nttTrackViewport.setScrollBarThickness (nttScrollBarThickness);
    addAndMakeVisible (nttTrackViewport);

    // Full-screen "working" overlay, styled like the app's "Preparing style..." overlay. Eats mouse
    // clicks so nothing behind it can be triggered while a save/update runs on the background thread.
    busyOverlay.setJustificationType (juce::Justification::centred);
    busyOverlay.setFont (juce::Font (busyFontHeight, juce::Font::bold));
    busyOverlay.setColour (juce::Label::textColourId, juce::Colours::white);
    busyOverlay.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (busyDimAlpha));
    busyOverlay.setInterceptsMouseClicks (true, true);
    addChildComponent (busyOverlay);   // hidden until setBusy(true)

    timeline.onWindowsChanged = [this] (const std::vector<SectionWindow>& editedWindows)
    {
        windows = editedWindows;
        rebuildPreview();
        restIdlePlayhead();   // when not previewing, keep the arrow parked at the first section's start
    };

    // Clicking a region while previewing switches to that section at the next bar line.
    timeline.onSectionSelected = [this] (int sectionIndex)
    {
        if (engine.isPlaying() && sectionIndex >= int {} && sectionIndex < (int) windows.size())
            engine.queueSection (windows[(size_t) sectionIndex].type, windows[(size_t) sectionIndex].name);
    };

    addIntroBtn.onClick     = [this] { addSectionOfType (ArrangerSectionType::Intro); };
    addVariationBtn.onClick = [this] { addSectionOfType (ArrangerSectionType::Variation); };
    addFillBtn.onClick      = [this] { addSectionOfType (ArrangerSectionType::Fill); };
    addBreakBtn.onClick     = [this] { addSectionOfType (ArrangerSectionType::Break); };
    addEndingBtn.onClick    = [this] { addSectionOfType (ArrangerSectionType::Ending); };
    removeBtn.onClick       = [this] { removeSelectedSection(); };
    previewBtn.onClick   = [this] { followPlayhead = true; rebuildPreview(); engine.setBpm (referenceBpm); engine.start (false); };  // preview always starts now (no Synchro/Count-In)
    stopBtn.onClick      = [this] { engine.stop(); };
    updateTracksBtn.onClick = [this] { updateTracksFromRecording(); };
    saveBtn.onClick      = [this] { requestSave(); };
    closeBtn.onClick = [this] { engine.stop(); if (onClose) onClose(); };

    engine.onElapsedBeats = [this] (double /*globalBeats*/)   // always marshalled to the message thread
    {
        const double beatsPerBarValue = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);

        // Show the arrow at the *active section's* position, not the global monotonic playhead, so a
        // jump (e.g. to the Ending) moves the arrow into that section's bars. A looping section wraps
        // its local beats within its own length so the arrow loops inside its window.
        const int    activeSection = engine.getActiveSectionIndex();
        const double localBeats    = juce::jmax (noBeats, engine.getActiveSectionLocalBeats());
        double bar;
        if (activeSection >= int {} && activeSection < (int) windows.size())
        {
            const auto&  window          = windows[(size_t) activeSection];
            const double sectionLenBeats = (double) juce::jmax (ArrangerStyleDefaults::minimumLengthBars, window.lengthBars) * beatsPerBarValue;
            const double wrapped         = sectionLenBeats > noBeats ? std::fmod (localBeats, sectionLenBeats) : noBeats;
            bar = (double) window.startBar + wrapped / juce::jmax (minimumBeatsPerBar, beatsPerBarValue);
        }
        else
        {
            bar = ArrangerStyleDefaults::firstBar + localBeats / juce::jmax (minimumBeatsPerBar, beatsPerBarValue);
        }
        timeline.setPlayheadBar (bar);

        // Auto-follow: keep the playhead in view, scrolling the viewport when it nears an edge.
        // Suspended once the user scrolls manually (so they can navigate to a far section); press
        // Preview to re-arm it.
        const int playheadX = timeline.xForBar (bar);
        const int viewX     = timelineViewport.getViewPositionX();
        const int viewWidth = timelineViewport.getViewWidth();
        if (followPlayhead && viewWidth > int {}
            && (playheadX < viewX + followEdgeMargin || playheadX > viewX + viewWidth - followEdgeMargin))
        {
            const int maxScrollX = juce::jmax (noScroll, timeline.getWidth() - viewWidth);
            programmaticScroll = true;   // our own scroll: don't treat it as a manual one
            timelineViewport.setViewPosition (juce::jlimit (noScroll, maxScrollX, playheadX - viewWidth / halfViewDivisor), viewTop);
            programmaticScroll = false;
        }
    };
}

void ArrangerStyleEditor::recomputeTotalBars()
{
    const double beatsPerBarValue = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);
    double lastBeat = noBeats;
    for (const auto& sourceTrack : sourceTracks)
        if (! sourceTrack.events.empty())
            lastBeat = juce::jmax (lastBeat, sourceTrack.events.back().beats);

    int bars = juce::jmax (ArrangerStyleDefaults::minimumLengthBars, ArrangerTime::barsForBeats (lastBeat, beatsPerBarValue));
    for (const auto& window : windows)
        bars = juce::jmax (bars, window.startBar + window.lengthBars - lastBarOffset);
    totalBars = juce::jmax (ArrangerStyleDefaults::minimumLengthBars, bars);
}

void ArrangerStyleEditor::loadRecording (const std::vector<TrackEntry>& tracks, double refBpm,
                                         int tsNum, int tsDenom, const juce::String& styleName)
{
    referenceBpm = (refBpm > noTempo ? refBpm : ArrangerStyleDefaults::tempoBpm);
    timeSigNum = tsNum; timeSigDenom = tsDenom;
    name = styleName.isNotEmpty() ? styleName : Text::defaultConfigName;
    nameEditor.setText (name, juce::dontSendNotification);
    styleId = juce::Uuid().toString();

    updateTracksBtn.setEnabled (false);   // brand-new config: nothing on disk to update yet

    sourceTracks = ArrangerSourceBuilder::fromTrackEntries (tracks, referenceBpm);
    rebuildTrackNttList();       // Phase 7a: per-track NTT cells for the fresh tracks
    autoDetectOriginalChord();   // guess the recorded key from the fresh recording
    populateKeyControls();
    windows = ArrangerDefaults::defaultWindowsForBars (ArrangerStyleDefaults::minimumLengthBars);
    recomputeTotalBars();
    windows = ArrangerDefaults::defaultWindowsForBars (totalBars);

    timeline.setTotalBars (totalBars);
    timeline.setWindows (windows);
    layoutTimeline();
    rebuildPreview();
    restIdlePlayhead();   // park the arrow at the first section's start, not bar 1
}

void ArrangerStyleEditor::loadFromFile (const ArrangerStyleFile& styleFile)
{
    styleId = styleFile.id.isNotEmpty() ? styleFile.id : juce::Uuid().toString();
    name = styleFile.name.isNotEmpty() ? styleFile.name : Text::defaultStyleName;
    nameEditor.setText (name, juce::dontSendNotification);
    referenceBpm = (styleFile.originalTempo > noTempo ? styleFile.originalTempo : ArrangerStyleDefaults::tempoBpm);
    timeSigNum = styleFile.timeSigNum; timeSigDenom = styleFile.timeSigDenom;
    originalRoot = styleFile.originalRoot; originalQuality = styleFile.originalQuality;   // keep the saved recorded key
    populateKeyControls();
    sourceTracks = styleFile.sourceTracks;
    rebuildTrackNttList();       // Phase 7a: per-track NTT cells from the loaded style
    windows = styleFile.sections;
    if (windows.empty())
        windows = ArrangerDefaults::defaultWindowsForBars (ArrangerStyleDefaults::minimumLengthBars);
    renumberSectionsByType();   // ensure unique per-type names (older files had them all as "<type> 1")

    recomputeTotalBars();
    timeline.setTotalBars (totalBars);
    timeline.setWindows (windows);
    layoutTimeline();
    rebuildPreview();
    restIdlePlayhead();   // park the arrow at the first section's start, not bar 1
}

ArrangerStyleFile ArrangerStyleEditor::toStyleFile() const
{
    ArrangerStyleFile styleFile;
    styleFile.id = styleId; styleFile.name = name;
    styleFile.originalTempo = referenceBpm;
    styleFile.timeSigNum = timeSigNum; styleFile.timeSigDenom = timeSigDenom;
    styleFile.originalRoot = originalRoot; styleFile.originalQuality = originalQuality;
    styleFile.sourceTracks = sourceTracks;
    styleFile.sections = windows;
    return styleFile;
}

void ArrangerStyleEditor::autoDetectOriginalChord()
{
    const ArrangerChord detectedKey = detectKeyFromEvents (pitchedEvents (sourceTracks));   // C major fallback on empty/ambiguous
    originalRoot    = detectedKey.root;
    originalQuality = detectedKey.quality;
}

void ArrangerStyleEditor::populateKeyControls()
{
    if (keyRootBox.getNumItems() == noItems)   // first call: fill the lists
    {
        for (int pitchClass {}; pitchClass < MusicFacts::semitonesPerOctave; ++pitchClass)
            keyRootBox.addItem (Text::pitchClassNames[pitchClass], pitchClass + comboIdOffset);   // id = root + 1 (ComboBox ids must be != 0)
        for (auto quality : { ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dom7, ChordQuality::Maj7,
                              ChordQuality::Min7, ChordQuality::Dim, ChordQuality::HalfDim, ChordQuality::Aug,
                              ChordQuality::Sus2, ChordQuality::Sus4 })
            keyQualityBox.addItem (toString (quality), (int) quality);   // id = enum value (Maj=1..)
    }

    keyRootBox.setSelectedId (juce::jlimit (PitchClasses::lowest, highestPitchClass, originalRoot) + comboIdOffset, juce::dontSendNotification);
    keyQualityBox.setSelectedId ((int) originalQuality, juce::dontSendNotification);
}

void ArrangerStyleEditor::rebuildTrackNttList()
{
    nttTrackCombos.clear();
    nttTrackNameLabels.clear();

    for (int trackIndex {}; trackIndex < (int) sourceTracks.size(); ++trackIndex)
    {
        const auto& sourceTrack = sourceTracks[(size_t) trackIndex];

        auto* trackNameLabel = new juce::Label ({}, sourceTrack.name.isNotEmpty()
                                                     ? sourceTrack.name
                                                     : (Text::trackPrefix + juce::String (trackIndex + numberStep)));
        trackNameLabel->setColour (juce::Label::textColourId, juce::Colours::white);
        trackNameLabel->setJustificationType (juce::Justification::centredLeft);
        trackNameLabel->setMinimumHorizontalScale (minimumNameScale);
        nttTrackContent.addAndMakeVisible (trackNameLabel);
        nttTrackNameLabels.add (trackNameLabel);

        auto* combo = new juce::ComboBox();
        combo->addItem (Text::nttNoTranspose, (int) NttType::NoTranspose + comboIdOffset);   // id = NttType + 1
        combo->addItem (Text::nttParallel,    (int) NttType::Parallel    + comboIdOffset);
        combo->addItem (Text::nttChord,       (int) NttType::Chord       + comboIdOffset);
        combo->addItem (Text::nttFixed,       (int) NttType::Fixed       + comboIdOffset);
        combo->setSelectedId ((int) sourceTrack.nttType + comboIdOffset, juce::dontSendNotification);
        combo->onChange = [this, combo, trackIndex]
        {
            if (trackIndex < (int) sourceTracks.size())
            {
                sourceTracks[(size_t) trackIndex].nttType = (NttType) (combo->getSelectedId() - comboIdOffset);
                rebuildPreview();   // keep the live preview in sync, like a region/key edit
            }
        };
        nttTrackContent.addAndMakeVisible (combo);
        nttTrackCombos.add (combo);
    }

    layoutTrackNttCells();
}

void ArrangerStyleEditor::layoutTrackNttCells()
{
    const int rowHeight = nttNameHeight + nttComboHeight + nttCellPadding;
    const int cellCount = nttTrackCombos.size();

    nttTrackContent.setSize (juce::jmax (nttTrackViewport.getWidth(), cellCount * (nttCellWidth + nttCellGap) + nttCellGap),
                             juce::jmax (rowHeight, nttTrackViewport.getHeight() - nttScrollBarRoom));

    int cellX = nttCellGap;
    for (int cellIndex {}; cellIndex < cellCount; ++cellIndex)
    {
        if (auto* trackNameLabel = nttTrackNameLabels[cellIndex]) trackNameLabel->setBounds (cellX, nttCellPadding, nttCellWidth, nttNameHeight);
        if (auto* combo = nttTrackCombos[cellIndex])              combo->setBounds (cellX, nttCellPadding + nttNameHeight, nttCellWidth, nttComboHeight);
        cellX += nttCellWidth + nttCellGap;
    }
}

ArrangerStyle ArrangerStyleEditor::buildStyle() const
{
    ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromWindows (
        sourceTracks, windows, timeSigNum, timeSigDenom, referenceBpm);
    style.originalRoot    = originalRoot;      // so the preview engine transposes from the right key
    style.originalQuality = originalQuality;
    return style;
}

void ArrangerStyleEditor::rebuildPreview()
{
    engine.setStyle (buildStyle());
}

int ArrangerStyleEditor::firstSectionStartBar() const
{
    int bar = noBar;
    for (const auto& window : windows)
        if (bar < int {} || window.startBar < bar)
            bar = window.startBar;
    return bar < int {} ? ArrangerStyleDefaults::firstBar : bar;
}

void ArrangerStyleEditor::restIdlePlayhead()
{
    // While stopped, the playhead has no live position to report, so park it at the start of the first
    // section (Korg-style) instead of leaving it at bar 1 -- which may now hold no section at all. During
    // preview the engine drives the arrow via onElapsedBeats, so don't fight it.
    if (! engine.isPlaying())
        timeline.setPlayheadBar ((double) firstSectionStartBar());
}

void ArrangerStyleEditor::setBusy (bool busy, const juce::String& text)
{
    if (busy)
    {
        busyOverlay.setText (text, juce::dontSendNotification);
        busyOverlay.setBounds (getLocalBounds());
        busyOverlay.setVisible (true);
        busyOverlay.toFront (false);
    }
    else
    {
        busyOverlay.setVisible (false);
    }

    // Gate the controls so nothing can be triggered (or the editor closed) mid-operation.
    for (auto* button : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                          &removeBtn, &previewBtn, &stopBtn, &saveBtn, &closeBtn })
        button->setEnabled (! busy);
    updateTracksBtn.setEnabled (! busy && loadedFile.existsAsFile());
}

void ArrangerStyleEditor::addSectionOfType (ArrangerSectionType type)
{
    SectionWindow window;
    window.id = juce::Uuid().toString();
    window.type = type;
    // Drop the new section where the user is currently looking (the bar at the timeline viewport's
    // left edge), not back at bar 1. Clamp to the valid range, and trim its length so it stays in bounds.
    const int viewBar = timeline.barForX (timelineViewport.getViewPositionX());
    window.startBar = juce::jlimit (ArrangerStyleDefaults::firstBar, juce::jmax (ArrangerStyleDefaults::firstBar, totalBars), viewBar);
    const int desiredLen = (type == ArrangerSectionType::Fill ? fillLengthBars : otherSectionLengthBars);
    window.lengthBars = juce::jmin (desiredLen, juce::jmax (ArrangerStyleDefaults::minimumLengthBars, totalBars - window.startBar + lastBarOffset));
    window.afterComplete = (type == ArrangerSectionType::Ending) ? ArrangerAfterComplete::Stop
                                                                 : ArrangerAfterComplete::FallThrough;

    // Unique, stable per-type name (max existing + 1) so the section is identifiable and the engine
    // can jump to this exact one (it resolves by type + name). Stable across removals (gaps are fine).
    int highestNumber {};
    for (const auto& existing : windows)
        if (existing.type == type)
            highestNumber = juce::jmax (highestNumber, existing.name.fromLastOccurrenceOf (Text::nameSeparator, includeSeparator, ignoreCase).getIntValue());
    window.name = ArrangerEnums::toString (type) + Text::nameSeparator + juce::String (highestNumber + numberStep);

    windows.push_back (window);
    timeline.setWindows (windows);
    rebuildPreview();
}

void ArrangerStyleEditor::updateTracksFromRecording()
{
    // Replace the configuration's recorded notes with the app's current recording while keeping the
    // sections (Intro/Variation/Fill/...) exactly as they are, then overwrite the saved .style.
    if (! loadedFile.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon, Text::updateTracksTitle, Text::saveFirst);
        return;
    }
    if (! onRequestCurrentTracks)
        return;

    juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
        Text::updateTracksTitle,
        Text::replaceTracksStart + name + Text::replaceTracksEnd,
        Text::updateButton, Text::cancelButton, this,
        juce::ModalCallbackFunction::create ([this] (int result)
        {
            if (result != dialogConfirmed)   // 1 = Update, 0 = Cancel
                return;

            auto rawTracks = onRequestCurrentTracks();   // raw entries (with sequences), copied on the message thread
            if (rawTracks.empty())
            {
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon, Text::updateTracksTitle, Text::noTracksToImport);
                return;
            }

            setBusy (true, Text::updatingTracks);

            // Build the new source events + serialize + write entirely on a background thread (all of
            // that is the heavy part), then apply to the editor/engine back on the message thread. The
            // sections (windows) are intentionally untouched. SafePointer guards a closed editor.
            auto tracks = std::make_shared<std::vector<TrackEntry>> (std::move (rawTracks));
            auto file   = std::make_shared<ArrangerStyleFile> (toStyleFile());   // metadata + current windows
            const juce::File target = loadedFile;
            const double      refBpm = referenceBpm;
            juce::Component::SafePointer<ArrangerStyleEditor> safe (this);

            juce::Thread::launch ([safe, tracks, file, target, refBpm]
            {
                file->sourceTracks = ArrangerSourceBuilder::fromTrackEntries (*tracks, refBpm);

                // Re-detect the recorded key from the new pitched tracks (off the message thread).
                const ArrangerChord detectedKey = detectKeyFromEvents (pitchedEvents (file->sourceTracks));
                file->originalRoot = detectedKey.root; file->originalQuality = detectedKey.quality;

                ArrangerStyleIOHelper::saveToFile (target, *file);
                const bool written = target.existsAsFile();
                auto applied = std::make_shared<std::vector<SourceTrackFile>> (file->sourceTracks);

                juce::MessageManager::callAsync ([safe, applied, file, target, written]
                {
                    if (safe == nullptr)
                        return;
                    safe->setBusy (false);
                    if (! written)
                    {
                        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                            Text::updateTracksTitle, Text::failedToWrite + target.getFullPathName());
                        return;
                    }
                    safe->sourceTracks = std::move (*applied);
                    safe->rebuildTrackNttList();   // Phase 7a: refresh per-track NTT cells for the new tracks
                    safe->originalRoot = file->originalRoot;        // keep the re-detected key
                    safe->originalQuality = file->originalQuality;
                    safe->populateKeyControls();
                    safe->recomputeTotalBars();
                    safe->timeline.setTotalBars (safe->totalBars);
                    safe->timeline.setWindows (safe->windows);
                    safe->layoutTimeline();
                    safe->rebuildPreview();
                    if (safe->onSaved) safe->onSaved (target, safe->buildStyle());
                    juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                        Text::updateTracksTitle, Text::tracksUpdated + target.getFileName());
                });
            });
        }));
}

void ArrangerStyleEditor::renumberSectionsByType()
{
    // Give every section a unique per-type number in list order (fixes legacy files where they were
    // all "<type> 1" and so couldn't be told apart or individually targeted).
    for (size_t windowIndex {}; windowIndex < windows.size(); ++windowIndex)
    {
        int sameTypeCount {};
        for (size_t earlierIndex {}; earlierIndex <= windowIndex; ++earlierIndex)
            if (windows[earlierIndex].type == windows[windowIndex].type)
                ++sameTypeCount;
        windows[windowIndex].name = ArrangerEnums::toString (windows[windowIndex].type) + Text::nameSeparator + juce::String (sameTypeCount);
    }
}

void ArrangerStyleEditor::removeSelectedSection()
{
    const int selected = timeline.getSelectedIndex();
    if (selected < int {} || selected >= (int) windows.size())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon, Text::removeSectionTitle, Text::selectSectionFirst);
        return;
    }

    windows.erase (windows.begin() + selected);
    recomputeTotalBars();
    timeline.setTotalBars (totalBars);
    timeline.setWindows (windows);   // also clears the now-stale selection
    layoutTimeline();
    rebuildPreview();
}

void ArrangerStyleEditor::requestSave()
{
    name = nameEditor.getText().trim();
    if (name.isEmpty()) name = Text::defaultConfigName;

    auto folder = IOHelper::getArrangerStylesFolder();
    auto target = folder.getChildFile (juce::File::createLegalFileName (name) + Text::styleExtension);

    // A *different* existing config would be clobbered: confirm before overwriting it.
    if (target.existsAsFile() && target != loadedFile)
    {
        juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
            Text::saveTitle,
            Text::overwriteStart + target.getFileNameWithoutExtension() + Text::overwriteEnd,
            Text::overwriteButton, Text::cancelButton, this,
            juce::ModalCallbackFunction::create ([this, target] (int result)
            {
                if (result == dialogConfirmed)            // 1 = Overwrite, 0 = Cancel
                    finishSave (target);
            }));
        return;
    }

    finishSave (target);
}

void ArrangerStyleEditor::finishSave (const juce::File& target)
{
    setBusy (true, Text::savingConfiguration);

    // Snapshot the editor state on the message thread, then serialize + write on a background thread
    // (the hex-encoding of every event + disk write is what froze the UI). Come back to the message
    // thread to update state + show the result. SafePointer guards against the editor being gone.
    auto data = std::make_shared<ArrangerStyleFile> (toStyleFile());
    const bool   willRename = loadedFile.existsAsFile() && loadedFile != target;
    const juce::File oldFile = loadedFile;
    juce::Component::SafePointer<ArrangerStyleEditor> safe (this);

    juce::Thread::launch ([safe, target, data, willRename, oldFile]
    {
        ArrangerStyleIOHelper::saveToFile (target, *data);
        const bool written = target.existsAsFile();
        if (written && willRename)
            oldFile.deleteFile();   // opened from a different file: don't leave a duplicate

        juce::MessageManager::callAsync ([safe, target, written, willRename]
        {
            if (safe == nullptr)
                return;
            safe->setBusy (false);
            if (! written)
            {
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                    Text::saveTitle, Text::failedToWrite + target.getFullPathName());
                return;
            }
            safe->loadedFile = target;
            safe->updateTracksBtn.setEnabled (true);   // there is now a file to update
            if (safe->onSaved) safe->onSaved (target, safe->buildStyle());
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                Text::saveTitle, (willRename ? Text::renamedTo : Text::savedAs) + target.getFileName());
        });
    });
}

void ArrangerStyleEditor::paint (juce::Graphics& g)
{
    g.fillAll (AppColours::styleEditorBackground);
    g.setColour (AppColours::styleEditorBorder);
    g.drawRect (getLocalBounds(), borderThickness);
}

void ArrangerStyleEditor::resized()
{
    auto area = getLocalBounds().reduced (editorMargin);

    auto top = area.removeFromTop (topRowHeight);
    nameLabel.setBounds (top.removeFromLeft (nameLabelWidth));
    nameEditor.setBounds (top.removeFromLeft (nameEditorWidth).reduced (noInsetX, fieldInsetY));
    top.removeFromLeft (afterNameGap);
    for (auto* button : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                          &removeBtn, &previewBtn, &stopBtn, &updateTracksBtn, &saveBtn, &closeBtn })
        button->setBounds (top.removeFromLeft (buttonWidth).reduced (buttonInset));

    area.removeFromTop (rowGap);
    auto keyRow = area.removeFromTop (keyRowHeight);                    // second row: recorded-key picker
    keyLabel.setBounds (keyRow.removeFromLeft (keyLabelWidth));
    keyRootBox.setBounds (keyRow.removeFromLeft (keyRootWidth).reduced (noInsetX, fieldInsetY));
    keyRow.removeFromLeft (rowGap);
    keyQualityBox.setBounds (keyRow.removeFromLeft (keyQualityWidth).reduced (noInsetX, fieldInsetY));

    // Phase 7a: per-track NTT strip along the bottom (header row + a horizontally-scrolling cell row),
    // so the timeline keeps its full width and only gives up a little height.
    {
        auto strip = area.removeFromBottom (nttStripHeight);
        nttHeaderLabel.setBounds (strip.removeFromTop (nttHeaderHeight));
        nttTrackViewport.setBounds (strip);
        layoutTrackNttCells();
    }

    area.removeFromBottom (rowGap);
    timelineViewport.setBounds (area);
    layoutTimeline();

    if (busyOverlay.isVisible())
        busyOverlay.setBounds (getLocalBounds());
}

void ArrangerStyleEditor::scrollBarMoved (juce::ScrollBar*, double)
{
    // A scroll the user initiated (not our auto-follow) suspends following so they can navigate
    // freely to a far section. Pressing Preview re-arms it.
    if (! programmaticScroll)
        followPlayhead = false;
}

void ArrangerStyleEditor::layoutTimeline()
{
    const int timelineHeight = juce::jmax (minimumTimelineHeight, timelineViewport.getHeight() - timelineScrollBarRoom); // leave room for the scrollbar
    // Fill the viewport when there are few bars; grow + scroll when there are many.
    const int timelineWidth = juce::jmax (timeline.getPreferredWidth(), timelineViewport.getWidth());
    timeline.setSize (timelineWidth, timelineHeight);
}
