#include "ArrangerStyleEditor.h"
#include "ArrangerSourceBuilder.h"
#include "ArrangerDefaults.h"
#include "ArrangerPatternBuilder.h"
#include "ArrangerStyleIOHelper.h"
#include "ArrangerEnums.h"
#include "ArrangerTime.h"
#include "ChordDetector.h"
#include "IOHelper.h"
#include <cmath>

ArrangerStyleEditor::ArrangerStyleEditor (ArrangerEngine& e) : engine (e)
{
    timelineViewport.setViewedComponent (&timeline, false);
    timelineViewport.setScrollBarsShown (false, true);   // horizontal only
    timelineViewport.setScrollBarThickness (12);
    timelineViewport.getHorizontalScrollBar().addListener (this);  // detect manual scroll
    addAndMakeVisible (timelineViewport);
    addAndMakeVisible (nameLabel);
    addAndMakeVisible (nameEditor);
    nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    nameEditor.setText ("New configuration", juce::dontSendNotification);
    for (auto* b : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                     &removeBtn, &previewBtn, &stopBtn, &updateTracksBtn, &saveBtn, &closeBtn })
        addAndMakeVisible (b);

    updateTracksBtn.setEnabled (false);   // only meaningful when editing an existing saved config

    addAndMakeVisible (keyLabel);
    addAndMakeVisible (keyRootBox);
    addAndMakeVisible (keyQualityBox);
    keyLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    populateKeyControls();
    keyRootBox.onChange    = [this] { originalRoot = keyRootBox.getSelectedId() - 1; rebuildPreview(); };
    keyQualityBox.onChange = [this] { originalQuality = (ChordQuality) keyQualityBox.getSelectedId(); rebuildPreview(); };

    // Phase 7a: per-track NTT selector strip (scrolls horizontally if there are many tracks).
    nttHeaderLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (nttHeaderLabel);
    nttTrackViewport.setViewedComponent (&nttTrackContent, false);
    nttTrackViewport.setScrollBarsShown (false, true);   // horizontal only
    nttTrackViewport.setScrollBarThickness (10);
    addAndMakeVisible (nttTrackViewport);

    // Full-screen "working" overlay, styled like the app's "Preparing style..." overlay. Eats mouse
    // clicks so nothing behind it can be triggered while a save/update runs on the background thread.
    busyOverlay.setJustificationType (juce::Justification::centred);
    busyOverlay.setFont (juce::Font (24.0f, juce::Font::bold));
    busyOverlay.setColour (juce::Label::textColourId, juce::Colours::white);
    busyOverlay.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (0.7f));
    busyOverlay.setInterceptsMouseClicks (true, true);
    addChildComponent (busyOverlay);   // hidden until setBusy(true)

    timeline.onWindowsChanged = [this] (const std::vector<SectionWindow>& w)
    {
        windows = w;
        rebuildPreview();
        restIdlePlayhead();   // when not previewing, keep the arrow parked at the first section's start
    };

    // Clicking a region while previewing switches to that section at the next bar line.
    timeline.onSectionSelected = [this] (int idx)
    {
        if (engine.isPlaying() && idx >= 0 && idx < (int) windows.size())
            engine.queueSection (windows[(size_t) idx].type, windows[(size_t) idx].name);
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
        const double bpb = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);

        // Show the arrow at the *active section's* position, not the global monotonic playhead, so a
        // jump (e.g. to the Ending) moves the arrow into that section's bars. A looping section wraps
        // its local beats within its own length so the arrow loops inside its window.
        const int    secIdx     = engine.getActiveSectionIndex();
        const double localBeats = juce::jmax (0.0, engine.getActiveSectionLocalBeats());
        double bar;
        if (secIdx >= 0 && secIdx < (int) windows.size())
        {
            const auto&  w          = windows[(size_t) secIdx];
            const double secLenBeats = (double) juce::jmax (1, w.lengthBars) * bpb;
            const double wrapped     = secLenBeats > 0.0 ? std::fmod (localBeats, secLenBeats) : 0.0;
            bar = (double) w.startBar + wrapped / juce::jmax (1.0, bpb);
        }
        else
        {
            bar = 1.0 + localBeats / juce::jmax (1.0, bpb);
        }
        timeline.setPlayheadBar (bar);

        // Auto-follow: keep the playhead in view, scrolling the viewport when it nears an edge.
        // Suspended once the user scrolls manually (so they can navigate to a far section); press
        // Preview to re-arm it.
        const int px    = timeline.xForBar (bar);
        const int viewX = timelineViewport.getViewPositionX();
        const int vw    = timelineViewport.getViewWidth();
        if (followPlayhead && vw > 0 && (px < viewX + 48 || px > viewX + vw - 48))
        {
            const int maxX = juce::jmax (0, timeline.getWidth() - vw);
            programmaticScroll = true;   // our own scroll: don't treat it as a manual one
            timelineViewport.setViewPosition (juce::jlimit (0, maxX, px - vw / 2), 0);
            programmaticScroll = false;
        }
    };
}

