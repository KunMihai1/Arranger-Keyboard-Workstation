#include "displayGUI.h"
#include "CustomTableContainer.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerSourceBuilder.h"
#include "Arranger/ArrangerStyleIOHelper.h"
#include "IOHelper.h"

void CurrentStyleComponent::startPlaying()
{
    int selectedID = lastPlayModeId;   // 1 = all tracks, 2 = solo; the active-config path below ignores this

    stopPlaying(false);

    std::vector<TrackEntry> selectedTracks;

    const bool hasMidiOut     = outputDevice.lock() != nullptr;
    const bool hasAudioInject = trackPlayer && bool(trackPlayer->onMidiMessage);

    if (!hasMidiOut && !hasAudioInject)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Playing tracks", "No output device selected.");
        return;
    }

    // Arranger mode with a loaded/saved configuration: play it directly (self-contained,
    // so it doesn't need the live tracks). No active config -> fall through to the demo below.
    if (arrangerModeEnabled && hasActiveArrangerConfig)
    {
        // Live volume sliders win over the volumes baked into the saved config: copy the config and
        // overlay each track's current on-screen slider value (matched by UUID) so a slider change
        // is heard on the next Start. Tracks with no live match keep their saved volume.
        ArrangerStyle cfg = activeArrangerConfig;
        applyLiveTrackVolumes(cfg);
        arrangerEngine->setStyle(cfg);
        arrangerEngine->setBpm(currentTempo);
        arrangerEngine->setBassInversion(arrangerBassInversion);   // re-apply: setStyle/rebuild may reset it
        arrangerEngine->setNttEnabled(arrangerNttEnabled);         // Phase 7a: re-apply on every Start
        arrangerEngine->setMinorScale(arrangerMinorScale);         // Phase 7a: re-apply on every Start
        arrangerEngine->selectStartSection(pendingStartType, pendingStartName);  // begin on the chosen start (stopPlaying cleared it)
        arrangerEngine->start();
        seedHeldChordIntoArranger();   // honour a chord already held when Start was pressed
        return;
    }

    if (selectedID == 1)
    {
        for (const auto& tr : allTracks)
        {
            auto it = mapUuidToTrackEntry.find(tr->getUsedID());
            if (it != mapUuidToTrackEntry.end() && it->second != nullptr)
            {
                it->second->instrumentAssociated = tr->getInstrumentNumber();
                it->second->volumeAssociated = tr->getVolume();
                selectedTracks.push_back(*it->second);
            }
        }
    }
    else if (selectedID == 2)
    {
        if (lastSelectedTrack)
        {
            auto it = mapUuidToTrackEntry.find(lastSelectedTrack->getUsedID());
            if (it != mapUuidToTrackEntry.end())
                selectedTracks.push_back(*it->second);
        }
    }

    if (selectedTracks.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Playing tracks", "There are no tracks to play");
        return;
    }

    if (arrangerModeEnabled)
    {
        // Phase 1: fixed 4/4; real time-signature wiring is Phase 2.
        ArrangerStyle style = ArrangerPatternBuilder::buildDemoMultiSectionStyle(selectedTracks, 4, 4, currentTempo);
        arrangerEngine->setStyle(style);
        arrangerEngine->setBpm(currentTempo);   // user's tempo slider wins over the style's original tempo
        arrangerEngine->selectStartSection(pendingStartType, pendingStartName);  // begin on the chosen start
        arrangerEngine->start();
        seedHeldChordIntoArranger();   // honour a chord already held when Start was pressed
        return;
    }

    trackPlayer->setTracks(selectedTracks);
    trackPlayer->syncPlaybackSettings();
    trackPlayer->start();

}

void CurrentStyleComponent::seedHeldChordIntoArranger()
{
    // A steady held chord sends no note event, so without this it wouldn't reach the engine until the
    // next press/release. Apply it right after start() so a chord held when Start was pressed plays
    // immediately, instead of starting on the home key and only catching up on the first key change.
    if (!arrangerEngine || !getHeldChord)
        return;
    const ArrangerChord held = getHeldChord();
    if (held.isValid())
        arrangerEngine->setActiveChord(held);
}

