#include "MainComponent.h"

namespace
{
    juce::String makeDeviceSettingsKey(const juce::String& VID, const juce::String& PID)
    {
        juce::String deviceKey = VID + "_" + PID;
        if (deviceKey == "_") deviceKey = "default";
        return deviceKey;
    }
}

//==============================================================================
MainComponent::MainComponent()
{
    settingsInit();
    playButtonInit();
    headerPanelInit();
    togglePlayButton();
    initalizeSaveFileForUser();
    cachedImageMainWindow = juce::ImageFileFormat::loadFrom(BinaryData::MainWindow_png, BinaryData::MainWindow_pngSize);
    playBackground = juce::ImageFileFormat::loadFrom(BinaryData::playingBackground_png, BinaryData::playingBackground_pngSize);
    currentBackground = cachedImageMainWindow;

    buildChordLibrary();

    displayInit();

    setBounds(0, 0, juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea.getWidth(),
        juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea.getHeight() - 40);
    this->addKeyListener(&this->keyListener);
    addKeyListener(this);

    client = std::make_shared<SupabaseClient>();

    loginWindowInitialize();

    openingAudioLabel.setText("Preparing session...", juce::dontSendNotification);
    openingAudioLabel.setJustificationType(juce::Justification::centred);
    openingAudioLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    openingAudioLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    openingAudioLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.7f));
    openingAudioLabel.setAlwaysOnTop(true);
    addAndMakeVisible(openingAudioLabel);
    openingAudioLabel.setVisible(false);

    midiHandler.addListener(&recordPlayer);
    midiHandler.addListener(&keyboard);
    
    display->addListener(&midiHandler);
    this->display->callingListeners();

    MIDIDevice.getAvailableDevicesMidiIN(this->devicesIN);
    MIDIDevice.getAvailableDevicesMidiOUT(this->devicesOUT);
    this->toFront(true);

    if(settingsPanel.isVisible())
        populateUpdateComboBoxDevices();

    midiHandler.onStartNoteSetting = [this]()
    {
        if (display)
            display->startingPlayer();
    };

    midiHandler.onEndNoteSetting = [this]()
    {
        if (display)
        {
            display->stoppingPlayer();
        }
    };

    // Phase 4: route the recognized left-hand/whole-keyboard chord to the arranger engine so the
    // accompaniment transposes to what the player holds.
    midiHandler.onChordChanged = [this](const ArrangerChord& chord)
    {
        if (display)
            display->setArrangerLiveChord(chord);
    };

    juce::Desktop::getInstance().addFocusChangeListener(this);

    // Korg/Yamaha-style focus policy: listen to mouse clicks on the whole play
    // scene (this component and every nested child) so we can always return
    // keyboard focus to the play surface — see mouseDown().
    addMouseListener(this, true);

    juce::MessageManager::callAsync([this]()
        {
            if (isVisible())
                grabKeyboardFocus();
        });

    initializeOverlay();


    sfzManager.load(IOHelper::getFile("SFZLibrary.json"));
    sfzLibraryUI = std::make_unique<SFZLibraryUI>(sfzManager);
    addAndMakeVisible(*sfzLibraryUI);
    sfzLibraryUI->setVisible(false);

    startTimer(1000);

    // Catch device hot-unplug the instant it happens (the 1s timer above is only a backstop). The
    // callback runs on the message thread (AsyncUpdater), so it can touch the UI directly.
    midiDeviceListConnection = juce::MidiDeviceListConnection::make ([this] { checkMidiDevicesValid(); });

}

MainComponent::~MainComponent()
{
    sfzManager.save(IOHelper::getFile("SFZLibrary.json"));

    if (this->MIDIDevice.isOpenIN())
        this->MIDIDevice.deviceCloseIN();
    if (this->MIDIDevice.isOpenOUT())
    {
        this->display->stoppingPlayer();
        this->MIDIDevice.deviceCloseOUT();
    }
    if(this->MIDIDevice.isOpenAudioOUT())
    {
        if (audioHandler != nullptr)
            this->MIDIDevice.getAudioDeviceManager().removeAudioCallback(audioHandler.get());
        this->display->stoppingPlayer();
        this->MIDIDevice.deviceCloseAudioOUT();
	}
    if (treeView != nullptr && treeView->getRootItem() != nullptr)
    {
        //DBG("Deleting root item dest main: " << treeView->getRootItem()->getUniqueName());
        treeView->deleteRootItem();
        treeView->deleteRootItem();
    }
    else;
        //DBG("No root item to delete destructor main");
    removeKeyListener(this);
    if(&keyListener)
        removeKeyListener(&this->keyListener);
    if(&recordPlayer)
        midiHandler.removeListener(&recordPlayer);
    if(noteLayer)
        midiHandler.removeListener(noteLayer.get());
    if(&keyboard)
        midiHandler.removeListener(&keyboard);
    if (display)
        display->removeListener(&midiHandler);

    volumeKnob.setLookAndFeel(nullptr);  

}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    if (currentBackground.isValid())
    {
            juce::Rectangle<float> bounds = getLocalBounds().toFloat();
            auto imageWidth = (float)cachedImageMainWindow.getWidth();
            auto imageHeight = (float)cachedImageMainWindow.getHeight();
            auto scale = juce::jmax(bounds.getWidth() / imageWidth, bounds.getHeight() / imageHeight);
            auto newWidth = imageWidth * scale;
            auto newHeight = imageHeight * scale;
            auto x = (bounds.getWidth() - newWidth) * 0.5f;
            auto y = (bounds.getHeight() - newHeight) * 0.5f;
            g.setColour(juce::Colours::grey);
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            g.drawImage(currentBackground, juce::Rectangle<float>(x, y, newWidth, newHeight));
        g.setFont(juce::FontOptions(16.0f));
        g.setColour(juce::Colours::white);
    }
}

void MainComponent::paintOverChildren(juce::Graphics& g)
{
    if (!openingAudioLabel.isVisible())
        return;

    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillAll();

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawFittedText(openingAudioLabel.getText(),
                     getLocalBounds().reduced(20),
                     juce::Justification::centred, 1);

    // The overlay is now actually on screen: run the deferred blocking init exactly once,
    // after this paint completes (callAsync keeps us out of the paint call itself).
    if (pendingPlayInit)
    {
        pendingPlayInit = false;
        juce::MessageManager::callAsync([this]
        {
            playButtonOnClick();
            openingAudioLabel.setVisible(false);

            // The play scene is now fully built (devices opened, samples loaded,
            // child components created). Reclaim keyboard focus once more — the
            // scene setup can land focus on a freshly created child, which is why
            // you sometimes had to click the screen before the PC keyboard played.
            juce::MessageManager::callAsync([this]()
            {
                if (isVisible())
                    grabKeyboardFocus();
            });
        });
    }
}

void MainComponent::focusGained(juce::Component::FocusChangeType)
{

    if (noteLayer)
    {
        noteLayer->toFront(false);
    }
}

void MainComponent::globalFocusChanged(juce::Component* focusedComponent)
{
    if (focusedComponent == nullptr)
    {
        // Focus left JUCE — but on Windows this also fires briefly during
        // within-app window switches.  Defer the hide so we don't flicker.
        juce::Timer::callAfterDelay(150,
            [safeThis = juce::Component::SafePointer<MainComponent>(this)]()
            {
                if (safeThis == nullptr) return;

                // After the delay, check if the app truly lost foreground.
                if (!juce::Process::isForegroundProcess())
                {
                    // overlayWindow is an in-app child now, so it isn't toggled on app focus loss.
                    if (safeThis->midiWindow)
                        safeThis->midiWindow->setVisible(false);

                    if (safeThis->soundEffectWindow)
                        safeThis->soundEffectWindow->setVisible(false);
                }
            });
    }
    else
    {
        // Focus returned to the app — only restore windows that were
        // actually hidden (e.g. after alt-tab away). During normal
        // within-app focus transitions nothing was hidden, so this
        // branch does nothing, avoiding any native-window side effects.
        // (overlayWindow is an in-app child now and isn't restored here.)
        if (midiWindow && !midiWindow->isVisible() && midiWindowShouldBeVisible)
            midiWindow->setVisible(true);

        if (soundEffectWindow && !soundEffectWindow->isVisible() && soundEffecttWindowShouldBeVisible)
            soundEffectWindow->setVisible(true);
    }
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    // Korg/Yamaha-style: the instrument keeps listening to the PC keyboard.
    // Any click on the play scene that isn't text entry returns keyboard focus
    // to the play surface, so the player can always keep playing after touching
    // a transport/section button, the config browser, etc.  Without this, focus
    // could drift onto a clicked control and never come back, silencing the
    // PC keyboard until the app was restarted.

    // Don't fight modal popups (combo menus, alert dialogs) or the in-app
    // overlay menu — those manage their own focus (and the overlay has a
    // documented native-window focus dance we must not disturb).
    if (juce::Component::getCurrentlyModalComponent() != nullptr)
        return;
    if (overlayWindow && overlayWindow->isVisible())
        return;

    // Leave focus alone while the user is typing into a text field.
    for (auto* c = e.eventComponent; c != nullptr; c = c->getParentComponent())
        if (dynamic_cast<juce::TextEditor*>(c) != nullptr)
            return;

    // Defer to after the click is fully handled (e.g. a button's own action),
    // then pull focus back to the play surface.
    juce::MessageManager::callAsync(
        [safeThis = juce::Component::SafePointer<MainComponent>(this)]()
        {
            if (safeThis != nullptr && safeThis->isVisible())
                safeThis->grabKeyboardFocus();
        });
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    openingAudioLabel.setBounds(getLocalBounds());
    if (overlayWindow)
        overlayWindow->setBounds(getLocalBounds());
    int buttonWidth = getWidth() / 16;
    int buttonHeight = getHeight() / 24;
    int x = (int)(getWidth() * 0.5 - getWidth() * 0.175);
    int y = (int)(getHeight() * 0.5 + getHeight() * 0.162);

    playButton.setBounds(x, y, buttonWidth, buttonHeight);
    settingsButton.setBounds(0, 0, 250, 70);
    settingsPanel.setBounds(0, 70, 250, getHeight());
    midiButton.setBounds(0, 5, 200, 50);
    updateNumberOfKeysDevice.setBounds(0, midiButton.getBottom() + 10, 200, 50);
    devicesCBUpdate.setBounds(0, updateNumberOfKeysDevice.getBottom() + 5, 200, 25);

    helpIcon.setBounds(210, 20, 100, 20);

    headerPanel.setBounds(0, 0, getWidth(), 100);
    homeButton.setBounds(10, 10, 75, 30);
    colourSelectorButton.setBounds(90, 10, 100, 30);
    instrumentSelectorButton.setBounds(195, 10, 100, 30);
    particleToggle.setBounds(colourSelectorButton.getX(), colourSelectorButton.getY() +colourSelectorButton.getHeight(), 100, 25);
    noteNumbersAnnotation.setBounds(particleToggle.getX(), particleToggle.getBottom(), 100, 25);

    leftHandInstrumentToggle.setBounds(instrumentSelectorButton.getX(), instrumentSelectorButton.getY() + instrumentSelectorButton.getHeight(), 100, 25);
    rightHandInstrumentToggle.setBounds(instrumentSelectorButton.getX(), leftHandInstrumentToggle.getY() + leftHandInstrumentToggle.getHeight(), 100, 25);

    startPlayback.setBounds(getWidth() - 30 - 35, 10, 30, 30);
    stopRecording.setBounds(startPlayback.getX()-30-10, 10, 30, 30);
    startRecording.setBounds(stopRecording.getX()-30-10, 10, 30, 30);

    chordHelperButton.setBounds(getLocalBounds().getRight()-205, startRecording.getBottom()+5, 200, 50);

    saveRecordingButton.setBounds(startRecording.getX()-140-10, 10, 140, 30);
    playRecordingFileButton.setBounds(305, 10, 100, 30);

    if (sfzLibraryUI)
        sfzLibraryUI->setBounds(getLocalBounds().withSizeKeepingCentre(500, 400));



    //volumeKnob.setBounds(getWidth() / 2, 0, 70, 50);

    // Sections (intros/endings) start right after playRecordingFileButton and need ~240px minimum.
    // Push display rightward on narrow screens rather than collapsing the sections.
    {
        const int minSectionWidth = 240;
        int sectionStart = playRecordingFileButton.getRight() + 10;
        int displayX = juce::jmax(sectionStart + minSectionWidth + 5,
                                   (headerPanel.getWidth() - 400) / 2);
        display->setBounds(displayX, 0, 400, 100);
    }

    if (noteLayer)
    {
        noteLayer->toFront(false);
        noteLayer->setAlwaysOnTop(true);
        noteLayer->setOpaque(true);
        keyboard.toFront(false);
        keyboard.setAlwaysOnTop(true);
        keyboard.setOpaque(true);
    }   

    if (introsEndings)
    {
        int width = display->getX() - playRecordingFileButton.getX() - playRecordingFileButton.getWidth() - 10;
        int height = playRecordingFileButton.getHeight() + 20;
        introsEndings->setBounds(playRecordingFileButton.getX() + playRecordingFileButton.getWidth() + 10, playRecordingFileButton.getY(), width, height);
    }

    if (variationsFills)
    {
        int varFillsX = playRecordingFileButton.getX();
        int varFillsY = introsEndings->getY() + introsEndings->getHeight()-7;
        int width = display->getX() - rightHandInstrumentToggle.getX() - rightHandInstrumentToggle.getWidth() - 10;
        int height = display->getHeight() - (introsEndings->getY() + introsEndings->getHeight());
        variationsFills->setBounds(varFillsX, varFillsY, width, height+10);
    }
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == colourSelector)
    {
        auto newColor = colourSelector->getCurrentColour();
        //DBG("Colour changed to: " << newColor.toString());
        this->noteLayer->setColourParticle(newColor);
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{

    if (key == juce::KeyPress::escapeKey)
    {

        if (!overlayWindow->isVisible())
        {
            overlayShouldBeVisible = true;
            setOverlayMenuVisible(true);
        }
        else
        {
            if (overlayWindow->onRequestClose)
                overlayWindow->onRequestClose();
            else
                setOverlayMenuVisible(false);
        }
    }

    if (!this->keyListener.getIsKeyboardInput())
        return false;

    int startNote = this->keyListener.getStartNoteKeyboardInput();
    int finishNote = this->keyListener.getFinishNoteKeyboardInput();
    //DBG("Start note:" << startNote);
    //DBG("End note:" << finishNote);

    if (key.getKeyCode() == 'Z')
    {
        if (startNote >= 24)
        {
            this->keyListener.setStartNoteKeyboardInput(startNote - 12);
            this->keyListener.setFinishNoteKeyboardInput(finishNote - 12);
            this->keyboard.set_min_and_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());
            
            this->midiHandler.allOffKeyboard();
            this->noteLayer->resetStateActiveNotes();
            this->keyListener.resetState();
            this->keyboard.setIsDrawn(false);
            this->keyboard.resetStateActiveNotes();
            this->display->set_min_max(startNote - 12, finishNote - 12);
            this->display->setNewSettingsHelperFunction(-12);
        }
    }
    else if (key.getKeyCode() == 'X')
    {
        //DBG("Finish note" << finishNote);
        if (finishNote <= 101)
        {
            this->keyListener.setStartNoteKeyboardInput(startNote + 12);
            this->keyListener.setFinishNoteKeyboardInput(finishNote + 12);
            this->keyboard.set_min_and_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());

            this->midiHandler.allOffKeyboard();
            this->noteLayer->resetStateActiveNotes();
            this->keyListener.resetState();
            this->keyboard.setIsDrawn(false);
            this->keyboard.resetStateActiveNotes();
            this->display->set_min_max(startNote + 12, finishNote + 12);
            this->display->setNewSettingsHelperFunction(12);
        }
    }
    return false;
}

