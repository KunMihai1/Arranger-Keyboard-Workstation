#include "SettingsWindow.h"
#include "MidiHandler.h"
#include "AppColours.h"
#include "IOHelper.h"


using namespace AppColours;

namespace
{
    namespace Text
    {
        const juce::String windowTitle  = "MIDI Settings";
        const juce::String midiIn       = "MIDI IN";
        const juce::String midiOut      = "MIDI OUT";
        const juce::String outputEngine = "Output Engine";
        const juce::String audioOut     = "Audio OUT";
        const juce::String chordMode    = "Chord Mode";
        const juce::String minorScale   = "Minor scale";
        const juce::String externalMidi = "External MIDI";        // OutputEngineIds::externalMidi
        const juce::String internalSfz  = "Internal SFZ Synth";   // OutputEngineIds::internalSfz
    }

    constexpr int   windowX             = 200;
    constexpr int   windowY             = 150;
    constexpr int   windowHeight        = 250;
    constexpr int   panelWidth          = 480;
    constexpr int   panelHeight         = 630;   // tall enough for every arranger row
    constexpr int   deviceRefreshMs     = 750;
    constexpr int   firstListIndex      = 0;
    constexpr int   labelTextOffset     = 8;
    constexpr int   labelHeight         = 20;
    constexpr int   engineComboWidth    = 180;
    constexpr int   sectionGap          = 55;
    constexpr int   deviceRowGap        = 46;
    constexpr int   centringDivisor     = 2;
    constexpr int   buttonGap           = 14;
    constexpr int   toggleWidth         = 320;
    constexpr int   toggleGap           = 6;
    constexpr int   choiceLabelWidth    = 92;
    constexpr int   choiceButtonsOffset = 96;
    constexpr int   choiceButtonWidth   = 104;
    constexpr int   choiceGap           = 6;
    constexpr int   thirdChoice         = 2;     // index of the third button in a row
    constexpr int   sfzLibraryInset     = 20;
    constexpr int   firstChordMode      = 0;     // ChordMode values as stored
    constexpr int   lastChordMode       = 2;
    constexpr float labelFontHeight     = 13.0f;
}

MIDIWindow::MIDIWindow(MidiDevice& mdevice, std::vector<std::string>& devicesListIN, std::vector<std::string>& devicesListOUT
    , std::vector<std::string>& devicesListAudioOUT, juce::PropertiesFile* prop, SFZLibraryManager* sfzManagerPtr)
    : juce::DocumentWindow{ Text::windowTitle, background, DocumentWindow::closeButton },
    MIDIDevice{ mdevice }, devicesListIN{ devicesListIN }, devicesListOUT{ devicesListOUT }, devicesListAudioOUT{ devicesListAudioOUT }, propertyFile{ prop }, sfzManager{ sfzManagerPtr }
{
    setUsingNativeTitleBar(false);
    setResizable(false, false);
    setTitleBarTextCentred(true);
    setColour(juce::DocumentWindow::backgroundColourId, background);
    setColour(juce::DocumentWindow::textColourId, titleText);
    setBounds(windowX, windowY, panelWidth, windowHeight);
    setVisible(true);
    setContentOwned(&settingsPanel, false);

    addKeyListener(this);
    setBounds_components();
    allInit();
    toggleSettingsAll();
    populateCBIN();
    populateCBOUT();
    populateCBaudioOUT();
	populateCBengine();

    restoreCBoxes(); // Apply saved settings
}

MIDIWindow::~MIDIWindow()
{
    stopTimer();
    removeKeyListener(this);
}


bool MIDIWindow::keyPressed(const juce::KeyPress& key, juce::Component* comp)
{
    if (key == juce::KeyPress::escapeKey)
    {
        closeButtonPressed();
        return true;          
    }
    return false;
}

void MIDIWindow::visibilityChanged()
{
    if (isVisible())
    {
        populateCBIN();  //selected id is set to 1 implicitly
        populateCBOUT(); //selected id is set to 1 implicitly
        restoreCBoxes();
        startTimer(deviceRefreshMs);
        //grabKeyboardFocus();
    }
    else stopTimer();

}

void MIDIWindow::closeButtonPressed()
{

    setVisible(false);

    if (onWindowClosed)
        onWindowClosed();
}