CurrentStyleComponent::CurrentStyleComponent(const juce::String& name, std::unordered_map<juce::Uuid, TrackEntry*>& map, std::weak_ptr<juce::MidiOutput> outputDevice,
    std::weak_ptr<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>> styleSMap) : name(name), mapUuidToTrackEntry(map), outputDevice(outputDevice), styleSectionsMap{styleSMap}
{
    addMouseListener(this, true);
    nameOfStyle.setText(name, juce::dontSendNotification);
    nameOfStyle.setTooltip(name);
    addAndMakeVisible(nameOfStyle);

    playSettingsTracks.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    playSettingsTracks.addListener(this);
    rebuildPlaySettingsItems();   // All tracks / Solo / [active config] / --- / Section Configurations / Settings

    addAndMakeVisible(playSettingsTracks);
    addAndMakeVisible(startPlayingTracks);
    addAndMakeVisible(stopPlayingTracks);
    addAndMakeVisible(customBeatBar);

    // Performance buttons must not steal keyboard focus when clicked, so the PC-keyboard keeps
    // playing notes after you press Start / Stop or open the play settings.
    startPlayingTracks.setMouseClickGrabsKeyboardFocus(false);
    stopPlayingTracks.setMouseClickGrabsKeyboardFocus(false);
    playSettingsTracks.setMouseClickGrabsKeyboardFocus(false);


    trackPlayer = std::make_unique<MultipleTrackPlayer>(outputDevice);

    trackPlayer->onElapsedUpdate = [this](double ElapsedBeats)
    {
        customBeatBar.setCurrentBeatsElapsed(ElapsedBeats);
    };

    arrangerEngine = std::make_unique<ArrangerEngine>(outputDevice);
    arrangerEngine->onElapsedBeats = [this](double elapsedBeats)
    {
        customBeatBar.setCurrentBeatsElapsed(elapsedBeats);
    };

    // Live section-button highlight follows the engine's actual active section.
    arrangerEngine->onActiveSectionChanged = [this](int idx, ArrangerSectionType type, juce::String name)
    {
        if (onArrangerSectionChanged)
            onArrangerSectionChanged(idx, type, name);
    };

    // Engine stopped itself (an Ending finished): drop the playing state so the beat bar leaves its
    // play colour (first beat goes back to the idle/yellow downbeat).
    arrangerEngine->onStoppedItself = [this]()
    {
        isPlaying = false;
        customBeatBar.setCurrentBeatsElapsed(0.0);
        customBeatBar.repaint();
        highlightPendingStartSection();   // back to the chosen start (Var 1 by default)
        if (arrangerModeEnabled) tempoSlider.setEnabled(true);   // unlock BPM after a self-stop (Ending)
    };

    // Phase 3: arranger-style authoring overlay (opened from the play-settings dropdown).
    addChildComponent(arrangerStyleList);
    arrangerStyleList.onCreateNew  = [this] { openStyleEditorNew(); };
    arrangerStyleList.onEditStyle  = [this] (const juce::File& f) { openStyleEditorFromFile(f); };
    arrangerStyleList.onLoadStyle  = [this] (const juce::File& f) { loadStyleFileIntoEngine(f); showStyleList(false); };
    arrangerStyleList.onDeleteStyle = [this] (const juce::File& f)
    {
        // If the deleted file was this style's active configuration, clear + persist that.
        if (hasActiveArrangerConfig && activeArrangerConfigName == f.getFileNameWithoutExtension())
        {
            hasActiveArrangerConfig  = false;
            activeArrangerConfigName = {};
            rebuildPlaySettingsItems();
            arrangerStyleList.setActiveConfigName(juce::String());
            if (anyTrackChanged) anyTrackChanged();
        }
    };
    arrangerStyleList.onClose      = [this] { showStyleList(false); };

    startPlayingTracks.onClick = [this]
    {
        isPlaying = true;
        startPlaying();
        if (arrangerModeEnabled) tempoSlider.setEnabled(false);   // lock BPM while the arranger plays

        // Starting rebuilds the play scene, which can land keyboard focus on a freshly created
        // child — return it to the play surface so the PC keyboard plays without a stray screen click.
        if (onRequestPlayFocus)
            onRequestPlayFocus();
    };

    stopPlayingTracks.onClick = [this]
    {
        isPlaying = false;
        stopPlaying();
        highlightPendingStartSection();   // back to the chosen start (Var 1 by default)
        if (arrangerModeEnabled) tempoSlider.setEnabled(true);    // unlock BPM when stopped
    };

    customBeatBar.isPlayingCheck = [this]()
    {
        return isPlaying;
    };

    trackPlayer->onStopTriggerClickFromPlayer = [this]()
    {
        juce::MessageManager::callAsync([this]()
            {
                isPlaying = false;
                customBeatBar.repaint();
            });
    };

    for (int i = 0; i < 8; i++)
    {
        auto* newTrack = new Track{};
        newTrack->onChange = [this]()
        {
            if (anyTrackChanged)
                anyTrackChanged();

        };

        newTrack->onCopy = [this](Track* copied)
        {
            copiedTrack = std::make_unique<Track>();
            copiedTrack->copyFrom(*copied);
        };

        newTrack->onPaste = [this](Track* toPaste)
        {
            if (copiedTrack == nullptr)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Pasting a track", "First you have to copy!");
                return;
            }

            toPaste->pasteFrom(*copiedTrack.get());

        };

        newTrack->onShowInformation= [this](const juce::Uuid& uuid, int channel)
        {
            showingTheInformationNotesFromTrack(uuid, channel);
        };

        newTrack->syncVolumePercussionTracks = [this](double newVolume) {
            syncPercussionTracksVolumeChange(newVolume);
        };

        newTrack->onRequestTrackSelection = [this,newTrack](std::function<void(const juce::String&, const juce::Uuid& uuid, const juce::String& type)> trackChosenCallback)
        {
            if (onRequestTrackSelectionFromTrack)
                onRequestTrackSelectionFromTrack(trackChosenCallback);



            //DBG("Current tempo before change:" + juce::String(currentTempo));
            //applyBPMchangeForOne(currentTempo, newTrack->getUsedID());
        };

        newTrack->isPlaying = [this]() {
            return isPlaying;
        };

        addAndMakeVisible(newTrack);
        allTracks.add(newTrack);
    }

    tempoSlider.setRange(60.0, 200.0, 1.0);
    tempoSlider.setSkewFactorFromMidPoint(120.0);
    tempoSlider.setValue(currentTempo);
    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);

    tempoSlider.setColour(juce::Slider::trackColourId, juce::Colours::cornflowerblue);
    tempoSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    tempoSlider.setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    tempoSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    tempoSlider.textFromValueFunction = [](double value)
    {
        return juce::String(static_cast<int>(value)) + " BPM";
    };
    tempoSlider.setValue(tempoSlider.getValue(), juce::sendNotificationSync);
    tempoSlider.updateText();

    /*
    tempoSlider.onValueChange = [this]()
    {
        oldTempo = currentTempo;
        currentTempo = tempoSlider.getValue();
    };
    */

    tempoSlider.onDragEnd = [this]()
    {
        oldTempo = currentTempo;
        currentTempo = tempoSlider.getValue();

        if (trackPlayer)
            trackPlayer->setCurrentBPM(currentTempo);

        applyBPMchangeBeforePlayback(currentTempo,true);

        if (anyTrackChanged)
            anyTrackChanged();
    };

    addAndMakeVisible(tempoSlider);

    trackPlayer->addSubjectTrackPlayerModifyListener(this);
}