void MainComponent::timerCallback()
{
    checkMidiDevicesValid();   // self-gates on which devices are open; also fires on hot-plug via the connection

    if (settingsPanel.isVisible())
        populateUpdateComboBoxDevices();
}

void MainComponent::checkMidiDevicesValid()
{
    // OUTPUT first: if the open MIDI output vanished (e.g. the keyboard was unplugged), stop cleanly.
    // The arranger/track-player run on their own HighResolutionTimer threads and would keep calling
    // sendMessageNow on the now-dead endpoint -- JUCE asserts isAlive() and a Debug build crashes.
    if (MIDIDevice.isOpenOUT())
    {
        const auto outId = MIDIDevice.getOutputIdentifier();
        bool present = false;
        for (auto& d : juce::MidiOutput::getAvailableDevices())
            if (d.identifier == outId) { present = true; break; }

        if (! present)
            handleMidiOutputDisconnected();
    }

    // INPUT: if the open MIDI input vanished, close it and clear any stuck-note visuals. Independent
    // of the output so a separate output device (or the internal synth) keeps playing on input loss.
    if (MIDIDevice.isOpenIN())
    {
        const auto inId = MIDIDevice.get_identifier();
        bool present = false;
        for (auto& d : juce::MidiInput::getAvailableDevices())
            if (d.identifier == inId) { present = true; break; }

        if (! present)
        {
            if (noteLayer)
                noteLayer->resetStateActiveNotes();

            keyboard.resetStateActiveNotes();
            this->MIDIDevice.deviceCloseIN();
        }
    }
}

void MainComponent::handleMidiOutputDisconnected()
{
    // Drop the shared output handle FIRST. Every sender (arranger engine, track player, MidiHandler)
    // holds only a weak_ptr to it, so once the single shared owner is gone their lock() returns null
    // and they stop sending to the dead endpoint -- without us writing their weak_ptr from this thread
    // (which would race their timer thread). One in-flight send may still slip through before this runs;
    // in Release that is harmless (the jassert is compiled out) and the classic MidiOutput exposes no
    // liveness query to close that last gap.
    this->MIDIDevice.deviceCloseOUT();
    deviceOpenedOUT = {};

    // Then stop the transport via the same path as the Stop button, so the play UI stays consistent.
    // Its note-offs are no-ops on the released external output and still reach the internal synth.
    if (display != nullptr)
        display->stoppingPlayer();

    // Tell the user why playback stopped (mirrors the existing transient-message pattern).
    const juce::String msg = "MIDI output disconnected - playback stopped";
    if (temporaryPopup) { temporaryPopup->updateText(msg); temporaryPopup->restartTimer(); }
    else
    {
        temporaryPopup = std::make_unique<TemporaryMessage>(msg);
        headerPanel.addChildComponent(temporaryPopup.get());
        temporaryPopup->setBounds(getWidth() / 2 - 150, 10, 300, 30);
        temporaryPopup->setFinishedCallBack([this] { temporaryPopup.reset(); });
        temporaryPopup->setVisible(true);
    }
}

void MainComponent::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &devicesCBUpdate)
    {

    }
}

void MainComponent::populateUpdateComboBoxDevices()
{
    bool ok = false;
    if (last == -1 || last != MIDIDevice.getNrInputActualDevices())
    {
        lastOfLast = last;
        last = MIDIDevice.getNrInputActualDevices();
        ok = true;
    }

    if (!ok)
        return;
    devicesCBUpdate.clear();
    int i = 1;
    for (auto& dev : this->MIDIDevice.getAvailableInputDevicesNameIdentifier())
    {
        juce::String name = dev.first;
        juce::String identifier = dev.second;
        juce::String PID = MIDIDevice.extractPID(identifier);
        juce::String VID = MIDIDevice.extractVID(identifier);
        if (dataBase.deviceExists(VID, PID)==false)
            continue;

        if (i == 1)
            updateDevicesMap.clear(); //map needs to be here because if we are here that means there has been at least a change in the devices, otherwise we clear the map each time


        updateDevicesMap[name] = identifier;

        this->devicesCBUpdate.addItem(name, i++);
    }
    if(lastOfLast!=-1)
    {
        devicesCBUpdate.hidePopup();
        devicesCBUpdate.showPopup();
    }
    devicesCBUpdate.setSelectedId(1);
}

void MainComponent::initalizeSaveFileForUser()
{
    juce::PropertiesFile::Options options;
    options.applicationName= "Piano Synth2";
    options.filenameSuffix = "settings";
    options.folderName="Piano Synth2";
    options.osxLibrarySubFolder = "Application Support";
    options.commonToAllUsers = false;

    appProperties.setStorageParameters(options);
    propertiesFile = appProperties.getUserSettings();
}

void MainComponent::loginWindowInitialize()
{
    playButton.setVisible(false);
    loginWindow = std::make_unique<LoginComponent>(client);
    addAndMakeVisible(loginWindow.get());

    auto parentBounds = getLocalBounds();
    int panelWidth = 400;
    int panelHeight = 400;

    int x = (parentBounds.getWidth() - panelWidth) / 2;
    int y = (parentBounds.getHeight() - panelHeight) / 2;

    loginWindow->setBounds(x, y, panelWidth, panelHeight);

    loginWindow->onSuccessfullLogin = [this]()
    {
        /*
        playtimeTracker = std::make_unique<PlaytimeTracker>([this](int secondsToIncreaseWith)
            {
                auto clientPtr = client; 
                auto VID = MIDIDevice.getVID();
                auto PID = MIDIDevice.getPID();

                std::thread([clientPtr,VID,PID,secondsToIncreaseWith]() {
                    clientPtr->incrementPlaytime(secondsToIncreaseWith,VID,PID);
                    }).detach();
            },300);
        */
        playButton.setVisible(true);
        loginWindow->setVisible(false);
       
        juce::MessageManager::callAsync([this]() {
            loginWindow.reset();
            });
        
    };

}

void MainComponent::applySettingsToChannel(const SoundSettings& s, int channel)
{
    MIDIDevice.setReverb(s.reverb, channel);
    MIDIDevice.setVolume(s.volume, channel);

    MIDIDevice.setBrightness(s.brightness, channel);
    MIDIDevice.setChorus(s.chorus, channel);
    MIDIDevice.setExpression(s.expression, channel);
    MIDIDevice.setResonance(s.resonance, channel);
    MIDIDevice.setSustainToggle(s.sustainToggle, channel);

    MIDIDevice.setAttack(s.attack, channel);
    MIDIDevice.setDecay(s.decay, channel);
    MIDIDevice.setRelease(s.release, channel);
    MIDIDevice.setVibrato(s.vibrato, channel);

    MIDIDevice.setDelay(s.delay, channel);
    MIDIDevice.setPan(s.pan, channel);

    MIDIDevice.setDistortion(s.distortion, channel);
    MIDIDevice.setFilterTrack(s.filterTrack, channel);
    MIDIDevice.setTremolo(s.tremolo, channel);
    MIDIDevice.setRandomMod(s.randomMod, channel);
}

juce::String MainComponent::applyEffectCC(int ccNumber, int value, int channel)
{
    switch (ccNumber)
    {
        case 74: MIDIDevice.setBrightness(value, channel);  return "brightness";
        case 11: MIDIDevice.setExpression(value, channel);  return "expression";
        case 93: MIDIDevice.setChorus(value, channel);      return "chorus";
        case 71: MIDIDevice.setResonance(value, channel);   return "resonance";
        case 64: MIDIDevice.setSustainToggle(value, channel); return "sustainToggle";

        case 1:  MIDIDevice.setVibrato(value, channel);     return "vibrato";
        case 73: MIDIDevice.setAttack(value, channel);      return "attack";
        case 75: MIDIDevice.setDecay(value, channel);       return "decay";
        case 72: MIDIDevice.setRelease(value, channel);     return "release";

        case 7:  MIDIDevice.setVolume(value, channel);      return "volume";
        case 91: MIDIDevice.setReverb(value, channel);      return "reverb";
        case 94: MIDIDevice.setDelay(value, channel);       return "delay";
        case 10: MIDIDevice.setPan(value, channel);         return "pan";

        case 80: MIDIDevice.setDistortion(value, channel);  return "distortion";
        case 76: MIDIDevice.setFilterTrack(value, channel); return "filterTrack";
        case 92: MIDIDevice.setTremolo(value, channel);     return "tremolo";
        case 95: MIDIDevice.setRandomMod(value, channel);   return "randomMod";

        default: return {};
    }
}

void MainComponent::getCurrentEffectDeviceIds(juce::String& VID, juce::String& PID)
{
    if (display)
    {
        VID = display->getVID();
        PID = display->getPID();
        return;
    }

    VID = MIDIDevice.getVID();
    PID = MIDIDevice.getPID();
}

juce::String MainComponent::getCurrentEffectSettingsKey(const juce::String& styleID)
{
    juce::String VID, PID;
    getCurrentEffectDeviceIds(VID, PID);
    return styleID + "." + makeDeviceSettingsKey(VID, PID);
}

void MainComponent::loadEffectsFromFile(const juce::String& styleID)
{
    SoundSettings firstChannel, secondChannel;
    juce::String VID, PID;
    getCurrentEffectDeviceIds(VID, PID);
    const juce::String settingsKey = styleID + "." + makeDeviceSettingsKey(VID, PID);

    auto it = styleSettingsMap.find(settingsKey);
    if (it != styleSettingsMap.end())
    {
        applySettingsToChannel(it->second.firstHand, 1);
        applySettingsToChannel(it->second.secondHand, 16);
        return;
    }

    SoundSettings first = EffectSettingsIOHelper::loadEffectsStyle(propertiesFile, styleID, VID, PID, 1);
    SoundSettings second = EffectSettingsIOHelper::loadEffectsStyle(propertiesFile, styleID, VID, PID, 16);

    styleSettingsMap[settingsKey] = StyleSettings{ first, second };

    applySettingsToChannel(first, 1);
    applySettingsToChannel(second, 16);
}

void MainComponent::loadSettings(const juce::String& styleID)
{
    if (propertiesFile)
    {

        loadEffectsFromFile(styleID);

        int leftHandInstrumentNumber = propertiesFile->getIntValue(styleID + "_leftInstrumentNumber", 0);
        int rightHandInstrumentNumber = propertiesFile->getIntValue(styleID + "_rightInstrumentNumber", 0);


        this->midiHandler.setProgramNumber(leftHandInstrumentNumber, "left");
        this->midiHandler.setProgramNumber(rightHandInstrumentNumber, "right");
    }
}

void MainComponent::playChordOnClick(const Chord& c)
{
    auto notes = ChordHelper::getNotesForChord(c);

    for (auto note : notes)
    {
        midiHandler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOn(1, note, (juce::uint8)100));

        juce::Timer::callAfterDelay(500, [this, note]() {
            midiHandler.handleIncomingMidiMessage(nullptr, juce::MidiMessage::noteOff(1, note));
        });
    }
}

