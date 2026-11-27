#include "MainComponent.h"
#include "AppInfo.h"

namespace
{
    namespace Text
    {
        const juce::String particleColours   = "Particle colours";
        const juce::String instruments       = "Instruments";
        const juce::String recordingStarted  = "Recording started!";
        const juce::String recordingStopped  = "Recording stopped!";
        const juce::String playbackStopped   = "Playback stopped!";
        const juce::String playRecordingTitle = "Play recording";
        const juce::String nothingToPlay     = "No recorded events to play.";
        const juce::String playbackStarted   = "Playback started!";
        const juce::String playFileRecording = "Play file recording";
        const juce::String preparingStyle    = "Preparing style...";
        const juce::String toggleParticles   = "Toggle particles";
        const juce::String midiHelp          = "This setting helps you adjust/select:\n *The volume\n *The reverb\n *The input device\n *The output device ";
        const juce::String helpMark          = "?";
        const juce::String leftHand          = "Left hand";
        const juce::String rightHand         = "Right hand";
        const juce::String introsTitle       = "Intros";
        const juce::String endingsTitle      = "Endings";
        const juce::String variationsTitle   = "Variations";
        const juce::String fillsTitle        = "Fills";
        const juce::String noteAnnotation    = "Note annotation";
    }

    constexpr int    recordingHasEvents    = 1;    // MidiRecordPlayer::stopRecording
    constexpr int    nothingToPlay         = 0;    // MidiRecordPlayer::startPlayBack
    constexpr int    playbackStarted       = 1;
    constexpr int    recordedLeftChannel   = 14;   // recordings store the hands on 14 / 15
    constexpr int    recordedRightChannel  = 15;
    constexpr int    noTextBox             = 0;
    constexpr int    styleListWidth        = 400;
    constexpr int    styleOverlayMinimumMs = 300;  // "Preparing style..." stays at least this long
    constexpr float  helpIconFontHeight    = 30.0f;
    constexpr float  helpIconAlpha         = 0.9f;
}


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
    playButton.setButtonText(MainComponentText::play);
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
    colourSelectorButton.setButtonText(Text::particleColours);
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
    instrumentSelectorButton.setButtonText(Text::instruments);
    instrumentSelectorButton.setLookAndFeel(&playScreenLAF);
    instrumentSelectorButton.setColour(juce::TextButton::buttonColourId, AppColours::accent2);
    instrumentSelectorButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    instrumentSelectorButton.onClick = [this] {
        openStagePage(StagePage::voices);
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
        lastMidiFileName.clear();   // startRecording empties the loaded file too
        recordPlayer.handleIncomingMessage(juce::MidiMessage::programChange(HandChannels::leftHand, midiHandler.getProgramNumberLeftHand()));
        recordPlayer.handleIncomingMessage(juce::MidiMessage::programChange(HandChannels::rightHand, midiHandler.getProgramNumberRightHand()));

        saveRecordingButton.setVisible(false);

        showStageMessage(Text::recordingStarted);
    };

    stopRecording.onClick = [this] {
        int result = recordPlayer.stopRecording();

        if (result == recordingHasEvents)
            saveRecordingButton.setVisible(true);

        showStageMessage(Text::recordingStopped);
    };

    recordPlayer.applyPresetFunction= [&]()
    {
        midiHandler.setProgramNumber(midiHandler.getProgramNumberLeftHand(), HandChoiceNames::left);
        midiHandler.setProgramNumber(midiHandler.getProgramNumberRightHand(), HandChoiceNames::right);
    };

    recordPlayer.onSfzMessage = [this](const juce::MidiMessage& msg)
    {
        if (audioHandler == nullptr || !MIDIDevice.isOpenAudioOUT())
            return;

        // Stored events use channels 14/15 (remapped from 1/16 at record time).
        // Remap back so the SFZ synth — which has instruments on 1 and 16 — renders them.
        int ch = msg.getChannel();
        int sfzCh = (ch == recordedLeftChannel) ? HandChannels::leftHand : (ch == recordedRightChannel ? HandChannels::rightHand : ch);

        if (msg.isNoteOn())
            midiHandler.injectMidiMessage(juce::MidiMessage::noteOn(sfzCh, msg.getNoteNumber(), msg.getVelocity()));
        else if (msg.isNoteOff())
            midiHandler.injectMidiMessage(juce::MidiMessage::noteOff(sfzCh, msg.getNoteNumber()));
    };

    recordPlayer.notifyFunction = [&]()
    {
        showStageMessage(Text::playbackStopped);
    };

    startPlayback.onClick = [this] {
        if (recordPlayer.getIsRecording())
            recordPlayer.stopRecording();

        int result=recordPlayer.startPlayBack();
        saveRecordingButton.setVisible(true);

        if(result==nothingToPlay)
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, Text::playRecordingTitle, Text::nothingToPlay);

        if (result == playbackStarted)
            showStageMessage(Text::playbackStarted);
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
    saveRecordingButton.setButtonText(MainComponentText::saveRecording);
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
    playRecordingFileButton.setButtonText(Text::playFileRecording);
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
    volumeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, noTextBox, noTextBox);

    volumeKnob.setLookAndFeel(&customKnobLookAndFeel);
    headerPanel.addAndMakeVisible(volumeKnob);
    volumeKnob.setVisible(false);
}

