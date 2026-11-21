#include "MainComponent.h"
#include "AppInfo.h"


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
