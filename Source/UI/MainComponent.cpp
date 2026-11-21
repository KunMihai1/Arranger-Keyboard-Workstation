#include "MainComponent.h"
#include "AppInfo.h"

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
    options.applicationName= AppInfo::appName;
    options.filenameSuffix = "settings";
    options.folderName=AppInfo::appName;
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
