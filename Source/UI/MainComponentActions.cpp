#include "MainComponent.h"
#include "AppInfo.h"

namespace
{
    namespace Text
    {
        const juce::String enterKeyCountTitle   = "Enter Number of Keys";
        const juce::String enterKeyCount        = "Please enter how many keys this device has:";
        const juce::String okButton             = "OK";
        const juce::String digits               = "0123456789";
        const juce::String invalidInputTitle    = "Invalid Input";
        const juce::String invalidKeyCountInput = "Please enter a valid number of keys.";
        const juce::String invalidKeyCountTitle = "Invalid Key Count";
        const juce::String supportedKeyCounts   = "Please enter one of: 49, 61, 76 or 88.";
        const juce::String successTitle         = "Success";
        const juce::String deviceUpdated        = "Device updated successfully!";
    }

    constexpr int rangeAccepted     = 0;    // handlePlayableRange: negative means the device was refused
    constexpr int usbIdLength       = 4;    // four hex digits
    constexpr int firstChordMode    = 0;    // ChordMode values as stored
    constexpr int lastChordMode     = 2;
    constexpr int defaultProgram    = 0;

    // The keyboard sizes the app knows (see the device database).
    namespace KeyboardSizes
    {
        constexpr int keys49 = 49;
        constexpr int keys61 = 61;
        constexpr int keys76 = 76;
        constexpr int keys88 = 88;
    }
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
    // The Stage panel replaces the old header, which stays hidden (its controls live on the panel and pages).
    if (stagePanel != nullptr)
        stagePanel->setVisible(true);   // on Log in / Home too (asleep), since the instrument look

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

    layoutStage();
    refreshStage();
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
    // The MIDI settings are the Settings page now (this home-screen button has been hidden since phase 1).
    openStagePage(StagePage::settings);
}