void CurrentStyleComponent::stopPlaying(bool shouldModify)
{
    if (arrangerModeEnabled)
    {
        if (arrangerEngine)
            arrangerEngine->stop();
        return;
    }

    if (trackPlayer)
        trackPlayer->stop(shouldModify);
}

void CurrentStyleComponent::setArrangerModeEnabled(bool shouldEnable)
{
    if (arrangerModeEnabled == shouldEnable)
        return;

    stopPlaying(false);          // stop whichever engine is currently active
    if (arrangerEngine)
        arrangerEngine->stop();

    arrangerModeEnabled = shouldEnable;
    isPlaying = false;
    tempoSlider.setEnabled(true);   // switching modes isn't playing -> BPM editable again
    customBeatBar.repaint();

    if (! shouldEnable)
    {
        showStyleList(false);
        arrangerStyleEditor.reset();
        restoreEngineBeatBar();
    }
    else
    {
        highlightPendingStartSection();   // show the armed start section (Var 1 by default)
    }
}

void CurrentStyleComponent::setArrangerAutoFillEnabled(bool enabled)
{
    if (arrangerEngine)
        arrangerEngine->setAutoFillEnabled(enabled);
}

void CurrentStyleComponent::setLiveChord(const ArrangerChord& chord)
{
    // Only steer the accompaniment while in arranger mode; the engine applies it on its next tick.
    if (arrangerModeEnabled && arrangerEngine)
        arrangerEngine->setActiveChord(chord);
}

void CurrentStyleComponent::setSynchroStartEnabled(bool enabled)
{
    synchroStartEnabled = enabled;   // remembered; re-applied on every Start (see startPlaying)
    if (arrangerEngine)
        arrangerEngine->setSynchroStartEnabled(enabled);
}

void CurrentStyleComponent::setCountInEnabled(bool enabled)
{
    countInEnabled = enabled;
    if (arrangerEngine)
        arrangerEngine->setCountInEnabled(enabled);
}

void CurrentStyleComponent::setArrangerBassInversion(bool shouldInvert)
{
    arrangerBassInversion = shouldInvert;   // remembered; re-applied on every Start (see startPlaying)
    if (arrangerEngine)
        arrangerEngine->setBassInversion(shouldInvert);
}

void CurrentStyleComponent::setArrangerNttEnabled(bool enabled)
{
    arrangerNttEnabled = enabled;   // remembered; re-applied on every Start (see startPlaying)
    if (arrangerEngine)
        arrangerEngine->setNttEnabled(enabled);
}

void CurrentStyleComponent::setArrangerMinorScale(MinorScaleChoice choice)
{
    arrangerMinorScale = choice;   // remembered; re-applied on every Start (see startPlaying)
    if (arrangerEngine)
        arrangerEngine->setMinorScale(choice);
}

void CurrentStyleComponent::restoreEngineBeatBar()
{
    if (arrangerEngine)
        arrangerEngine->onElapsedBeats = [this](double elapsedBeats)
        {
            customBeatBar.setCurrentBeatsElapsed(elapsedBeats);
        };
}