void MIDIWindow::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &this->comboBoxDevicesIN)
    {
        int index = comboBoxDevicesIN.getSelectedId() - firstItemId;
        if (index >= firstListIndex && index < this->devicesListIN.size())
        {
            this->MIDIDevice.setDeviceIN(index);
            lastIndexIN = index + firstItemId;
            if (propertyFile) { propertyFile->setValue(SettingsFileKeys::midiInIndex, lastIndexIN); propertyFile->saveIfNeeded(); }
        }
    }
    else if (comboBoxThatHasChanged == &this->comboBoxDevicesOUT)
    {
        int index = comboBoxDevicesOUT.getSelectedId() - firstItemId;
        if (index >= firstListIndex && index < this->devicesListOUT.size())
        {
            this->MIDIDevice.setDeviceOUT(index);
            lastIndexOUT = index + firstItemId;
            if (propertyFile) { propertyFile->setValue(SettingsFileKeys::midiOutIndex, lastIndexOUT); propertyFile->saveIfNeeded(); }
        }
    }
    else if (comboBoxThatHasChanged == &this->comboBoxOutputEngineType)
    {
		lastIndexEngineOption = comboBoxOutputEngineType.getSelectedId();
        if (propertyFile) { propertyFile->setValue(SettingsFileKeys::engineOptionIndex, lastIndexEngineOption); propertyFile->saveIfNeeded(); }
		switchBetweenEngineOptions(lastIndexEngineOption);
        if (onOutputEngineChanged && isMidiDeviceOpen && isMidiDeviceOpen())
            onOutputEngineChanged(lastIndexEngineOption);
    }
    else if(comboBoxThatHasChanged == &this->comboBoxAudioDevicesOUT)
    {
        int index = comboBoxAudioDevicesOUT.getSelectedId() - firstItemId;
        if (index >= firstListIndex && index < this->devicesListAudioOUT.size())
        {
            this->MIDIDevice.setDeviceAudioOUT(index);
			lastIndexAudioOut = index + firstItemId;
            if (propertyFile) { propertyFile->setValue(SettingsFileKeys::audioOutIndex, lastIndexAudioOut); propertyFile->saveIfNeeded(); }
        }
	}
}

void MIDIWindow::timerCallback()
{
    std::vector<std::string> devicesNewIN;
    this->MIDIDevice.getAvailableDevicesMidiIN(devicesNewIN);
    if (devicesNewIN != this->devicesListIN)
    {
        if (this->comboBoxDevicesIN.isPopupActive())
            this->comboBoxDevicesIN.hidePopup();
        devicesListIN = devicesNewIN;
        populateCBIN();
    }
    std::vector<std::string> devicesNewOUT;
    this->MIDIDevice.getAvailableDevicesMidiOUT(devicesNewOUT);
    if (devicesNewOUT != this->devicesListOUT)
    {
        if (this->comboBoxDevicesOUT.isPopupActive())
            this->comboBoxDevicesOUT.hidePopup();
        devicesListOUT = devicesNewOUT;
        populateCBOUT();
    }

    std::vector<std::string> devicesNewAudioOUT;
    MIDIDevice.getAvailableAudioDevicesOUT(devicesNewAudioOUT);
    if (devicesNewAudioOUT != devicesListAudioOUT)
    {
        if (comboBoxAudioDevicesOUT.isPopupActive())
            comboBoxAudioDevicesOUT.hidePopup();

        devicesListAudioOUT = devicesNewAudioOUT;
        populateCBaudioOUT();
    }
}

void MIDIWindow::toggleSettingsPanel()
{
    if (this->settingsPanel.isVisible())
        this->settingsPanel.setVisible(false);
    else this->settingsPanel.setVisible(true);
}

void MIDIWindow::toggleSettingsCB()
{
	toggleSettingsInMIDICB();
	toggleSettingsOutMIDICB();
    toggleSettingsOutputEngineCB();
}

void MIDIWindow::toggleSettingsInMIDICB()
{
    if (this->comboBoxDevicesIN.isVisible()) {
        this->comboBoxDevicesIN.setVisible(false);
        this->midiDevicesLabelIN.setVisible(false);
    }
    else {
        this->midiDevicesLabelIN.setVisible(true);
        this->comboBoxDevicesIN.setVisible(true);
    }
}