void ArrangerStyleEditor::recomputeTotalBars()
{
    const double bpb = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);
    double lastBeat = 0.0;
    for (const auto& st : sourceTracks)
        if (! st.events.empty())
            lastBeat = juce::jmax (lastBeat, st.events.back().beats);

    int bars = juce::jmax (1, ArrangerTime::barsForBeats (lastBeat, bpb));
    for (const auto& w : windows)
        bars = juce::jmax (bars, w.startBar + w.lengthBars - 1);
    totalBars = juce::jmax (1, bars);
}

void ArrangerStyleEditor::loadRecording (const std::vector<TrackEntry>& tracks, double refBpm,
                                         int tsNum, int tsDenom, const juce::String& styleName)
{
    referenceBpm = (refBpm > 0.0 ? refBpm : 120.0);
    timeSigNum = tsNum; timeSigDenom = tsDenom;
    name = styleName.isNotEmpty() ? styleName : "New configuration";
    nameEditor.setText (name, juce::dontSendNotification);
    styleId = juce::Uuid().toString();

    updateTracksBtn.setEnabled (false);   // brand-new config: nothing on disk to update yet

    sourceTracks = ArrangerSourceBuilder::fromTrackEntries (tracks, referenceBpm);
    rebuildTrackNttList();       // Phase 7a: per-track NTT cells for the fresh tracks
    autoDetectOriginalChord();   // guess the recorded key from the fresh recording
    populateKeyControls();
    windows = ArrangerDefaults::defaultWindowsForBars (1);
    recomputeTotalBars();
    windows = ArrangerDefaults::defaultWindowsForBars (totalBars);

    timeline.setTotalBars (totalBars);
    timeline.setWindows (windows);
    layoutTimeline();
    rebuildPreview();
    restIdlePlayhead();   // park the arrow at the first section's start, not bar 1
}

void ArrangerStyleEditor::loadFromFile (const ArrangerStyleFile& f)
{
    styleId = f.id.isNotEmpty() ? f.id : juce::Uuid().toString();
    name = f.name.isNotEmpty() ? f.name : "Style";
    nameEditor.setText (name, juce::dontSendNotification);
    referenceBpm = (f.originalTempo > 0.0 ? f.originalTempo : 120.0);
    timeSigNum = f.timeSigNum; timeSigDenom = f.timeSigDenom;
    originalRoot = f.originalRoot; originalQuality = f.originalQuality;   // keep the saved recorded key
    populateKeyControls();
    sourceTracks = f.sourceTracks;
    rebuildTrackNttList();       // Phase 7a: per-track NTT cells from the loaded style
    windows = f.sections;
    if (windows.empty())
        windows = ArrangerDefaults::defaultWindowsForBars (1);
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
    ArrangerStyleFile f;
    f.id = styleId; f.name = name;
    f.originalTempo = referenceBpm;
    f.timeSigNum = timeSigNum; f.timeSigDenom = timeSigDenom;
    f.originalRoot = originalRoot; f.originalQuality = originalQuality;
    f.sourceTracks = sourceTracks;
    f.sections = windows;
    return f;
}

