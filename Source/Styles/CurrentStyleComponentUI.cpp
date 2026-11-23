#include "displayGUI.h"
#include "CustomTableContainer.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerSourceBuilder.h"
#include "Arranger/ArrangerStyleIOHelper.h"
#include "IOHelper.h"
#include "UI/Stage/StagePageWidgets.h"
#include "UI/Stage/StageMetrics.h"

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
        const char* const  tracksHeadingUtf8     = "Tracks \xc2\xb7 ";   // "Tracks · <style>"
        const juce::String clickInstrument       = "Click an instrument to change it";
    }

    constexpr int   soloTrackIndex      = 1;      // comboBoxChangeIndex(): 0 = all tracks, 1 = solo
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
    // Board StageRevised openPage=styles, tracks area: the heading, then 8 equal strips.
    using namespace StageMetrics;
    startPlayingTracks.setButtonText(Text::start);   // hidden; the panel's Start / Stop click them
    stopPlayingTracks.setButtonText(Text::stop);

    const auto stripTop    = static_cast<float>(headingHeight + tracksHeadingGap) * scale;
    const auto stripWidth  = static_cast<float>(getWidth()) / static_cast<float>(trackSlots);
    const auto stripHigh   = static_cast<float>(stripHeight) * scale;
    for (int trackIndex {}; trackIndex < allTracks.size(); trackIndex++)
    {
        allTracks[trackIndex]->setScale(scale);
        allTracks[trackIndex]->setBounds(juce::Rectangle<float>(static_cast<float>(trackIndex) * stripWidth, stripTop,
                                                                stripWidth, stripHigh).toNearestInt());
    }

    // Phase 3 authoring overlays are parented to the top-level window (full screen),
    // so they are sized in presentOverlay(), not here.
}

void CurrentStyleComponent::paint(juce::Graphics& g)
{
    const auto heading = getLocalBounds().toFloat().withHeight(static_cast<float>(StageMetrics::headingHeight) * scale);
    StageLcd::paintHeading(g, heading, scale, juce::String::fromUTF8(Text::tracksHeadingUtf8) + name, Text::clickInstrument);
}

void CurrentStyleComponent::setScale(float newScale)
{
    scale = newScale;
    resized();
    repaint();
}

void CurrentStyleComponent::setPlayModeSolo(bool solo)
{
    // Through the combo with a notification: comboBoxChanged() keeps today's rules.
    playSettingsTracks.setSelectedId(solo ? PlayModeIds::soloTrack : PlayModeIds::allTracks, juce::sendNotificationSync);
}

bool CurrentStyleComponent::isPlayModeSolo() const
{
    return lastPlayModeId == PlayModeIds::soloTrack;
}

void CurrentStyleComponent::openSectionConfigurations()
{
    showStyleList(true);
}


void CurrentStyleComponent::updateName(const juce::String& newName)
{
    name = newName;
    nameOfStyle.setText(name, juce::dontSendNotification);
    nameOfStyle.setTooltip(name);
    repaint();   // the tracks heading shows the name
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
