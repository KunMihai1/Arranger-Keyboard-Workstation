#include "MainComponent.h"
#include "AppInfo.h"

namespace
{
    namespace Text
    {
        const juce::String preparingStyle    = "Preparing style...";
        const juce::String preparingSession  = "Preparing session...";
        const juce::String andSeparator      = " and ";
        const juce::String listSeparator     = ", ";
        const juce::String moreSuffix        = " more";
        const juce::String noSfzLoaded       = "No SFZ loaded for ";
        const juce::String inStyleStart      = " in style \"";
        const juce::String inStyleEnd        = "\"";
        const juce::String roleStart         = " (";
        const juce::String roleEnd           = ")";
        const juce::String leftHandRole      = "left hand";
        const juce::String rightHandRole     = "right hand";
        const juce::String styleTrackRole    = "style track";
        const juce::String channelPrefix     = "channel ";
        const juce::String errorTitle        = "ERROR";
        const juce::String okButton          = "OK";
        const juce::String noInputDevices    = "There are no available input devices!";
        const juce::String noOutputDevices   = "There are no available output devices!";
        const juce::String inputFailed       = "Failed to open input device.";
        const juce::String outputFailed      = "Failed to open output device.";
        const juce::String audioOutputFailed = "Failed to open audio output device.";
        const juce::String externalOpened    = "EXTERNAL opened!";
        const juce::String internalOpened    = "INTERNAL OPENED!";
        const juce::String colourPicker      = "Colour Picker";
        const juce::String nothingToSave     = "No recorded events to play.";
        const juce::String saveFileTitle     = "Save file";
        const juce::String midiFilePattern   = "*.mid";
        const juce::String saveFailedTitle   = "Save Failed";
        const juce::String savedTitle        = "Saved";
        const juce::String savedTo           = "Recording saved to:\n";
        const juce::String playFromFileTitle = "Play from file";
        const juce::String playFailedTitle   = "Play Failed";
    }

    constexpr int maskBit            = 1;    // bit n of the channel mask is channel n + 1
    constexpr int slotsListedByName  = 2;    // more than this and the popup says "and N more"
    constexpr int firstSlot          = 0;
    constexpr int secondSlot         = 1;
    constexpr int defaultProgram     = 0;
    constexpr int pickerWidth        = 500;
    constexpr int pickerHeight       = 300;
    constexpr int calloutLeft        = 0;
    constexpr int calloutTop         = 0;
    constexpr int tooFewEventsToSave = 1;
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
            setLoadingOverlayVisible(true, Text::preparingStyle);
        };
        audioHandler->onSfzLoadComplete = [this]() {
            setLoadingOverlayVisible(false);
            openingAudioLabel.setText(Text::preparingSession, juce::dontSendNotification);
        };
        audioHandler->onNoSfzForChannels = [this](int channelMask) {
            juce::StringArray slots;
            for (int ch = MidiFacts::lowestChannel; ch <= MidiFacts::highestChannel; ++ch)
                if (channelMask & (maskBit << (ch - MidiFacts::lowestChannel)))
                    slots.add(describeSfzSlot(ch));

            if (slots.isEmpty())
                return;

            // Keep the popup readable when a whole style is unmapped.
            juce::String list = slots.size() <= slotsListedByName
                ? slots.joinIntoString(Text::andSeparator)
                : slots[firstSlot] + Text::listSeparator + slots[secondSlot] + Text::andSeparator + juce::String(slots.size() - slotsListedByName) + Text::moreSuffix;

            const juce::String styleName = display != nullptr ? display->getStyleName() : juce::String();

            juce::String msg = Text::noSfzLoaded + list;
            if (styleName.isNotEmpty())
                msg += Text::inStyleStart + styleName + Text::inStyleEnd;

            showStageMessage(msg);
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
    int leftProg  = propertiesFile ? propertiesFile->getIntValue(styleId + SettingsFileKeys::leftInstrumentSuffix,  defaultProgram) : defaultProgram;
    int rightProg = propertiesFile ? propertiesFile->getIntValue(styleId + SettingsFileKeys::rightInstrumentSuffix, defaultProgram) : defaultProgram;
    audioHandler->loadSfz(sfzManager.getSfzForStyleInstrument(styleId, leftProg),  HandChannels::leftHand);
    audioHandler->loadSfz(sfzManager.getSfzForStyleInstrument(styleId, rightProg), HandChannels::rightHand);
    for (const auto& track : display->getTrackChannelInstruments())
        audioHandler->loadSfz(sfzManager.getSfzForStyleInstrument(styleId, track.instrument), track.channel);
}

