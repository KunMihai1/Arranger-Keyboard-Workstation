#include "displayGUI.h"
#include "CustomTableContainer.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerSourceBuilder.h"
#include "Arranger/ArrangerStyleIOHelper.h"
#include "IOHelper.h"

namespace
{
    namespace Text
    {
        const juce::String allTracks             = "All tracks";
        const juce::String soloTrack             = "Solo track(last selected)";
        const juce::String sectionConfigurations = "Section Configurations";
        const juce::String settings              = "Settings";
        const juce::String playModeTooltip       = "Play mode - manage configurations in Section Configurations";
        const juce::String start                 = "Start";
        const juce::String stop                  = "Stop";
    }

    constexpr int   soloTrackIndex      = 1;      // comboBoxChangeIndex(): 0 = all tracks, 1 = solo
    constexpr float trackRowHeight      = 80.0f;
    constexpr int   topEdge             = 0;
    constexpr int   leftEdge            = 0;
    constexpr int   styleLabelWidth     = 50;
    constexpr int   playSettingsWidth   = 40;
    constexpr int   controlRowHeight    = 20;
    constexpr int   startStopWidth      = 35;
    constexpr int   padding             = 2;
    constexpr int   paddedSides         = 2;      // above and below
    constexpr int   centringDivisor     = 2;
    constexpr int   nameWidthDivisor    = 6;      // the style name takes a sixth of the width...
    constexpr int   nameWidthTrim       = 15;     // ...minus this
    constexpr int   startButtonTrim     = 2;
    constexpr int   controlGap          = 5;
}

void CurrentStyleComponent::mouseDown(const juce::MouseEvent& event)
{
    auto* clickedComponent = event.originalComponent;

    while (clickedComponent != nullptr)
    {
        auto* clickedTrack = dynamic_cast<Track*>(clickedComponent);
        if (allTracks.contains(clickedTrack))
        {
            lastSelectedTrack = clickedTrack;
            return;
        }
        clickedComponent = clickedComponent->getParentComponent();
    }
}

void CurrentStyleComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged != &playSettingsTracks)
        return;

    const int selectedId = playSettingsTracks.getSelectedId();

    if (selectedId == PlayModeIds::allTracks || selectedId == PlayModeIds::soloTrack)          // live-track play mode -> also means "play from live tracks"
    {
        lastPlayModeId = selectedId;
        if (hasActiveArrangerConfig)   // choosing live tracks clears the active configuration
        {
            hasActiveArrangerConfig  = false;
            activeArrangerConfigName = {};
            rebuildPlaySettingsItems();
            arrangerStyleList.setActiveConfigName(juce::String());   // drop the marker in the browser
            if (anyTrackChanged) anyTrackChanged();   // persist the cleared selection to allStyles.json
        }
    }
    else if (selectedId == PlayModeIds::sectionConfigurations)   // action: open the config browser; keep the tick on the play mode
    {
        showStyleList(true);
        playSettingsTracks.setSelectedId(lastPlayModeId, juce::dontSendNotification);
    }
    else if (selectedId == PlayModeIds::settings)                // action: open the settings/keybinds tab
    {
        keybindTabStarting();
        playSettingsTracks.setSelectedId(lastPlayModeId, juce::dontSendNotification);
    }
}

void CurrentStyleComponent::rebuildPlaySettingsItems()
{
    // The active configuration is no longer shown here: it lives in the Section
    // Configurations browser (marked active). This dropdown is play-mode + actions only.
    playSettingsTracks.clear(juce::dontSendNotification);
    playSettingsTracks.addItem(Text::allTracks, PlayModeIds::allTracks);
    playSettingsTracks.addItem(Text::soloTrack, PlayModeIds::soloTrack);
    playSettingsTracks.addSeparator();
    playSettingsTracks.addItem(Text::sectionConfigurations, PlayModeIds::sectionConfigurations);   // opens the config browser/editor overlay
    playSettingsTracks.addItem(Text::settings, PlayModeIds::settings);

    playSettingsTracks.setSelectedId(lastPlayModeId, juce::dontSendNotification);

    playSettingsTracks.setTooltip(Text::playModeTooltip);
}

void CurrentStyleComponent::comboBoxChangeIndex(int playModeIndex)
{
    // Visual play-mode reset (e.g. on Home). Does not touch the active arranger configuration.
    lastPlayModeId = (playModeIndex == soloTrackIndex ? PlayModeIds::soloTrack : PlayModeIds::allTracks);
    playSettingsTracks.setSelectedId(lastPlayModeId, juce::dontSendNotification);
}

void CurrentStyleComponent::highlightPendingStartSection()
{
    // Show (and arm) the section Start will begin on, e.g. Var 1 by default, while stopped.
    if (! arrangerModeEnabled || arrangerEngine == nullptr || arrangerEngine->isPlaying())
        return;

    arrangerEngine->selectStartSection (pendingStartType, pendingStartName);
    if (onArrangerSectionChanged)
        onArrangerSectionChanged (SectionHighlight::byTypeAndName, pendingStartType, pendingStartName);
}