std::vector<TrackEntry> CurrentStyleComponent::collectSelectedTracks()
{
    std::vector<TrackEntry> selected;
    for (const auto& tr : allTracks)
    {
        auto it = mapUuidToTrackEntry.find(tr->getUsedID());
        if (it != mapUuidToTrackEntry.end() && it->second != nullptr)
        {
            it->second->instrumentAssociated = tr->getInstrumentNumber();
            it->second->volumeAssociated     = tr->getVolume();
            selected.push_back(*it->second);
        }
    }
    return selected;
}

void CurrentStyleComponent::applyLiveTrackVolumes(ArrangerStyle& style) const
{
    // allTracks and the config's tracks are both small (a handful each), so a direct nested match
    // by UUID is fine and avoids needing a hash for juce::String.
    for (auto& section : style.sections)
        for (auto& track : section.tracks)
            for (const auto* live : allTracks)
                if (live != nullptr && live->getUsedID().toString() == track.id)
                {
                    track.volume = live->getVolume();
                    break;
                }
}

void CurrentStyleComponent::presentOverlay(juce::Component& c)
{
    // Authoring (browser/editor) is a separate mode from live performance and shares the single
    // ArrangerEngine. Stop any live playback before showing the overlay so the editor's preview
    // doesn't inherit/fight the live engine state (which made entering edit show a stale section
    // and fire a queued Ending on close).
    if (arrangerEngine && arrangerEngine->isPlaying())
    {
        arrangerEngine->stop();
        isPlaying = false;
        customBeatBar.setCurrentBeatsElapsed(0.0);
        customBeatBar.repaint();
    }

    // Authoring overlays are full-screen; tell the host to hide the GL note layer (it renders on
    // top of normal painting and would otherwise show through the middle of the overlay).
    if (onAuthoringOverlayVisible) onAuthoringOverlayVisible(true);

    auto* top = getTopLevelComponent();
    if (top == nullptr)
        top = this;
    top->addAndMakeVisible(c);
    c.setBounds(top->getLocalBounds());
    c.toFront(true);
}

void CurrentStyleComponent::showStyleList(bool shouldShow)
{
    if (shouldShow)
    {
        arrangerStyleList.refresh();
        arrangerStyleList.setActiveConfigName(hasActiveArrangerConfig ? activeArrangerConfigName : juce::String());
        presentOverlay(arrangerStyleList);
    }
    else
    {
        arrangerStyleList.setVisible(false);
        if (onAuthoringOverlayVisible) onAuthoringOverlayVisible(false);   // restore the note layer
    }
}

void CurrentStyleComponent::openStyleEditorNew()
{
    auto tracks = collectSelectedTracks();
    if (tracks.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
            "Section configuration", "Record or add some tracks first.");
        return;
    }

    arrangerStyleEditor = std::make_unique<ArrangerStyleEditor>(*arrangerEngine);
    configureEditorCallbacks();
    presentOverlay(*arrangerStyleEditor);
    arrangerStyleEditor->loadRecording(tracks, currentTempo, 4, 4, juce::String()); // default generic config name
}

void CurrentStyleComponent::configureEditorCallbacks()
{
    if (! arrangerStyleEditor)
        return;

    arrangerStyleEditor->onClose = [this] { closeStyleEditor(); };
    // Update Tracks pulls the app's current recording (instrument/volume synced from the sliders) as
    // raw entries; the editor builds + writes them on its own background thread.
    arrangerStyleEditor->onRequestCurrentTracks = [this] { return collectSelectedTracks(); };
    // Refresh the active config from the style the editor already built (no file re-parse -> no freeze).
    arrangerStyleEditor->onSaved = [this] (const juce::File& f, const ArrangerStyle& s)
    {
        arrangerStyleList.refresh();
        applyActiveConfig(f, s);
    };
}

void CurrentStyleComponent::applyActiveConfig(const juce::File& f, const ArrangerStyle& s)
{
    activeArrangerConfig     = s;          // this configuration is now what Start + section buttons use
    hasActiveArrangerConfig  = true;
    activeArrangerConfigName = f.getFileNameWithoutExtension();
    rebuildPlaySettingsItems();
    arrangerStyleList.setActiveConfigName(activeArrangerConfigName);
    if (anyTrackChanged) anyTrackChanged();   // persist the selection to allStyles.json

    // A freshly-loaded config starts armed on Variation 1 (highlighted); the user can pick another
    // start section before pressing Start.
    pendingStartType = ArrangerSectionType::Variation;
    pendingStartName = "Variation 1";
    highlightPendingStartSection();
}