juce::String MainComponent::describeSfzSlot(int midiChannel)
{
    auto named = [](int instrument, const juce::String& role)
    {
        return getGMInstrumentName(instrument) + Text::roleStart + role + Text::roleEnd;
    };

    if (midiChannel == HandChannels::leftHand)
        return named(midiHandler.getProgramNumberLeftHand(), Text::leftHandRole);

    if (midiChannel == HandChannels::rightHand)
        return named(midiHandler.getProgramNumberRightHand(), Text::rightHandRole);

    if (display != nullptr)
        for (const auto& track : display->getTrackChannelInstruments())
            if (track.channel == midiChannel)
                return named(track.instrument, Text::styleTrackRole);

    return Text::channelPrefix + juce::String(midiChannel);
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
        if (device == DeviceNames::noDevicesFound)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::errorTitle, Text::noInputDevices, Text::okButton);
            return false;
        }
    }
    for (const auto& device : devicesOUT)
    {
        if (device == DeviceNames::noDevicesFound)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::errorTitle, Text::noOutputDevices, Text::okButton);
            return false;
        }
    }



    int indexIN = this->MIDIDevice.getDeviceIndexIN();
    int indexOUT = this->MIDIDevice.getDeviceIndexOUT();
    bool result;

    if (this->MIDIDevice.getDeviceNameBasedOnIndex(indexIN, DeviceListChoice::midiInput) == DeviceNames::pcKeyboard)
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

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::errorTitle, Text::inputFailed, Text::okButton);
            return false;
        }
        this->deviceOpenedIN = this->MIDIDevice.getDeviceIN();
    }
    disconnectCurrentOutput();

    int engineOption = OutputEngineIds::externalMidi; // default to External MIDI
    if (propertiesFile)
        engineOption = propertiesFile->getIntValue(SettingsFileKeys::engineOptionIndex, OutputEngineIds::externalMidi);

    if (engineOption == OutputEngineIds::externalMidi)
    {
        DBG(Text::externalOpened);
        result = this->MIDIDevice.deviceOpenOUT(indexOUT);
        if (!result)
        {
            this->MIDIDevice.deviceCloseIN();  
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::errorTitle, Text::outputFailed, Text::okButton);
            return false;
        }
        this->deviceOpenedOUT = this->MIDIDevice.getDeviceOUT();
    }
    else if (engineOption == OutputEngineIds::internalSfz)
    {
        DBG(Text::internalOpened);
        int indexAudioOUT = this->MIDIDevice.getDeviceIndexAudioOUT();
        result = this->MIDIDevice.deviceOpenAudioOUT(indexAudioOUT);
        if (!result)
        {
            this->MIDIDevice.deviceCloseIN();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::errorTitle, Text::audioOutputFailed, Text::okButton);
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
    colourSelector->setName(Text::colourPicker);
    colourSelector->setColour(juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
    colourSelector->addChangeListener(this);
    colourSelector->setSize(pickerWidth, pickerHeight);
    auto area1 = juce::Rectangle<int>(calloutLeft, calloutTop, pickerWidth, headerPanel.getHeight()+headerPanel.getY());


    noteLayer->setVisible(false);
    noteLayer->resetState();
    //noteLayer->repaint();
    this->keyboard.setIsDrawn(false);
    //this->keyboard.repaint();


    repaint();


    juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::ColourSelector>(colourSelector),area1,this);
}

void MainComponent::saveRecordingToFile(double tempo)
{
    if (recordPlayer.getSizeRecorded() <= tooFewEventsToSave)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, MainComponentText::saveRecording, Text::nothingToSave);
        return;
    }

    fileChooser = std::make_unique<juce::FileChooser>(Text::saveFileTitle, juce::File::getCurrentWorkingDirectory(), Text::midiFilePattern);

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this,tempo](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                juce::String errorMsg;
                if (!recordPlayer.saveRecordingToFile(file, errorMsg,tempo))
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::saveFailedTitle, errorMsg);
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::savedTitle, Text::savedTo + file.getFullPathName());
                }
            }
            fileChooser.reset();
        });
}

void MainComponent::playRecordingFromFile(double tempo)
{
    fileChooser = std::make_unique<juce::FileChooser>(Text::playFromFileTitle, juce::File::getCurrentWorkingDirectory(), Text::midiFilePattern);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,   // pick a file to play: Open, not Save
        [this, tempo](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                juce::String errorMsg;
                if (!recordPlayer.parseRecordingFromFile(file, errorMsg))
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::playFailedTitle, errorMsg);
                else
                {
                    lastMidiFileName = file.getFileName();
                    recordPlayer.startRecordingFilePlaying();
                }
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
