#include "MainComponent.h"
#include "AppInfo.h"

namespace
{
    namespace Text
    {
        const juce::String preparingSession = "Preparing session...";
    }

    constexpr int deviceCheckIntervalMs  = 1000;   // backstop poll for unplugged MIDI devices
    constexpr int focusLossCheckDelayMs  = 150;
    constexpr int firstItemId            = 1;
    constexpr int semitonesPerOctave     = 12;
    constexpr int octaveDownKey          = 'Z';    // PC keyboard: shift the playable range an octave
    constexpr int octaveUpKey            = 'X';
    constexpr int lowestShiftableStart   = 24;     // the range may move down while it starts at C1 or above
    constexpr int highestShiftableFinish = 101;    // and up while it ends at F7 or below

    namespace Layout
    {
        constexpr int   origin                 = 0;
        constexpr int   taskbarAllowance       = 40;
        constexpr float loadingFontHeight      = 24.0f;
        constexpr float loadingLabelAlpha      = 0.7f;
        constexpr float loadingDimAlpha        = 0.75f;
        constexpr int   loadingTextPadding     = 20;
        constexpr int   singleLine             = 1;
        constexpr float defaultFontHeight      = 16.0f;
        constexpr float half                   = 0.5f;
        constexpr double centre                = 0.5;
        constexpr int   centringDivisor        = 2;
        constexpr int   playButtonWidthDivisor  = 16;
        constexpr int   playButtonHeightDivisor = 24;
        constexpr double playButtonLeftOfCentre = 0.175;   // fractions of the window, matching the artwork
        constexpr double playButtonBelowCentre  = 0.162;
        constexpr int   settingsWidth          = 250;
        constexpr int   settingsButtonHeight   = 70;
        constexpr int   panelButtonWidth       = 200;
        constexpr int   panelButtonHeight      = 50;
        constexpr int   smallGap               = 5;
        constexpr int   headerMargin           = 10;
        constexpr int   toggleHeight           = 25;
        constexpr int   helpIconX              = 210;
        constexpr int   helpIconY              = 20;
        constexpr int   helpIconHeight         = 20;
        constexpr int   headerHeight           = 100;
        constexpr int   homeButtonWidth        = 75;
        constexpr int   headerButtonWidth      = 100;
        constexpr int   headerButtonHeight     = 30;
        constexpr int   colourButtonX          = 90;
        constexpr int   instrumentButtonX      = 195;
        constexpr int   transportButtonSize    = 30;
        constexpr int   transportRightMargin   = 35;
        constexpr int   saveButtonWidth        = 140;
        constexpr int   playRecordingX         = 305;
        constexpr int   loginPanelSize         = 400;
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
    stagePanelInit();

    setBounds(Layout::origin, Layout::origin, juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea.getWidth(),
        juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea.getHeight() - Layout::taskbarAllowance);
    this->addKeyListener(&this->keyListener);
    addKeyListener(this);

    client = std::make_shared<SupabaseClient>();

    loginWindowInitialize();

    openingAudioLabel.setText(Text::preparingSession, juce::dontSendNotification);
    openingAudioLabel.setJustificationType(juce::Justification::centred);
    openingAudioLabel.setFont(juce::Font(Layout::loadingFontHeight, juce::Font::bold));
    openingAudioLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    openingAudioLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(Layout::loadingLabelAlpha));
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

        // A hardware keyboard calls this on the MIDI input thread: the screen is updated on the
        // message thread (it is UI, and its text is read while painting).
        const juce::Component::SafePointer<MainComponent> safeThis(this);
        juce::MessageManager::callAsync([safeThis, chord]
        {
            if (safeThis == nullptr)
                return;
            safeThis->stageChord = chord;   // shown on the Stage screen
            safeThis->updateStageScreen();
        });
    };

    juce::Desktop::getInstance().addFocusChangeListener(this);
    tooltipWindow.setLookAndFeel(&tooltipLook);

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


    sfzManager.load(IOHelper::getFile(AppDataNames::sfzLibraryFile));
    if (sfzLibraryUI)
        sfzLibraryUI->refresh();   // the Voices page's library list (created with the pages, before the load)

    startTimer(deviceCheckIntervalMs);

    // Catch device hot-unplug the instant it happens (the 1s timer above is only a backstop). The
    // callback runs on the message thread (AsyncUpdater), so it can touch the UI directly.
    midiDeviceListConnection = juce::MidiDeviceListConnection::make ([this] { checkMidiDevicesValid(); });

}

MainComponent::~MainComponent()
{
    sfzManager.save(IOHelper::getFile(AppDataNames::sfzLibraryFile));

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
            auto imageX = (bounds.getWidth() - newWidth) * Layout::half;
            auto imageY = (bounds.getHeight() - newHeight) * Layout::half;
            g.setColour(juce::Colours::grey);
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            g.drawImage(currentBackground, juce::Rectangle<float>(imageX, imageY, newWidth, newHeight));
        g.setFont(juce::FontOptions(Layout::defaultFontHeight));
        g.setColour(juce::Colours::white);
    }

    paintStageStrips(g);
}