void MainComponent::buildChordLibrary()
{
    myChordLibrary.clear();

    std::vector<juce::String> roots =
    {
        "C","C#","D","D#","E",
        "F","F#","G","G#","A","A#","B"
    };

    std::vector<juce::String> types =
    {
        "Major",
        "Minor",
        "7",
        "Augmented",
        "Diminished"
    };

    for (const auto& root : roots)
    {
        for (const auto& type : types)
        {
            Chord chord;

            if (type == "7")
                chord.name = root + "7";
            else
                chord.name = root + " " + type;

            myChordLibrary.push_back(chord);
        }
    }

}

void MainComponent::setCallBacksForEffectWindow()
{
    std::function<void(int, int)> saveCallback = [this](int ccNumber, int value)
    {
        int currentChannel = this->soundEffectWindow->getContent().getSelectedChannel();
        juce::String styleID = display->getStyleID();
        juce::String VID, PID;
        getCurrentEffectDeviceIds(VID, PID);
        const juce::String settingsKey = styleID + "." + makeDeviceSettingsKey(VID, PID);

        const juce::String key = applyEffectCC(ccNumber, value, currentChannel);
        if (key.isEmpty())
            return;

        StyleSettings& s = styleSettingsMap[settingsKey];
        if (currentChannel == 1) s.firstHand.setValue(key, value);
        else if (currentChannel == 16) s.secondHand.setValue(key, value);

        EffectSettingsIOHelper::saveSingleEffect(propertiesFile, styleID, VID, PID, currentChannel, key, value);

        if (!playButton.isVisible())
        {
            if (MIDIDevice.isOpenOUT())
                MIDIDevice.sendMidiCC(currentChannel, ccNumber, value);
            if (MIDIDevice.isOpenAudioOUT())
                midiHandler.injectCC(currentChannel, ccNumber, value);
        }
    };


    auto& content = this->soundEffectWindow->getContent();

    // Line 1 – Tone
    content.getBrightnessKnob().onValueChanged = saveCallback;
    content.getExpressionKnob().onValueChanged = saveCallback;
    content.getChorusKnob().onValueChanged = saveCallback;
    content.getResonanceKnob().onValueChanged = saveCallback;
    content.getSustainToggle().onValueChanged = saveCallback;

    // Line 2 – Envelope
    content.getAttackKnob().onValueChanged   = saveCallback;
    content.getDecayKnob().onValueChanged     = saveCallback;
    content.getReleaseKnob().onValueChanged   = saveCallback;
    content.getVibratoKnob().onValueChanged = saveCallback;

    // Line 3 – Space
    content.getVolumeKnob().onValueChanged = saveCallback;
    content.getReverbKnob().onValueChanged = saveCallback;
    content.getDelayKnob().onValueChanged     = saveCallback;
    content.getPanKnob().onValueChanged       = saveCallback;

    // Line 4 – Modulation
    content.getDistortionKnob().onValueChanged  = saveCallback;
    content.getFilterTrackKnob().onValueChanged = saveCallback;
    content.getTremoloKnob().onValueChanged     = saveCallback;
    content.getRandomModKnob().onValueChanged   = saveCallback;

    content.onChannelChanged = [this](int channel)
    {
        auto& content = this->soundEffectWindow->getContent();

        // Line 1 – Tone
        content.getBrightnessKnob().setValue(MIDIDevice.getBrightness(channel));
        content.getExpressionKnob().setValue(MIDIDevice.getExpression(channel));
        content.getChorusKnob().setValue(MIDIDevice.getChorus(channel));
        content.getResonanceKnob().setValue(MIDIDevice.getResonance(channel));
        content.getSustainToggle().setOn(MIDIDevice.getSustainToggle(channel));

        // Line 2 – Envelope
        content.getAttackKnob().setValue(MIDIDevice.getAttack(channel));
        content.getDecayKnob().setValue(MIDIDevice.getDecay(channel));
        content.getReleaseKnob().setValue(MIDIDevice.getRelease(channel));
        content.getVibratoKnob().setValue(MIDIDevice.getVibrato(channel));

        // Line 3 – Space
        content.getVolumeKnob().setValue(MIDIDevice.getVolume(channel));
        content.getReverbKnob().setValue(MIDIDevice.getReverb(channel));
        content.getDelayKnob().setValue(MIDIDevice.getDelay(channel));
        content.getPanKnob().setValue(MIDIDevice.getPan(channel));

        // Line 4 – Modulation
        content.getDistortionKnob().setValue(MIDIDevice.getDistortion(channel));
        content.getFilterTrackKnob().setValue(MIDIDevice.getFilterTrack(channel));
        content.getTremoloKnob().setValue(MIDIDevice.getTremolo(channel));
        content.getRandomModKnob().setValue(MIDIDevice.getRandomMod(channel));
    };
}
void MainComponent::setInitialValuesFromSettingsFileEffectWindow()
{
    auto& content = soundEffectWindow->getContent();

    content.getBrightnessKnob().setValue(MIDIDevice.getBrightness(1));
    content.getChorusKnob().setValue(MIDIDevice.getChorus(1));
    content.getExpressionKnob().setValue(MIDIDevice.getExpression(1));
    content.getResonanceKnob().setValue(MIDIDevice.getResonance(1));
    content.getSustainToggle().setOn(MIDIDevice.getSustainToggle(1));

    // Line 2 - Envelope & Modulation
    content.getAttackKnob().setValue(MIDIDevice.getAttack(1));
    content.getDecayKnob().setValue(MIDIDevice.getDecay(1));
    content.getReleaseKnob().setValue(MIDIDevice.getRelease(1));
    content.getVibratoKnob().setValue(MIDIDevice.getVibrato(1));

    // Line 3 - Effects & Pan
    content.getVolumeKnob().setValue(MIDIDevice.getVolume(1));
    content.getReverbKnob().setValue(MIDIDevice.getReverb(1));
    content.getDelayKnob().setValue(MIDIDevice.getDelay(1));
    content.getPanKnob().setValue(MIDIDevice.getPan(1));

    // Line 4 - Additional Effects
    content.getDistortionKnob().setValue(MIDIDevice.getDistortion(1));
    content.getFilterTrackKnob().setValue(MIDIDevice.getFilterTrack(1));
    content.getTremoloKnob().setValue(MIDIDevice.getTremolo(1));
    content.getRandomModKnob().setValue(MIDIDevice.getRandomMod(1));
}

void MainComponent::sendEffectsBeforePlaying()
{
    const bool hasOut   = MIDIDevice.isOpenOUT();
    const bool hasAudio = MIDIDevice.isOpenAudioOUT();

    if (!hasOut && !hasAudio)
        return;

    struct EffectCC
    {
        int ccNumber;
        std::function<int(int)> getter;
    };

    std::vector<EffectCC> effects = {
    { 74, [&](int ch) { return MIDIDevice.getBrightness(ch); } },
    { 11, [&](int ch) { return MIDIDevice.getExpression(ch); } },
    { 93, [&](int ch) { return MIDIDevice.getChorus(ch); } },
    { 71, [&](int ch) { return MIDIDevice.getResonance(ch); } },
    { 64, [&](int ch) { return MIDIDevice.getSustainToggle(ch) ? 127 : 0; } },

    { 73, [&](int ch) { return MIDIDevice.getAttack(ch); } },
    { 75, [&](int ch) { return MIDIDevice.getDecay(ch); } },
    { 72, [&](int ch) { return MIDIDevice.getRelease(ch); } },
    { 1,  [&](int ch) { return MIDIDevice.getVibrato(ch); } },

    { 94, [&](int ch) { return MIDIDevice.getDelay(ch); } },
    { 10, [&](int ch) { return MIDIDevice.getPan(ch); } },
    { 91, [&](int ch) { return MIDIDevice.getReverb(ch); } },
    { 7,  [&](int ch) { return MIDIDevice.getVolume(ch); } },

    { 80, [&](int ch) { return MIDIDevice.getDistortion(ch); } },
    { 76, [&](int ch) { return MIDIDevice.getFilterTrack(ch); } },
    { 92, [&](int ch) { return MIDIDevice.getTremolo(ch); } },
    { 95, [&](int ch) { return MIDIDevice.getRandomMod(ch); } }
    };

    for (int ch : { 1, 16 })
    {
        for (auto& effect : effects)
        {
            const int value = effect.getter(ch);
            if (hasOut)
                MIDIDevice.sendMidiCC(ch, effect.ccNumber, value);
            if (hasAudio)
                midiHandler.injectCC(ch, effect.ccNumber, value);
        }
    }
}

void MainComponent::setCallBacksForOverlayWindow()
{
    overlayWindow->onRequestClose = [this]()
    {
        overlayShouldBeVisible = false;
        setOverlayMenuVisible(false);
    };

    overlayWindow->setWindowFlag = [this]()
    {
        overlayShouldBeVisible = false;
    };

    overlayWindow->bringSeparateWindowFront = [this]()
    {
        // Bring the child windows above the overlay without stealing
        // focus.  toFront(true) + grabKeyboardFocus from inside the
        // overlay's mouseDown causes a focus fight: after the handler
        // returns the OS restores activation to the overlay (the
        // clicked native window), leaving both DocumentWindows grey.
        if (midiWindow)
            midiWindow->toFront(false);

        if (soundEffectWindow)
            soundEffectWindow->toFront(false);

    };

    overlayWindow->onSettingsClick = [this]()
    {
        if (!midiWindow)
        {
            midiWindowShouldBeVisible = true;
            if (soundEffectWindow)
                soundEffectWindow->closeButtonPressed();

            midiWindow = std::make_unique<MIDIWindow>(
                this->MIDIDevice, devicesIN, devicesOUT, devicesAudioOUT, propertiesFile, &sfzManager);


            midiWindow->setAlwaysOnTop(true);

            midiWindow->setVisible(true);

            midiWindow->onWindowClosed = [this]()
            {
                midiWindowShouldBeVisible = false;
                midiWindow.reset();
            };

            midiWindow->isMidiDeviceOpen = [this]()
            {
                return !playButton.isVisible();
            };

            midiWindow->getCurrentStyleId = [this]() -> juce::String
            {
                if (display)
                    return display->getStyleID();
                return {};
            };

            midiWindow->getCurrentStyleName = [this]() -> juce::String
            {
                if (display)
                    return display->getStyleName();
                return {};
            };

            midiWindow->onSfzLibraryChanged = [this]()
            {
                if (audioHandler == nullptr || !MIDIDevice.isOpenAudioOUT())
                    return;
                loadSfzForCurrentStyle();
            };

            midiWindow->onArrangerModeChanged = [this](bool enabled)
            {
                if (display != nullptr)
                    display->setArrangerModeEnabled(enabled);
                setSectionButtonsEngineDriven(enabled);
                midiHandler.setArrangerEngaged(enabled);   // gates chord-zone muting (off => normal playing)
            };

            // Phase 4 chord-mode toggles.
            midiWindow->onChordBassInversionChanged = [this](bool on)
            {
                if (display != nullptr)
                    display->setArrangerBassInversion(on);
            };
            midiWindow->onChordModeChanged = [this](ChordMode m)
            {
                midiHandler.setChordMode(m);
            };
            midiWindow->onChordMemoryChanged = [this](bool on)
            {
                midiHandler.setChordMemory(on);
            };
            midiWindow->onSynchroStartChanged = [this](bool on)
            {
                if (display != nullptr)
                    display->setArrangerSynchroStart(on);
            };
            midiWindow->onCountInChanged = [this](bool on)
            {
                if (display != nullptr)
                    display->setArrangerCountIn(on);
            };
            midiWindow->onChordZoneMuteChanged = [this](bool on)
            {
                midiHandler.setChordZoneMute(on);   // Korg-style: silence the chord keys when the arranger is engaged
            };

            // Phase 7a NTT controls.
            midiWindow->onNttEnabledChanged = [this](bool on)
            {
                if (display != nullptr)
                    display->setArrangerNttEnabled(on);
            };
            midiWindow->onNttMinorScaleChanged = [this](int idx)
            {
                const auto c = idx == 1 ? MinorScaleChoice::Aeolian
                             : idx == 2 ? MinorScaleChoice::HarmonicMinor
                                        : MinorScaleChoice::Dorian;
                if (display != nullptr)
                    display->setArrangerMinorScale(c);
            };

            midiWindow->onOutputEngineChanged = [this](int engineOption)
            {
                disconnectCurrentOutput();

                if (engineOption == 1) // External MIDI
                {
                    if (!MIDIDevice.deviceOpenOUT(MIDIDevice.getDeviceIndexOUT()))
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ERROR", "Failed to open MIDI output device.", "OK");
                        return;
                    }
                    deviceOpenedOUT = MIDIDevice.getDeviceOUT();

                    if (display != nullptr)
                    {
                        // Route style/arranger playback to the MIDI device and stop sending to the
                        // (now closed) SFZ sampler. Mirrors the Play-button wiring path; without this,
                        // switching engine mid-session leaves the players with a stale output -> silence.
                        display->setMidiInjectCallback(nullptr);
                        display->setDeviceOutput(MIDIDevice.getDeviceOUT());
                    }
                }
                else if (engineOption == 2) // Internal SFZ
                {
                    if (!MIDIDevice.deviceOpenAudioOUT(MIDIDevice.getDeviceIndexAudioOUT()))
                    {
                        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ERROR", "Failed to open audio output device.", "OK");
                        return;
                    }
                    ensureAudioHandlerReady();   // re-wires the SFZ inject callback into the players
                    loadSfzForCurrentStyle();

                    if (display != nullptr)
                        display->setDeviceOutput({});   // stop sending to the (now closed) MIDI device
                }

                applyCurrentStyleToOutput();
            };

        }
        else
        {
            if (!midiWindow->isVisible())
            {
                midiWindowShouldBeVisible = true;
                midiWindow->setVisible(true);
                midiWindow->toFront(true);
                juce::MessageManager::callAsync([this]()
                    {
                        if (midiWindow)
                        {
                            midiWindow->grabKeyboardFocus();
                        }

                    });
            }
            else
            {
                midiWindowShouldBeVisible = false;
                midiWindow->setVisible(false);
            }
        }
    };

    overlayWindow->onEffectsClick = [this]()
    {

        if (!soundEffectWindow)
        {
            soundEffecttWindowShouldBeVisible = true;
            if (midiWindow)
                midiWindow->closeButtonPressed();

            soundEffectWindow = std::make_unique<SoundEffectWindow>();

            soundEffectWindow->setAlwaysOnTop(true);
            soundEffectWindow->setVisible(true);

            soundEffectWindow->onWindowClosed = [this]()
            {
                soundEffecttWindowShouldBeVisible = false;
                soundEffectWindow.reset();
            };

            setCallBacksForEffectWindow();

            setInitialValuesFromSettingsFileEffectWindow();


        }
        else {
            if (!soundEffectWindow->isVisible())
            {
                soundEffecttWindowShouldBeVisible = true;
                soundEffectWindow->setVisible(true);
                soundEffectWindow->toFront(true);

                juce::MessageManager::callAsync([this]()
                    {
                        if (soundEffectWindow)
                        {
                            soundEffectWindow->grabKeyboardFocus();
                        }
                    });
            }
            else {
                soundEffectWindow->setVisible(false);
                soundEffecttWindowShouldBeVisible = false;
            }
        }
    };
}