void MIDIWindow::toggleSettingsOutMIDICB()
{
    if (this->comboBoxDevicesOUT.isVisible()) {
        this->comboBoxDevicesOUT.setVisible(false);
        this->midiDevicesLabelOUT.setVisible(false);
    }
    else {
        this->comboBoxDevicesOUT.setVisible(true);
        this->midiDevicesLabelOUT.setVisible(true);
    }
}

void MIDIWindow::toggleSettingsOutputEngineCB()
{
    if (this->comboBoxOutputEngineType.isVisible())
    {
        this->comboBoxOutputEngineType.setVisible(false);
        this->outputEngineTypeLabel.setVisible(false);
    }
    else
    {
        this->comboBoxOutputEngineType.setVisible(true);
        this->outputEngineTypeLabel.setVisible(true);
    }
}

void MIDIWindow::toggleSettingsAudioOutCB()
{
    if (this->comboBoxAudioDevicesOUT.isVisible())
    {
        this->comboBoxAudioDevicesOUT.setVisible(false);
        this->audioDevicesLabelOUT.setVisible(false);
    }
    else
    {
        this->comboBoxAudioDevicesOUT.setVisible(true);
        this->audioDevicesLabelOUT.setVisible(true);
    }
}

void MIDIWindow::toggleSFZbutton()
{
    if(this->sfzLibraryButton.isVisible())
    {
        this->sfzLibraryButton.setVisible(false);
    }
    else
    {
        this->sfzLibraryButton.setVisible(true);
	}
}

void MIDIWindow::toggleSettingsAll()
{
    toggleSettingsPanel();
    toggleSettingsCB();
}

void MIDIWindow::setBounds_components()
{
    const int pad = 16;
    const int rowH = 36;
    int rowTop = pad;

    const int labelW = 80;
    const int cbX = 100;
    const int cbW = 260;


    outputEngineTypeLabel.setBounds(pad, rowTop + labelTextOffset, labelW, labelHeight);
    comboBoxOutputEngineType.setBounds(cbX, rowTop, engineComboWidth, rowH);

    rowTop += sectionGap;


    midiDevicesLabelIN.setBounds(pad, rowTop + labelTextOffset, labelW, labelHeight);
    comboBoxDevicesIN.setBounds(cbX, rowTop, cbW, rowH);

    rowTop += deviceRowGap;

    midiDevicesLabelOUT.setBounds(pad, rowTop + labelTextOffset, labelW, labelHeight);
    comboBoxDevicesOUT.setBounds(cbX, rowTop, cbW, rowH);

	audioDevicesLabelOUT.setBounds(pad, rowTop + labelTextOffset, labelW, labelHeight);
	comboBoxAudioDevicesOUT.setBounds(cbX, rowTop, cbW, rowH);

    rowTop += sectionGap;

    const int buttonW = 140;
    const int buttonH = 34;
    const int buttonX = (settingsPanel.getWidth() - buttonW) / centringDivisor;

    sfzLibraryButton.setBounds(buttonX, rowTop, buttonW, buttonH);

    rowTop += buttonH + buttonGap;
    arrangerModeToggle.setBounds(pad, rowTop, toggleWidth, rowH);
    rowTop += rowH + toggleGap;
    chordBassInversionToggle.setBounds(pad, rowTop, toggleWidth, rowH);
    rowTop += rowH + toggleGap;
    {
        chordModeLabel.setBounds(pad, rowTop, choiceLabelWidth, rowH);
        const int bx = pad + choiceButtonsOffset, bw = choiceButtonWidth, gap = choiceGap;
        chordModeFingered.setBounds(bx,                  rowTop, bw, rowH);
        chordModeSingle.setBounds  (bx + (bw + gap),     rowTop, bw, rowH);
        chordModeFull.setBounds    (bx + thirdChoice * (bw + gap), rowTop, bw, rowH);
    }
    rowTop += rowH + toggleGap;
    chordMemoryToggle.setBounds(pad, rowTop, toggleWidth, rowH);
    rowTop += rowH + toggleGap;
    synchroStartToggle.setBounds(pad, rowTop, toggleWidth, rowH);
    rowTop += rowH + toggleGap;
    countInToggle.setBounds(pad, rowTop, toggleWidth, rowH);
    rowTop += rowH + toggleGap;
    chordZoneMuteToggle.setBounds(pad, rowTop, toggleWidth, rowH);
    rowTop += rowH + toggleGap;
    nttEnabledToggle.setBounds(pad, rowTop, toggleWidth, rowH);
    rowTop += rowH + toggleGap;
    {
        nttMinorScaleLabel.setBounds(pad, rowTop, choiceLabelWidth, rowH);
        const int bx = pad + choiceButtonsOffset, bw = choiceButtonWidth, gap = choiceGap;
        nttMinorDorian.setBounds  (bx,                  rowTop, bw, rowH);
        nttMinorAeolian.setBounds (bx + (bw + gap),     rowTop, bw, rowH);
        nttMinorHarmonic.setBounds(bx + thirdChoice * (bw + gap), rowTop, bw, rowH);
    }
}