void CurrentStyleComponent::openStyleEditorFromFile(const juce::File& f)
{
    // Parse the (large) file off the message thread so the UI doesn't freeze; show the app overlay
    // meanwhile. Build the editor + present it back on the message thread once parsing is done.
    if (onBusy) onBusy(true, "Loading configuration...");
    juce::Component::SafePointer<CurrentStyleComponent> safe(this);

    juce::Thread::launch([safe, f]
    {
        auto sf = std::make_shared<ArrangerStyleFile>();
        juce::String err;
        const bool ok = ArrangerStyleIOHelper::loadFromFile(f, *sf, err);

        juce::MessageManager::callAsync([safe, f, sf, ok, err]
        {
            if (safe == nullptr)
                return;
            if (safe->onBusy) safe->onBusy(false, {});
            if (! ok)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Section configuration", err);
                return;
            }
            safe->arrangerStyleEditor = std::make_unique<ArrangerStyleEditor>(*safe->arrangerEngine);
            safe->configureEditorCallbacks();
            safe->presentOverlay(*safe->arrangerStyleEditor);
            safe->arrangerStyleEditor->loadFromFile(*sf);
            safe->arrangerStyleEditor->setSourceFile(f);   // so Save renames this file instead of duplicating it
        });
    });
}

bool CurrentStyleComponent::buildConfigFromFile(const juce::File& f, ArrangerStyle& out, bool showError)
{
    ArrangerStyleFile sf; juce::String err;
    if (! ArrangerStyleIOHelper::loadFromFile(f, sf, err))
    {
        if (showError)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Section configuration", err);
        return false;
    }
    out = ArrangerPatternBuilder::buildStyleFromFile(sf);
    return true;
}

void CurrentStyleComponent::loadStyleFileIntoEngine(const juce::File& f)
{
    // Parse the (large) file off the message thread (the hex-decode of every event is what froze the
    // UI); show the app overlay meanwhile. Build + apply back on the message thread when it's done.
    if (onBusy) onBusy(true, "Loading configuration...");
    juce::Component::SafePointer<CurrentStyleComponent> safe(this);

    juce::Thread::launch([safe, f]
    {
        auto sf = std::make_shared<ArrangerStyleFile>();
        juce::String err;
        const bool ok = ArrangerStyleIOHelper::loadFromFile(f, *sf, err);

        juce::MessageManager::callAsync([safe, f, sf, ok]
        {
            if (safe == nullptr)
                return;
            if (safe->onBusy) safe->onBusy(false, {});
            if (! ok)
                return;

            ArrangerStyle s = ArrangerPatternBuilder::buildStyleFromFile(*sf);
            safe->applyActiveConfig(f, s);   // this configuration is now what Start + section buttons use

            if (safe->arrangerEngine)
            {
                safe->arrangerEngine->setStyle(s);          // prime the engine, but don't start:
                safe->arrangerEngine->setBpm(safe->currentTempo); // Load only makes it active; user presses Start to play.
            }
        });
    });
}

void CurrentStyleComponent::closeStyleEditor()
{
    if (arrangerEngine)
        arrangerEngine->stop();
    isPlaying = false;
    customBeatBar.repaint();
    arrangerStyleEditor.reset();
    restoreEngineBeatBar();
    arrangerStyleList.refresh();
    arrangerStyleList.setActiveConfigName(hasActiveArrangerConfig ? activeArrangerConfigName : juce::String());
    presentOverlay(arrangerStyleList);   // return to the browser the editor was opened from
}

double CurrentStyleComponent::getTempo()
{
    return currentTempo;
}

double CurrentStyleComponent::getBaseTempo()
{
    return baseTempo;
}

void CurrentStyleComponent::setDeviceOutputCurrentStyle(std::weak_ptr<juce::MidiOutput> newOutput)
{
    this->outputDevice = newOutput;
    if (trackPlayer)
        trackPlayer->setDeviceOutputTrackPlayer(newOutput);
    if (arrangerEngine)
        arrangerEngine->setDeviceOutput(newOutput);
}

void CurrentStyleComponent::setMidiInjectCallback(std::function<void(const juce::MidiMessage&)> cb)
{
    if (trackPlayer)
        trackPlayer->onMidiMessage = cb;
    if (arrangerEngine)
        arrangerEngine->onMidiMessage = cb;
}

void CurrentStyleComponent::applyChangesForOneTrack(TrackEntry& track)
{

    auto it = track.styleChangesMap.find(styleID);
    if (it != track.styleChangesMap.end())
    {
        std::vector<TrackIOHelper::NotePair> toApply;
        TrackIOHelper::extractNotePairEvents(track.sequence, toApply);
        TrackIOHelper::applyChangesToASequence(toApply, it->second);

        track.sequence.sort();
        track.sequence.updateMatchedPairs();
    }
}

void CurrentStyleComponent::applyChangesForAllTracksCurrentStyle()
{
    for (auto& [uuid, track] : mapUuidToTrackEntry)
    {
        if (track)
            applyChangesForOneTrack(*track);
    }
}

void CurrentStyleComponent::removingTrack(const juce::Uuid& uuid)
{
    for (auto& track : allTracks)
    {
        if (track->getUsedID() == uuid)
        {
            track->setNameLabel("None");
            track->setUUID(juce::Uuid{ "noID" });
            track->setTypeOfTrack("None");
            track->setChannel(2);
        }
    }
}

