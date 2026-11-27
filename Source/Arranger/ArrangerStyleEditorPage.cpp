/*
  ==============================================================================

    ArrangerStyleEditorPage.cpp

    The style editor as the Sections page (board PageSections, spec 2026-11-27
    sections v2): its controls in the Stage materials, their layout, the lanes'
    headers, the inspector, the jump pads, Preview and zoom. The editing logic
    (load, save, update tracks, add / remove, preview build) stays in
    ArrangerStyleEditor.cpp. Split to keep both under the file cap (ADR-003).

  ==============================================================================
*/

#include "ArrangerStyleEditor.h"
#include "ArrangerTime.h"
#include "AppColours.h"
#include "UI/Sections/SectionsPageLayout.h"
#include "UI/Sections/SectionEditModel.h"
#include "UI/Stage/StageFonts.h"

namespace
{
    // Board px (PageSections).
    constexpr int   nameFieldWidth     = 130;
    constexpr int   keyRootWidth       = 60;      // room for a sharp ("C#") before the arrows
    constexpr int   keyQualityWidth    = 84;
    constexpr int   fieldHeight        = 22;
    constexpr int   scrollBarThickness = 12;    // the lanes box is 206: 194 of lanes + the scroll bar
    constexpr float busyTextSize       = 16.0f;
    constexpr float busyDimAlpha       = 0.7f;
    constexpr int   pollHz             = 8;     // Preview's label / LED and the playing pad follow the engine
    constexpr int   zoomStep           = 1;
    constexpr int   halfView           = 2;
    constexpr int   viewTop            = 0;
    constexpr int   noScroll           = 0;
    constexpr bool  semiBold           = true;
    constexpr float busySpacing        = 0.12f;
    const juce::String nameSeparator   = " ";
    constexpr bool  includeSeparator   = false;
    constexpr bool  ignoreCase         = false;

    int scaled (int boardPixels, float scale) { return juce::roundToInt (static_cast<float> (boardPixels) * scale); }
}

void ArrangerStyleEditor::setUpPageControls()
{
    for (auto* control : std::initializer_list<juce::Component*> { &laneHeaders, &jumpPads, &inspector, &addHeading,
                                                                 &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn })
        addAndMakeVisible (control);
    // The title-row controls are placed by the page frame (getTitleActions), not added here.
    for (auto* button : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                          &zoomOutBtn, &zoomInBtn, &previewBtn, &updateTracksBtn, &saveBtn })
        button->setMouseClickGrabsKeyboardFocus (false);   // the PC keyboard keeps playing after a press
    updateTracksBtn.setEnabled (false);   // only meaningful when editing an existing saved config

    auto& scrollBar = timelineViewport.getHorizontalScrollBar();
    scrollBar.setColour (juce::ScrollBar::thumbColourId, StagePalette::ruleLine);
    scrollBar.setColour (juce::ScrollBar::trackColourId, StagePalette::timelineLanes);

    laneHeaders.onModeChanged = [this] (int trackIndex, NttType mode)
    {
        if (juce::isPositiveAndBelow (trackIndex, (int) sourceTracks.size()))
        {
            sourceTracks[(size_t) trackIndex].nttType = mode;
            rebuildPreview();   // keep the live preview in sync, like a region/key edit
        }
    };
    jumpPads.onPadClicked = [this] (int padIndex)
    {
        timeline.setSelectedIndex (padIndex);
        selectSection (padIndex);
    };

    inspector.onArrow = [this] (InspectorArrow arrow)
    {
        const int selected = timeline.getSelectedIndex();
        bool changed = false;
        switch (arrow)
        {
            case InspectorArrow::typeEarlier:   changed = SectionEditModel::stepType (windows, selected, SectionEditModel::earlier); break;
            case InspectorArrow::typeLater:     changed = SectionEditModel::stepType (windows, selected, SectionEditModel::later); break;
            case InspectorArrow::startEarlier:  changed = SectionEditModel::stepStart (windows, selected, SectionEditModel::earlier, totalBars); break;
            case InspectorArrow::startLater:    changed = SectionEditModel::stepStart (windows, selected, SectionEditModel::later, totalBars); break;
            case InspectorArrow::lengthShorter: changed = SectionEditModel::stepLength (windows, selected, SectionEditModel::earlier, totalBars); break;
            case InspectorArrow::lengthLonger:  changed = SectionEditModel::stepLength (windows, selected, SectionEditModel::later, totalBars); break;
        }
        if (changed)
            windowsEdited();
    };
    inspector.onAfterComplete = [this] (ArrangerAfterComplete afterComplete)
    {
        if (SectionEditModel::setAfterComplete (windows, timeline.getSelectedIndex(), afterComplete))
            windowsEdited();
    };
    inspector.onPlayThis = [this] { playSelected(); };

    zoomOutBtn.onClick = [this] { stepZoom (-zoomStep); };
    zoomInBtn.onClick  = [this] { stepZoom (zoomStep); };

    // Full-page "working" cover: eats clicks so nothing behind it runs while a save / update is on its thread.
    busyOverlay.setJustificationType (juce::Justification::centred);
    busyOverlay.setColour (juce::Label::textColourId, StagePalette::silkscreenText);
    busyOverlay.setColour (juce::Label::backgroundColourId, juce::Colours::black.withAlpha (busyDimAlpha));
    busyOverlay.setInterceptsMouseClicks (true, true);
    addChildComponent (busyOverlay);   // hidden until setBusy(true)

    setScale (scale);
    startTimerHz (pollHz);
}