void MIDIWindow::panelInit()
{
    settingsPanel.setSize(panelWidth, panelHeight);   // tall enough for the arranger + Phase 4/5/6 chord rows + Phase 7a NTT rows

    setBackgroundColour(panelBg);

    setContentOwned(&settingsPanel, true);
    settingsPanel.setVisible(false);
}

void MIDIWindow::devicesCBinit()
{
	deviceInMIDICBinit();
	deviceOutMIDICBinit();
	deviceAudioOutCBinit();
}

void MIDIWindow::deviceInMIDICBinit()
{
    comboBoxDevicesIN.addListener(this);
    setStyleComboBox(comboBoxDevicesIN);
    settingsPanel.addAndMakeVisible(this->comboBoxDevicesIN);
    comboBoxDevicesIN.setVisible(false);

    midiDevicesLabelIN.setText(Text::midiIn, juce::dontSendNotification);
    setStyleLabel(midiDevicesLabelIN);
    settingsPanel.addAndMakeVisible(midiDevicesLabelIN);
    midiDevicesLabelIN.setVisible(false);
}

void MIDIWindow::deviceOutMIDICBinit()
{
    comboBoxDevicesOUT.addListener(this);
    setStyleComboBox(comboBoxDevicesOUT);
    settingsPanel.addAndMakeVisible(this->comboBoxDevicesOUT);
    comboBoxDevicesOUT.setVisible(false);

    midiDevicesLabelOUT.setText(Text::midiOut, juce::dontSendNotification);
    setStyleLabel(midiDevicesLabelOUT);
    settingsPanel.addAndMakeVisible(this->midiDevicesLabelOUT);
    midiDevicesLabelOUT.setVisible(false);
}

void MIDIWindow::outputEngineCBinit()
{
    comboBoxOutputEngineType.addListener(this);
	setStyleComboBox(comboBoxOutputEngineType);
	settingsPanel.addAndMakeVisible(this->comboBoxOutputEngineType);
	comboBoxOutputEngineType.setVisible(false);

	outputEngineTypeLabel.setText(Text::outputEngine, juce::dontSendNotification);
	setStyleLabel(outputEngineTypeLabel);
	settingsPanel.addAndMakeVisible(this->outputEngineTypeLabel);
	outputEngineTypeLabel.setVisible(false);
}

void MIDIWindow::deviceAudioOutCBinit()
{
	comboBoxAudioDevicesOUT.addListener(this);
	setStyleComboBox(comboBoxAudioDevicesOUT);
	settingsPanel.addAndMakeVisible(this->comboBoxAudioDevicesOUT);
	comboBoxAudioDevicesOUT.setVisible(false);

	audioDevicesLabelOUT.setText(Text::audioOut, juce::dontSendNotification);
	setStyleLabel(audioDevicesLabelOUT);
	settingsPanel.addAndMakeVisible(this->audioDevicesLabelOUT);
	audioDevicesLabelOUT.setVisible(false);
}

