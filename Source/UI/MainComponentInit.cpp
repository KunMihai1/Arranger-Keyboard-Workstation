#include "MainComponent.h"
#include "AppInfo.h"


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
