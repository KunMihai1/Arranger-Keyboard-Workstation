#include "SettingsWindow.h"
#include "MidiHandler.h"
#include "AppColours.h"
#include "IOHelper.h"


using namespace AppColours;

//==============================================================================
//  Custom LookAndFeel for themed linear sliders
//==============================================================================
class MidiWindowSliderLnF : public juce::LookAndFeel_V4
{
public:
    MidiWindowSliderLnF()
    {
        setColour(juce::Slider::backgroundColourId,    knobTrack);
        setColour(juce::Slider::trackColourId,         accent1);
        setColour(juce::Slider::thumbColourId,         juce::Colours::white);
        setColour(juce::Slider::textBoxTextColourId,   titleText);
        setColour(juce::Slider::textBoxBackgroundColourId, cardBg);
        setColour(juce::Slider::textBoxOutlineColourId, separator);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        const float trackHeight = 6.0f;
        const float trackY = (float)y + ((float)height - trackHeight) * 0.5f;
        const float cornerSize = 3.0f;

        // Background track
        g.setColour(knobTrack);
        g.fillRoundedRectangle((float)x, trackY, (float)width, trackHeight, cornerSize);

        // Filled portion with gradient
        float filledWidth = sliderPos - (float)x;
        if (filledWidth > 0.0f)
        {
            juce::ColourGradient grad(accent1.darker(0.3f), (float)x, trackY,
                                      accent1, sliderPos, trackY, false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle((float)x, trackY, filledWidth, trackHeight, cornerSize);
        }

        // Thumb glow
        {
            const float glowR = 12.0f;
            juce::ColourGradient glow(accent1.withAlpha(0.3f), sliderPos, trackY + trackHeight * 0.5f,
                                     accent1.withAlpha(0.0f), sliderPos + glowR, trackY + trackHeight * 0.5f + glowR, true);
            g.setGradientFill(glow);
            g.fillEllipse(sliderPos - glowR, trackY + trackHeight * 0.5f - glowR, glowR * 2.0f, glowR * 2.0f);
        }

        // Thumb circle
        const float thumbR = 7.0f;
        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - thumbR, trackY + trackHeight * 0.5f - thumbR,
                      thumbR * 2.0f, thumbR * 2.0f);

        // thin white border on thumb
        g.setColour(accent1.withAlpha(0.5f));
        g.drawEllipse(sliderPos - thumbR, trackY + trackHeight * 0.5f - thumbR,
                      thumbR * 2.0f, thumbR * 2.0f, 1.5f);
    }
};

static MidiWindowSliderLnF& getSliderLnF()
{
    static MidiWindowSliderLnF instance;
    return instance;
}

