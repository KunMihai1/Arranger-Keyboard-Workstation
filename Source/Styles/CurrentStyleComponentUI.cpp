#include "displayGUI.h"
#include "CustomTableContainer.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerSourceBuilder.h"
#include "Arranger/ArrangerStyleIOHelper.h"
#include "IOHelper.h"


void CurrentStyleComponent::mouseDown(const juce::MouseEvent& ev)
{
    auto* clickedComponent = ev.originalComponent;

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

    const int id = playSettingsTracks.getSelectedId();

    if (id == 1 || id == 2)            // live-track play mode -> also means "play from live tracks"
    {
        lastPlayModeId = id;
        if (hasActiveArrangerConfig)   // choosing live tracks clears the active configuration
        {
            hasActiveArrangerConfig  = false;
            activeArrangerConfigName = {};
            rebuildPlaySettingsItems();
            arrangerStyleList.setActiveConfigName(juce::String());   // drop the marker in the browser
            if (anyTrackChanged) anyTrackChanged();   // persist the cleared selection to allStyles.json
        }
    }
    else if (id == 4)                  // action: open the config browser; keep the tick on the play mode
    {
        showStyleList(true);
        playSettingsTracks.setSelectedId(lastPlayModeId, juce::dontSendNotification);
    }
    else if (id == 3)                  // action: open the settings/keybinds tab
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
    playSettingsTracks.addItem("All tracks", 1);
    playSettingsTracks.addItem("Solo track(last selected)", 2);
    playSettingsTracks.addSeparator();
    playSettingsTracks.addItem("Section Configurations", 4);   // opens the config browser/editor overlay
    playSettingsTracks.addItem("Settings", 3);

    playSettingsTracks.setSelectedId(lastPlayModeId, juce::dontSendNotification);

    playSettingsTracks.setTooltip("Play mode - manage configurations in Section Configurations");
}

void CurrentStyleComponent::comboBoxChangeIndex(int Index)
{
    // Visual play-mode reset (e.g. on Home). Does not touch the active arranger configuration.
    lastPlayModeId = (Index == 1 ? 2 : 1);
    playSettingsTracks.setSelectedId(lastPlayModeId, juce::dontSendNotification);
}

void CurrentStyleComponent::highlightPendingStartSection()
{
    // Show (and arm) the section Start will begin on, e.g. Var 1 by default, while stopped.
    if (! arrangerModeEnabled || arrangerEngine == nullptr || arrangerEngine->isPlaying())
        return;

    arrangerEngine->selectStartSection (pendingStartType, pendingStartName);
    if (onArrangerSectionChanged)
        onArrangerSectionChanged (0, pendingStartType, pendingStartName);
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
    float width = getWidth() / 8.0f;
    float height = 80.0f;
    float initialX = 0;
    float y = getHeight() - height;
    int yInt = static_cast<int>(y);

    int labelWidth = 50;
    int playSettingsWidth = 40;
    int heightFirst = 20;
    int startStopWidth = 35;
    int padding = 2;

    // Clamp heights so they never go below or above the track start y
    int playSettingsHeight = juce::jmin(heightFirst, yInt)-padding;
    int nameOfStyleHeight = juce::jmin(heightFirst, yInt)-padding;
    int startStopHeight = juce::jmin(heightFirst - padding * 2, yInt)-padding;
    int beatBarHeight = juce::jmin(heightFirst - padding * 2, yInt)-padding;
    int tempoSliderHeight = juce::jmin(heightFirst, yInt)-padding;

    playSettingsTracks.setBounds(getWidth() - playSettingsWidth, 0, playSettingsWidth, playSettingsHeight);
    playSettingsTracks.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    nameOfStyle.setBounds((getWidth() - labelWidth) / 2, 0, getWidth() / 6 - 15, nameOfStyleHeight);

    startPlayingTracks.setBounds(0, padding, startStopWidth - 2, startStopHeight);
    startPlayingTracks.setButtonText("Start");
    startPlayingTracks.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    stopPlayingTracks.setBounds(startPlayingTracks.getWidth() + 5, padding, startStopWidth, startStopHeight);
    stopPlayingTracks.setButtonText("Stop");
    stopPlayingTracks.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    int xStarting = stopPlayingTracks.getX() + stopPlayingTracks.getWidth() + 5;
    customBeatBar.setBounds(xStarting, padding, nameOfStyle.getX() - xStarting, beatBarHeight);

    int paddingBetween = 5;
    int sliderX = nameOfStyle.getX() + nameOfStyle.getWidth() + paddingBetween;
    int sliderWidth = playSettingsTracks.getX() - sliderX - paddingBetween;
    tempoSlider.setBounds(sliderX, 0, sliderWidth, tempoSliderHeight);

    for (int i = 0; i < allTracks.size(); i++)
    {
        allTracks[i]->setBounds(juce::Rectangle<int>(
            static_cast<int>(i * width),
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