void CurrentStyleComponent::removingTracks(const std::vector<juce::Uuid>& uuids)
{
    for (auto& track : allTracks)
    {
        auto trackUUID = track->getUsedID();
        for (auto& uuid : uuids)
        {
            if (trackUUID == uuid)
            {
                track->setNameLabel("None");
                track->setUUID(juce::Uuid{ "noID" });
                track->setTypeOfTrack("None");
                track->setChannel(2);
                break;
            }
        }
    }
}

void CurrentStyleComponent::renamingTrack(const juce::Uuid& uuid, const juce::String& newName)
{
    for (auto& track:allTracks)
    {
        if (track->getUsedID() == uuid)
            track->setNameLabel(newName);
    }
}

void CurrentStyleComponent::showingTheInformationNotesFromTrack(const juce::Uuid& uuid, int channel)
{
    juce::MidiMessageSequence* sequence = nullptr;
    std::unordered_map<int, MidiChangeInfo>* changeMap = nullptr;
    juce::String displayName = "<Unnamed>";
    double originalBPMfromFile = 120.0;

    auto it = mapUuidToTrackEntry.find(uuid);
    if (it != mapUuidToTrackEntry.end())
    {
        auto& track = it->second;
        if (track != nullptr)
        {
            sequence = &track->sequence;
            changeMap = &track->styleChangesMap[styleID];
            originalBPMfromFile = track->originalBPM;

            if (track->displayName.isNotEmpty())
                displayName = track->displayName;
        }
    }

    // Fallbacks if no data
    static juce::MidiMessageSequence emptySequence;
    static std::unordered_map<int, MidiChangeInfo> emptyMap;


    auto container = std::make_unique<TableContainer>(
        sequence ? *sequence : emptySequence,
        displayName,
        channel,
        changeMap ? *changeMap : emptyMap,
        isPlaying
        );

    trackPlayer->addSubjectTrackPlayerModifyListener(container.get());

    container->setSize(350, 340);

    container->updateToFile = [this]()
    {
        if (updateTrackFile)
            updateTrackFile();
    };

    container->addModelAsListenerToTrackPlayer(trackPlayer.get());
    container->removeModelFromListener = [this](TrackPlayerListener* listener)
    {
        if (trackPlayer)
            trackPlayer->removeSubjectTrackPlayerListener(listener);
    };

    container->removeContainerFromListeners = [this, container=container.get()]()
    {
        trackPlayer->removeSubjectTrackPlayerModifyListener(container);
    };

    container->getCurrentBPMstyle = [this]()
    {
        return this->currentTempo;
    };

    juce::CallOutBox::launchAsynchronously(
        std::move(container),
        getScreenBounds(),
        nullptr
    );

}

juce::OwnedArray<Track>& CurrentStyleComponent::getAllTracks()
{
    return allTracks;
}

MultipleTrackPlayer* CurrentStyleComponent::getTrackPlayer()
{
    return trackPlayer.get();
}

std::vector<CurrentStyleComponent::TrackChannelInstrument> CurrentStyleComponent::getTrackChannelInstruments() const
{
    std::vector<TrackChannelInstrument> result;
    for (const auto* track : allTracks)
        result.push_back({ track->getChannel(), track->getInstrumentNumber() });
    return result;
}

void CurrentStyleComponent::syncPercussionTracksVolumeChange(double newVolume)
{
    for (auto& track : allTracks)
    {
        if (track->getTypeOfTrack() == TrackTypeConversion::toString(TrackType::Percussion))
        {
            if (track->getVolume() != newVolume)
                track->setVolumeSlider(newVolume);
        }
    }
}

void CurrentStyleComponent::applyBPMchangeBeforePlayback(double userBPM, bool applyStyleChanges)
{
    if (userBPM <= 0.0)
        userBPM = 120.0;

    currentTempo = userBPM;

    if (trackPlayer)
        trackPlayer->setCurrentBPM(currentTempo);

    int channelCounter = 0;

    for (const auto& trackPtr : allTracks)
    {
        auto& tr = mapUuidToTrackEntry[trackPtr->getUsedID()];
        if (tr == nullptr)
            continue;

        int targetChannel = (tr->type == TrackType::Percussion) ? 10 : (channelCounter + 2);
        if (tr->type != TrackType::Percussion)
            ++channelCounter;

        double originalBPM = (tr->originalBPM > 0.0) ? tr->originalBPM : 120.0;
        double ratio = originalBPM / userBPM;

        juce::MidiMessageSequence scaledSequence;

        for (int i = 0; i < tr->originalSequenceTicks.getNumEvents(); ++i)
        {
            const auto& event = tr->originalSequenceTicks.getEventPointer(i)->message;
            double scaledTime = event.getTimeStamp() * ratio;

            juce::MidiMessage newMsg = event;
            newMsg.setTimeStamp(scaledTime);
            newMsg.setChannel(targetChannel);

            scaledSequence.addEvent(newMsg);
        }

        scaledSequence.sort();
        scaledSequence.updateMatchedPairs();


        tr->sequence = scaledSequence;
        if (applyStyleChanges)
        {
            applyChangesForOneTrack(*tr);
        }

        auto& toApply = tr->sequence;

        double firstNoteOnTime = 0.01;
        for (int i = 0; i < toApply.getNumEvents(); ++i)
        {
            const auto& msg = toApply.getEventPointer(i)->message;
            if (msg.isNoteOn())
            {
                firstNoteOnTime = std::max(0.01, msg.getTimeStamp() - 0.005);
                break;
            }
        }

        if (tr->volumeAssociated != -1)
        {
            toApply.addEvent(
                juce::MidiMessage::controllerEvent(targetChannel, 7, (int)tr->volumeAssociated)
                .withTimeStamp(firstNoteOnTime - 0.004));
        }

        if (targetChannel != 10 && tr->instrumentAssociated != -1)
        {
            toApply.addEvent(
                juce::MidiMessage::programChange(targetChannel, tr->instrumentAssociated)
                .withTimeStamp(firstNoteOnTime - 0.002));
        }
    }
}