void ArrangerStyleEditor::autoDetectOriginalChord()
{
    // Detect the recorded key from the pitched (Bass/Acc) tracks only — drums/perc carry no harmony.
    std::vector<TimedBeatEvent> pitched;
    for (const auto& t : sourceTracks)
        if (t.partType == ArrangerPartType::Bass || t.partType == ArrangerPartType::Acc)
            for (const auto& e : t.events)
                pitched.push_back (e);

    const ArrangerChord k = detectKeyFromEvents (pitched);   // C major fallback on empty/ambiguous
    originalRoot    = k.root;
    originalQuality = k.quality;
}

void ArrangerStyleEditor::populateKeyControls()
{
    static const char* const noteNames[12] =
        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    if (keyRootBox.getNumItems() == 0)   // first call: fill the lists
    {
        for (int i = 0; i < 12; ++i)
            keyRootBox.addItem (noteNames[i], i + 1);   // id = root + 1 (ComboBox ids must be != 0)
        for (auto q : { ChordQuality::Maj, ChordQuality::Min, ChordQuality::Dom7, ChordQuality::Maj7,
                        ChordQuality::Min7, ChordQuality::Dim, ChordQuality::HalfDim, ChordQuality::Aug,
                        ChordQuality::Sus2, ChordQuality::Sus4 })
            keyQualityBox.addItem (toString (q), (int) q);   // id = enum value (Maj=1..)
    }

    keyRootBox.setSelectedId (juce::jlimit (0, 11, originalRoot) + 1, juce::dontSendNotification);
    keyQualityBox.setSelectedId ((int) originalQuality, juce::dontSendNotification);
}