MIDIWindow::MIDIWindow(MidiDevice& mdevice, std::vector<std::string>& devicesListIN, std::vector<std::string>& devicesListOUT
    , std::vector<std::string>& devicesListAudioOUT, juce::PropertiesFile* prop, SFZLibraryManager* sfzManagerPtr)
    : juce::DocumentWindow{ "MIDI Settings", background, DocumentWindow::closeButton },
    MIDIDevice{ mdevice }, devicesListIN{ devicesListIN }, devicesListOUT{ devicesListOUT }, devicesListAudioOUT{ devicesListAudioOUT }, propertyFile{ prop }, sfzManager{ sfzManagerPtr }
{
    setUsingNativeTitleBar(false);
    setResizable(false, false);
    setTitleBarTextCentred(true);
    setColour(juce::DocumentWindow::backgroundColourId, background);
    setColour(juce::DocumentWindow::textColourId, titleText);
    setBounds(200, 150, 480, 250);
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
        startTimer(750);
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
        int index = comboBoxDevicesIN.getSelectedId() - 1;
        if (index >= 0 && index < this->devicesListIN.size())
        {
            this->MIDIDevice.setDeviceIN(index);
            lastIndexIN = index + 1;
            if (propertyFile) { propertyFile->setValue("MidiInIndex", lastIndexIN); propertyFile->saveIfNeeded(); }
        }
    }
    else if (comboBoxThatHasChanged == &this->comboBoxDevicesOUT)
    {
        int index = comboBoxDevicesOUT.getSelectedId() - 1;
        if (index >= 0 && index < this->devicesListOUT.size())
        {
            this->MIDIDevice.setDeviceOUT(index);
            lastIndexOUT = index + 1;
            if (propertyFile) { propertyFile->setValue("MidiOutIndex", lastIndexOUT); propertyFile->saveIfNeeded(); }
        }
    }
    else if (comboBoxThatHasChanged == &this->comboBoxOutputEngineType)
    {
		lastIndexEngineOption = comboBoxOutputEngineType.getSelectedId();
        if (propertyFile) { propertyFile->setValue("EngineOptionIndex", lastIndexEngineOption); propertyFile->saveIfNeeded(); }
		switchBetweenEngineOptions(lastIndexEngineOption);
        if (onOutputEngineChanged && isMidiDeviceOpen && isMidiDeviceOpen())
            onOutputEngineChanged(lastIndexEngineOption);
    }
    else if(comboBoxThatHasChanged == &this->comboBoxAudioDevicesOUT)
    {
        int index = comboBoxAudioDevicesOUT.getSelectedId() - 1;
        if (index >= 0 && index < this->devicesListAudioOUT.size())
        {
            this->MIDIDevice.setDeviceAudioOUT(index);
			lastIndexAudioOut = index + 1;
            if (propertyFile) { propertyFile->setValue("AudioOutIndex", lastIndexAudioOut); propertyFile->saveIfNeeded(); }
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
    int y = pad;

    const int labelW = 80;
    const int cbX = 100;
    const int cbW = 260;


    outputEngineTypeLabel.setBounds(pad, y + 8, labelW, 20);
    comboBoxOutputEngineType.setBounds(cbX, y, 180, rowH);

    y += 55;


    midiDevicesLabelIN.setBounds(pad, y + 8, labelW, 20);
    comboBoxDevicesIN.setBounds(cbX, y, cbW, rowH);

    y += 46;

    midiDevicesLabelOUT.setBounds(pad, y + 8, labelW, 20);
    comboBoxDevicesOUT.setBounds(cbX, y, cbW, rowH);

	audioDevicesLabelOUT.setBounds(pad, y + 8, labelW, 20);
	comboBoxAudioDevicesOUT.setBounds(cbX, y, cbW, rowH);

    y += 55;

    const int buttonW = 140;
    const int buttonH = 34;
    const int buttonX = (settingsPanel.getWidth() - buttonW) / 2;

    sfzLibraryButton.setBounds(buttonX, y, buttonW, buttonH);

    y += buttonH + 14;
    arrangerModeToggle.setBounds(pad, y, 320, rowH);
    y += rowH + 6;
    chordBassInversionToggle.setBounds(pad, y, 320, rowH);
    y += rowH + 6;
    {
        chordModeLabel.setBounds(pad, y, 92, rowH);
        const int bx = pad + 96, bw = 104, gap = 6;
        chordModeFingered.setBounds(bx,                  y, bw, rowH);
        chordModeSingle.setBounds  (bx + (bw + gap),     y, bw, rowH);
        chordModeFull.setBounds    (bx + 2 * (bw + gap), y, bw, rowH);
    }
    y += rowH + 6;
    chordMemoryToggle.setBounds(pad, y, 320, rowH);
    y += rowH + 6;
    synchroStartToggle.setBounds(pad, y, 320, rowH);
    y += rowH + 6;
    countInToggle.setBounds(pad, y, 320, rowH);
    y += rowH + 6;
    chordZoneMuteToggle.setBounds(pad, y, 320, rowH);
    y += rowH + 6;
    nttEnabledToggle.setBounds(pad, y, 320, rowH);
    y += rowH + 6;
    {
        nttMinorScaleLabel.setBounds(pad, y, 92, rowH);
        const int bx = pad + 96, bw = 104, gap = 6;
        nttMinorDorian.setBounds  (bx,                  y, bw, rowH);
        nttMinorAeolian.setBounds (bx + (bw + gap),     y, bw, rowH);
        nttMinorHarmonic.setBounds(bx + 2 * (bw + gap), y, bw, rowH);
    }
}

void MIDIWindow::panelInit()
{
    settingsPanel.setSize(480, 630);   // tall enough for the arranger + Phase 4/5/6 chord rows + Phase 7a NTT rows

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

    midiDevicesLabelIN.setText("MIDI IN", juce::dontSendNotification);
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

    midiDevicesLabelOUT.setText("MIDI OUT", juce::dontSendNotification);
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

	outputEngineTypeLabel.setText("Output Engine", juce::dontSendNotification);
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

	audioDevicesLabelOUT.setText("Audio OUT", juce::dontSendNotification);
	setStyleLabel(audioDevicesLabelOUT);
	settingsPanel.addAndMakeVisible(this->audioDevicesLabelOUT);
	audioDevicesLabelOUT.setVisible(false);
}

void MIDIWindow::sfzButtonInit()
{
	sfzLibraryButton.setButtonText("SFZ Library");
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
            return getCurrentStyleName ? getCurrentStyleName() : juce::String("DEFAULT Style");
        };
        sfzLibraryUI->onLibraryChanged = [this]()
        {
            sfzManager->save(IOHelper::getFile("SFZLibrary.json"));
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
                sfzLibraryUI->setBounds(settingsPanel.getLocalBounds().reduced(20));
                sfzLibraryUI->toFront(true);
            }
        }
	};
}

