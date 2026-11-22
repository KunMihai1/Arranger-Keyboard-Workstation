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
        const juce::String playingTracksTitle   = "Playing tracks";
        const juce::String noOutputDevice       = "No output device selected.";
        const juce::String noTracksToPlay       = "There are no tracks to play";
        const juce::String pastingTitle         = "Pasting a track";
        const juce::String copyFirst            = "First you have to copy!";
        const juce::String bpmSuffix            = " BPM";
        const juce::String sectionConfigTitle   = "Section configuration";
        const juce::String recordTracksFirst    = "Record or add some tracks first.";
        const juce::String loadingConfiguration = "Loading configuration...";
    }

    constexpr double beatsAtStart        = 0.0;
    constexpr double minTempoBpm         = 60.0;
    constexpr double maxTempoBpm         = 200.0;
    constexpr double tempoStepBpm        = 1.0;
    constexpr int    tempoTextBoxWidth   = 50;
    constexpr int    tempoTextBoxHeight  = 20;
}

bool CurrentStyleComponent::startPlaying()
{
    int selectedID = lastPlayModeId;   // all tracks or solo; the active-config path below ignores this

    stopPlaying(false);

    std::vector<TrackEntry> selectedTracks;

    const bool hasMidiOut     = outputDevice.lock() != nullptr;
    const bool hasAudioInject = trackPlayer && bool(trackPlayer->onMidiMessage);

    if (!hasMidiOut && !hasAudioInject)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::playingTracksTitle, Text::noOutputDevice);
        return false;
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
        return true;
    }

    if (selectedID == PlayModeIds::allTracks)
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
    else if (selectedID == PlayModeIds::soloTrack)
    {
        if (lastSelectedTrack)
        {
            // A "None" track has a null entry (applyBPMchangeBeforePlayback inserts one), so check
            // it like the all-tracks path does: nothing to play, not a crash.
            auto it = mapUuidToTrackEntry.find(lastSelectedTrack->getUsedID());
            if (it != mapUuidToTrackEntry.end() && it->second != nullptr)
                selectedTracks.push_back(*it->second);
        }
    }

    if (selectedTracks.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::playingTracksTitle, Text::noTracksToPlay);
        return false;
    }

    if (arrangerModeEnabled)
    {
        // Phase 1: fixed 4/4; real time-signature wiring is Phase 2.
        ArrangerStyle style = ArrangerPatternBuilder::buildDemoMultiSectionStyle(selectedTracks, ArrangerStyleDefaults::beatsPerBar, ArrangerStyleDefaults::beatUnit, currentTempo);
        arrangerEngine->setStyle(style);
        arrangerEngine->setBpm(currentTempo);   // user's tempo slider wins over the style's original tempo
        arrangerEngine->selectStartSection(pendingStartType, pendingStartName);  // begin on the chosen start
        arrangerEngine->start();
        seedHeldChordIntoArranger();   // honour a chord already held when Start was pressed
        return true;
    }

    trackPlayer->setTracks(selectedTracks);
    trackPlayer->syncPlaybackSettings();
    trackPlayer->start();
    return true;
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
        customBeatBar.setCurrentBeatsElapsed(beatsAtStart);
        customBeatBar.repaint();
        highlightPendingStartSection();   // back to the chosen start (Var 1 by default)
        if (arrangerModeEnabled) tempoSlider.setEnabled(true);   // unlock BPM after a self-stop (Ending)
    };

    // Phase 3: arranger-style authoring overlay (opened from the play-settings dropdown).
    addChildComponent(arrangerStyleList);
    arrangerStyleList.onCreateNew  = [this] { openStyleEditorNew(); };
    arrangerStyleList.onEditStyle  = [this] (const juce::File& styleFile) { openStyleEditorFromFile(styleFile); };
    arrangerStyleList.onLoadStyle  = [this] (const juce::File& styleFile) { loadStyleFileIntoEngine(styleFile); showStyleList(false); };
    arrangerStyleList.onDeleteStyle = [this] (const juce::File& styleFile)
    {
        // If the deleted file was this style's active configuration, clear + persist that.
        if (hasActiveArrangerConfig && activeArrangerConfigName == styleFile.getFileNameWithoutExtension())
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
        // Enter the playing state only if something actually started. With no output device or no
        // tracks, startPlaying() tells the user why and returns false; marking the panel as playing
        // anyway left the beat bar red and (arranger mode) the BPM slider locked until Stop.
        if (! startPlaying())
            return;

        isPlaying = true;
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

    for (int trackIndex {}; trackIndex < trackSlots; trackIndex++)
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
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::pastingTitle, Text::copyFirst);
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

    tempoSlider.setRange(minTempoBpm, maxTempoBpm, tempoStepBpm);
    tempoSlider.setSkewFactorFromMidPoint(ArrangerStyleDefaults::tempoBpm);
    tempoSlider.setValue(currentTempo);
    tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, tempoTextBoxWidth, tempoTextBoxHeight);

    tempoSlider.setColour(juce::Slider::trackColourId, juce::Colours::cornflowerblue);
    tempoSlider.setColour(juce::Slider::backgroundColourId, juce::Colours::darkgrey);
    tempoSlider.setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    tempoSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    tempoSlider.textFromValueFunction = [](double value)
    {
        return juce::String(static_cast<int>(value)) + Text::bpmSuffix;
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

void CurrentStyleComponent::presentOverlay(juce::Component& overlay)
{
    // Authoring (browser/editor) is a separate mode from live performance and shares the single
    // ArrangerEngine. Stop any live playback before showing the overlay so the editor's preview
    // doesn't inherit/fight the live engine state (which made entering edit show a stale section
    // and fire a queued Ending on close).
    if (arrangerEngine && arrangerEngine->isPlaying())
    {
        arrangerEngine->stop();
        isPlaying = false;
        customBeatBar.setCurrentBeatsElapsed(beatsAtStart);
        customBeatBar.repaint();
    }

    // Authoring overlays are full-screen; tell the host to hide the GL note layer (it renders on
    // top of normal painting and would otherwise show through the middle of the overlay).
    if (onAuthoringOverlayVisible) onAuthoringOverlayVisible(true);

    auto* top = getTopLevelComponent();
    if (top == nullptr)
        top = this;
    top->addAndMakeVisible(overlay);
    overlay.setBounds(top->getLocalBounds());
    overlay.toFront(true);
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
            Text::sectionConfigTitle, Text::recordTracksFirst);
        return;
    }

    arrangerStyleEditor = std::make_unique<ArrangerStyleEditor>(*arrangerEngine);
    configureEditorCallbacks();
    presentOverlay(*arrangerStyleEditor);
    arrangerStyleEditor->loadRecording(tracks, currentTempo, ArrangerStyleDefaults::beatsPerBar, ArrangerStyleDefaults::beatUnit, juce::String()); // default generic config name
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
    arrangerStyleEditor->onSaved = [this] (const juce::File& styleFile, const ArrangerStyle& style)
    {
        arrangerStyleList.refresh();
        applyActiveConfig(styleFile, style);
    };
}