void ArrangerStyleEditor::rebuildTrackNttList()
{
    nttTrackCombos.clear();
    nttTrackNameLabels.clear();

    for (int i = 0; i < (int) sourceTracks.size(); ++i)
    {
        const auto& st = sourceTracks[(size_t) i];

        auto* nameLbl = new juce::Label ({}, st.name.isNotEmpty() ? st.name : ("Track " + juce::String (i + 1)));
        nameLbl->setColour (juce::Label::textColourId, juce::Colours::white);
        nameLbl->setJustificationType (juce::Justification::centredLeft);
        nameLbl->setMinimumHorizontalScale (0.7f);
        nttTrackContent.addAndMakeVisible (nameLbl);
        nttTrackNameLabels.add (nameLbl);

        auto* combo = new juce::ComboBox();
        combo->addItem ("No Transpose", 1);   // id = NttType + 1
        combo->addItem ("Parallel",     2);
        combo->addItem ("Chord",        3);
        combo->addItem ("Fixed",        4);
        combo->setSelectedId ((int) st.nttType + 1, juce::dontSendNotification);
        const int trackIndex = i;
        combo->onChange = [this, combo, trackIndex]
        {
            if (trackIndex < (int) sourceTracks.size())
            {
                sourceTracks[(size_t) trackIndex].nttType = (NttType) (combo->getSelectedId() - 1);
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
    const int cellW = 150, gap = 8, nameH = 18, comboH = 24, pad = 4;
    const int rowH  = nameH + comboH + pad;
    const int n     = nttTrackCombos.size();

    nttTrackContent.setSize (juce::jmax (nttTrackViewport.getWidth(), n * (cellW + gap) + gap),
                             juce::jmax (rowH, nttTrackViewport.getHeight() - 12));

    int x = gap;
    for (int i = 0; i < n; ++i)
    {
        if (auto* lbl = nttTrackNameLabels[i]) lbl->setBounds (x, pad, cellW, nameH);
        if (auto* cb  = nttTrackCombos[i])     cb->setBounds  (x, pad + nameH, cellW, comboH);
        x += cellW + gap;
    }
}

ArrangerStyle ArrangerStyleEditor::buildStyle() const
{
    ArrangerStyle s = ArrangerPatternBuilder::buildStyleFromWindows (
        sourceTracks, windows, timeSigNum, timeSigDenom, referenceBpm);
    s.originalRoot    = originalRoot;      // so the preview engine transposes from the right key
    s.originalQuality = originalQuality;
    return s;
}

void ArrangerStyleEditor::rebuildPreview()
{
    engine.setStyle (buildStyle());
}

int ArrangerStyleEditor::firstSectionStartBar() const
{
    int bar = -1;
    for (const auto& w : windows)
        if (bar < 0 || w.startBar < bar)
            bar = w.startBar;
    return bar < 0 ? 1 : bar;
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
    for (auto* b : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                     &removeBtn, &previewBtn, &stopBtn, &saveBtn, &closeBtn })
        b->setEnabled (! busy);
    updateTracksBtn.setEnabled (! busy && loadedFile.existsAsFile());
}

void ArrangerStyleEditor::addSectionOfType (ArrangerSectionType type)
{
    SectionWindow w;
    w.id = juce::Uuid().toString();
    w.type = type;
    // Drop the new section where the user is currently looking (the bar at the timeline viewport's
    // left edge), not back at bar 1. Clamp to the valid range, and trim its length so it stays in bounds.
    const int viewBar = timeline.barForX (timelineViewport.getViewPositionX());
    w.startBar = juce::jlimit (1, juce::jmax (1, totalBars), viewBar);
    const int desiredLen = (type == ArrangerSectionType::Fill ? 1 : 2);
    w.lengthBars = juce::jmin (desiredLen, juce::jmax (1, totalBars - w.startBar + 1));
    w.afterComplete = (type == ArrangerSectionType::Ending) ? ArrangerAfterComplete::Stop
                                                            : ArrangerAfterComplete::FallThrough;

    // Unique, stable per-type name (max existing + 1) so the section is identifiable and the engine
    // can jump to this exact one (it resolves by type + name). Stable across removals (gaps are fine).
    int maxN = 0;
    for (const auto& existing : windows)
        if (existing.type == type)
            maxN = juce::jmax (maxN, existing.name.fromLastOccurrenceOf (" ", false, false).getIntValue());
    w.name = ArrangerEnums::toString (type) + " " + juce::String (maxN + 1);

    windows.push_back (w);
    timeline.setWindows (windows);
    rebuildPreview();
}

void ArrangerStyleEditor::updateTracksFromRecording()
{
    // Replace the configuration's recorded notes with the app's current recording while keeping the
    // sections (Intro/Variation/Fill/...) exactly as they are, then overwrite the saved .style.
    if (! loadedFile.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
            "Update tracks", "Save this configuration first, then Update Tracks can replace its notes.");
        return;
    }
    if (! onRequestCurrentTracks)
        return;

    juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
        "Update tracks",
        "Replace the tracks of \"" + name + "\" with the current recording and overwrite the saved "
        "configuration?\nYour sections (intros/variations/fills) stay as they are.",
        "Update", "Cancel", this,
        juce::ModalCallbackFunction::create ([this] (int result)
        {
            if (result != 1)   // 1 = Update, 0 = Cancel
                return;

            auto rawTracks = onRequestCurrentTracks();   // raw entries (with sequences), copied on the message thread
            if (rawTracks.empty())
            {
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                    "Update tracks", "No recorded tracks to import.");
                return;
            }

            setBusy (true, "Updating tracks...");

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
                {
                    std::vector<TimedBeatEvent> pitched;
                    for (const auto& t : file->sourceTracks)
                        if (t.partType == ArrangerPartType::Bass || t.partType == ArrangerPartType::Acc)
                            for (const auto& e : t.events) pitched.push_back (e);
                    const ArrangerChord k = detectKeyFromEvents (pitched);
                    file->originalRoot = k.root; file->originalQuality = k.quality;
                }

                ArrangerStyleIOHelper::saveToFile (target, *file);
                const bool ok = target.existsAsFile();
                auto applied = std::make_shared<std::vector<SourceTrackFile>> (file->sourceTracks);

                juce::MessageManager::callAsync ([safe, applied, file, target, ok]
                {
                    if (safe == nullptr)
                        return;
                    safe->setBusy (false);
                    if (! ok)
                    {
                        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                            "Update tracks", "Failed to write " + target.getFullPathName());
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
                        "Update tracks", "Tracks updated: " + target.getFileName());
                });
            });
        }));
}

void ArrangerStyleEditor::renumberSectionsByType()
{
    // Give every section a unique per-type number in list order (fixes legacy files where they were
    // all "<type> 1" and so couldn't be told apart or individually targeted).
    for (size_t i = 0; i < windows.size(); ++i)
    {
        int n = 0;
        for (size_t j = 0; j <= i; ++j)
            if (windows[j].type == windows[i].type)
                ++n;
        windows[i].name = ArrangerEnums::toString (windows[i].type) + " " + juce::String (n);
    }
}