//if this function is used, there's an issue with the sync of the whole tracks
void CurrentStyleComponent::applyBPMchangeForOne(double userBPM, const juce::Uuid& uuid)
{

    if (userBPM <= 0.0)
        userBPM = 120.0;

    currentTempo = userBPM;

    if (trackPlayer)
        trackPlayer->setCurrentBPM(currentTempo);

    auto it = mapUuidToTrackEntry.find(uuid);
    if (it == mapUuidToTrackEntry.end() || it->second == nullptr)
        return;

    auto& tr = it->second;


    int channelCounter = 0;
    int targetChannel = (tr->type == TrackType::Percussion) ? 10 : 2;
    for (const auto& trackPtr : allTracks)
    {
        if (trackPtr->getUsedID() == uuid)
            break;

        auto& other = mapUuidToTrackEntry[trackPtr->getUsedID()];
        if (other && other->type != TrackType::Percussion)
            ++channelCounter;
    }
    if (tr->type != TrackType::Percussion)
        targetChannel = channelCounter + 2;

    double originalBPM = (tr->originalBPM > 0.0) ? tr->originalBPM : 120.0;
    double ratio = originalBPM / userBPM;

    juce::MidiMessageSequence scaledSequence;

    for (int i = 0; i < tr->originalSequenceTicks.getNumEvents(); ++i)
    {
        const auto& event = tr->originalSequenceTicks.getEventPointer(i)->message;
        double scaledTime = event.getTimeStamp() * ratio;

        juce::MidiMessage newMsg = event;
        newMsg.setTimeStamp(scaledTime);
        newMsg.setChannel(targetChannel);
        scaledSequence.addEvent(newMsg);
    }

    scaledSequence.sort();
    scaledSequence.updateMatchedPairs();

    double firstNoteOnTime = 0.01;
    for (int i = 0; i < scaledSequence.getNumEvents(); ++i)
    {
        const auto& msg = scaledSequence.getEventPointer(i)->message;
        if (msg.isNoteOn())
        {
            firstNoteOnTime = std::max(0.01, msg.getTimeStamp() - 0.005);
            break;
        }
    }

    if (tr->volumeAssociated != -1)
    {
        scaledSequence.addEvent(
            juce::MidiMessage::controllerEvent(targetChannel, 7, (int)tr->volumeAssociated)
            .withTimeStamp(firstNoteOnTime - 0.004));
    }

    if (targetChannel != 10 && tr->instrumentAssociated != -1)
    {
        scaledSequence.addEvent(
            juce::MidiMessage::programChange(targetChannel, tr->instrumentAssociated)
            .withTimeStamp(firstNoteOnTime - 0.002));
    }

    tr->sequence = scaledSequence;
}

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

bool CurrentStyleComponent::getIsPlaying()
{
    return isPlaying;
}

