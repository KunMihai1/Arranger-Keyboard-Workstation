#include "MainComponent.h"
#include "AppInfo.h"


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
            juce::StringArray slots;
            for (int ch = 1; ch <= 16; ++ch)
                if (channelMask & (1 << (ch - 1)))
                    slots.add(describeSfzSlot(ch));

            if (slots.isEmpty())
                return;

            // Keep the popup readable when a whole style is unmapped.
            juce::String list = slots.size() <= 2
                ? slots.joinIntoString(" and ")
                : slots[0] + ", " + slots[1] + " and " + juce::String(slots.size() - 2) + " more";

            const juce::String styleName = display != nullptr ? display->getStyleName() : juce::String();

            juce::String msg = "No SFZ loaded for " + list;
            if (styleName.isNotEmpty())
                msg += " in style \"" + styleName + "\"";

            if (temporaryPopup) { temporaryPopup->updateText(msg); temporaryPopup->restartTimer(); }
            else {
                temporaryPopup = std::make_unique<TemporaryMessage>(msg);
                headerPanel.addChildComponent(temporaryPopup.get());
                temporaryPopup->setFinishedCallBack([this] { temporaryPopup.reset(); });
                temporaryPopup->setVisible(true);
            }
            // Set on both paths: a popup left over from a shorter message is too narrow for this one.
            const int popupWidth = juce::jmin(getWidth() - 20, 460);
            temporaryPopup->setBounds(getWidth() / 2 - popupWidth / 2, 10, popupWidth, 48);
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

juce::String MainComponent::describeSfzSlot(int midiChannel)
{
    auto named = [](int instrument, const juce::String& role)
    {
        return getGMInstrumentName(instrument) + " (" + role + ")";
    };

    if (midiChannel == 1)
        return named(midiHandler.getProgramNumberLeftHand(), "left hand");

    if (midiChannel == 16)
        return named(midiHandler.getProgramNumberRightHand(), "right hand");

    if (display != nullptr)
        for (const auto& t : display->getTrackChannelInstruments())
            if (t.channel == midiChannel)
                return named(t.instrument, "style track");

    return "channel " + juce::String(midiChannel);
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