void MIDIWindow::allInit()
{
    if (propertyFile != nullptr)
    {
        lastIndexIN = propertyFile->getIntValue("MidiInIndex", 1);
        lastIndexOUT = propertyFile->getIntValue("MidiOutIndex", 1);
        lastIndexEngineOption = propertyFile->getIntValue("EngineOptionIndex", 1);
        lastIndexAudioOut = propertyFile->getIntValue("AudioOutIndex", 1);
    }

    panelInit();
    devicesCBinit();
    outputEngineCBinit();

    settingsPanel.addAndMakeVisible(arrangerModeToggle);
    arrangerModeToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue("ArrangerModeEnabled", false),
                                      juce::dontSendNotification);
    arrangerModeToggle.onClick = [this]()
    {
        const bool on = arrangerModeToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue("ArrangerModeEnabled", on); propertyFile->saveIfNeeded(); }
        if (onArrangerModeChanged) onArrangerModeChanged(on);
    };

    // Phase 4 arranger chord toggles. Defaults: bass inversion off, split scan, memory on.
    // Each onClick captures `this` and references its own member directly (a previous helper captured
    // a reference parameter that dangled, so the callback never fired).
    settingsPanel.addAndMakeVisible(chordBassInversionToggle);
    chordBassInversionToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue("ChordBassInversion", false),
                                            juce::dontSendNotification);
    chordBassInversionToggle.onClick = [this]()
    {
        const bool on = chordBassInversionToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue("ChordBassInversion", on); propertyFile->saveIfNeeded(); }
        if (onChordBassInversionChanged) onChordBassInversionChanged(on);
    };

    // Phase 5: 3-way chord-recognition mode selector (replaces the old Full-Keyboard on/off toggle).
    auto readStoredMode = [this]() -> ChordMode
    {
        if (propertyFile == nullptr) return ChordMode::Fingered;
        if (propertyFile->containsKey("ChordMode"))
            return (ChordMode) juce::jlimit(0, 2, propertyFile->getIntValue("ChordMode", 0));
        // migrate legacy bool: Full-Keyboard on -> FullKeyboard, else Fingered
        return propertyFile->getBoolValue("ChordFullKeyboard", false) ? ChordMode::FullKeyboard
                                                                      : ChordMode::Fingered;
    };

    chordModeLabel.setText("Chord Mode", juce::dontSendNotification);
    settingsPanel.addAndMakeVisible(chordModeLabel);

    const int kChordModeRadio = 0x5C40DE;   // unique non-zero radio group id
    for (auto* b : { &chordModeFingered, &chordModeSingle, &chordModeFull })
    {
        b->setClickingTogglesState(true);
        b->setRadioGroupId(kChordModeRadio);
        b->setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        // Selected = warm-orange fill with dark text; unselected = dark card with muted text, so the
        // active mode reads at a glance (the default TextButton on/off states were near-identical).
        b->setColour(juce::TextButton::buttonColourId,   cardBg);
        b->setColour(juce::TextButton::buttonOnColourId, accent1);
        b->setColour(juce::TextButton::textColourOffId,  subtitleText);
        b->setColour(juce::TextButton::textColourOnId,   background);
        settingsPanel.addAndMakeVisible(*b);
    }

    const ChordMode storedMode = readStoredMode();
    (storedMode == ChordMode::FullKeyboard   ? chordModeFull
     : storedMode == ChordMode::SingleFinger ? chordModeSingle
     :                                         chordModeFingered).setToggleState(true, juce::dontSendNotification);

    auto pickMode = [this](ChordMode m)
    {
        if (propertyFile != nullptr) { propertyFile->setValue("ChordMode", (int) m); propertyFile->saveIfNeeded(); }
        if (onChordModeChanged) onChordModeChanged(m);
    };
    chordModeFingered.onClick = [pickMode]{ pickMode(ChordMode::Fingered); };
    chordModeSingle.onClick   = [pickMode]{ pickMode(ChordMode::SingleFinger); };
    chordModeFull.onClick     = [pickMode]{ pickMode(ChordMode::FullKeyboard); };

    settingsPanel.addAndMakeVisible(chordMemoryToggle);
    chordMemoryToggle.setToggleState(propertyFile == nullptr || propertyFile->getBoolValue("ChordMemory", true),
                                     juce::dontSendNotification);
    chordMemoryToggle.onClick = [this]()
    {
        const bool on = chordMemoryToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue("ChordMemory", on); propertyFile->saveIfNeeded(); }
        if (onChordMemoryChanged) onChordMemoryChanged(on);
    };

    settingsPanel.addAndMakeVisible(synchroStartToggle);
    synchroStartToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue("SynchroStart", false),
                                      juce::dontSendNotification);
    synchroStartToggle.onClick = [this]()
    {
        const bool on = synchroStartToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue("SynchroStart", on); propertyFile->saveIfNeeded(); }
        if (onSynchroStartChanged) onSynchroStartChanged(on);
    };

    settingsPanel.addAndMakeVisible(countInToggle);
    countInToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue("CountIn", false),
                                 juce::dontSendNotification);
    countInToggle.onClick = [this]()
    {
        const bool on = countInToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue("CountIn", on); propertyFile->saveIfNeeded(); }
        if (onCountInChanged) onCountInChanged(on);
    };

    settingsPanel.addAndMakeVisible(chordZoneMuteToggle);
    chordZoneMuteToggle.setToggleState(propertyFile != nullptr && propertyFile->getBoolValue("ChordZoneMute", false),
                                       juce::dontSendNotification);
    chordZoneMuteToggle.onClick = [this]()
    {
        const bool on = chordZoneMuteToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue("ChordZoneMute", on); propertyFile->saveIfNeeded(); }
        if (onChordZoneMuteChanged) onChordZoneMuteChanged(on);
    };

    // Phase 7a: master Chord-Transposition (NTT) on/off. Default ON.
    settingsPanel.addAndMakeVisible(nttEnabledToggle);
    nttEnabledToggle.setToggleState(propertyFile == nullptr || propertyFile->getBoolValue("NttEnabled", true),
                                    juce::dontSendNotification);
    nttEnabledToggle.onClick = [this]()
    {
        const bool on = nttEnabledToggle.getToggleState();
        if (propertyFile != nullptr) { propertyFile->setValue("NttEnabled", on); propertyFile->saveIfNeeded(); }
        if (onNttEnabledChanged) onNttEnabledChanged(on);
    };

    // Phase 7a: curated minor-scale selector for NTT passing-tone snap (Dorian default).
    nttMinorScaleLabel.setText("Minor scale", juce::dontSendNotification);
    settingsPanel.addAndMakeVisible(nttMinorScaleLabel);

    const int kNttMinorRadio = 0x77017A;   // unique non-zero radio group id
    int minorIdx = 0;
    for (auto* b : { &nttMinorDorian, &nttMinorAeolian, &nttMinorHarmonic })
    {
        b->setClickingTogglesState(true);
        b->setRadioGroupId(kNttMinorRadio);
        b->setMouseClickGrabsKeyboardFocus(false);   // keep the PC keyboard playable
        b->setColour(juce::TextButton::buttonColourId,   cardBg);
        b->setColour(juce::TextButton::buttonOnColourId, accent1);
        b->setColour(juce::TextButton::textColourOffId,  subtitleText);
        b->setColour(juce::TextButton::textColourOnId,   background);
        const int idx = minorIdx++;
        b->onClick = [this, idx]()
        {
            if (propertyFile != nullptr) { propertyFile->setValue("NttMinorScale", idx); propertyFile->saveIfNeeded(); }
            if (onNttMinorScaleChanged) onNttMinorScaleChanged(idx);
        };
        settingsPanel.addAndMakeVisible(*b);
    }
    const int storedMinor = propertyFile != nullptr ? juce::jlimit(0, 2, propertyFile->getIntValue("NttMinorScale", 0)) : 0;
    (storedMinor == 1 ? nttMinorAeolian : storedMinor == 2 ? nttMinorHarmonic : nttMinorDorian)
        .setToggleState(true, juce::dontSendNotification);

	sfzButtonInit();
}