void MainComponent::displayInit()
{
    display = std::make_unique<Display>(deviceOpenedOUT, propertiesFile, styleListWidth);
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
            setLoadingOverlayVisible(true, Text::preparingStyle);
            loadSfzForCurrentStyle(styleID);
            // Fallback hide if no SFZ file actually changed (onSfzLoadComplete won't fire)
            juce::Timer::callAfterDelay(styleOverlayMinimumMs, [this]()
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
            setLoadingOverlayVisible(true, Text::preparingStyle);
            juce::Timer::callAfterDelay(styleOverlayMinimumMs, [this]()
            {
                setLoadingOverlayVisible(false);
            });
        }
    };
}

void MainComponent::toggleButtonInit()
{
    particleToggle.setButtonText(Text::toggleParticles);
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
    this->keyboard.set_min_and_max(min, max);
    noteLayer = std::make_unique<NoteLayer>(this->keyboard);
    noteLayer->setParticleStyle(particleStyleId);   // the stored style (Settings page), chosen before the layer existed
    midiHandler.addListener(noteLayer.get());
    addAndMakeVisible(noteLayer.get());
    layoutStage();   // keys and note area take their Stage regions

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
    helpIcon.setFont(juce::Font(helpIconFontHeight));  // Set the font size
    helpIcon.setColour(juce::Label::textColourId, juce::Colours::lightgrey.withAlpha(helpIconAlpha));
    helpIcon.setTooltip(Text::midiHelp);
    helpIcon.setJustificationType(juce::Justification::left);
    settingsPanel.addAndMakeVisible(helpIcon);
    helpIcon.setText(Text::helpMark, juce::dontSendNotification);
    helpIcon.setVisible(false);
}

void MainComponent::midiSettingsInit()
{
    midiButton.setButtonText(MainComponentText::midiSettings);
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
}

void MainComponent::homeButtonInit()
{
    homeButton.setButtonText(MainComponentText::home);
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
    leftHandInstrumentToggle.setButtonText(Text::leftHand);
    leftHandInstrumentToggle.setMouseCursor(juce::MouseCursor::PointingHandCursor);

    leftHandInstrumentToggle.setColour(juce::ToggleButton::textColourId,        AppColours::titleText);
    leftHandInstrumentToggle.setColour(juce::ToggleButton::tickColourId,         AppColours::accent3);
    leftHandInstrumentToggle.setColour(juce::ToggleButton::tickDisabledColourId, AppColours::rowLabel);

    leftHandInstrumentToggle.setToggleState(false,juce::dontSendNotification);
    headerPanel.addAndMakeVisible(leftHandInstrumentToggle);
    leftHandInstrumentToggle.setVisible(false);


    rightHandInstrumentToggle.setButtonText(Text::rightHand);
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
    updateNumberOfKeysDevice.setButtonText(MainComponentText::updateKeyCount);
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
    devicesCBUpdate.setText(MainComponentText::selectDevice, juce::dontSendNotification);
    devicesCBUpdate.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    settingsPanel.addAndMakeVisible(devicesCBUpdate);
    devicesCBUpdate.setVisible(false);
}

void MainComponent::sectionsInit()
{
    // The live section buttons; each label is also the name handed to the arranger.
    static const juce::StringArray introLabels     { "Intro 1", "Intro 2", "Intro 3" };
    static const juce::StringArray endingLabels    { "Ending 1", "Ending 2", "Ending 3" };
    static const juce::StringArray variationLabels { "Var 1", "Var 2", "Var 3", "Var 4" };
    static const juce::StringArray fillLabels      { "Fill 1", "Fill 2", "Fill 3", "Fill 4" };

    auto labelsOf = [](const juce::StringArray& labels) { return std::vector<juce::String>(labels.begin(), labels.end()); };

    std::unordered_map<juce::String, std::function<void()>> callbacksIntroEnding;
    for (auto& label : introLabels)  callbacksIntroEnding[label] = [this, label]() { handleIntro(label); };
    for (auto& label : endingLabels) callbacksIntroEnding[label] = [this, label]() { handleEnding(label); };

    introsEndings = std::make_unique<StyleSectionComponent>(std::vector<juce::String>{Text::introsTitle, Text::endingsTitle},
        std::vector<std::vector<juce::String>>{ labelsOf(introLabels), labelsOf(endingLabels) },callbacksIntroEnding);

    std::unordered_map<juce::String, std::function<void()>> callbacksVarFillBreak;
    for (auto& label : variationLabels) callbacksVarFillBreak[label] = [this, label]() { handleVar(label); };
    for (auto& label : fillLabels)      callbacksVarFillBreak[label] = [this, label]() { handleFill(label); };
    callbacksVarFillBreak[SectionGroupNames::breakButton] = [this]() { handleBreak(SectionGroupNames::breakButton); };

    variationsFills = std::make_unique<StyleSectionComponent>(std::vector<juce::String>{Text::variationsTitle, Text::fillsTitle},
        std::vector<std::vector<juce::String>>{ labelsOf(variationLabels), labelsOf(fillLabels) }, callbacksVarFillBreak);

    // Wire the Auto Fill toggle (lives on the "Variations" group) to the arranger engine.
    for (auto* group : variationsFills->getSectionGroups())
        if (group->getToggleButton() != nullptr)
            group->onToggleChanged = [this](bool on)
            {
                if (display)
                    display->setArrangerAutoFillEnabled(on);
                if (propertiesFile != nullptr)   // remember the choice across restarts (app-wide)
                {
                    propertiesFile->setValue(SettingsFileKeys::arrangerAutoFillEnabled, on);
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
    noteNumbersAnnotation.setButtonText(Text::noteAnnotation);
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