void ArrangerStyleEditor::removeSelectedSection()
{
    const int idx = timeline.getSelectedIndex();
    if (idx < 0 || idx >= (int) windows.size())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
            "Remove section", "Click a section on the timeline to select it first.");
        return;
    }

    windows.erase (windows.begin() + idx);
    recomputeTotalBars();
    timeline.setTotalBars (totalBars);
    timeline.setWindows (windows);   // also clears the now-stale selection
    layoutTimeline();
    rebuildPreview();
}

void ArrangerStyleEditor::requestSave()
{
    name = nameEditor.getText().trim();
    if (name.isEmpty()) name = "New configuration";

    auto folder = IOHelper::getArrangerStylesFolder();
    auto target = folder.getChildFile (juce::File::createLegalFileName (name) + ".style");

    // A *different* existing config would be clobbered: confirm before overwriting it.
    if (target.existsAsFile() && target != loadedFile)
    {
        juce::AlertWindow::showOkCancelBox (juce::AlertWindow::WarningIcon,
            "Save configuration",
            "A configuration named \"" + target.getFileNameWithoutExtension()
                + "\" already exists.\nOverwrite it?",
            "Overwrite", "Cancel", this,
            juce::ModalCallbackFunction::create ([this, target] (int result)
            {
                if (result == 1)            // 1 = Overwrite, 0 = Cancel
                    finishSave (target);
            }));
        return;
    }

    finishSave (target);
}

void ArrangerStyleEditor::finishSave (const juce::File& target)
{
    setBusy (true, "Saving configuration...");

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
        const bool ok = target.existsAsFile();
        if (ok && willRename)
            oldFile.deleteFile();   // opened from a different file: don't leave a duplicate

        juce::MessageManager::callAsync ([safe, target, ok, willRename]
        {
            if (safe == nullptr)
                return;
            safe->setBusy (false);
            if (! ok)
            {
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon,
                    "Save configuration", "Failed to write " + target.getFullPathName());
                return;
            }
            safe->loadedFile = target;
            safe->updateTracksBtn.setEnabled (true);   // there is now a file to update
            if (safe->onSaved) safe->onSaved (target, safe->buildStyle());
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
                "Save configuration", (willRename ? "Renamed to: " : "Saved: ") + target.getFileName());
        });
    });
}

void ArrangerStyleEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff121212));
    g.setColour (juce::Colour (0xff2a2a2a));
    g.drawRect (getLocalBounds(), 2);
}

void ArrangerStyleEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    auto top = area.removeFromTop (30);
    nameLabel.setBounds (top.removeFromLeft (50));
    nameEditor.setBounds (top.removeFromLeft (180).reduced (0, 2));
    top.removeFromLeft (8);
    for (auto* b : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                     &removeBtn, &previewBtn, &stopBtn, &updateTracksBtn, &saveBtn, &closeBtn })
        b->setBounds (top.removeFromLeft (84).reduced (2));

    area.removeFromTop (6);
    auto keyRow = area.removeFromTop (26);                        // second row: recorded-key picker
    keyLabel.setBounds (keyRow.removeFromLeft (90));
    keyRootBox.setBounds (keyRow.removeFromLeft (60).reduced (0, 2));
    keyRow.removeFromLeft (6);
    keyQualityBox.setBounds (keyRow.removeFromLeft (90).reduced (0, 2));

    // Phase 7a: per-track NTT strip along the bottom (header row + a horizontally-scrolling cell row),
    // so the timeline keeps its full width and only gives up a little height.
    {
        auto strip = area.removeFromBottom (78);
        nttHeaderLabel.setBounds (strip.removeFromTop (20));
        nttTrackViewport.setBounds (strip);
        layoutTrackNttCells();
    }

    area.removeFromBottom (6);
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
    const int h = juce::jmax (40, timelineViewport.getHeight() - 14); // leave room for the scrollbar
    // Fill the viewport when there are few bars; grow + scroll when there are many.
    const int w = juce::jmax (timeline.getPreferredWidth(), timelineViewport.getWidth());
    timeline.setSize (w, h);
}