void MainComponent::toggleSettingsPanel()
{
    if (settingsPanel.isVisible()) {
        settingsPanel.setVisible(false);
    }
    else {
        settingsPanel.setVisible(true);
    }
}

void MainComponent::toggleSettingsButton()
{
    if (settingsButton.isVisible())
        settingsButton.setVisible(false);
    else settingsButton.setVisible(true);
}

void MainComponent::toggleMIDIButton()
{
    if (this->midiButton.isVisible())
        midiButton.setVisible(false);
    else midiButton.setVisible(true);
}

void MainComponent::togglePlayButton()
{
    if (this->playButton.isVisible())
        this->playButton.setVisible(false);
    else playButton.setVisible(true);
}

void MainComponent::toggleMIDIsettingsIcon()
{
    if (this->helpIcon.isVisible())
        helpIcon.setVisible(false);
    else helpIcon.setVisible(true);
}

void MainComponent::toggleHPanel()
{
    if (headerPanel.isVisible())
        headerPanel.setVisible(false);
    else headerPanel.setVisible(true);

    toggleHomeButton();
    toggleColourSelectorButton();
    toggleInstrumentSelectorButton();
    toggleRecordButtons();
    togglePlayRecordingButton();
    toggleKnobs();
    toggleDisplay();
    toggleParticleToggle();
    toggleHandInstrumentToggle();
    toggleAnnotation();
    toggleSections();
    toggleChordHelperButton();
}

void MainComponent::toggleHomeButton()
{
    if (homeButton.isVisible())
        homeButton.setVisible(false);
    else homeButton.setVisible(true);
}

void MainComponent::toggleColourSelectorButton()
{
    if (colourSelectorButton.isVisible())
        colourSelectorButton.setVisible(false);
    else colourSelectorButton.setVisible(true);
}

void MainComponent::toggleInstrumentSelectorButton()
{
    if (instrumentSelectorButton.isVisible())
        instrumentSelectorButton.setVisible(false);
    else instrumentSelectorButton.setVisible(true);
}

void MainComponent::toggleRecordButtons()
{
    if (startRecording.isVisible())
        startRecording.setVisible(false);
    else startRecording.setVisible(true);

    if (stopRecording.isVisible())
        stopRecording.setVisible(false);
    else stopRecording.setVisible(true);

    if (startPlayback.isVisible())
        startPlayback.setVisible(false);
    else startPlayback.setVisible(true);
}

void MainComponent::toggleSaveRecordingButton()
{
    if (saveRecordingButton.isVisible())
        saveRecordingButton.setVisible(false);
    else saveRecordingButton.setVisible(true);
}

void MainComponent::togglePlayRecordingButton()
{
    if (playRecordingFileButton.isVisible())
        playRecordingFileButton.setVisible(false);
    else playRecordingFileButton.setVisible(true);
}

void MainComponent::toggleKnobs()
{
    if (volumeKnob.isVisible())
        volumeKnob.setVisible(false);
    else volumeKnob.setVisible(true);
}

void MainComponent::toggleDisplay()
{
    if (display->isVisible())
        display->setVisible(false);
    display->setVisible(true);
}

void MainComponent::toggleParticleToggle()
{
    if (particleToggle.isVisible())
        particleToggle.setVisible(false);
    else particleToggle.setVisible(true);
}

void MainComponent::toggleHandInstrumentToggle()
{
    if (leftHandInstrumentToggle.isVisible())
        leftHandInstrumentToggle.setVisible(false);
    else leftHandInstrumentToggle.setVisible(true);

    if (rightHandInstrumentToggle.isVisible())
        rightHandInstrumentToggle.setVisible(false);
    else rightHandInstrumentToggle.setVisible(true);
}

void MainComponent::toggleUpdateKeysButton()
{
    if (updateNumberOfKeysDevice.isVisible())
        updateNumberOfKeysDevice.setVisible(false);
    else updateNumberOfKeysDevice.setVisible(true);
}

void MainComponent::toggleDevicesCBUpdate()
{
    if (devicesCBUpdate.isVisible())
        devicesCBUpdate.setVisible(false);
    else devicesCBUpdate.setVisible(true);
}

void MainComponent::toggleSections()
{
    if (introsEndings->isVisible())
        introsEndings->setVisible(false);
    else introsEndings->setVisible(true);

    if (variationsFills->isVisible())
        variationsFills->setVisible(false);
    else variationsFills->setVisible(true);
}

void MainComponent::toggleAnnotation()
{
    if (noteNumbersAnnotation.isVisible())
        noteNumbersAnnotation.setVisible(false);
    else noteNumbersAnnotation.setVisible(true);
}

void MainComponent::toggleChordHelperButton()
{
    if (chordHelperButton.isVisible())
        chordHelperButton.setVisible(false);
    else chordHelperButton.setVisible(true);
}

void MainComponent::toggleForPlaying()
{
    if (this->midiButton.isVisible())
        this->midiButton.setVisible(false);
    if (this->helpIcon.isVisible())
        this->helpIcon.setVisible(false);
    if (this->playButton.isVisible())
        this->playButton.setVisible(false);
    if (this->settingsPanel.isVisible())
        this->settingsPanel.setVisible(false);
    if (this->settingsButton.isVisible())
        this->settingsButton.setVisible(false);
    if (this->updateNumberOfKeysDevice.isVisible())
        this->updateNumberOfKeysDevice.setVisible(false);
    if (this->devicesCBUpdate.isVisible())
        this->devicesCBUpdate.setVisible(false);
}

void MainComponent::settingsInit()
{
    addAndMakeVisible(settingsButton);
    
    settingsButton.setVisible(false); //making settings button invisible for now

    settingsButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    settingsButton.onClick = [this] {  
        settingsButtonOnClick();
    };
    panelInit();
    midiSettingsInit();
    midiIconInit();
    updateKeysButtonInit();
    devicesCBUpdateInit();
}


void MainComponent::playButtonInit()
{
    playButton.setButtonText("Play");
    playButton.setLookAndFeel(&this->customLookAndFeel);
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    playButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    playButton.onClick = [this] {
        // playButtonOnClick() opens devices + loads SFZ samples synchronously and blocks the
        // message thread, so the "Preparing session..." overlay must be on screen *before* it runs.
        // setVisible() only queues a repaint, and the old fixed delay raced it (overlay showed only
        // sometimes). Force the paint now, then yield to the message loop via callAsync so the frame
        // is actually presented to the screen before we block the thread with the heavy init.
        // playButtonOnClick() opens devices + loads SFZ samples synchronously and blocks the
        // message thread. With an OpenGL context in the window, forcing a synchronous paint of the
        // overlay before blocking is unreliable, so instead we just show the overlay and let it
        // paint normally; paintOverChildren() kicks off the blocking init once it has actually
        // drawn (pendingPlayInit). That guarantees the overlay is on screen before we block.
        pendingPlayInit = true;
        openingAudioLabel.setVisible(true);
        repaint();
    };
    addAndMakeVisible(this->playButton);
    playButton.setVisible(false);
}

void MainComponent::colourSelectorButtonInit()
{
    colourSelectorButton.setButtonText("Particle colours");
    colourSelectorButton.setLookAndFeel(&playScreenLAF);
    colourSelectorButton.setColour(juce::TextButton::buttonColourId, AppColours::accent2);
    colourSelectorButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    colourSelectorButton.onClick = [this] {
        showColourSelector();
    };
    headerPanel.addAndMakeVisible(colourSelectorButton);
    colourSelectorButton.setVisible(false);
}

void MainComponent::instrumentSelectorButtonInit()
{
    instrumentSelectorButton.setButtonText("Instruments");
    instrumentSelectorButton.setLookAndFeel(&playScreenLAF);
    instrumentSelectorButton.setColour(juce::TextButton::buttonColourId, AppColours::accent2);
    instrumentSelectorButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    instrumentSelectorButton.onClick = [this] {
        showInstrumentSelector();
    };
    headerPanel.addAndMakeVisible(instrumentSelectorButton);
    instrumentSelectorButton.setVisible(false);
}

void MainComponent::recordButtonsInit()
{
    auto xmlRecord = juce::XmlDocument::parse(BinaryData::recordPlayback_svg);
    recordDrawable = juce::Drawable::createFromSVG(*xmlRecord);

    auto xmlStop = juce::XmlDocument::parse(BinaryData::stopButton_svg);
    stopDrawable = juce::Drawable::createFromSVG(*xmlStop);


    auto xmlPlay = juce::XmlDocument::parse(BinaryData::playButton_svg);
    playDrawable = juce::Drawable::createFromSVG(*xmlPlay);

    startRecording.setImages(recordDrawable.get());
    stopRecording.setImages(stopDrawable.get());
    startPlayback.setImages(playDrawable.get());

    startRecording.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    stopRecording.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    startPlayback.setMouseCursor(juce::MouseCursor::PointingHandCursor);


    startRecording.onClick = [this] {
        recordPlayer.startRecording();
        recordPlayer.handleIncomingMessage(juce::MidiMessage::programChange(1, midiHandler.getProgramNumberLeftHand()));
        recordPlayer.handleIncomingMessage(juce::MidiMessage::programChange(16, midiHandler.getProgramNumberRightHand()));

        saveRecordingButton.setVisible(false);

        if (temporaryPopup)
        {
            temporaryPopup->updateText("Recording started!");
            temporaryPopup->restartTimer();
        }
        else {
            temporaryPopup = std::make_unique<TemporaryMessage>("Recording started!");
            headerPanel.addChildComponent(temporaryPopup.get());
            temporaryPopup->setBounds(getWidth() / 2 - 50, 10, 100, 30);
            temporaryPopup->setFinishedCallBack([this] {
                //saveRecording.setVisible(true);
                temporaryPopup.reset();
                });
            temporaryPopup->setVisible(true);
        }
    };

    stopRecording.onClick = [this] {
        int result = recordPlayer.stopRecording();

        if (result == 1)
            saveRecordingButton.setVisible(true);

        if (temporaryPopup)
        {
            temporaryPopup->updateText("Recording stopped!");
            temporaryPopup->restartTimer();
        }
        else {
            temporaryPopup = std::make_unique<TemporaryMessage>("Recording stopped!");
            headerPanel.addChildComponent(temporaryPopup.get());
            temporaryPopup->setBounds(getWidth() / 2 - 50, 10, 100, 30);
            temporaryPopup->setFinishedCallBack([this] {
                temporaryPopup.reset();

                });
            temporaryPopup->setVisible(true);
        }
    };

    recordPlayer.applyPresetFunction= [&]()
    {
        midiHandler.setProgramNumber(midiHandler.getProgramNumberLeftHand(), "left");
        midiHandler.setProgramNumber(midiHandler.getProgramNumberRightHand(), "right");
    };

    recordPlayer.onSfzMessage = [this](const juce::MidiMessage& msg)
    {
        if (audioHandler == nullptr || !MIDIDevice.isOpenAudioOUT())
            return;

        // Stored events use channels 14/15 (remapped from 1/16 at record time).
        // Remap back so the SFZ synth — which has instruments on 1 and 16 — renders them.
        int ch = msg.getChannel();
        int sfzCh = (ch == 14) ? 1 : (ch == 15 ? 16 : ch);

        if (msg.isNoteOn())
            midiHandler.injectMidiMessage(juce::MidiMessage::noteOn(sfzCh, msg.getNoteNumber(), msg.getVelocity()));
        else if (msg.isNoteOff())
            midiHandler.injectMidiMessage(juce::MidiMessage::noteOff(sfzCh, msg.getNoteNumber()));
    };

    recordPlayer.notifyFunction = [&]()
    {
        if (temporaryPopup)
        {
            temporaryPopup->updateText("Playback stopped!");
            temporaryPopup->restartTimer();
        }
        else
        {
            temporaryPopup = std::make_unique<TemporaryMessage>("Playback Stopped!");
            headerPanel.addChildComponent(temporaryPopup.get());
            temporaryPopup->setBounds(getWidth() / 2 - 50, 10, 100, 30);
            temporaryPopup->setFinishedCallBack([this] {
                temporaryPopup.reset();
                });
            temporaryPopup->setVisible(true);
        }
    };

    startPlayback.onClick = [this] {
        if (recordPlayer.getIsRecording())
            recordPlayer.stopRecording();

        int result=recordPlayer.startPlayBack();
        saveRecordingButton.setVisible(true);

        if(result==0)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Play recording", "No recorded events to play.");

        if (temporaryPopup && result==1)
        {
            temporaryPopup->updateText("Playback started!");
            temporaryPopup->restartTimer();
        }
        else if(temporaryPopup==nullptr) {
            temporaryPopup = std::make_unique<TemporaryMessage>("Playback started!");
            headerPanel.addChildComponent(temporaryPopup.get());
            temporaryPopup->setBounds(getWidth() / 2-50, 10, 100, 30);
            temporaryPopup->setFinishedCallBack([this] {
                temporaryPopup.reset();
                });
            temporaryPopup->setVisible(true);
        }
    };

    startRecording.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopRecording.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    startPlayback.setColour(juce::TextButton::buttonColourId, juce::Colours::green);

    headerPanel.addAndMakeVisible(startRecording);
    headerPanel.addAndMakeVisible(stopRecording);
    headerPanel.addAndMakeVisible(startPlayback);

    startRecording.setVisible(false);
    stopRecording.setVisible(false);
    startPlayback.setVisible(false);

}