void MIDIWindow::sfzButtonInit()
{
	sfzLibraryButton.setButtonText(SettingsWindowText::sfzLibrary);
	settingsPanel.addAndMakeVisible(sfzLibraryButton);
	sfzLibraryButton.setVisible(false);
	sfzLibraryButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    
    if (sfzManager != nullptr)
    {
        sfzLibraryUI = std::make_unique<SFZLibraryUI>(*sfzManager);
        settingsPanel.addChildComponent(sfzLibraryUI.get());
        sfzLibraryUI->setVisible(false);
        sfzLibraryUI->getCurrentStyleId = [this]() -> juce::String
        {
            return getCurrentStyleId ? getCurrentStyleId() : juce::String();
        };
        sfzLibraryUI->getCurrentStyleName = [this]() -> juce::String
        {
            return getCurrentStyleName ? getCurrentStyleName() : StyleNames::defaultStyle;
        };
        sfzLibraryUI->onLibraryChanged = [this]()
        {
            sfzManager->save(IOHelper::getFile(AppDataNames::sfzLibraryFile));
            if (onSfzLibraryChanged) onSfzLibraryChanged();
        };
    }

    sfzLibraryButton.onClick = [this]()
    {
        if (sfzLibraryUI)
        {
            sfzLibraryUI->setVisible(!sfzLibraryUI->isVisible());
            if (sfzLibraryUI->isVisible())
            {
                sfzLibraryUI->setBounds(settingsPanel.getLocalBounds().reduced(sfzLibraryInset));
                sfzLibraryUI->toFront(true);
            }
        }
	};
}