void MainComponent::paintOverChildren(juce::Graphics& g)
{
    if (!openingAudioLabel.isVisible())
        return;

    g.setColour(juce::Colours::black.withAlpha(Layout::loadingDimAlpha));
    g.fillAll();

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(Layout::loadingFontHeight, juce::Font::bold));
    g.drawFittedText(openingAudioLabel.getText(),
                     getLocalBounds().reduced(Layout::loadingTextPadding),
                     juce::Justification::centred, Layout::singleLine);

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
        juce::Timer::callAfterDelay(focusLossCheckDelayMs,
            [safeThis = juce::Component::SafePointer<MainComponent>(this)]()
            {
                if (safeThis == nullptr) return;

                // After the delay, check if the app truly lost foreground.
                if (!juce::Process::isForegroundProcess())
                {
                    // overlayWindow is an in-app child now, so it isn't toggled on app focus loss.
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
    }
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
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

    // An in-app dialog keeps the focus it gave its input (Enter / Esc / typing go to it).
    if (StageDialog::isInsideDialog(event.eventComponent))
        return;

    // Leave focus alone while the user is typing into a text field.
    for (auto* clicked = event.eventComponent; clicked != nullptr; clicked = clicked->getParentComponent())
        if (dynamic_cast<juce::TextEditor*>(clicked) != nullptr)
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
    using namespace Layout;
    int buttonWidth = getWidth() / playButtonWidthDivisor;
    int buttonHeight = getHeight() / playButtonHeightDivisor;
    int playX = (int)(getWidth() * centre - getWidth() * playButtonLeftOfCentre);
    int playY = (int)(getHeight() * centre + getHeight() * playButtonBelowCentre);

    playButton.setBounds(playX, playY, buttonWidth, buttonHeight);
    settingsButton.setBounds(origin, origin, settingsWidth, settingsButtonHeight);
    settingsPanel.setBounds(origin, settingsButtonHeight, settingsWidth, getHeight());
    midiButton.setBounds(origin, smallGap, panelButtonWidth, panelButtonHeight);
    updateNumberOfKeysDevice.setBounds(origin, midiButton.getBottom() + headerMargin, panelButtonWidth, panelButtonHeight);
    devicesCBUpdate.setBounds(origin, updateNumberOfKeysDevice.getBottom() + smallGap, panelButtonWidth, toggleHeight);

    helpIcon.setBounds(helpIconX, helpIconY, headerButtonWidth, helpIconHeight);

    headerPanel.setBounds(origin, origin, getWidth(), headerHeight);
    homeButton.setBounds(headerMargin, headerMargin, homeButtonWidth, headerButtonHeight);
    colourSelectorButton.setBounds(colourButtonX, headerMargin, headerButtonWidth, headerButtonHeight);
    instrumentSelectorButton.setBounds(instrumentButtonX, headerMargin, headerButtonWidth, headerButtonHeight);
    particleToggle.setBounds(colourSelectorButton.getX(), colourSelectorButton.getY() +colourSelectorButton.getHeight(), headerButtonWidth, toggleHeight);
    noteNumbersAnnotation.setBounds(particleToggle.getX(), particleToggle.getBottom(), headerButtonWidth, toggleHeight);

    leftHandInstrumentToggle.setBounds(instrumentSelectorButton.getX(), instrumentSelectorButton.getY() + instrumentSelectorButton.getHeight(), headerButtonWidth, toggleHeight);
    rightHandInstrumentToggle.setBounds(instrumentSelectorButton.getX(), leftHandInstrumentToggle.getY() + leftHandInstrumentToggle.getHeight(), headerButtonWidth, toggleHeight);

    startPlayback.setBounds(getWidth() - transportButtonSize - transportRightMargin, headerMargin, transportButtonSize, transportButtonSize);
    stopRecording.setBounds(startPlayback.getX()-transportButtonSize-headerMargin, headerMargin, transportButtonSize, transportButtonSize);
    startRecording.setBounds(stopRecording.getX()-transportButtonSize-headerMargin, headerMargin, transportButtonSize, transportButtonSize);


    saveRecordingButton.setBounds(startRecording.getX()-saveButtonWidth-headerMargin, headerMargin, saveButtonWidth, headerButtonHeight);
    playRecordingFileButton.setBounds(playRecordingX, headerMargin, headerButtonWidth, headerButtonHeight);




    //volumeKnob.setBounds(getWidth() / 2, 0, 70, 50);

    // The display (style tabs) now lives on the Styles page and the old section groups stay hidden as
    // the section-state holders, so neither is laid out in the old header any more (layoutStage below).

    if (noteLayer)
    {
        noteLayer->toFront(false);
        noteLayer->setAlwaysOnTop(true);
        noteLayer->setOpaque(true);
        keyboard.toFront(false);
        keyboard.setAlwaysOnTop(true);
        keyboard.setOpaque(true);
    }   

    layoutStage();
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
        // Esc first closes an open page (spec part 2 P.12), then opens the Esc menu.
        if (pageRouter.open() != StagePage::none && ! overlayWindow->isVisible())
        {
            if (pageRouter.open() == StagePage::styles && display != nullptr && display->getView() != StyleView::style)
            {
                display->closeSubView();   // the track picker / notes / settings go back to the style first
                return true;
            }
            closeStagePage();
            refreshStage();
            return true;
        }

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

    if (key.getKeyCode() == octaveDownKey)
    {
        if (startNote >= lowestShiftableStart)
        {
            this->keyListener.setStartNoteKeyboardInput(startNote - semitonesPerOctave);
            this->keyListener.setFinishNoteKeyboardInput(finishNote - semitonesPerOctave);
            this->keyboard.set_min_and_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());
            
            this->midiHandler.allOffKeyboard();
            this->noteLayer->resetStateActiveNotes();
            this->keyListener.resetState();
            this->keyboard.setIsDrawn(false);
            this->keyboard.resetStateActiveNotes();
            this->display->set_min_max(startNote - semitonesPerOctave, finishNote - semitonesPerOctave);
            this->display->setNewSettingsHelperFunction(-semitonesPerOctave);
        }
    }
    else if (key.getKeyCode() == octaveUpKey)
    {
        //DBG("Finish note" << finishNote);
        if (finishNote <= highestShiftableFinish)
        {
            this->keyListener.setStartNoteKeyboardInput(startNote + semitonesPerOctave);
            this->keyListener.setFinishNoteKeyboardInput(finishNote + semitonesPerOctave);
            this->keyboard.set_min_and_max(keyListener.getStartNoteKeyboardInput(), keyListener.getFinishNoteKeyboardInput());

            this->midiHandler.allOffKeyboard();
            this->noteLayer->resetStateActiveNotes();
            this->keyListener.resetState();
            this->keyboard.setIsDrawn(false);
            this->keyboard.resetStateActiveNotes();
            this->display->set_min_max(startNote + semitonesPerOctave, finishNote + semitonesPerOctave);
            this->display->setNewSettingsHelperFunction(semitonesPerOctave);
        }
    }
    return false;
}