void MainComponent::saveRecordingButtonInit()
{
    saveRecordingButton.setButtonText("Save recording");
    saveRecordingButton.setLookAndFeel(&playScreenLAF);
    saveRecordingButton.setColour(juce::TextButton::buttonColourId, AppColours::accent2);

    saveRecordingButton.onClick = [this]{
        saveRecordingToFile();
        };

    saveRecordingButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    headerPanel.addAndMakeVisible(saveRecordingButton);
    saveRecordingButton.setVisible(false);
}

void MainComponent::playRecordingButtonInit()
{
    playRecordingFileButton.setButtonText("Play file recording");
    playRecordingFileButton.setLookAndFeel(&playScreenLAF);
    playRecordingFileButton.setColour(juce::TextButton::buttonColourId, AppColours::accent2);

    playRecordingFileButton.onClick = [this] {
        playRecordingFromFile();
    };

    playRecordingFileButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    headerPanel.addAndMakeVisible(playRecordingFileButton);
    playRecordingFileButton.setVisible(false);
}

void MainComponent::knobsInit()
{
    volumeKnob.setSliderStyle(juce::Slider::Rotary);
    volumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

    volumeKnob.setLookAndFeel(&customKnobLookAndFeel);
    headerPanel.addAndMakeVisible(volumeKnob);
    volumeKnob.setVisible(false);
}

void MainComponent::displayInit()
{
    display = std::make_unique<Display>(deviceOpenedOUT, propertiesFile, 400);
    headerPanel.addAndMakeVisible(display.get());
    display->setVisible(false);

    display->onArrangerOverlayVisible = [this](bool visible)
    {
        setArrangerOverlaySceneHidden(visible);

        // When the config browser/editor closes (e.g. after Load), the Load/Close button still holds
        // keyboard focus, so the PC-keyboard note layer goes dead until you click. Return focus to the
        // playing surface so you can play immediately. Deferred so it lands after the overlay is gone.
        if (! visible)
            juce::MessageManager::callAsync([this]() { if (isVisible()) grabKeyboardFocus(); });
    };

    // Arranger Start rebuilds the play scene and can leave focus on a freshly created child, so the
    // PC keyboard sometimes stayed dead until you clicked. Return focus to the play surface (deferred
    // so it lands after the scene is built; skipped if a modal dialog is up).
    display->onRequestPlayFocus = [this]()
    {
        juce::MessageManager::callAsync([this]()
        {
            if (isVisible() && juce::Component::getCurrentlyModalComponent() == nullptr)
                grabKeyboardFocus();
        });
    };

    // So the arranger can begin on a chord the player is already holding when Start is pressed
    // (a steady hold sends no note event, so it otherwise wouldn't reach the engine until release).
    // heldChord() returns invalid when nothing is physically held, so a chord merely remembered by
    // Chord Memory does not hijack Start — that case starts in the home key.
    display->getHeldChord = [this]() -> ArrangerChord
    {
        return midiHandler.heldChord();
    };

    display->onArrangerBusy = [this](bool show, const juce::String& text)
    {
        setLoadingOverlayVisible(show, text);
    };

    display->onArrangerSectionChanged = [this](int idx, ArrangerSectionType type, juce::String name)
    {
        highlightArrangerSection(idx, type, name);
    };

    display->loadSettingsOnStyleChange = [this](const juce::String& styleID)
    {
        loadSettings(styleID);

        // Defer bounds update — Display calls readPlaybackSettingsFromProperties()
        // AFTER this callback returns, so reading bounds here gives stale values.
        juce::MessageManager::callAsync([this]() {
            if (display)
            {
                midiHandler.set_start_end_notes(display->getStartNote(), display->getEndNote());
                midiHandler.set_left_right_bounds(display->getLeftBound(), display->getRightBound());
            }
        });

        if (MIDIDevice.isOpenOUT() || MIDIDevice.isOpenAudioOUT())
            sendEffectsBeforePlaying();

        if (MIDIDevice.isOpenAudioOUT())
        {
            setLoadingOverlayVisible(true, "Preparing style...");
            loadSfzForCurrentStyle(styleID);
            // Fallback hide if no SFZ file actually changed (onSfzLoadComplete won't fire)
            juce::Timer::callAfterDelay(300, [this]()
            {
                setLoadingOverlayVisible(false);
            });
        }
        else if (MIDIDevice.isOpenOUT())
        {
            // External MIDI has no SFZ to stream and fires no load-complete event,
            // but mirror the internal "Preparing style..." overlay so switching styles
            // feels identical. Effects/CCs were just pushed to the device above; keep
            // the overlay up briefly (matching the SFZ fallback) so the swap reads as
            // deliberate, then hide on a timed delay.
            setLoadingOverlayVisible(true, "Preparing style...");
            juce::Timer::callAfterDelay(300, [this]()
            {
                setLoadingOverlayVisible(false);
            });
        }
    };
}

void MainComponent::toggleButtonInit()
{
    particleToggle.setButtonText("Toggle particles");
    particleToggle.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    particleToggle.onClick = [this]()
    {
        noteLayer->setSpawnParticleState(particleToggle.getToggleState());
    };

    particleToggle.setColour(juce::ToggleButton::textColourId,        AppColours::titleText);
    particleToggle.setColour(juce::ToggleButton::tickColourId,         AppColours::accent3);
    particleToggle.setColour(juce::ToggleButton::tickDisabledColourId, AppColours::rowLabel);

    particleToggle.setToggleState(false,juce::dontSendNotification);

    headerPanel.addAndMakeVisible(particleToggle);
    particleToggle.setVisible(false);
}

void MainComponent::keyBoardUIinit(int min, int max)
{
    keyboardInitialized = true;
    addAndMakeVisible(keyboard);
    int keyboardHeight = (int)getHeight() * 0.21;

    keyboard.setBounds(0, getHeight() - keyboardHeight, getWidth(), keyboardHeight);

    this->keyboard.set_min_and_max(min, max);
    noteLayer = std::make_unique<NoteLayer>(this->keyboard);
    midiHandler.addListener(noteLayer.get());
    noteLayer->setBounds(0, headerPanel.getHeight(), getWidth(), getHeight() - keyboardHeight - headerPanel.getHeight());
    addAndMakeVisible(noteLayer.get());

    this->keyboard.repaint();
}

void MainComponent::panelInit()
{
    addAndMakeVisible(settingsPanel);
    settingsPanel.toFront(true);
    settingsPanel.setVisible(false);
}

void MainComponent::midiIconInit()
{
    helpIcon.setFont(juce::Font(30.0f));  // Set the font size
    helpIcon.setColour(juce::Label::textColourId, juce::Colours::lightgrey.withAlpha(0.9f));
    helpIcon.setTooltip("This setting helps you adjust/select:\n *The volume\n *The reverb\n *The input device\n *The output device ");
    helpIcon.setJustificationType(juce::Justification::left);
    settingsPanel.addAndMakeVisible(helpIcon);
    helpIcon.setText("?", juce::dontSendNotification);
    helpIcon.setVisible(false);
}

void MainComponent::midiSettingsInit()
{
    midiButton.setButtonText("MIDI Settings");
    midiButton.onClick = [this] {
        midiButtonOnClick();
    };
    settingsPanel.addAndMakeVisible(midiButton);
    midiButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    midiButton.setVisible(false);
}

void MainComponent::headerPanelInit()
{
    addAndMakeVisible(headerPanel);
    headerPanel.toFront(true);
    headerPanel.setVisible(false);

    homeButtonInit();
    colourSelectorButtonInit();
    instrumentSelectorButtonInit();
    recordButtonsInit();
    saveRecordingButtonInit();
    playRecordingButtonInit();
    toggleButtonInit();
    toggleHandButtonsInit();
    knobsInit();
    annotationInit();
    sectionsInit();
    chordHelperButtonInit();
}

void MainComponent::homeButtonInit()
{
    homeButton.setButtonText("Home");
    homeButton.setLookAndFeel(&playScreenLAF);
    homeButton.setColour(juce::TextButton::buttonColourId, AppColours::accent2);
    homeButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    homeButton.onClick = [this] {
        homeButtonOnClick();
    };

    headerPanel.addAndMakeVisible(homeButton);
    homeButton.setVisible(false);

}

void MainComponent::toggleHandButtonsInit()
{
    leftHandInstrumentToggle.setButtonText("Left hand");
    leftHandInstrumentToggle.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    leftHandInstrumentToggle.setColour(juce::ToggleButton::textColourId,        AppColours::titleText);
    leftHandInstrumentToggle.setColour(juce::ToggleButton::tickColourId,         AppColours::accent3);
    leftHandInstrumentToggle.setColour(juce::ToggleButton::tickDisabledColourId, AppColours::rowLabel);

    leftHandInstrumentToggle.setToggleState(false,juce::dontSendNotification);
    headerPanel.addAndMakeVisible(leftHandInstrumentToggle);
    leftHandInstrumentToggle.setVisible(false);


    rightHandInstrumentToggle.setButtonText("Right hand");
    rightHandInstrumentToggle.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    rightHandInstrumentToggle.setColour(juce::ToggleButton::textColourId,        AppColours::titleText);
    rightHandInstrumentToggle.setColour(juce::ToggleButton::tickColourId,         AppColours::accent3);
    rightHandInstrumentToggle.setColour(juce::ToggleButton::tickDisabledColourId, AppColours::rowLabel);

    rightHandInstrumentToggle.setToggleState(false, juce::dontSendNotification);
    headerPanel.addAndMakeVisible(rightHandInstrumentToggle);
    rightHandInstrumentToggle.setVisible(false);

}

void MainComponent::updateKeysButtonInit()
{
    updateNumberOfKeysDevice.setButtonText("Update number of keys");
    updateNumberOfKeysDevice.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    updateNumberOfKeysDevice.onClick = [this]()
    {
        updateKeysButtonOnClick();
    };

    settingsPanel.addAndMakeVisible(updateNumberOfKeysDevice);
    updateNumberOfKeysDevice.setVisible(false);
}

void MainComponent::devicesCBUpdateInit()
{
    devicesCBUpdate.setText("Select a device", juce::dontSendNotification);
    devicesCBUpdate.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    settingsPanel.addAndMakeVisible(devicesCBUpdate);
    devicesCBUpdate.setVisible(false);
}