void MIDIWindow::populateCBIN()
{
    this->MIDIDevice.getAvailableDevicesMidiIN(this->devicesListIN);
    this->comboBoxDevicesIN.clear();
    int id = 1;
    for (const auto& device : devicesListIN) {
        this->comboBoxDevicesIN.addItem(juce::String(device), id);
        id++;
    }
    comboBoxDevicesIN.setSelectedId(1, juce::dontSendNotification);
}

void MIDIWindow::populateCBOUT()
{
    this->MIDIDevice.getAvailableDevicesMidiOUT(this->devicesListOUT);
    this->comboBoxDevicesOUT.clear();
    int id = 1;
    for (const auto& device : devicesListOUT) {
        this->comboBoxDevicesOUT.addItem(juce::String(device), id);
        id++;
    }
    comboBoxDevicesOUT.setSelectedId(1, juce::dontSendNotification);
}

void MIDIWindow::populateCBengine()
{
    this->comboBoxOutputEngineType.clear();
    int id = 1;
    for (const auto& engine : { "External MIDI", "Internal SFZ Synth" })
    {
        this->comboBoxOutputEngineType.addItem(juce::String(engine), id);
        id++;
    }
    comboBoxOutputEngineType.setSelectedId(1, juce::dontSendNotification);
}

void MIDIWindow::populateCBaudioOUT()
{
    MIDIDevice.getAvailableAudioDevicesOUT(this->devicesListAudioOUT);

    comboBoxAudioDevicesOUT.clear();

    int id = 1;
    for (const auto& device : devicesListAudioOUT)
    {
        comboBoxAudioDevicesOUT.addItem(juce::String(device), id);
		id++;
    }

    comboBoxAudioDevicesOUT.setSelectedId(1, juce::dontSendNotification);
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
    label.setFont(juce::FontOptions(13.0f));
    label.setColour(juce::Label::textColourId, accent1);
}

void MIDIWindow::switchBetweenEngineOptions(int selectedId)
{
    if (selectedId == 1)
    {
		toggleSettingsOutMIDICB();
		toggleSettingsAudioOutCB();
        toggleSFZbutton();
    }
    else if(selectedId == 2)
    {
        toggleSettingsOutMIDICB();
        toggleSettingsAudioOutCB();
        toggleSFZbutton();
	}
}