void CurrentStyleComponent::applyActiveConfig(const juce::File& styleFile, const ArrangerStyle& style)
{
    activeArrangerConfig     = style;      // this configuration is now what Start + section buttons use
    hasActiveArrangerConfig  = true;
    activeArrangerConfigName = styleFile.getFileNameWithoutExtension();
    rebuildPlaySettingsItems();
    arrangerStyleList.setActiveConfigName(activeArrangerConfigName);
    if (anyTrackChanged) anyTrackChanged();   // persist the selection to allStyles.json

    // A freshly-loaded config starts armed on Variation 1 (highlighted); the user can pick another
    // start section before pressing Start.
    pendingStartType = ArrangerSectionType::Variation;
    pendingStartName = DefaultSectionNames::variation;
    highlightPendingStartSection();
}

void CurrentStyleComponent::openStyleEditorFromFile(const juce::File& styleFile)
{
    // Parse the (large) file off the message thread so the UI doesn't freeze; show the app overlay
    // meanwhile. Build the editor + present it back on the message thread once parsing is done.
    if (onBusy) onBusy(true, Text::loadingConfiguration);
    juce::Component::SafePointer<CurrentStyleComponent> safe(this);

    juce::Thread::launch([safe, styleFile]
    {
        auto sf = std::make_shared<ArrangerStyleFile>();
        juce::String err;
        const bool ok = ArrangerStyleIOHelper::loadFromFile(styleFile, *sf, err);

        juce::MessageManager::callAsync([safe, styleFile, sf, ok, err]
        {
            if (safe == nullptr)
                return;
            if (safe->onBusy) safe->onBusy(false, {});
            if (! ok)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::sectionConfigTitle, err);
                return;
            }
            safe->arrangerStyleEditor = std::make_unique<ArrangerStyleEditor>(*safe->arrangerEngine);
            safe->configureEditorCallbacks();
            safe->presentOverlay(*safe->arrangerStyleEditor);
            safe->arrangerStyleEditor->loadFromFile(*sf);
            safe->arrangerStyleEditor->setSourceFile(styleFile);   // so Save renames this file instead of duplicating it
        });
    });
}

bool CurrentStyleComponent::buildConfigFromFile(const juce::File& styleFile, ArrangerStyle& out, bool showError)
{
    ArrangerStyleFile sf; juce::String err;
    if (! ArrangerStyleIOHelper::loadFromFile(styleFile, sf, err))
    {
        if (showError)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::sectionConfigTitle, err);
        return false;
    }
    out = ArrangerPatternBuilder::buildStyleFromFile(sf);
    return true;
}

void CurrentStyleComponent::loadStyleFileIntoEngine(const juce::File& styleFile)
{
    // Parse the (large) file off the message thread (the hex-decode of every event is what froze the
    // UI); show the app overlay meanwhile. Build + apply back on the message thread when it's done.
    if (onBusy) onBusy(true, Text::loadingConfiguration);
    juce::Component::SafePointer<CurrentStyleComponent> safe(this);

    juce::Thread::launch([safe, styleFile]
    {
        auto sf = std::make_shared<ArrangerStyleFile>();
        juce::String err;
        const bool ok = ArrangerStyleIOHelper::loadFromFile(styleFile, *sf, err);

        juce::MessageManager::callAsync([safe, styleFile, sf, ok]
        {
            if (safe == nullptr)
                return;
            if (safe->onBusy) safe->onBusy(false, {});
            if (! ok)
                return;

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromFile(*sf);
            safe->applyActiveConfig(styleFile, style);   // this configuration is now what Start + section buttons use

            if (safe->arrangerEngine)
            {
                safe->arrangerEngine->setStyle(style);          // prime the engine, but don't start:
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
        onArrangerSectionChanged (SectionHighlight::byTypeAndName, type, name);    // idx>=0 -> highlight by type + name
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


void CurrentStyleComponent::stoppingPlayer()
{
    if (trackPlayer)
    {
        this->trackPlayer->stop();
        trackPlayer.reset();
    }
}