void MainComponent::sectionsInit()
{
    std::unordered_map<juce::String, std::function<void()>> callbacksIntroEnding = {
        {"Intro 1", [this]() { handleIntro("Intro 1");  }},
        {"Intro 2", [this]() { handleIntro("Intro 2");  }},
        {"Intro 3", [this]() { handleIntro("Intro 3");  }},

        {"Ending 1", [this]() { handleEnding("Ending 1"); }},
        {"Ending 2", [this]() { handleEnding("Ending 2");  }},
        {"Ending 3", [this]() { handleEnding("Ending 3");  }}

    };


    introsEndings = std::make_unique<StyleSectionComponent>(std::vector<juce::String>{"Intros", "Endings"},
        std::vector<std::vector<juce::String>>{ {"Intro 1", "Intro 2", "Intro 3"}, { "Ending 1", "Ending 2", "Ending 3"}},callbacksIntroEnding);



    std::unordered_map<juce::String, std::function<void()>> callbacksVarFillBreak = {
        {"Var 1", [this]() { handleVar("Var 1");  }},
        {"Var 2", [this]() { handleVar("Var 2");  }},
        {"Var 3", [this]() { handleVar("Var 3");  }},
        {"Var 4", [this]() { handleVar("Var 4");  }},

        {"Fill 1", [this]() { handleFill("Fill 1");  }},
        {"Fill 2", [this]() { handleFill("Fill 2");  }},
        {"Fill 3", [this]() { handleFill("Fill 3");  }},
        {"Fill 4", [this]() { handleFill("Fill 4");  }},

        {"Break", [this]() { handleBreak("Break");  }}

    };

    variationsFills = std::make_unique<StyleSectionComponent>(std::vector<juce::String>{"Variations", "Fills"},
        std::vector<std::vector<juce::String>>{ {"Var 1", "Var 2", "Var 3", "Var 4"}, {"Fill 1", "Fill 2", "Fill 3", "Fill 4"}}, callbacksVarFillBreak);

    // Wire the Auto Fill toggle (lives on the "Variations" group) to the arranger engine.
    for (auto* group : variationsFills->getSectionGroups())
        if (group->getToggleButton() != nullptr)
            group->onToggleChanged = [this](bool on)
            {
                if (display)
                    display->setArrangerAutoFillEnabled(on);
                if (propertiesFile != nullptr)   // remember the choice across restarts (app-wide)
                {
                    propertiesFile->setValue("ArrangerAutoFillEnabled", on);
                    propertiesFile->saveIfNeeded();
                }
            };

    
    introsEndings->undoLastClickedForOtherGroupSections = [this]()
    {
        variationsFills->deactivateLastClicked();
    };

    variationsFills->undoLastClickedForOtherGroupSections = [this]()
    {
        introsEndings->deactivateLastClicked();
    };

    headerPanel.addAndMakeVisible(introsEndings.get());
    introsEndings->setVisible(false);

    headerPanel.addAndMakeVisible(variationsFills.get());
    variationsFills->setVisible(false);

}

void MainComponent::annotationInit()
{
    noteNumbersAnnotation.setButtonText("Note annotation");
    noteNumbersAnnotation.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    noteNumbersAnnotation.setColour(juce::ToggleButton::textColourId,        AppColours::titleText);
    noteNumbersAnnotation.setColour(juce::ToggleButton::tickColourId,         AppColours::accent3);
    noteNumbersAnnotation.setColour(juce::ToggleButton::tickDisabledColourId, AppColours::rowLabel);

    noteNumbersAnnotation.setToggleState(false, juce::dontSendNotification);
    headerPanel.addAndMakeVisible(noteNumbersAnnotation);

    noteNumbersAnnotation.onClick = [this]()
    {
        if (noteNumbersAnnotation.getToggleState() == true)
            keyboard.setAnnotationState(true);
        else keyboard.setAnnotationState(false);

        keyboard.repaint();
    };

    noteNumbersAnnotation.setVisible(false);
}

void MainComponent::chordHelperButtonInit()
{
    chordHelperButton.setButtonText("Chord helper");
    chordHelperButton.setLookAndFeel(&playScreenLAF);
    chordHelperButton.setColour(juce::TextButton::buttonColourId, AppColours::accent2);
    chordHelperButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    chordHelperButton.onClick = [this]()
    {
        std::vector<Chord> lib=myChordLibrary;

        auto chordBrowser = std::make_unique<ChordBrowserComponent>(lib);

        chordBrowser->onChordChosen = [this](const Chord& c)
        {
            playChordOnClick(c);
        };

        juce::CallOutBox::launchAsynchronously(std::move(chordBrowser), chordHelperButton.getScreenBounds(), nullptr);
    };

    headerPanel.addAndMakeVisible(chordHelperButton);
    chordHelperButton.setVisible(false);
}

void MainComponent::settingsButtonOnClick()
{
    toggleSettingsPanel(); toggleMIDIButton(); toggleMIDIsettingsIcon(); toggleUpdateKeysButton(); toggleDevicesCBUpdate();
}

void MainComponent::midiButtonOnClick()
{
    if (!midiWindow)
    {
        midiWindow = std::make_unique<MIDIWindow>(this->MIDIDevice, devicesIN, devicesOUT, devicesAudioOUT, propertiesFile, &sfzManager);
        midiWindow->getCurrentStyleId = [this]() -> juce::String
        {
            if (display)
                return display->getStyleID();
            return {};
        };
        midiWindow->getCurrentStyleName = [this]() -> juce::String
        {
            if (display)
                return display->getStyleName();
            return {};
        };
        loadSettings();
    }
    else {
        midiWindow->setVisible(true);
        midiWindow->toFront(true);
    }
}

void MainComponent::homeButtonOnClick()
{
    currentBackground = cachedImageMainWindow;
    repaint();

    toggleHPanel();

    togglePlayButton();
    //toggleSettingsButton(); don't need this button for now
    keyboard.setVisible(false);
    this->noteLayer->setVisible(false);
    this->noteLayer->resetState();
    if(this->recordPlayer.getIsPlaying())
        this->recordPlayer.stopPlayBack();

    if (display && display->getNumTabs()==3)
        display->homeButtonInteraction();

    grabKeyboardFocus();
}

void MainComponent::playButtonOnClick()
{
    MIDIDevice.getAvailableDevicesMidiIN(devicesIN);
    MIDIDevice.getAvailableDevicesMidiOUT(devicesOUT);

    if (openingDevicesForPlay()) {
        if (!this->keyListener.getIsKeyboardInput())
        {
            midiHandler.onAddCallBack = [this](const juce::String& vid, const juce::String& pid, std::function<void(const juce::String&, int)> resultCallback)
            {
                addDeviceWindow = std::make_unique<AddDeviceWindow>(vid, pid);
                addDeviceWindow->onAddDevice = [this,vid,pid,resultCallback](const juce::String& name, int keys)
                {
                    resultCallback(name, keys);
                    dataBase.addDeviceJson(vid, pid, name, keys);
                };
                addDeviceWindow->setVisible(true);
            };
        }

        juce::String PID = MIDIDevice.extractPID(MIDIDevice.get_identifier());
        juce::String VID = MIDIDevice.extractVID(MIDIDevice.get_identifier());
        juce::String name = dataBase.getDeviceName(VID, PID);
        int nrKeys = dataBase.getNrKeysPidVid(VID, PID);

        if (midiHandler.handlePlayableRange(VID, PID, dataBase.getNrKeysPidVid(VID, PID), this->keyListener.getIsKeyboardInput()) < 0)
        {
            this->MIDIDevice.deviceCloseIN();
            this->MIDIDevice.deviceCloseOUT();
            return;
        }



        this->recordPlayer.setOutputDevice(MIDIDevice.getDeviceOUT());
        if(midiWindow)
            this->midiWindow->setVisible(false);


        
        

        MIDIDevice.setPID(PID);
        MIDIDevice.setVID(VID);
        MIDIDevice.setDeviceName(name);

        if (VID.length() < 4 || PID.length() < 4)
            this->display->set_VID_PID("", "");
        else this->display->set_VID_PID(VID, PID);

        this->display->readPlaybackSettingsFromProperties();
        loadEffectsFromFile(this->display->getStyleID());

        currentBackground = playBackground;
        repaint();
        toggleHPanel();

        sendEffectsBeforePlaying();

        /*
        MIDIDevice.sendMidiCC(1, 7, MIDIDevice.getVolume(1));
        MIDIDevice.sendMidiCC(1,91,MIDIDevice.getReverb(1));
        MIDIDevice.sendMidiCC(16,91,MIDIDevice.getReverb(16));
        MIDIDevice.sendMidiCC(16,7,MIDIDevice.getVolume(16));
        */

        recordPlayer.setReverb(MIDIDevice.getReverb(1),1);
        recordPlayer.setReverb(MIDIDevice.getReverb(16), 16);

        recordPlayer.setVolume(MIDIDevice.getVolume(1), 1);
        recordPlayer.setVolume(MIDIDevice.getVolume(16), 16);

        display->setDeviceOutput(MIDIDevice.getDeviceOUT());

        if (audioHandler != nullptr && display != nullptr)
        {
            display->setMidiInjectCallback([this](const juce::MidiMessage& msg) {
                midiHandler.injectMidiMessage(msg);
            });
        }

        if (display != nullptr && propertiesFile != nullptr)
        {
            const bool arrangerOn = propertiesFile->getBoolValue("ArrangerModeEnabled", false);
            display->setArrangerModeEnabled(arrangerOn);
            setSectionButtonsEngineDriven(arrangerOn);
            midiHandler.setArrangerEngaged(arrangerOn);   // restore the chord-zone-mute gate

            // Restore the remembered Auto Fill choice (app-wide, not per style): apply to the engine
            // and reflect the tick in the toggle button.
            const bool autoFillOn = propertiesFile->getBoolValue("ArrangerAutoFillEnabled", false);
            display->setArrangerAutoFillEnabled(autoFillOn);
            if (variationsFills != nullptr)
                for (auto* group : variationsFills->getSectionGroups())
                    if (auto* toggle = group->getToggleButton())
                        toggle->setToggleState(autoFillOn, juce::dontSendNotification);

            // Phase 4/5: apply the remembered chord choices (defaults: Fingered mode, memory on,
            // bass inversion off) so they take effect without needing to toggle them each launch.
            display->setArrangerBassInversion(propertiesFile->getBoolValue("ChordBassInversion", false));
            {
                ChordMode m = ChordMode::Fingered;
                if (propertiesFile->containsKey("ChordMode"))
                    m = (ChordMode) juce::jlimit(0, 2, propertiesFile->getIntValue("ChordMode", 0));
                else if (propertiesFile->getBoolValue("ChordFullKeyboard", false))
                    m = ChordMode::FullKeyboard;   // migrate legacy bool
                midiHandler.setChordMode(m);
            }
            midiHandler.setChordMemory(propertiesFile->getBoolValue("ChordMemory", true));
            midiHandler.setChordZoneMute(propertiesFile->getBoolValue("ChordZoneMute", false));
            display->setArrangerSynchroStart(propertiesFile->getBoolValue("SynchroStart", false));
            display->setArrangerCountIn(propertiesFile->getBoolValue("CountIn", false));

            // Phase 7a: restore the NTT master enable (default on) + curated minor scale (default Dorian).
            display->setArrangerNttEnabled(propertiesFile->getBoolValue("NttEnabled", true));
            {
                const int m = juce::jlimit(0, 2, propertiesFile->getIntValue("NttMinorScale", 0));
                const auto c = m == 1 ? MinorScaleChoice::Aeolian
                             : m == 2 ? MinorScaleChoice::HarmonicMinor
                                      : MinorScaleChoice::Dorian;
                display->setArrangerMinorScale(c);
            }
        }

        if (!keyboardInitialized)
        {
            keyBoardUIinit(MIDIDevice.get_minNote(), MIDIDevice.get_maxNote());

            auto styleId = display ? display->getStyleID() : "style_default";
            int leftProg  = propertiesFile ? propertiesFile->getIntValue(styleId + "_leftInstrumentNumber",  0)
                                           : midiHandler.getProgramNumberLeftHand();
            int rightProg = propertiesFile ? propertiesFile->getIntValue(styleId + "_rightInstrumentNumber", 0)
                                           : midiHandler.getProgramNumberRightHand();

            midiHandler.setProgramNumber(leftProg, "left");
            midiHandler.setProgramNumber(rightProg, "right");

            recordPlayer.setProgarmNumber(leftProg,"left");
            recordPlayer.setProgarmNumber(rightProg, "right");
        }
        else {
            keyboard.setVisible(true);
            this->noteLayer->setVisible(true);
        }

        if (MIDIDevice.isOpenAudioOUT())
            loadSfzForCurrentStyle();

        if (this->keyListener.getIsKeyboardInput())
        {
            this->keyboard.set_min_and_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());
            this->display->set_min_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());
        }
        else
        {
            this->keyboard.set_min_and_max(MIDIDevice.get_minNote(), MIDIDevice.get_maxNote());
            this->display->set_min_max(MIDIDevice.get_minNote(), MIDIDevice.get_maxNote());
        }
        
        this->midiHandler.set_start_end_notes(this->display->getStartNote(), this->display->getEndNote());
        this->midiHandler.set_left_right_bounds(this->display->getLeftBound(), this->display->getRightBound());

        loadSettings();



        keyboard.setIsDrawn(false);
        keyboard.repaint();
        toggleForPlaying();

        if (!this->keyListener.getIsKeyboardInput())
        {
            
            juce::String vid = MIDIDevice.getVID();
            juce::String pid = MIDIDevice.getPID();
            juce::String name = MIDIDevice.getName();
            int nrKeys = MIDIDevice.getNrKeysAfterInitialized();
            std::thread([this, vid, pid, name, nrKeys]()
                {
                    client->addOrUpdateDevice(vid, pid, name, nrKeys);
                }).detach();
        }
    }
}