void MainComponent::timerCallback()
{
    checkMidiDevicesValid();   // self-gates on which devices are open; also fires on hot-plug via the connection

    if (settingsPanel.isVisible())
        populateUpdateComboBoxDevices();

    if (stagePanel != nullptr && stagePanel->isVisible())
        refreshStage();   // backstop for state changed outside the panel (style tab, windows)
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
        for (auto& device : juce::MidiOutput::getAvailableDevices())
            if (device.identifier == outId) { present = true; break; }

        if (! present)
            handleMidiOutputDisconnected();
    }

    // INPUT: if the open MIDI input vanished, close it and clear any stuck-note visuals. Independent
    // of the output so a separate output device (or the internal synth) keeps playing on input loss.
    if (MIDIDevice.isOpenIN())
    {
        const auto inId = MIDIDevice.get_identifier();
        bool present = false;
        for (auto& device : juce::MidiInput::getAvailableDevices())
            if (device.identifier == inId) { present = true; break; }

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

    // Tell the user why playback stopped, on the Stage screen.
    const juce::String msg = "MIDI output disconnected - playback stopped";
    showStageMessage(msg);
    refreshStage();
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
    if (last == noDeviceCountYet || last != MIDIDevice.getNrInputActualDevices())
    {
        lastOfLast = last;
        last = MIDIDevice.getNrInputActualDevices();
        ok = true;
    }

    if (!ok)
        return;
    devicesCBUpdate.clear();
    int itemId = firstItemId;
    for (auto& dev : this->MIDIDevice.getAvailableInputDevicesNameIdentifier())
    {
        juce::String name = dev.first;
        juce::String identifier = dev.second;
        juce::String PID = MIDIDevice.extractPID(identifier);
        juce::String VID = MIDIDevice.extractVID(identifier);
        if (dataBase.deviceExists(VID, PID)==false)
            continue;

        if (itemId == firstItemId)
            updateDevicesMap.clear(); //map needs to be here because if we are here that means there has been at least a change in the devices, otherwise we clear the map each time


        updateDevicesMap[name] = identifier;

        this->devicesCBUpdate.addItem(name, itemId++);
    }
    if(lastOfLast!=noDeviceCountYet)
    {
        devicesCBUpdate.hidePopup();
        devicesCBUpdate.showPopup();
    }
    devicesCBUpdate.setSelectedId(firstItemId);
}

void MainComponent::initalizeSaveFileForUser()
{
    juce::PropertiesFile::Options options;
    options.applicationName= AppInfo::appName;
    options.filenameSuffix = AppDataNames::settingsFileSuffix;
    options.folderName=AppInfo::appName;
    options.osxLibrarySubFolder = AppDataNames::macLibrarySubfolder;
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
    int panelWidth = Layout::loginPanelSize;
    int panelHeight = Layout::loginPanelSize;

    int panelX = (parentBounds.getWidth() - panelWidth) / Layout::centringDivisor;
    int panelY = (parentBounds.getHeight() - panelHeight) / Layout::centringDivisor;

    loginWindow->setBounds(panelX, panelY, panelWidth, panelHeight);

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