void MainComponent::homeButtonOnClick()
{
    currentBackground = {};   // Home is on the instrument now, not on the picture
    repaint();

    toggleHPanel();
    inPlayView = false;
    //toggleSettingsButton(); don't need this button for now
    keyboard.setVisible(false);
    this->noteLayer->setVisible(false);
    this->noteLayer->resetState();
    if(this->recordPlayer.getIsPlaying())
        this->recordPlayer.stopPlayBack();

    if (display && display->hasOpenStyle() && display->getView() != StyleView::style)
        display->homeButtonInteraction();   // a sub-page of the style was open (formerly a temporary tab)

    showFrontScreen(FrontScreen::home);
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

        if (midiHandler.handlePlayableRange(VID, PID, dataBase.getNrKeysPidVid(VID, PID), this->keyListener.getIsKeyboardInput()) < rangeAccepted)
        {
            this->MIDIDevice.deviceCloseIN();
            this->MIDIDevice.deviceCloseOUT();
            return;
        }



        this->recordPlayer.setOutputDevice(MIDIDevice.getDeviceOUT());


        
        

        MIDIDevice.setPID(PID);
        MIDIDevice.setVID(VID);
        MIDIDevice.setDeviceName(name);

        if (VID.length() < usbIdLength || PID.length() < usbIdLength)
            this->display->set_VID_PID({}, {});
        else this->display->set_VID_PID(VID, PID);

        this->display->readPlaybackSettingsFromProperties();
        loadEffectsFromFile(this->display->getStyleID());

        currentBackground = playBackground;
        repaint();
        toggleHPanel();
        inPlayView = true;
        showFrontScreen(FrontScreen::none);

        sendEffectsBeforePlaying();

        /*
        MIDIDevice.sendMidiCC(1, 7, MIDIDevice.getVolume(1));
        MIDIDevice.sendMidiCC(1,91,MIDIDevice.getReverb(1));
        MIDIDevice.sendMidiCC(16,91,MIDIDevice.getReverb(16));
        MIDIDevice.sendMidiCC(16,7,MIDIDevice.getVolume(16));
        */

        recordPlayer.setReverb(MIDIDevice.getReverb(HandChannels::leftHand),HandChannels::leftHand);
        recordPlayer.setReverb(MIDIDevice.getReverb(HandChannels::rightHand), HandChannels::rightHand);

        recordPlayer.setVolume(MIDIDevice.getVolume(HandChannels::leftHand), HandChannels::leftHand);
        recordPlayer.setVolume(MIDIDevice.getVolume(HandChannels::rightHand), HandChannels::rightHand);

        display->setDeviceOutput(MIDIDevice.getDeviceOUT());

        if (audioHandler != nullptr && display != nullptr)
        {
            display->setMidiInjectCallback([this](const juce::MidiMessage& msg) {
                midiHandler.injectMidiMessage(msg);
            });
        }

        if (display != nullptr && propertiesFile != nullptr)
        {
            const bool arrangerOn = propertiesFile->getBoolValue(SettingsFileKeys::arrangerModeEnabled, false);
            display->setArrangerModeEnabled(arrangerOn);
            setSectionButtonsEngineDriven(arrangerOn);
            midiHandler.setArrangerEngaged(arrangerOn);   // restore the chord-zone-mute gate

            // Restore the remembered Auto Fill choice (app-wide, not per style): apply to the engine
            // and reflect the tick in the toggle button.
            const bool autoFillOn = propertiesFile->getBoolValue(SettingsFileKeys::arrangerAutoFillEnabled, false);
            display->setArrangerAutoFillEnabled(autoFillOn);
            if (variationsFills != nullptr)
                for (auto* group : variationsFills->getSectionGroups())
                    if (auto* toggle = group->getToggleButton())
                        toggle->setToggleState(autoFillOn, juce::dontSendNotification);

            // Phase 4/5: apply the remembered chord choices (defaults: Fingered mode, memory on,
            // bass inversion off) so they take effect without needing to toggle them each launch.
            display->setArrangerBassInversion(propertiesFile->getBoolValue(SettingsFileKeys::chordBassInversion, false));
            {
                ChordMode mode = ChordMode::Fingered;
                if (propertiesFile->containsKey(SettingsFileKeys::chordMode))
                    mode = (ChordMode) juce::jlimit(firstChordMode, lastChordMode, propertiesFile->getIntValue(SettingsFileKeys::chordMode, firstChordMode));
                else if (propertiesFile->getBoolValue(SettingsFileKeys::chordFullKeyboard, false))
                    mode = ChordMode::FullKeyboard;   // migrate legacy bool
                midiHandler.setChordMode(mode);
            }
            midiHandler.setChordMemory(propertiesFile->getBoolValue(SettingsFileKeys::chordMemory, true));
            midiHandler.setChordZoneMute(propertiesFile->getBoolValue(SettingsFileKeys::chordZoneMute, false));
            display->setArrangerSynchroStart(propertiesFile->getBoolValue(SettingsFileKeys::synchroStart, false));
            display->setArrangerCountIn(propertiesFile->getBoolValue(SettingsFileKeys::countIn, false));

            // Phase 7a: restore the NTT master enable (default on) + curated minor scale (default Dorian).
            display->setArrangerNttEnabled(propertiesFile->getBoolValue(SettingsFileKeys::nttEnabled, true));
            {
                const int minorScale = juce::jlimit(MinorScaleButtons::dorian, MinorScaleButtons::harmonic, propertiesFile->getIntValue(SettingsFileKeys::nttMinorScale, MinorScaleButtons::dorian));
                const auto scale = minorScale == MinorScaleButtons::aeolian ? MinorScaleChoice::Aeolian
                                 : minorScale == MinorScaleButtons::harmonic ? MinorScaleChoice::HarmonicMinor
                                                                             : MinorScaleChoice::Dorian;
                display->setArrangerMinorScale(scale);
            }
        }

        if (!keyboardInitialized)
        {
            keyBoardUIinit(MIDIDevice.get_minNote(), MIDIDevice.get_maxNote());

            auto styleId = display ? display->getStyleID() : StyleNames::defaultStyleId;
            int leftProg  = propertiesFile ? propertiesFile->getIntValue(styleId + SettingsFileKeys::leftInstrumentSuffix,  defaultProgram)
                                           : midiHandler.getProgramNumberLeftHand();
            int rightProg = propertiesFile ? propertiesFile->getIntValue(styleId + SettingsFileKeys::rightInstrumentSuffix, defaultProgram)
                                           : midiHandler.getProgramNumberRightHand();

            midiHandler.setProgramNumber(leftProg, HandChoiceNames::left);
            midiHandler.setProgramNumber(rightProg, HandChoiceNames::right);

            recordPlayer.setProgarmNumber(leftProg,HandChoiceNames::left);
            recordPlayer.setProgarmNumber(rightProg, HandChoiceNames::right);
        }
        else {
            keyboard.setVisible(true);
            this->noteLayer->setVisible(true);
            if (StageAlert::current() != nullptr)
                keepNoteLayerUnderAlert();   // an alert opened on the way here stays on top
        }

        if (MIDIDevice.isOpenAudioOUT())
            loadSfzForCurrentStyle();

        if (this->keyListener.getIsKeyboardInput())
        {
            this->keyboard.set_min_and_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());
            this->display->set_min_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput(), true);
        }
        else
        {
            this->keyboard.set_min_and_max(MIDIDevice.get_minNote(), MIDIDevice.get_maxNote());
            this->display->set_min_max(MIDIDevice.get_minNote(), MIDIDevice.get_maxNote(), false);
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
    if (selectedText == MainComponentText::selectDevice || selectedText == DeviceNames::noDevicesFound)
        return;

    juce::String identifier = updateDevicesMap[selectedText];
    juce::String VID = MIDIDevice.extractVID(identifier);
    juce::String PID = MIDIDevice.extractPID(identifier);


    StageAlert::showInput (Text::enterKeyCountTitle, Text::enterKeyCount, Text::okButton, {},
        [this, VID, PID] (const juce::String& keysStr)
            {
                if (!keysStr.containsOnly(Text::digits))
                {
                    StageAlert::showMessage (Text::invalidInputTitle, Text::invalidKeyCountInput);
                    return;
                }

                int numKeys = keysStr.getIntValue();
                if (numKeys != KeyboardSizes::keys49 && numKeys != KeyboardSizes::keys61 && numKeys != KeyboardSizes::keys76 && numKeys != KeyboardSizes::keys88)
                {
                    StageAlert::showMessage (Text::invalidKeyCountTitle, Text::supportedKeyCounts);
                    return;
                }

                dataBase.updateDeviceJson(VID, PID, {}, numKeys);

                StageAlert::showMessage (Text::successTitle, Text::deviceUpdated);
            });

}