void MainComponent::updateKeysButtonOnClick()
{
    juce::String selectedText = devicesCBUpdate.getText();
    if (selectedText == "Select a device" || selectedText == "No devices found!")
        return;

    juce::String identifier = updateDevicesMap[selectedText];
    juce::String VID = MIDIDevice.extractVID(identifier);
    juce::String PID = MIDIDevice.extractPID(identifier);


    auto* window = new juce::AlertWindow(
        "Enter Number of Keys",
        "Please enter how many keys this device has:",
        juce::AlertWindow::QuestionIcon
    );

    window->addTextEditor("keys", "", "Number of keys");
    window->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    window->enterModalState(
        true, // make modal
        juce::ModalCallbackFunction::create(
            [this, VID, PID, selectedText, window](int result)
            {
                if (result == 0)
                {
                    delete window;
                    return;
                }

                juce::String keysStr = window->getTextEditorContents("keys").trim();
                delete window;

                if (!keysStr.containsOnly("0123456789"))
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Invalid Input",
                        "Please enter a valid number of keys.");
                    return;
                }

                int numKeys = keysStr.getIntValue();
                if (numKeys != 49 && numKeys != 61 && numKeys != 76 && numKeys != 88)
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Invalid Key Count",
                        "Please enter one of: 49, 61, 76 or 88.");
                    return;
                }

                dataBase.updateDeviceJson(VID, PID, "", numKeys);

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    "Success",
                    "Device updated successfully!");
            }
        )
    );

}

juce::Image MainComponent::getImageForInstruments(const std::string& type)
{
    const void* binaryData=nullptr;
    size_t binaryDataSize = 0;
    if (type == "AGP")
    {
        binaryData = BinaryData::AcousticGrandPiano_png;
        binaryDataSize = BinaryData::AcousticGrandPiano_pngSize;
    }
    else if (type == "EP")
    {
        binaryData = BinaryData::ElectricPiano_png;
        binaryDataSize = BinaryData::ElectricPiano_pngSize;
    }
    else if (type == "ACB")
    {
        binaryData = BinaryData::AcousticBass_png;
        binaryDataSize = BinaryData::AcousticBass_pngSize;
    }
    else if (type == "EBF")
    {
        binaryData = BinaryData::ElectricBass_png;
        binaryDataSize = BinaryData::ElectricBass_pngSize;
    }
    else if (type == "EBP")
    {
        binaryData = BinaryData::ElectricBass2_png;
        binaryDataSize = BinaryData::ElectricBass2_pngSize;
    }
    else if (type == "FTB")
    {
        binaryData = BinaryData::FretlessBass_png;
        binaryDataSize = BinaryData::FretlessBass_pngSize;
    }
    else if (type == "SLB1")
    {
        binaryData = BinaryData::SlapBass_png;
        binaryDataSize = BinaryData::SlapBass_pngSize;
    }
    else if (type == "SLB2")
    {
        binaryData = BinaryData::SlapBass2_png;
        binaryDataSize = BinaryData::SlapBass2_pngSize;
    }
    else if (type == "SYB1")
    {
        binaryData = BinaryData::SynthBass_png;
        binaryDataSize = BinaryData::SynthBass_pngSize;
    }
    else if (type == "SYB2")
    {
        binaryData = BinaryData::SynthBass2_png;
        binaryDataSize = BinaryData::SynthBass2_pngSize;
    }
    if (binaryData)
    {
        juce::MemoryInputStream imageStream{ binaryData,binaryDataSize,false };
        juce::Image img = juce::ImageFileFormat::loadFrom(imageStream);
        return img;
    }
    return juce::Image();
}

void MainComponent::buildTree()
{
    auto dummyRoot = std::make_unique< InstrumentTreeItem>("Instruments");

    auto rootPianos = std::make_unique<InstrumentTreeItem>( "Pianos" );
    rootPianos->addSubItem(createInstrumentItem(getImageForInstruments("AGP"), "Acoustic Grand Piano", 0));
    rootPianos->addSubItem(createInstrumentItem(getImageForInstruments("EP"),"Electric Piano", 4));
    rootPianos->setOpen(false);
    dummyRoot->addSubItem(rootPianos.release());

    auto rootBasses = std::make_unique<InstrumentTreeItem>( "Basses" );
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("ACB"),"Acoustic Bass", 32));
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("EBF"),"Electric Bass (finger)", 33));
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("EBP"),"Electric Bass (pick)", 34));
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("FTB"),"Fretless Bass", 35));
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("SLB1"),"Slap Bass 1", 36));
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("SLB2"),"Slap Bass 2", 37));
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("SYB1"),"Synth Bass 1", 38));
    rootBasses->addSubItem(createInstrumentItem(getImageForInstruments("SYB2"),"Synth Bass 2", 39));
    rootBasses->setOpen(false);
    dummyRoot->addSubItem(rootBasses.release());

    auto rootGuitars = std::make_unique<InstrumentTreeItem>("Guitars");
    rootGuitars->addSubItem(createInstrumentItem(getImageForInstruments("ACB"),"Nylon Acoustic Guitar",24));
    rootGuitars->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Steel Acoustic Guitar", 25));
    rootGuitars->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Jazz Electric Guitar", 26));
    rootGuitars->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Clean Electric Guitar", 27));
    rootGuitars->setOpen(false);
    dummyRoot->addSubItem(rootGuitars.release());

    auto woodWinds = std::make_unique<InstrumentTreeItem>("WoodWinds");
    woodWinds->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Flute", 73));
    woodWinds->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Clarinet", 71));
    woodWinds->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Oboe", 68));
    woodWinds->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Bassoon", 70));
    woodWinds->setOpen(false);
    dummyRoot->addSubItem(woodWinds.release());

    auto rootBrass = std::make_unique<InstrumentTreeItem>("Brass");
    rootBrass->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Trumpet", 56));
    rootBrass->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Trombone", 57));
    rootBrass->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "French Horn", 59));
    rootBrass->setOpen(false);
    dummyRoot->addSubItem(rootBrass.release());


    auto rootStrings = std::make_unique<InstrumentTreeItem>("Strings");
    rootStrings->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Violin", 40));
    rootStrings->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Viola", 41));
    rootStrings->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Cello", 42));
    rootStrings->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Contrabass", 43));
    rootStrings->setOpen(false);
    dummyRoot->addSubItem(rootStrings.release());

    auto rootReeds = std::make_unique<InstrumentTreeItem>("Reeds");
    rootReeds->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Accordion", 21));   // GM program 21
    rootReeds->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Harmonica", 22));   // GM program 22
    rootReeds->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Bandoneon", 23));   // Custom / assign if needed
    rootReeds->setOpen(false);
    dummyRoot->addSubItem(rootReeds.release());

    auto rootOrgans = std::make_unique<InstrumentTreeItem>("Organs");
    rootOrgans->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Hammond Organ", 16));     // GM Program 17
    rootOrgans->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Percussive Organ", 17)); // GM Program 18
    rootOrgans->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Rock Organ", 18));       // GM Program 19
    rootOrgans->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Church Organ", 19));     // GM Program 20
    rootOrgans->addSubItem(createInstrumentItem(getImageForInstruments("ACB"), "Reed Organ", 20));       // GM Program 21
    rootOrgans->setOpen(false);
    dummyRoot->addSubItem(rootOrgans.release());

    dummyRoot->setOpen(true);
    treeView->setRootItem(dummyRoot.release());
    
}

InstrumentTreeItem* MainComponent::createInstrumentItem(const juce::Image& img, const juce::String& name, int program)
{
    auto item = std::make_unique<InstrumentTreeItem>(img,name,program);
    item->onProgramSelected = [&](int programNumber, juce::String name="")
    {
        auto styleId = display ? display->getStyleID() : "style_default";

        if (leftHandInstrumentToggle.getToggleState() == true)
        {
            midiHandler.setProgramNumber(programNumber, "left"); //setting the actual sound to the new program for both midiHandler and also the recordPlayer by listener
            if (propertiesFile)
            {
                propertiesFile->setValue(styleId + "_leftInstrumentNumber", programNumber);
                propertiesFile->saveIfNeeded();
            }
            if (audioHandler != nullptr)
            {
                auto sfzFile = sfzManager.getSfzForStyleInstrument(styleId, programNumber);
                audioHandler->loadSfz(sfzFile, 1);
            }
        }

        if (rightHandInstrumentToggle.getToggleState() == true)
        {
            midiHandler.setProgramNumber(programNumber, "right");
            if (propertiesFile)
            {
                propertiesFile->setValue(styleId + "_rightInstrumentNumber", programNumber);
                propertiesFile->saveIfNeeded();
            }
            if (audioHandler != nullptr)
            {
                auto sfzFile = sfzManager.getSfzForStyleInstrument(styleId, programNumber);
                audioHandler->loadSfz(sfzFile, 16);
            }
        }

        recordPlayer.setProgarmNumber(programNumber); //setting only the value
    };

    return item.release();
}

void MainComponent::disconnectCurrentOutput()
{
    if (MIDIDevice.isOpenOUT())
        MIDIDevice.deviceCloseOUT();
    if (MIDIDevice.isOpenAudioOUT())
    {
        if (audioHandler != nullptr)
            MIDIDevice.getAudioDeviceManager().removeAudioCallback(audioHandler.get());
        MIDIDevice.deviceCloseAudioOUT();
    }
}

void MainComponent::ensureAudioHandlerReady()
{
    if (audioHandler == nullptr)
    {
        audioHandler = std::make_unique<AudioHandler>(midiHandler);
        audioHandler->onSfzLoadStart = [this]() {
            setLoadingOverlayVisible(true, "Preparing style...");
        };
        audioHandler->onSfzLoadComplete = [this]() {
            setLoadingOverlayVisible(false);
            openingAudioLabel.setText("Preparing session...", juce::dontSendNotification);
        };
        audioHandler->onNoSfzForChannels = [this](int channelMask) {
            juce::String channels;
            int count = 0;
            for (int i = 0; i < 16; ++i)
                if (channelMask & (1 << i)) { channels += juce::String(i + 1) + ", "; ++count; }
            if (channels.isNotEmpty()) channels = channels.dropLastCharacters(2);
            juce::String msg = "No SFZ loaded for channel" + juce::String(count > 1 ? "s: " : ": ") + channels;
            if (temporaryPopup) { temporaryPopup->updateText(msg); temporaryPopup->restartTimer(); }
            else {
                temporaryPopup = std::make_unique<TemporaryMessage>(msg);
                headerPanel.addChildComponent(temporaryPopup.get());
                temporaryPopup->setBounds(getWidth() / 2 - 150, 10, 300, 30);
                temporaryPopup->setFinishedCallBack([this] { temporaryPopup.reset(); });
                temporaryPopup->setVisible(true);
            }
        };
    }
    if (display != nullptr)
        display->setMidiInjectCallback([this](const juce::MidiMessage& msg) {
            midiHandler.injectMidiMessage(msg);
        });

    MIDIDevice.getAudioDeviceManager().addAudioCallback(audioHandler.get());
}

void MainComponent::loadSfzForCurrentStyle(const juce::String& overrideStyleId)
{
    if (audioHandler == nullptr || display == nullptr)
        return;
    const juce::String styleId = overrideStyleId.isNotEmpty() ? overrideStyleId : display->getStyleID();
    int leftProg  = propertiesFile ? propertiesFile->getIntValue(styleId + "_leftInstrumentNumber",  0) : 0;
    int rightProg = propertiesFile ? propertiesFile->getIntValue(styleId + "_rightInstrumentNumber", 0) : 0;
    audioHandler->loadSfz(sfzManager.getSfzForStyleInstrument(styleId, leftProg),  1);
    audioHandler->loadSfz(sfzManager.getSfzForStyleInstrument(styleId, rightProg), 16);
    for (const auto& t : display->getTrackChannelInstruments())
        audioHandler->loadSfz(sfzManager.getSfzForStyleInstrument(styleId, t.instrument), t.channel);
}

void MainComponent::applyCurrentStyleToOutput()
{
    if (display != nullptr)
    {
        loadSettings(display->getStyleID());
        midiHandler.set_start_end_notes(display->getStartNote(), display->getEndNote());
        midiHandler.set_left_right_bounds(display->getLeftBound(), display->getRightBound());
    }
    sendEffectsBeforePlaying();
}

bool MainComponent::openingDevicesForPlay()
{
    for (const auto& device : devicesIN)
    {
        if (device == "No devices found!")
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ERROR", "There are no available input devices!", "OK");
            return false;
        }
    }
    for (const auto& device : devicesOUT)
    {
        if (device == "No devices found!")
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ERROR", "There are no available output devices!", "OK");
            return false;
        }
    }



    int indexIN = this->MIDIDevice.getDeviceIndexIN();
    int indexOUT = this->MIDIDevice.getDeviceIndexOUT();
    bool result;

    if (this->MIDIDevice.getDeviceNameBasedOnIndex(indexIN, 0) == "PC Keyboard")
    {
        //here is the case where user selects keyboard from pc.
        this->MIDIDevice.set_minNote(keyListener.getStartNoteKeyboardInput());
        this->MIDIDevice.set_maxNote(keyListener.getFinishNoteKeyboardInput());
        this->keyboard.set_min_and_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());
        this->keyListener.setIsKeyboardInput(true);
        this->grabKeyboardFocus();
    }
    else {
        this->keyListener.setIsKeyboardInput(false);
        result = this->MIDIDevice.deviceOpenIN(indexIN, &midiHandler);
        if (!result)
        {

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ERROR", "Failed to open input device.", "OK");
            return false;
        }
        this->deviceOpenedIN = this->MIDIDevice.getDeviceIN();
    }
    disconnectCurrentOutput();

    int engineOption = 1; // default to External MIDI
    if (propertiesFile)
        engineOption = propertiesFile->getIntValue("EngineOptionIndex", 1);

    if (engineOption == 1) // External MIDI
    {
        DBG("EXTERNAL opened!");
        result = this->MIDIDevice.deviceOpenOUT(indexOUT);
        if (!result)
        {
            this->MIDIDevice.deviceCloseIN();  
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ERROR", "Failed to open output device.", "OK");
            return false;
        }
        this->deviceOpenedOUT = this->MIDIDevice.getDeviceOUT();
    }
    else if (engineOption == 2) // Internal SFZ Synth
    {
        DBG("INTERNAL OPENED!");
        int indexAudioOUT = this->MIDIDevice.getDeviceIndexAudioOUT();
        result = this->MIDIDevice.deviceOpenAudioOUT(indexAudioOUT);
        if (!result)
        {
            this->MIDIDevice.deviceCloseIN();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ERROR", "Failed to open audio output device.", "OK");
            return false;
        }
        ensureAudioHandlerReady();
    }

    return true;
}