void CurrentStyleComponent::handleIntroCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section)
{
    if (arrangerModeEnabled)
    {
        selectOrQueueSection(ArrangerSectionType::Intro, name);
        return;
    }

    if (trackPlayer)
        trackPlayer->setLastSectionUsed(section.at(name));
}

void CurrentStyleComponent::handleEndingCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section)
{
    if (arrangerModeEnabled)
    {
        selectOrQueueSection(ArrangerSectionType::Ending, name);
        return;
    }

    if (trackPlayer)
        trackPlayer->setLastSectionUsed(section.at(name));
}

void CurrentStyleComponent::handleVarCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section)
{
    if (arrangerModeEnabled)
    {
        selectOrQueueSection(ArrangerSectionType::Variation, name);
        return;
    }

    if (trackPlayer)
        trackPlayer->setLastSectionUsed(section.at(name));
}

void CurrentStyleComponent::handleFillCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section)
{
    if (arrangerModeEnabled)
    {
        selectOrQueueSection(ArrangerSectionType::Fill, name);
        return;
    }

    if (trackPlayer)
        trackPlayer->setLastSectionUsed(section.at(name));
}

void CurrentStyleComponent::handleBreakCurrentStyle(const juce::String& name, const std::unordered_map<juce::String, StyleSection>& section)
{
    if (arrangerModeEnabled)
    {
        selectOrQueueSection(ArrangerSectionType::Break, name);
        return;
    }

    if (trackPlayer)
        trackPlayer->setLastSectionUsed(section.at(name));
}



void CurrentStyleComponent::resized()
{
    float width = getWidth() / static_cast<float>(trackSlots);
    float height = trackRowHeight;
    float trackRowY = getHeight() - height;
    int yInt = static_cast<int>(trackRowY);

    int labelWidth = styleLabelWidth;
    int heightFirst = controlRowHeight;

    // Clamp heights so they never go below or above the track start y
    int playSettingsHeight = juce::jmin(heightFirst, yInt)-padding;
    int nameOfStyleHeight = juce::jmin(heightFirst, yInt)-padding;
    int startStopHeight = juce::jmin(heightFirst - padding * paddedSides, yInt)-padding;
    int beatBarHeight = juce::jmin(heightFirst - padding * paddedSides, yInt)-padding;
    int tempoSliderHeight = juce::jmin(heightFirst, yInt)-padding;

    playSettingsTracks.setBounds(getWidth() - playSettingsWidth, topEdge, playSettingsWidth, playSettingsHeight);
    playSettingsTracks.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    nameOfStyle.setBounds((getWidth() - labelWidth) / centringDivisor, topEdge, getWidth() / nameWidthDivisor - nameWidthTrim, nameOfStyleHeight);

    startPlayingTracks.setBounds(leftEdge, padding, startStopWidth - startButtonTrim, startStopHeight);
    startPlayingTracks.setButtonText(Text::start);
    startPlayingTracks.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    stopPlayingTracks.setBounds(startPlayingTracks.getWidth() + controlGap, padding, startStopWidth, startStopHeight);
    stopPlayingTracks.setButtonText(Text::stop);
    stopPlayingTracks.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    int xStarting = stopPlayingTracks.getX() + stopPlayingTracks.getWidth() + controlGap;
    customBeatBar.setBounds(xStarting, padding, nameOfStyle.getX() - xStarting, beatBarHeight);

    int paddingBetween = controlGap;
    int sliderX = nameOfStyle.getX() + nameOfStyle.getWidth() + paddingBetween;
    int sliderWidth = playSettingsTracks.getX() - sliderX - paddingBetween;
    tempoSlider.setBounds(sliderX, topEdge, sliderWidth, tempoSliderHeight);

    for (int trackIndex {}; trackIndex < allTracks.size(); trackIndex++)
    {
        allTracks[trackIndex]->setBounds(juce::Rectangle<int>(
            static_cast<int>(trackIndex * width),
            yInt,
            static_cast<int>(width),
            static_cast<int>(height)
            ));
    }

    // Phase 3 authoring overlays are parented to the top-level window (full screen),
    // so they are sized in presentOverlay(), not here.
}


void CurrentStyleComponent::updateName(const juce::String& newName)
{
    name = newName;
    nameOfStyle.setText(name, juce::dontSendNotification);
    nameOfStyle.setTooltip(name);
}

void CurrentStyleComponent::updateObjects()
{
    // Track-player notifications only. In arranger mode the arranger owns the BPM lock, and a
    // notification queued before the mode switch must not unlock it while the arranger plays.
    if (arrangerModeEnabled)
        return;

    // Follow the player's actual state rather than flipping: Start while playing posts a notification
    // without a matching stop, and Stop while stopped posts one too, so flips drifted out of sync.
    tempoSlider.setEnabled(! (trackPlayer && trackPlayer->isPlaying()));
}

void CurrentStyleComponent::triggerStopClick()
{
    this->stopPlayingTracks.triggerClick();
}

void CurrentStyleComponent::triggerStartClick()
{
    this->startPlayingTracks.triggerClick();
}