ArrangerStyleEditor::~ArrangerStyleEditor()
{
    stopTimer();
    timelineViewport.getHorizontalScrollBar().removeListener (this);
}

std::vector<juce::Component*> ArrangerStyleEditor::getTitleActions()
{
    return { &nameEditor, &keyLabel, &keyRootBox, &keyQualityBox, &zoomLabel, &zoomOutBtn, &zoomInBtn,
             &previewBtn, &updateTracksBtn, &saveBtn };
}

void ArrangerStyleEditor::setScale (float newScale)
{
    scale = newScale;
    nameEditor.setScale (scale);
    keyRootBox.setScale (scale);
    keyQualityBox.setScale (scale);
    for (auto* label : { &keyLabel, &zoomLabel, &addHeading })
        label->setScale (scale);
    for (auto* button : { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn,
                          &zoomOutBtn, &zoomInBtn, &previewBtn, &updateTracksBtn, &saveBtn })
        button->setScale (scale);
    // The frame takes these widths as they are (it sizes seg buttons and labels itself).
    nameEditor.setSize (scaled (nameFieldWidth, scale), scaled (fieldHeight, scale));
    keyRootBox.setSize (scaled (keyRootWidth, scale), scaled (fieldHeight, scale));
    keyQualityBox.setSize (scaled (keyQualityWidth, scale), scaled (fieldHeight, scale));
    busyOverlay.setFont (StageFonts::silkscreen (busyTextSize * scale, semiBold, busySpacing));

    timeline.setScale (scale);
    laneHeaders.setScale (scale);
    jumpPads.setScale (scale);
    inspector.setScale (scale);
    timelineViewport.setScrollBarThickness (scaled (scrollBarThickness, scale));
    resized();
}

void ArrangerStyleEditor::paint (juce::Graphics&) {}   // the page frame paints the page

void ArrangerStyleEditor::resized()
{
    using namespace SectionLaneMetrics;
    const int lanesHeight = timeline.getPreferredHeight() + scaled (scrollBarThickness, scale);
    const auto rects = sectionsPageLayout (getLocalBounds(), scale, lanesHeight);

    addHeading.setBounds (rects.addHeading);
    const std::array<SegButton*, SectionsPageMetrics::addSectionCount> addButtons { &addIntroBtn, &addVariationBtn, &addFillBtn, &addBreakBtn, &addEndingBtn };
    for (size_t index {}; index < addButtons.size(); ++index)
        addButtons[index]->setBounds (rects.addButtons[index]);

    auto centre = rects.centre;
    laneHeaders.setBounds (centre.removeFromLeft (scaled (headerWidth, scale)).withHeight (timeline.getPreferredHeight()));
    timelineViewport.setBounds (centre);
    layoutTimeline();
    jumpPads.setBounds (rects.pads.withTrimmedLeft (scaled (headerWidth, scale)));   // the pads line up with the lanes
    inspector.setBounds (rects.rightColumn);

    if (busyOverlay.isVisible())
        busyOverlay.setBounds (getLocalBounds());
}