void MIDIWindow::allInit()
{
    if (propertyFile != nullptr)
    {
        lastIndexIN = propertyFile->getIntValue(SettingsFileKeys::midiInIndex, firstItemId);
        lastIndexOUT = propertyFile->getIntValue(SettingsFileKeys::midiOutIndex, firstItemId);
        lastIndexEngineOption = propertyFile->getIntValue(SettingsFileKeys::engineOptionIndex, firstItemId);
        lastIndexAudioOut = propertyFile->getIntValue(SettingsFileKeys::audioOutIndex, firstItemId);
    }

    panelInit();
    devicesCBinit();
    outputEngineCBinit();

    settingsPanel.addAndMakeVisible(arrangerModeToggle);
    arrangerModeToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue(SettingsFileKeys::arrangerModeEnabled, false),
                                      juce::dontSendNotification);
    arrangerModeToggle.onClick = [this]()
    {
        const bool on = arrangerModeToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::arrangerModeEnabled, on); propertyFile->saveIfNeeded(); }
        if (onArrangerModeChanged) onArrangerModeChanged(on);
    };

    // Phase 4 arranger chord toggles. Defaults: bass inversion off, split scan, memory on.
    // Each onClick captures `this` and references its own member directly (a previous helper captured
    // a reference parameter that dangled, so the callback never fired).
    settingsPanel.addAndMakeVisible(chordBassInversionToggle);
    chordBassInversionToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue(SettingsFileKeys::chordBassInversion, false),
                                            juce::dontSendNotification);
    chordBassInversionToggle.onClick = [this]()
    {
        const bool on = chordBassInversionToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::chordBassInversion, on); propertyFile->saveIfNeeded(); }
        if (onChordBassInversionChanged) onChordBassInversionChanged(on);
    };

    // Phase 5: 3-way chord-recognition mode selector (replaces the old Full-Keyboard on/off toggle).
    auto readStoredMode = [this]() -> ChordMode
    {
        if (propertyFile == nullptr) return ChordMode::Fingered;
        if (propertyFile->containsKey(SettingsFileKeys::chordMode))
            return (ChordMode) juce::jlimit(firstChordMode, lastChordMode, propertyFile->getIntValue(SettingsFileKeys::chordMode, firstChordMode));
        // migrate legacy bool: Full-Keyboard on -> FullKeyboard, else Fingered
        return propertyFile->getBoolValue(SettingsFileKeys::chordFullKeyboard, false) ? ChordMode::FullKeyboard
                                                                      : ChordMode::Fingered;
    };

    chordModeLabel.setText(Text::chordMode, juce::dontSendNotification);
    settingsPanel.addAndMakeVisible(chordModeLabel);

    const int kChordModeRadio = 0x5C40DE;   // unique non-zero radio group id
    for (auto* button : { &chordModeFingered, &chordModeSingle, &chordModeFull })
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(kChordModeRadio);
        button->setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        // Selected = warm-orange fill with dark text; unselected = dark card with muted text, so the
        // active mode reads at a glance (the default TextButton on/off states were near-identical).
        button->setColour(juce::TextButton::buttonColourId,   cardBg);
        button->setColour(juce::TextButton::buttonOnColourId, accent1);
        button->setColour(juce::TextButton::textColourOffId,  subtitleText);
        button->setColour(juce::TextButton::textColourOnId,   background);
        settingsPanel.addAndMakeVisible(*button);
    }

    const ChordMode storedMode = readStoredMode();
    (storedMode == ChordMode::FullKeyboard   ? chordModeFull
     : storedMode == ChordMode::SingleFinger ? chordModeSingle
     :                                         chordModeFingered).setToggleState(true, juce::dontSendNotification);

    auto pickMode = [this](ChordMode mode)
    {
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::chordMode, (int) mode); propertyFile->saveIfNeeded(); }
        if (onChordModeChanged) onChordModeChanged(mode);
    };
    chordModeFingered.onClick = [pickMode]{ pickMode(ChordMode::Fingered); };
    chordModeSingle.onClick   = [pickMode]{ pickMode(ChordMode::SingleFinger); };
    chordModeFull.onClick     = [pickMode]{ pickMode(ChordMode::FullKeyboard); };

    settingsPanel.addAndMakeVisible(chordMemoryToggle);
    chordMemoryToggle.setToggleState(propertyFile == nullptr || propertyFile->getBoolValue(SettingsFileKeys::chordMemory, true),
                                     juce::dontSendNotification);
    chordMemoryToggle.onClick = [this]()
    {
        const bool on = chordMemoryToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::chordMemory, on); propertyFile->saveIfNeeded(); }
        if (onChordMemoryChanged) onChordMemoryChanged(on);
    };

    settingsPanel.addAndMakeVisible(synchroStartToggle);
    synchroStartToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue(SettingsFileKeys::synchroStart, false),
                                      juce::dontSendNotification);
    synchroStartToggle.onClick = [this]()
    {
        const bool on = synchroStartToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::synchroStart, on); propertyFile->saveIfNeeded(); }
        if (onSynchroStartChanged) onSynchroStartChanged(on);
    };

    settingsPanel.addAndMakeVisible(countInToggle);
    countInToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue(SettingsFileKeys::countIn, false),
                                 juce::dontSendNotification);
    countInToggle.onClick = [this]()
    {
        const bool on = countInToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::countIn, on); propertyFile->saveIfNeeded(); }
        if (onCountInChanged) onCountInChanged(on);
    };

    settingsPanel.addAndMakeVisible(chordZoneMuteToggle);
    chordZoneMuteToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue(SettingsFileKeys::chordZoneMute, false),
                                       juce::dontSendNotification);
    chordZoneMuteToggle.onClick = [this]()
    {
        const bool on = chordZoneMuteToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::chordZoneMute, on); propertyFile->saveIfNeeded(); }
        if (onChordZoneMuteChanged) onChordZoneMuteChanged(on);
    };

    // Phase 7a: master Chord-Transposition (NTT) on/off. Default ON.
    settingsPanel.addAndMakeVisible(nttEnabledToggle);
    nttEnabledToggle.setToggleState(propertyFile == nullptr || propertyFile->getBoolValue(SettingsFileKeys::nttEnabled, true),
                                    juce::dontSendNotification);
    nttEnabledToggle.onClick = [this]()
    {
        const bool on = nttEnabledToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::nttEnabled, on); propertyFile->saveIfNeeded(); }
        if (onNttEnabledChanged) onNttEnabledChanged(on);
    };

    // Phase 7a: curated minor-scale selector for NTT passing-tone snap (Dorian default).
    nttMinorScaleLabel.setText(Text::minorScale, juce::dontSendNotification);
    settingsPanel.addAndMakeVisible(nttMinorScaleLabel);

    const int kNttMinorRadio = 0x77017A;   // unique non-zero radio group id
    int minorIdx {};
    for (auto* button : { &nttMinorDorian, &nttMinorAeolian, &nttMinorHarmonic })
    {
        button->setClickingTogglesState(true);
        button->setRadioGroupId(kNttMinorRadio);
        button->setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        button->setColour(juce::TextButton::buttonColourId,   cardBg);
        button->setColour(juce::TextButton::buttonOnColourId, accent1);
        button->setColour(juce::TextButton::textColourOffId,  subtitleText);
        button->setColour(juce::TextButton::textColourOnId,   background);
        const int idx = minorIdx++;
        button->onClick = [this, idx]()
        {
            if (propertyFile != nullptr) { propertyFile->setValue(SettingsFileKeys::nttMinorScale, idx); propertyFile->saveIfNeeded(); }
            if (onNttMinorScaleChanged) onNttMinorScaleChanged(idx);
        };
        settingsPanel.addAndMakeVisible(*button);
    }
    const int storedMinor = propertyFile != nullptr ? juce::jlimit(MinorScaleButtons::dorian, MinorScaleButtons::harmonic, propertyFile->getIntValue(SettingsFileKeys::nttMinorScale, MinorScaleButtons::dorian)) : MinorScaleButtons::dorian;
    (storedMinor == MinorScaleButtons::aeolian ? nttMinorAeolian : storedMinor == MinorScaleButtons::harmonic ? nttMinorHarmonic : nttMinorDorian)
        .setToggleState(true, juce::dontSendNotification);

	sfzButtonInit();
}

