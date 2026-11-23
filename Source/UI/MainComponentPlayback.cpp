#include "MainComponent.h"
#include "AppInfo.h"

namespace
{
    // Short codes of the instrument pictures in BinaryData.
    namespace ImageCodes
    {
        const std::string acousticGrandPiano = "AGP";
        const std::string electricPiano = "EP";
        const std::string acousticBass = "ACB";
        const std::string electricBassFinger = "EBF";
        const std::string electricBassPick = "EBP";
        const std::string fretlessBass = "FTB";
        const std::string slapBass1 = "SLB1";
        const std::string slapBass2 = "SLB2";
        const std::string synthBass1 = "SYB1";
        const std::string synthBass2 = "SYB2";
    }

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


juce::Image MainComponent::getImageForInstruments(const std::string& type)
{
    const void* binaryData=nullptr;
    size_t binaryDataSize {};
    if (type == ImageCodes::acousticGrandPiano)
    {
        binaryData = BinaryData::AcousticGrandPiano_png;
        binaryDataSize = BinaryData::AcousticGrandPiano_pngSize;
    }
    else if (type == ImageCodes::electricPiano)
    {
        binaryData = BinaryData::ElectricPiano_png;
        binaryDataSize = BinaryData::ElectricPiano_pngSize;
    }
    else if (type == ImageCodes::acousticBass)
    {
        binaryData = BinaryData::AcousticBass_png;
        binaryDataSize = BinaryData::AcousticBass_pngSize;
    }
    else if (type == ImageCodes::electricBassFinger)
    {
        binaryData = BinaryData::ElectricBass_png;
        binaryDataSize = BinaryData::ElectricBass_pngSize;
    }
    else if (type == ImageCodes::electricBassPick)
    {
        binaryData = BinaryData::ElectricBass2_png;
        binaryDataSize = BinaryData::ElectricBass2_pngSize;
    }
    else if (type == ImageCodes::fretlessBass)
    {
        binaryData = BinaryData::FretlessBass_png;
        binaryDataSize = BinaryData::FretlessBass_pngSize;
    }
    else if (type == ImageCodes::slapBass1)
    {
        binaryData = BinaryData::SlapBass_png;
        binaryDataSize = BinaryData::SlapBass_pngSize;
    }
    else if (type == ImageCodes::slapBass2)
    {
        binaryData = BinaryData::SlapBass2_png;
        binaryDataSize = BinaryData::SlapBass2_pngSize;
    }
    else if (type == ImageCodes::synthBass1)
    {
        binaryData = BinaryData::SynthBass_png;
        binaryDataSize = BinaryData::SynthBass_pngSize;
    }
    else if (type == ImageCodes::synthBass2)
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
    struct InstrumentEntry { std::string imageCode; juce::String name; int program; };
    struct InstrumentGroup { juce::String name; std::vector<InstrumentEntry> instruments; };

    // The instrument picker: groups in order, each instrument with its General MIDI program (0-based).
    static const std::vector<InstrumentGroup> instrumentGroups = {
        { InstrumentGroupNames::pianos, {
            { ImageCodes::acousticGrandPiano, "Acoustic Grand Piano", 0 },
            { ImageCodes::electricPiano, "Electric Piano", 4 } } },
        { InstrumentGroupNames::basses, {
            { ImageCodes::acousticBass, "Acoustic Bass", 32 },
            { ImageCodes::electricBassFinger, "Electric Bass (finger)", 33 },
            { ImageCodes::electricBassPick, "Electric Bass (pick)", 34 },
            { ImageCodes::fretlessBass, "Fretless Bass", 35 },
            { ImageCodes::slapBass1, "Slap Bass 1", 36 },
            { ImageCodes::slapBass2, "Slap Bass 2", 37 },
            { ImageCodes::synthBass1, "Synth Bass 1", 38 },
            { ImageCodes::synthBass2, "Synth Bass 2", 39 } } },
        { InstrumentGroupNames::guitars, {
            { ImageCodes::acousticBass, "Nylon Acoustic Guitar", 24 },
            { ImageCodes::acousticBass, "Steel Acoustic Guitar", 25 },
            { ImageCodes::acousticBass, "Jazz Electric Guitar", 26 },
            { ImageCodes::acousticBass, "Clean Electric Guitar", 27 } } },
        { InstrumentGroupNames::woodWinds, {
            { ImageCodes::acousticBass, "Flute", 73 },
            { ImageCodes::acousticBass, "Clarinet", 71 },
            { ImageCodes::acousticBass, "Oboe", 68 },
            { ImageCodes::acousticBass, "Bassoon", 70 } } },
        { InstrumentGroupNames::brass, {
            { ImageCodes::acousticBass, "Trumpet", 56 },
            { ImageCodes::acousticBass, "Trombone", 57 },
            { ImageCodes::acousticBass, "French Horn", 59 } } },
        { InstrumentGroupNames::strings, {
            { ImageCodes::acousticBass, "Violin", 40 },
            { ImageCodes::acousticBass, "Viola", 41 },
            { ImageCodes::acousticBass, "Cello", 42 },
            { ImageCodes::acousticBass, "Contrabass", 43 } } },
        { InstrumentGroupNames::reeds, {
            { ImageCodes::acousticBass, "Accordion", 21 },
            { ImageCodes::acousticBass, "Harmonica", 22 },
            { ImageCodes::acousticBass, "Bandoneon", 23 } } },
        { InstrumentGroupNames::organs, {
            { ImageCodes::acousticBass, "Hammond Organ", 16 },
            { ImageCodes::acousticBass, "Percussive Organ", 17 },
            { ImageCodes::acousticBass, "Rock Organ", 18 },
            { ImageCodes::acousticBass, "Church Organ", 19 },
            { ImageCodes::acousticBass, "Reed Organ", 20 } } }
    };

    auto dummyRoot = std::make_unique< InstrumentTreeItem>(InstrumentGroupNames::root);

    for (const auto& group : instrumentGroups)
    {
        auto groupItem = std::make_unique<InstrumentTreeItem>(group.name);
        for (const auto& instrument : group.instruments)
            groupItem->addSubItem(createInstrumentItem(getImageForInstruments(instrument.imageCode), instrument.name, instrument.program));
        groupItem->setOpen(false);
        dummyRoot->addSubItem(groupItem.release());
    }

    dummyRoot->setOpen(true);
    treeView->setRootItem(dummyRoot.release());
    
}

InstrumentTreeItem* MainComponent::createInstrumentItem(const juce::Image& img, const juce::String& name, int program)
{
    auto item = std::make_unique<InstrumentTreeItem>(img,name,program);
    item->onProgramSelected = [&](int programNumber, juce::String name="")
    {
        auto styleId = display ? display->getStyleID() : StyleNames::defaultStyleId;

        if (leftHandInstrumentToggle.getToggleState() == true)
        {
            midiHandler.setProgramNumber(programNumber, HandChoiceNames::left); //setting the actual sound to the new program for both midiHandler and also the recordPlayer by listener
            if (propertiesFile)
            {
                propertiesFile->setValue(styleId + SettingsFileKeys::leftInstrumentSuffix, programNumber);
                propertiesFile->saveIfNeeded();
            }
            if (audioHandler != nullptr)
            {
                auto sfzFile = sfzManager.getSfzForStyleInstrument(styleId, programNumber);
                audioHandler->loadSfz(sfzFile, HandChannels::leftHand);
            }
        }

        if (rightHandInstrumentToggle.getToggleState() == true)
        {
            midiHandler.setProgramNumber(programNumber, HandChoiceNames::right);
            if (propertiesFile)
            {
                propertiesFile->setValue(styleId + SettingsFileKeys::rightInstrumentSuffix, programNumber);
                propertiesFile->saveIfNeeded();
            }
            if (audioHandler != nullptr)
            {
                auto sfzFile = sfzManager.getSfzForStyleInstrument(styleId, programNumber);
                audioHandler->loadSfz(sfzFile, HandChannels::rightHand);
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

void MainComponent::showInstrumentSelector()
{
    if (treeView == nullptr)
    {
        treeView = std::make_unique<juce::TreeView>();
        treeView->setSize(pickerWidth, pickerHeight);
    }
    if (treeView->getRootItem() != nullptr)
    {
        treeView->deleteRootItem();
        //DBG("deleting root from show");
    }
    //else DBG("not deleting root from show");
    buildTree();
    treeView->setColour(juce::TreeView::backgroundColourId, juce::Colours::white);
    auto area1 = juce::Rectangle<int>(calloutLeft, calloutTop, pickerWidth, headerPanel.getHeight() + headerPanel.getY());
    
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
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, tempo](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
            {
                juce::String errorMsg;
                if (!recordPlayer.parseRecordingFromFile(file, errorMsg))
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::playFailedTitle, errorMsg);
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