void ArrangerStyleEditor::layoutTimeline()
{
    // Fill the viewport when there are few bars; grow + scroll when there are many.
    timeline.setSize (juce::jmax (timeline.getPreferredWidth(), timelineViewport.getMaximumVisibleWidth()), timeline.getPreferredHeight());
}

void ArrangerStyleEditor::refreshLanes()
{
    std::vector<SectionLaneHeaders::Track> tracks;
    for (const auto& sourceTrack : sourceTracks)
        tracks.push_back ({ sourceTrack.name, sourceTrack.nttType });
    laneHeaders.setTracks (tracks);
    timeline.setSourceTracks (sourceTracks, ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom));
    resized();   // more than eight tracks: taller lanes
}

void ArrangerStyleEditor::refreshSelection()
{
    juce::StringArray padNames;
    for (const auto& window : windows)
        padNames.add (SectionEditModel::typeText (window.type) + nameSeparator
                      + window.name.fromLastOccurrenceOf (nameSeparator, includeSeparator, ignoreCase));
    jumpPads.setPads (padNames);

    const int selected = timeline.getSelectedIndex();
    inspector.setSection (juce::isPositiveAndBelow (selected, (int) windows.size()) ? std::optional<SectionWindow> (windows[(size_t) selected])
                                                                                    : std::nullopt);
}

void ArrangerStyleEditor::windowsEdited()
{
    // The inspector made the same kind of edit a drag makes: take the same path.
    timeline.setWindows (windows);
    rebuildPreview();
    restIdlePlayhead();
    refreshSelection();
}

void ArrangerStyleEditor::selectSection (int sectionIndex)
{
    // While previewing, a picked section is switched to at the next bar line (a bare pick is not an edit).
    if (engine.isPlaying() && juce::isPositiveAndBelow (sectionIndex, (int) windows.size()))
        engine.queueSection (windows[(size_t) sectionIndex].type, windows[(size_t) sectionIndex].name);
    refreshSelection();
}

void ArrangerStyleEditor::startPreview()
{
    followPlayhead = true;
    rebuildPreview();
    engine.setBpm (referenceBpm);
    engine.start (false);   // preview always starts now (no Synchro / Count-In)
    syncPreviewState();
}

void ArrangerStyleEditor::togglePreview()
{
    if (engine.isPlaying())
        engine.stop();
    else
        startPreview();
    syncPreviewState();
}

void ArrangerStyleEditor::playSelected()
{
    const int selected = timeline.getSelectedIndex();
    if (! juce::isPositiveAndBelow (selected, (int) windows.size()))
        return;
    const auto& window = windows[(size_t) selected];
    if (engine.isPlaying())
    {
        engine.queueSection (window.type, window.name);   // switch there at the next bar line
        return;
    }
    rebuildPreview();
    engine.selectStartSection (window.type, window.name);   // start the preview on it
    followPlayhead = true;
    engine.setBpm (referenceBpm);
    engine.start (false);
    syncPreviewState();
}

void ArrangerStyleEditor::stepZoom (int direction)
{
    // Keep the bar in the middle of the view where it was.
    const int viewWidth = timelineViewport.getViewWidth();
    const double middleBar = (double) timeline.barForX (timelineViewport.getViewPositionX() + viewWidth / halfView);
    timeline.setZoomLevel (timeline.getZoomLevel() + direction);
    layoutTimeline();
    const int maxScroll = juce::jmax (noScroll, timeline.getWidth() - viewWidth);
    programmaticScroll = true;
    timelineViewport.setViewPosition (juce::jlimit (noScroll, maxScroll, timeline.xForBar (middleBar) - viewWidth / halfView), viewTop);
    programmaticScroll = false;
}

void ArrangerStyleEditor::timerCallback()
{
    if (isShowing())
        syncPreviewState();
}

void ArrangerStyleEditor::syncPreviewState()
{
    const bool playing = engine.isPlaying();
    previewBtn.setLedOn (playing);
    if (playing != previewShown)
    {
        previewShown = playing;
        previewBtn.setButtonText (playing ? StyleEditorText::stop : StyleEditorText::preview);
        if (onTitleActionsChanged)
            onTitleActionsChanged();   // Stop is narrower than Preview: lay the title row out again
    }
    jumpPads.setPlayingIndex (playing ? engine.getActiveSectionIndex() : SectionJumpPadValues::noPad);
}