void MIDIWindow::populateCBIN()
{
    this->MIDIDevice.getAvailableDevicesMidiIN(this->devicesListIN);
    this->comboBoxDevicesIN.clear();
    int id = firstItemId;
    for (const auto& device : devicesListIN) {
        this->comboBoxDevicesIN.addItem(juce::String(device), id);
        id++;
    }
    comboBoxDevicesIN.setSelectedId(firstItemId, juce::dontSendNotification);
}

void MIDIWindow::populateCBOUT()
{
    this->MIDIDevice.getAvailableDevicesMidiOUT(this->devicesListOUT);
    this->comboBoxDevicesOUT.clear();
    int id = firstItemId;
    for (const auto& device : devicesListOUT) {
        this->comboBoxDevicesOUT.addItem(juce::String(device), id);
        id++;
    }
    comboBoxDevicesOUT.setSelectedId(firstItemId, juce::dontSendNotification);
}

void MIDIWindow::populateCBengine()
{
    this->comboBoxOutputEngineType.clear();
    int id = firstItemId;
    for (const auto& engine : { Text::externalMidi, Text::internalSfz })
    {
        this->comboBoxOutputEngineType.addItem(juce::String(engine), id);
        id++;
    }
    comboBoxOutputEngineType.setSelectedId(firstItemId, juce::dontSendNotification);
}

void MIDIWindow::populateCBaudioOUT()
{
    MIDIDevice.getAvailableAudioDevicesOUT(this->devicesListAudioOUT);

    comboBoxAudioDevicesOUT.clear();

    int id = firstItemId;
    for (const auto& device : devicesListAudioOUT)
    {
        comboBoxAudioDevicesOUT.addItem(juce::String(device), id);
		id++;
    }

    comboBoxAudioDevicesOUT.setSelectedId(firstItemId, juce::dontSendNotification);
}


void MIDIWindow::restoreCBoxes()
{
    if(lastIndexIN <= comboBoxDevicesIN.getNumItems())
        comboBoxDevicesIN.setSelectedId(lastIndexIN, juce::sendNotificationSync);
    if(lastIndexOUT <= comboBoxDevicesOUT.getNumItems())
        comboBoxDevicesOUT.setSelectedId(lastIndexOUT, juce::sendNotificationSync);
    if(lastIndexAudioOut <= comboBoxAudioDevicesOUT.getNumItems())
		comboBoxAudioDevicesOUT.setSelectedId(lastIndexAudioOut, juce::sendNotificationSync);
    if(lastIndexEngineOption <= comboBoxOutputEngineType.getNumItems())
        comboBoxOutputEngineType.setSelectedId(lastIndexEngineOption, juce::sendNotificationSync);
}

void MIDIWindow::setStyleComboBox(juce::ComboBox& cb)
{
    cb.setColour(juce::ComboBox::backgroundColourId, cardBg);
    cb.setColour(juce::ComboBox::outlineColourId, separator);
    cb.setColour(juce::ComboBox::textColourId, titleText);
    cb.setColour(juce::ComboBox::arrowColourId, accent1);
    cb.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void MIDIWindow::setStyleLabel(juce::Label& label)
{
    label.setFont(juce::FontOptions(labelFontHeight));
    label.setColour(juce::Label::textColourId, accent1);
}

void MIDIWindow::switchBetweenEngineOptions(int selectedId)
{
    if (selectedId == OutputEngineIds::externalMidi)
    {
		toggleSettingsOutMIDICB();
		toggleSettingsAudioOutCB();
        toggleSFZbutton();
    }
    else if(selectedId == OutputEngineIds::internalSfz)
    {
        toggleSettingsOutMIDICB();
        toggleSettingsAudioOutCB();
        toggleSFZbutton();
	}
}
