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

    // Auto-follow: scroll when the playhead comes this close to a viewport edge.
    constexpr int followEdgeMargin = 48;
    constexpr int halfViewDivisor  = 2;
    constexpr int viewTop          = 0;
    constexpr int noScroll         = 0;

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
    timelineViewport.getHorizontalScrollBar().addListener (this);  // detect manual scroll
    addAndMakeVisible (timelineViewport);
    nameEditor.setText (Text::defaultConfigName, juce::dontSendNotification);

    setUpPageControls();   // the page's controls and their look (ArrangerStyleEditorPage.cpp)

    populateKeyControls();
    keyRootBox.onChange    = [this] { originalRoot = keyRootBox.getSelectedId() - comboIdOffset; rebuildPreview(); };
    keyQualityBox.onChange = [this] { originalQuality = (ChordQuality) keyQualityBox.getSelectedId(); rebuildPreview(); };

    timeline.onWindowsChanged = [this] (const std::vector<SectionWindow>& editedWindows)
    {
        windows = editedWindows;
        rebuildPreview();
        restIdlePlayhead();   // when not previewing, keep the arrow parked at the first section's start
        refreshSelection();
    };

    // Clicking a region while previewing switches to that section at the next bar line.
    timeline.onSectionSelected = [this] (int sectionIndex) { selectSection (sectionIndex); };

    addIntroBtn.onClick     = [this] { addSectionOfType (ArrangerSectionType::Intro); };
    addVariationBtn.onClick = [this] { addSectionOfType (ArrangerSectionType::Variation); };
    addFillBtn.onClick      = [this] { addSectionOfType (ArrangerSectionType::Fill); };
    addBreakBtn.onClick     = [this] { addSectionOfType (ArrangerSectionType::Break); };
    addEndingBtn.onClick    = [this] { addSectionOfType (ArrangerSectionType::Ending); };
    inspector.onRemove      = [this] { removeSelectedSection(); };
    previewBtn.onClick      = [this] { togglePreview(); };
    updateTracksBtn.onClick = [this] { updateTracksFromRecording(); };
    saveBtn.onClick         = [this] { requestSave(); };

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
    refreshLanes();              // lane headers (transpose mode) + the lanes' notes
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
    refreshSelection();
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
    refreshLanes();              // lane headers (transpose mode) + the lanes' notes
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
    refreshSelection();
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

    // Gate the controls so nothing can be triggered (or the editor closed) mid-operation. The title row and the
    // configurations list sit outside this component: they are gated too (the host asks isBusy() before Exit).
    busyNow = busy;
    for (auto* button : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                          &previewBtn, &saveBtn, &zoomOutBtn, &zoomInBtn })
        button->setEnabled (! busy);
    for (auto* control : std::initializer_list<juce::Component*> { &nameEditor, &keyRootBox, &keyQualityBox, &inspector, &laneHeaders, &jumpPads })
        control->setEnabled (! busy);
    updateTracksBtn.setEnabled (! busy && loadedFile.existsAsFile());
    if (onBusyChanged)
        onBusyChanged (busy);
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
    refreshSelection();
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
                    safe->refreshLanes();          // lane headers (transpose mode) + the lanes' notes
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
    refreshSelection();
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

void ArrangerStyleEditor::scrollBarMoved (juce::ScrollBar*, double)
{
    // A scroll the user initiated (not our auto-follow) suspends following so they can navigate
    // freely to a far section. Pressing Preview re-arms it.
    if (! programmaticScroll)
        followPlayhead = false;
}