void MainComponent::showColourSelector()
{
    colourSelector = new juce::ColourSelector(
        juce::ColourSelector::showSliders |
        juce::ColourSelector::showColourAtTop |
        juce::ColourSelector::showColourspace
        );
    colourSelector->setName("Colour Picker");
    colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
    colourSelector->addChangeListener(this);
    colourSelector->setSize(500, 300);
    auto area1 = juce::Rectangle<int>(0, 0, 500, headerPanel.getHeight()+headerPanel.getY());


    noteLayer->setVisible(false);
    noteLayer->resetState();
    //noteLayer->repaint();
    this->keyboard.setIsDrawn(false);
    //this->keyboard.repaint();


    repaint();


    juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::ColourSelector>(colourSelector),area1,this);
}

void MainComponent::showInstrumentSelector()
{
    if (treeView == nullptr)
    {
        treeView = std::make_unique<juce::TreeView>();
        treeView->setSize(500, 300);
    }
    if (treeView->getRootItem() != nullptr)
    {
        treeView->deleteRootItem();
        //DBG("deleting root from show");
    }
    //else DBG("not deleting root from show");
    buildTree();
    treeView->setColour(juce::TreeView::backgroundColourId, juce::Colours::white);
    auto area1 = juce::Rectangle<int>(0, 0, 500, headerPanel.getHeight() + headerPanel.getY());
    
    noteLayer->setVisible(false);
    noteLayer->resetState();
    //noteLayer->repaint();
    this->keyboard.setIsDrawn(false);
    //this->keyboard.repaint();

    repaint();

    auto holder = std::make_unique<TreeViewHolder>(treeView.get());
    holder->setOpaque(true);

    juce::CallOutBox::launchAsynchronously(std::move(holder), area1, this);
}

void MainComponent::saveRecordingToFile(double tempo)
{
    if (recordPlayer.getSizeRecorded() <= 1)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Save recording", "No recorded events to play.");
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser>("Save file", juce::File::getCurrentWorkingDirectory(), "*.mid");

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this,tempo](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                juce::String errorMsg;
                if (!recordPlayer.saveRecordingToFile(file, errorMsg,tempo))
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Save Failed", errorMsg);
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Saved", "Recording saved to:\n" + file.getFullPathName());
                }
            }
            fileChooser.reset();
        });
}

void MainComponent::playRecordingFromFile(double tempo)
{
    fileChooser = std::make_unique<juce::FileChooser>("Play from file", juce::File::getCurrentWorkingDirectory(), "*.mid");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, tempo](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                juce::String errorMsg;
                if (!recordPlayer.parseRecordingFromFile(file, errorMsg))
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Play Failed", errorMsg);
                else
                    recordPlayer.startRecordingFilePlaying();
            }
            fileChooser.reset();
        });
}

void MainComponent::handleIntro(const juce::String& name)
{
    if (display)
        display->handleIntroDisplay(name);
}

void MainComponent::handleEnding(const juce::String& name)
{
    if (display)
        display->handleEndingDisplay(name);
}

void MainComponent::handleVar(const juce::String& name)
{
    if (display)
        display->handleVarDisplay(name);
}

void MainComponent::handleFill(const juce::String& name)
{
    if (display)
        display->handleFillDisplay(name);
}

void MainComponent::handleBreak(const juce::String& name)
{
    if (display)
        display->handleBreakDisplay(name);
}

void MainComponent::initializeOverlay()
{
    // In-app child (not a top-level/OS window): it dies with the app, so a crash can't leave a
    // stuck window behind. It's hidden until ESC; setOverlayMenuVisible() hides the GL note layer
    // while it's up so it isn't covered. Bounds are set in resized().
    overlayWindow = std::make_unique<OverlayComponent>();
    addChildComponent(*overlayWindow);
    overlayWindow->setAlwaysOnTop(true);
    overlayWindow->setOpaque(false);

    overlayShouldBeVisible = false;

    setCallBacksForOverlayWindow();

    loadEffectsFromFile();
}

void MainComponent::setLoadingOverlayVisible(bool show, const juce::String& text)
{
    if (show)
    {
        if (text.isNotEmpty())
            openingAudioLabel.setText(text, juce::dontSendNotification);

        // The GL note layer renders on top of normal painting, so it would cover the centered
        // label. Remember its visibility once, then hide it for the duration of the load.
        if (! loadingOverlayActive)
        {
            noteLayerVisibleBeforeLabel = (noteLayer && noteLayer->isVisible());
            loadingOverlayActive = true;
        }
        if (noteLayer) noteLayer->setVisible(false);
        openingAudioLabel.setVisible(true);
    }
    else
    {
        openingAudioLabel.setVisible(false);
        if (loadingOverlayActive && noteLayer)
        {
            // The animation timer was stopped when the layer was hidden (visibilityChanged),
            // so any notes that were mid-flight would otherwise stay frozen on screen until the
            // next note-on restarts the timer. Clear them, matching the arranger-overlay/menu paths.
            noteLayer->resetState();
            noteLayer->setVisible(noteLayerVisibleBeforeLabel);
        }
        loadingOverlayActive = false;
    }
    repaint();
}

void MainComponent::setArrangerOverlaySceneHidden(bool hidden)
{
    if (hidden == sceneHiddenForArrangerOverlay)
        return;                                  // guard re-entrancy (browser->editor presents twice)
    sceneHiddenForArrangerOverlay = hidden;

    if (hidden)
    {
        noteLayerVisibleBeforeArranger = (noteLayer && noteLayer->isVisible());
        if (noteLayer) noteLayer->setVisible(false);   // would render through the full-screen overlay
    }
    else if (noteLayer)
    {
        noteLayer->setVisible(noteLayerVisibleBeforeArranger);
        noteLayer->resetState();                 // don't return to stale frozen notes
    }
}

void MainComponent::highlightArrangerSection(int sectionIndex, ArrangerSectionType type, const juce::String& name)
{
    if (!variationsFills || !introsEndings)
        return;

    if (sectionIndex < 0)   // nothing active (stopped before first play): clear both groups
    {
        variationsFills->setHighlightedButton({});
        introsEndings->setHighlightedButton({});
        return;
    }

    // Map the engine section (e.g. "Variation 2"/"Ending 1") to its live button label by type + number.
    const int    n = juce::jmax(1, name.getTrailingIntValue());
    juce::String buttonName;
    bool         inVariationsFills = true;
    switch (type)
    {
        case ArrangerSectionType::Variation: buttonName = "Var "    + juce::String(n); break;
        case ArrangerSectionType::Fill:      buttonName = "Fill "   + juce::String(n); break;
        case ArrangerSectionType::Break:     buttonName = "Break";                     break;
        case ArrangerSectionType::Intro:     buttonName = "Intro "  + juce::String(n); inVariationsFills = false; break;
        case ArrangerSectionType::Ending:    buttonName = "Ending " + juce::String(n); inVariationsFills = false; break;
        case ArrangerSectionType::CountIn:   variationsFills->setHighlightedButton({});
                                             introsEndings->setHighlightedButton({});
                                             return;
    }

    // Highlight in the owning group; clear the other so only one button is ever lit.
    if (inVariationsFills)
    {
        variationsFills->setHighlightedButton(buttonName);
        introsEndings->setHighlightedButton({});
    }
    else
    {
        introsEndings->setHighlightedButton(buttonName);
        variationsFills->setHighlightedButton({});
    }
}

void MainComponent::setSectionButtonsEngineDriven(bool engineDriven)
{
    if (variationsFills)
    {
        variationsFills->setEngineDrivenHighlight(engineDriven);
        if (!engineDriven) variationsFills->setHighlightedButton({});
    }
    if (introsEndings)
    {
        introsEndings->setEngineDrivenHighlight(engineDriven);
        if (!engineDriven) introsEndings->setHighlightedButton({});
    }
}

void MainComponent::setOverlayMenuVisible(bool show)
{
    if (overlayWindow == nullptr)
        return;

    if (show)
    {
        overlayWindow->setBounds(getLocalBounds());
        // Hide the GL note layer (would cover the overlay) and the keyboard, remembering both.
        noteLayerVisibleBeforeMenu = (noteLayer && noteLayer->isVisible());
        keyboardVisibleBeforeMenu  = keyboard.isVisible();
        if (noteLayer) noteLayer->setVisible(false);
        keyboard.setVisible(false);

        overlayWindow->setVisible(true);
        overlayWindow->toFront(true);
        overlayWindow->grabKeyboardFocus();
    }
    else
    {
        overlayWindow->setVisible(false);
        if (noteLayer)
        {
            noteLayer->resetState();   // clear leftover falling/active notes so we return to a clean screen
            noteLayer->setVisible(noteLayerVisibleBeforeMenu);
        }
        keyboard.setVisible(keyboardVisibleBeforeMenu);
        grabKeyboardFocus();   // so ESC/keys keep working after the menu closes
    }
}

SmoothRotarySlider::SmoothRotarySlider()
{
    setSliderStyle(juce::Slider::Rotary);
}

void SmoothRotarySlider::mouseDown(const juce::MouseEvent& e)
{
    dragStartValue = getValue(); 
    dragStartPos = e.position; 
    juce::Slider::mouseDown(e);
}

void SmoothRotarySlider::mouseDrag(const juce::MouseEvent& e)
{
    float dx = e.position.x - dragStartPos.x;
    float dy = dragStartPos.y - e.position.y;

    float sensitivity = 0.005f;

    float delta = dx + dy;

    float newValue = dragStartValue + delta * sensitivity * (getMaximum() - getMinimum());
    setValue(newValue, juce::dontSendNotification);
}

KnobLookAndFeel::KnobLookAndFeel()
{
    rawKnobImage = juce::ImageCache::getFromMemory(BinaryData::Knob_png, BinaryData::Knob_pngSize);
    rotaryStartAngle = juce::MathConstants<float>::pi * 1.25f;
    rotaryEndAngle = juce::MathConstants<float>::pi * 2.75f; 
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStart, float rotaryEnd, juce::Slider& slider)
{
    float startAngle = rotaryStartAngle; 
    
    
    float endAngle = rotaryEndAngle;

    sliderPosProportional = juce::jlimit(0.0f, 1.0f, sliderPosProportional);

    float angle = startAngle + sliderPosProportional * (endAngle - startAngle);

    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f; 
    const int knobSize = juce::jmin(width, height);

    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    if (rawKnobImage.isValid())
    {
        juce::Image scaledKnob = rawKnobImage.rescaled(knobSize, knobSize);
        juce::AffineTransform transform = juce::AffineTransform::translation(-scaledKnob.getWidth() * 0.5f, -scaledKnob.getHeight() * 0.5f).rotated(angle).translated(cx, cy);
        g.drawImageTransformed(scaledKnob, transform);
    }
    else
    {
        float radius = knobSize * 0.5f - 4.0f;
        g.setColour(juce::Colours::darkgrey); 
        
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colours::orange);

        juce::Point<float> lineStart(cx, cy); 
        juce::Point<float> lineEnd(cx + radius * std::cos(angle), cy + radius * std::sin(angle)); 
        g.drawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, 3.0f);
    }
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    return juce::Font(40.0f, juce::Font::bold);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown)
{
    juce::ignoreUnused(button, isMouseOverButton, isButtonDown); g.fillAll(backgroundColour);
}

void MainComponent::Panel::paint(juce::Graphics& g)
{
    juce::Colour translucentGray = juce::Colour::fromRGBA(169, 169, 169, 204);
    juce::Colour option = juce::Colour::fromRGBA(50, 100, 95, 170);
    juce::Colour subtleBorderColor = juce::Colour::fromRGBA(169, 169, 169, 200);
    juce::Colour secondBorder = juce::Colour::fromRGBA(140, 140, 140, 220);

    g.fillAll(option); // Background of the panel
    g.setColour(subtleBorderColor); 
    g.drawRect(getLocalBounds(), 3);
}

void MainComponent::headerPanel::paint(juce::Graphics& g)
{
    // Base dark fill
    g.fillAll(AppColours::panelBg);

    // Subtle purple gradient overlay (fades from top)
    juce::ColourGradient overlay(AppColours::accent2.withAlpha(0.18f), 0.0f, 0.0f,
                                 juce::Colours::transparentBlack, 0.0f, (float)getHeight(), false);
    g.setGradientFill(overlay);
    g.fillAll();

    // Top accent strip
    g.setColour(AppColours::accent2);
    g.fillRect(0, 0, getWidth(), 3);
}