void CurrentStyleComponent::selectOrQueueSection (ArrangerSectionType type, const juce::String& name)
{
    if (arrangerEngine == nullptr)
        return;

    if (arrangerEngine->isPlaying())
    {
        arrangerEngine->queueSection (type, name);   // switch at the next bar line
        return;
    }

    // Stopped: this becomes the section Start will begin on. Remember it (Start re-applies it because
    // stopPlaying() clears the engine's pending index), arm the engine, and light the chosen button.
    pendingStartType = type;
    pendingStartName = name;
    arrangerEngine->selectStartSection (type, name);
    if (onArrangerSectionChanged)
        onArrangerSectionChanged (0, type, name);    // idx>=0 -> highlight by type + name
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

void CurrentStyleComponent::setTempo(double newTempo)
{
    oldTempo = currentTempo;
    this->currentTempo = newTempo;
    this->tempoSlider.setValue(newTempo);
}

void CurrentStyleComponent::setStyleID(const juce::String& newID)
{
    this->styleID = newID;
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
    tempoSlider.setEnabled(!tempoSlider.isEnabled());
}

CurrentStyleComponent::~CurrentStyleComponent()
{
    // Tear down the authoring editor before the engine: it holds a callback into the engine.
    if (arrangerEngine)
        arrangerEngine->stop();
    arrangerStyleEditor.reset();

    if (trackPlayer)
    {
        for (auto& track : allTracks)
            track->removeListener(trackPlayer.get());

        trackPlayer->removeSubjectTrackPlayerModifyListener(this);
    }
    if (&playSettingsTracks)
        playSettingsTracks.removeListener(this);
}

juce::String CurrentStyleComponent::getName()
{
    return name;
}

juce::String CurrentStyleComponent::getStyleID()
{
    return this->styleID;
}

void CurrentStyleComponent::triggerStopClick()
{
    this->stopPlayingTracks.triggerClick();
}

void CurrentStyleComponent::triggerStartClick()
{
    this->startPlayingTracks.triggerClick();
}


void CurrentStyleComponent::stoppingPlayer()
{
    if (trackPlayer)
    {
        this->trackPlayer->stop();
        trackPlayer.reset();
    }
}

juce::DynamicObject* CurrentStyleComponent::getJson() const
{
    auto* styleObj = new juce::DynamicObject{};

    styleObj->setProperty("name", name);
    styleObj->setProperty("BPM", currentTempo);
    styleObj->setProperty("StyleID", styleID);

    if (hasActiveArrangerConfig && activeArrangerConfigName.isNotEmpty())
        styleObj->setProperty("arrangerConfig", activeArrangerConfigName);   // remembered selection

    juce::Array<juce::var> tracksArray;

    for (auto* track : allTracks)
    {
        tracksArray.add(juce::var(track->getJson()));
    }

    styleObj->setProperty("tracks", tracksArray);

    return styleObj;
}

void CurrentStyleComponent::loadJson(const juce::var& styleVar)
{
    if (!styleVar.isObject())
        return;


    auto* obj = styleVar.getDynamicObject();
    if (!obj)
        return;

    updateName(obj->getProperty("name").toString());
    setTempo(static_cast<double>(obj->getProperty("BPM")));
    setStyleID(obj->getProperty("StyleID").toString());

    // Restore the remembered section configuration: selected (primed), not auto-playing.
    // Silent + graceful if the .style was since deleted/renamed (falls back to live tracks).
    {
        // Configuration selection is strictly per-style. This component is reused across
        // style tabs, so drop any config carried over from the previously-shown style
        // before restoring this style's own (if it has one).
        hasActiveArrangerConfig  = false;
        activeArrangerConfig     = {};
        activeArrangerConfigName = {};

        const auto cfgName = obj->getProperty("arrangerConfig").toString();
        if (cfgName.isNotEmpty())
        {
            auto file = IOHelper::getArrangerStylesFolder().getChildFile(cfgName + ".style");
            ArrangerStyle s;
            if (buildConfigFromFile(file, s, false))
            {
                activeArrangerConfig     = s;
                hasActiveArrangerConfig  = true;
                activeArrangerConfigName = cfgName;
                if (arrangerEngine) { arrangerEngine->setStyle(s); arrangerEngine->setBpm(currentTempo); }
            }
        }

        rebuildPlaySettingsItems();
        arrangerStyleList.setActiveConfigName(hasActiveArrangerConfig ? activeArrangerConfigName : juce::String());
    }

    auto tracksVar = obj->getProperty("tracks");

    if (!tracksVar.isArray())
        return;

    auto* tracksArray = tracksVar.getArray();
    int current = 2;

    for (int i = 0; i < allTracks.size() && i < tracksArray->size(); ++i)
    {
        auto trackVar = tracksArray->getReference(i);
        if (!trackVar.isObject())
            continue;

        auto* trackObj = trackVar.getDynamicObject();
        if (!trackObj)
            continue;

        auto name = trackObj->getProperty("name").toString();
        double volume = static_cast<double>(trackObj->getProperty("volume"));
        int instrumentNumber = static_cast<int>(trackObj->getProperty("instrumentNumber"));

        auto uuidSTR = trackObj->getProperty("uuid");

        juce::Uuid uuid{ uuidSTR };
        allTracks[i]->addListener(trackPlayer.get());
        auto type = trackObj->getProperty("type");
        allTracks[i]->setTypeOfTrack(type);

        if (type == "percussion")
        {
            allTracks[i]->setChannel(10);
        }
        else
        {
            allTracks[i]->setChannel(current);
            current++;
        }
        allTracks[i]->setNameLabel(name);
        allTracks[i]->setVolumeSlider(volume);
        allTracks[i]->setVolumeLabel(juce::String(volume));
        allTracks[i]->setInstrumentNumber(instrumentNumber);
        allTracks[i]->setUUID(uuid);
    }
}
