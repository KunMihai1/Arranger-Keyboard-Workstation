/*
  ==============================================================================

    MainComponentStage.cpp

    The Stage panel replaces the old 100 px header (plan 2026-11-23 task 7,
    spec part 2 P.13). Every panel control calls the handler that already
    exists, so behaviour stays what it was:

      section buttons  -> the (hidden) old section buttons' own handlers;
                          the panel LEDs mirror whichever of them is lit
      Start / Stop     -> the open style's Start / Stop
      Rec              -> today's record start / stop
      Auto Fill        -> today's Auto Fill toggle; Sync Start -> SynchroStart
      tempo            -> the open style, as if its BPM slider were released
      pages            -> Styles / Recorder / Settings / Chords / Effects / Voices
                          host today's controls on a page frame (Voices:
                          MainComponentVoices.cpp); Home does what it did
      Quick FX         -> the Effects window's save path, on the selected part

  ==============================================================================
*/

#include "MainComponent.h"

namespace
{
    namespace StageWiringText
    {
        const juce::String playMode         = "PLAY";
        const juce::String stylesMode       = "STYLE EDIT";
        const juce::String recorderMode     = "RECORDER";
        const juce::String settingsMode     = "SETTINGS";
        const juce::String chordsMode       = "CHORDS";
        const juce::String effectsMode      = "EFFECTS";
        const juce::String voicesMode       = "VOICES";
        const juce::String effectsTitle     = "Effects";
        const juce::String particles        = "Particles";
        const juce::String noteNames        = "Note names on keys";
        const char* const  particleColoursUtf8 = "Particle colours\xe2\x80\xa6";
        const juce::String particleStyle    = "Particle style";
        const juce::String sparks           = "Sparks";
        const juce::String dust             = "Dust";
        const juce::String smoke            = "Smoke";
        const juce::String tracksMode       = "TRACKS";
        const juce::String notesMode        = "NOTES";
        const juce::String instrumentsMode  = "INSTRUMENT";
        const juce::String noStyleTitle     = "No style";
        const juce::String noStyleOpen      = "Open a style first (Styles)";
        const juce::String choosePart       = "Choose a part: Left or Right";
        const juce::String signatureDivider = "/";
        const juce::String stylesTitle      = "Styles";
        const juce::String recorderTitle    = "Recorder";
        const juce::String settingsTitle    = "Settings";
        const juce::String chordsTitle      = "Chord helper";
    }

    constexpr int beatUnit = 4;   // styles carry no other time-signature denominator today (BeatBar's default)

    // Legacy page controls, reference px (scaled with the Stage).
    constexpr int pageButtonWidth  = 180;
    constexpr int pageButtonHeight = 30;
    constexpr int pageButtonGap    = 12;
    constexpr int pageIconSize     = 30;
    constexpr size_t displayParticlesRow = 0;   // Settings page, Display column rows
    constexpr size_t displayNoteNamesRow = 1;
    constexpr size_t displayColoursRow   = 2;
}

//==============================================================================
void MainComponent::stagePanelInit()
{
    stagePanel = std::make_unique<StagePanel> (display ? display->getOpenStyleTempo() : ArrangerStyleDefaults::tempoBpm);
    addChildComponent (*stagePanel);   // shown with the play scene (toggleHPanel)

    stagePanel->onSection   = [this] (const juce::String& sectionName) { pressSectionFromPanel (sectionName); };
    stagePanel->onAutoFill  = [this] { toggleAutoFillFromPanel(); };
    stagePanel->onSyncStart = [this] { toggleSyncStartFromPanel(); };
    stagePanel->onRec       = [this] { toggleRecordingFromPanel(); };
    stagePanel->onFile      = [this] { openStagePage (StagePage::file); };
    stagePanel->onStart     = [this] { startOrStopStyleFromPanel (true); };
    stagePanel->onStop      = [this] { startOrStopStyleFromPanel (false); };
    stagePanel->onPage      = [this] (StagePage page) { openStagePage (page); };
    stagePanel->onPart      = [this] (bool left) { togglePartFromPanel (left); };
    // Sound follows the knob live; the setting is written to disk once, when the knob is let go.
    stagePanel->onQuickKnob         = [this] (StagePanel::QuickKnob knob, int value) { applyQuickKnob (knob, value, false); };
    stagePanel->onQuickKnobReleased = [this] (StagePanel::QuickKnob knob, int value) { applyQuickKnob (knob, value, true); };

    stagePanel->screen().onTempoChanged = [this] (double tempo)
    {
        if (display != nullptr && display->hasOpenStyle())
            display->commitOpenStyleTempo (tempo);
    };

    if (display != nullptr)
        display->onBeatPosition = [this] (double elapsedBeats, int beatsPerBar)
        {
            stageBeats = elapsedBeats;
            stageBeatsPerBar = beatsPerBar;
            updateStageScreen();
        };

    // Panel buttons must not keep keyboard focus: the PC keyboard keeps playing after a press.
    for (auto* child : stagePanel->getChildren())
        child->setMouseClickGrabsKeyboardFocus (false);

    stagePagesInit();
    refreshStage();
}

void MainComponent::stagePagesInit()
{
    auto makePage = [this] (const juce::String& title)
    {
        auto page = std::make_unique<StagePageFrame> (title);
        page->onExit = [this] { closeStagePage(); refreshStage(); };   // spec §6: Exit returns to the play view
        addChildComponent (*page);
        return page;
    };

    // Styles (board StageStyles): the styles list and the open style's track strips; the track picker,
    // notes and style settings open in the same page. Exit / Esc go back from those first.
    stylesPage = makePage (StageWiringText::stylesTitle);
    stylesPage->addAndMakeVisible (display.get());
    stylesPage->onLayoutContent = [this] (juce::Rectangle<int> area, float scale)
    {
        display->setScale (scale);
        display->setBounds (area);
    };
    stylesPage->onExit = [this]
    {
        if (display->getView() != StyleView::style)
            display->closeSubView();
        else
        {
            closeStagePage();
            refreshStage();
        }
    };
    display->onViewChanged = [this] { syncStylesPage(); };
    display->onInfo = [this] (const juce::String& text) { showStageMessage (text); };
    display->onTrackInstrumentChosen = [this] (int, int)
    {
        // The SFZ engine ignores program changes: load the SFZ the style maps to the new instrument
        // (channels whose file did not change are skipped).
        if (MIDIDevice.isOpenAudioOUT())
            loadSfzForCurrentStyle();
    };
    syncStylesPage();

    // Recorder: play the recording, save it, play a MIDI file.
    filePage = makePage (StageWiringText::recorderTitle);
    for (juce::Component* control : { static_cast<juce::Component*> (&startPlayback), static_cast<juce::Component*> (&saveRecordingButton),
                                       static_cast<juce::Component*> (&playRecordingFileButton) })
        filePage->addChildComponent (control);
    filePage->onLayoutContent = [this] (juce::Rectangle<int> area, float scale)
    {
        auto row = area.withHeight (juce::roundToInt (pageButtonHeight * scale));
        startPlayback.setBounds (row.removeFromLeft (juce::roundToInt (pageIconSize * scale)));
        row.removeFromLeft (juce::roundToInt (pageButtonGap * scale));
        saveRecordingButton.setBounds (row.removeFromLeft (juce::roundToInt (pageButtonWidth * scale)));
        row.removeFromLeft (juce::roundToInt (pageButtonGap * scale));
        playRecordingFileButton.setBounds (row.removeFromLeft (juce::roundToInt (pageButtonWidth * scale)));
    };

    // Settings (board StageSettings): MIDI / Arranger / Chords = midiWindow (it was the MIDI Settings window), Display =
    // today's display toggles and the particle style (spec B9).
    settingsPage = makePage (StageWiringText::settingsTitle);
    createSettingsContent();
    settingsPage->addAndMakeVisible (*midiWindow);
    particleToggle.setButtonText (StageWiringText::particles);
    noteNumbersAnnotation.setButtonText (StageWiringText::noteNames);
    colourSelectorButton.setButtonText (juce::String::fromUTF8 (StageWiringText::particleColoursUtf8));
    particleStyleLabel.setText (StageWiringText::particleStyle);
    particleSparks.setButtonText (StageWiringText::sparks);
    particleDust.setButtonText (StageWiringText::dust);
    particleSmoke.setButtonText (StageWiringText::smoke);
    particleSparks.onClick = [this] { setParticleStyle (ParticleStyles::sparks, true); };
    particleDust.onClick   = [this] { setParticleStyle (ParticleStyles::dust, true); };
    particleSmoke.onClick  = [this] { setParticleStyle (ParticleStyles::smoke, true); };
    setParticleStyle (ParticleStyles::fromStored (propertiesFile != nullptr ? propertiesFile->getIntValue (SettingsFileKeys::particleStyle, ParticleStyles::dust)
                                                                            : ParticleStyles::dust), false);
    // These three are shown and hidden with the play view (toggleHPanel), as in phase 1: add them hidden.
    for (juce::Component* control : { static_cast<juce::Component*> (&particleToggle), static_cast<juce::Component*> (&noteNumbersAnnotation),
                                       static_cast<juce::Component*> (&colourSelectorButton) })
        settingsPage->addChildComponent (control);
    for (juce::Component* control : { static_cast<juce::Component*> (&particleStyleLabel), static_cast<juce::Component*> (&particleSparks),
                                       static_cast<juce::Component*> (&particleDust), static_cast<juce::Component*> (&particleSmoke) })
        settingsPage->addAndMakeVisible (control);
    midiWindow->setInterceptsMouseClicks (false, true);   // its empty area (the Display column) clicks through
    settingsPage->onLayoutContent = [this] (juce::Rectangle<int> area, float scale)
    {
        midiWindow->setScale (scale);
        midiWindow->setBounds (area);
        const auto rects = settingsPageLayout (area, scale);
        for (auto* toggle : { &particleToggle, &noteNumbersAnnotation, &colourSelectorButton })
            toggle->setScale (scale);   // first: the bounds leave room for the LED glow at this scale
        particleToggle.setBounds (particleToggle.boundsForBoardBox (rects.displayToggles[displayParticlesRow]));
        noteNumbersAnnotation.setBounds (noteNumbersAnnotation.boundsForBoardBox (rects.displayToggles[displayNoteNamesRow]));
        colourSelectorButton.setBounds (colourSelectorButton.boundsForBoardBox (rects.displayToggles[displayColoursRow]));
        particleStyleLabel.setBounds (rects.particleStyleLabel);
        auto left = static_cast<float> (rects.particleStyleRow.getX());   // adjacent seg buttons, as the board
        for (auto* button : { &particleSparks, &particleDust, &particleSmoke })
        {
            button->setScale (scale);
            button->setBounds (juce::Rectangle<float> (left, static_cast<float> (rects.particleStyleRow.getY()), button->getExactWidth(),
                                                       static_cast<float> (rects.particleStyleRow.getHeight())).toNearestInt());
            left += button->getExactWidth();
        }
        particleStyleLabel.setScale (scale);
    };

    // Chords (board PageChords): the chord helper - search, the chord list, the chosen chord's voicings on
    // mini keyboards; a click shows and plays the chord.
    chordsPage = makePage (StageWiringText::chordsTitle);
    chordsPage->setRowGap (StageMetrics::subPageRowGap);
    chordsBrowser = std::make_unique<ChordBrowserComponent> (myChordLibrary);
    chordsBrowser->onChordChosen = [this] (const Chord& chord) { playChordOnClick (chord); };
    chordsBrowser->onEscape = [this] { closeStagePage(); refreshStage(); };
    chordsPage->addAndMakeVisible (*chordsBrowser);
    chordsPage->onLayoutContent = [this] (juce::Rectangle<int> area, float scale)
    {
        chordsBrowser->setScale (scale);
        chordsBrowser->setBounds (area);
    };

    // Effects (board StageEffects): the effect knobs on a page (they used to be the Sound Effects window).
    effectsPage = makePage (StageWiringText::effectsTitle);
    effectsContent = std::make_unique<SoundEffectContentComponent>();
    setCallBacksForEffectWindow();
    effectsPage->addAndMakeVisible (*effectsContent);
    effectsPage->setActions (effectsContent->getTitleActions());
    effectsPage->onLayoutContent = [this] (juce::Rectangle<int> area, float scale)
    {
        effectsContent->setScale (scale);
        effectsContent->setBounds (area);
    };

    voicesPageInit();
}

void MainComponent::syncStylesPage()
{
    // The Style page's frame follows the view Display shows (style, track picker, notes, settings).
    if (stylesPage == nullptr || display == nullptr)
        return;
    const bool styleView = display->getView() == StyleView::style;
    stylesPage->setTitle (display->getViewTitle());
    stylesPage->setSubtitle (display->getViewSubtitle());
    stylesPage->setRowGap (styleView ? StageMetrics::pageRowGap : StageMetrics::subPageRowGap);
    stylesPage->setActions (display->getTitleActions());
    updateStageScreen();
}

std::array<StagePageFrame*, MainComponent::stagePageCount> MainComponent::stagePageFrames() const
{
    return { stylesPage.get(), filePage.get(), settingsPage.get(), chordsPage.get(), effectsPage.get(), voicesPage.get() };
}

//==============================================================================
void MainComponent::layoutStage()
{
    if (stagePanel == nullptr)
        return;

    stageRegions = StageScale::regionsFor (getLocalBounds());
    stagePanel->setScale (stageRegions.scale);
    stagePanel->setBounds (stageRegions.panel);

    for (auto* page : stagePageFrames())
        if (page != nullptr)
        {
            page->setScale (stageRegions.scale);
            page->setBounds (stageRegions.noteArea);
        }

    if (keyboardInitialized)
        keyboard.setBounds (stageRegions.keys);
    if (noteLayer)
        noteLayer->setBounds (stageRegions.noteArea);
}

void MainComponent::paintStageStrips (juce::Graphics& g)
{
    if (stagePanel == nullptr || ! stagePanel->isVisible())
        return;

    const auto felt = stageRegions.felt.toFloat();
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::feltTop, felt.getY(), StagePalette::feltBottom, felt.getBottom()));
    g.fillRect (felt);

    const auto slip = stageRegions.slip.toFloat();
    g.setGradientFill (juce::ColourGradient::vertical (StagePalette::slipTop, slip.getY(), StagePalette::slipBottom, slip.getBottom()));
    g.fillRect (slip);
    g.setColour (StagePalette::slipEdge);
    g.fillRect (slip.withHeight (stageRegions.scale * StageMetrics::panelBottomEdge));
}

//==============================================================================
void MainComponent::showStageMessage (const juce::String& text)
{
    if (stagePanel != nullptr)
        stagePanel->screen().postMessage (text);
}

void MainComponent::updateStageScreen()
{
    if (stagePanel == nullptr)
        return;

    const bool styleOpen = display != nullptr && display->hasOpenStyle();
    const bool playing   = styleOpen && display->isOpenStylePlaying();

    StageScreen::Content content;
    content.styleName     = styleOpen ? display->getStyleName() : StageWiringText::noStyleTitle;
    switch (pageRouter.open())
    {
        case StagePage::styles:
        {
            const auto view = display != nullptr ? display->getView() : StyleView::style;
            content.mode = view == StyleView::tracks ? StageWiringText::tracksMode
                         : view == StyleView::notes  ? StageWiringText::notesMode
                         : view == StyleView::instruments ? StageWiringText::instrumentsMode
                                                     : StageWiringText::stylesMode;
        }
            break;
        case StagePage::file:     content.mode = StageWiringText::recorderMode; break;
        case StagePage::settings: content.mode = StageWiringText::settingsMode; break;
        case StagePage::chords:   content.mode = StageWiringText::chordsMode;   break;
        case StagePage::effects:  content.mode = StageWiringText::effectsMode;  break;
        case StagePage::voices:   content.mode = StageWiringText::voicesMode;   break;
        default:                  content.mode = StageWiringText::playMode;     break;
    }
    content.variation     = stageLitSection.toUpperCase();
    content.chord         = StageText::chordName (stageChord);
    content.timeSignature = juce::String (stageBeatsPerBar) + StageWiringText::signatureDivider + juce::String (beatUnit);
    content.bar           = StageText::barNumber (stageBeats, stageBeatsPerBar);
    content.beatsPerBar   = stageBeatsPerBar;
    content.elapsedBeats  = stageBeats;
    content.playing       = playing;
    stagePanel->screen().setContent (content);
}

void MainComponent::refreshStage()
{
    if (stagePanel == nullptr)
        return;

    const bool styleOpen = display != nullptr && display->hasOpenStyle();
    const bool playing   = styleOpen && display->isOpenStylePlaying();

    stagePanel->setPlaying (playing);
    stagePanel->setRecording (recordPlayer.getIsRecording());
    stagePanel->setSyncStart (propertiesFile != nullptr && propertiesFile->getBoolValue (SettingsFileKeys::synchroStart, false));
    stagePanel->setPartLeds (leftHandInstrumentToggle.getToggleState(), rightHandInstrumentToggle.getToggleState());
    syncVoicesPage();   // its title row shows the same two toggles

    auto page = pageRouter.open();
    stagePanel->setPageLed (page);

    if (variationsFills != nullptr)
        for (auto* group : variationsFills->getSectionGroups())
            if (auto* toggle = group->getToggleButton())
                stagePanel->setAutoFill (toggle->getToggleState());

    if (const auto channel = selectedPartChannel(); channel != int {})
        stagePanel->setQuickKnobValues (MIDIDevice.getVolume (channel), MIDIDevice.getReverb (channel));

    auto& screen = stagePanel->screen();
    screen.setTempoLocked (playing && display->isOpenStyleInArrangerMode());
    if (styleOpen)
        screen.showTempo (display->getOpenStyleTempo(), display->getOpenStyleBaseTempo());

    updateStageScreen();
}

//==============================================================================
void MainComponent::syncSectionLedsFromGroups()
{
    juce::String lit;
    for (auto* sections : { introsEndings.get(), variationsFills.get() })
        if (sections != nullptr)
            for (auto* group : sections->getSectionGroups())
                for (const auto& [buttonName, isLit] : group->getActivationMap())
                    if (isLit)
                        lit = buttonName;

    stageLitSection = lit;
    if (stagePanel != nullptr)
        stagePanel->setSectionLit (lit);
    updateStageScreen();
}

void MainComponent::pressSectionFromPanel (const juce::String& sectionName)
{
    for (auto* sections : { introsEndings.get(), variationsFills.get() })
        if (sections != nullptr)
            for (auto* group : sections->getSectionGroups())
                for (auto& button : group->getButtons())
                    if (button->getButtonText() == sectionName && button->onClick != nullptr)
                    {
                        button->onClick();   // the old button's own handler: same highlight and arranger routing
                        syncSectionLedsFromGroups();
                        return;
                    }
}

void MainComponent::toggleAutoFillFromPanel()
{
    if (variationsFills == nullptr)
        return;

    for (auto* group : variationsFills->getSectionGroups())
        if (auto* toggle = group->getToggleButton(); toggle != nullptr && toggle->onClick != nullptr)
        {
            toggle->setToggleState (! toggle->getToggleState(), juce::dontSendNotification);
            toggle->onClick();   // what a click on the old checkbox runs
        }
    refreshStage();
}

void MainComponent::toggleSyncStartFromPanel()
{
    const bool on = ! (propertiesFile != nullptr && propertiesFile->getBoolValue (SettingsFileKeys::synchroStart, false));
    if (propertiesFile != nullptr)
    {
        propertiesFile->setValue (SettingsFileKeys::synchroStart, on);
        propertiesFile->saveIfNeeded();
    }
    if (display != nullptr)
        display->setArrangerSynchroStart (on);
    refreshStage();
}

void MainComponent::toggleRecordingFromPanel()
{
    if (recordPlayer.getIsRecording())
        stopRecording.onClick();
    else
        startRecording.onClick();
    refreshStage();
}

void MainComponent::startOrStopStyleFromPanel (bool start)
{
    if (display == nullptr || ! display->hasOpenStyle())
    {
        showStageMessage (StageWiringText::noStyleOpen);
        return;
    }

    if (start)
        display->startOpenStyle();
    else
        display->stopOpenStyle();

    // Start / Stop are the style tab's buttons, clicked asynchronously: refresh after they ran.
    juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<MainComponent> (this)]
    {
        if (safeThis != nullptr)
            safeThis->refreshStage();
    });
}

//==============================================================================
void MainComponent::openStagePage (StagePage page)
{
    switch (page)
    {
        case StagePage::home:
            if (display != nullptr && display->hasOpenStyle() && display->getView() != StyleView::style)
                display->homeButtonInteraction();   // as Home did with a temporary style tab open
            closeStagePage();
            homeButtonOnClick();
            return;

        case StagePage::effects:
            if (pageRouter.toggle (page) == StagePage::none)
                closeStagePage();
            else
            {
                showStagePage (page);
                // As a newly opened window did: the knobs show the selected instrument's stored values.
                if (effectsContent && effectsContent->onChannelChanged)
                    effectsContent->onChannelChanged (effectsContent->getSelectedChannel());
            }
            break;

        case StagePage::styles:
        case StagePage::file:
        case StagePage::settings:
        case StagePage::chords:
        case StagePage::voices:
            if (pageRouter.toggle (page) == StagePage::none)
                closeStagePage();
            else
            {
                showStagePage (page);
                if (page == StagePage::settings && midiWindow)
                    midiWindow->pageShown();   // fresh device lists, polled while the page is open (as the window did)
                if (page == StagePage::voices)
                    voicesPageShown();
            }
            break;

        case StagePage::none:
            closeStagePage();
            break;
    }
    refreshStage();
}

void MainComponent::showStagePage (StagePage page)
{
    StagePageFrame* shown = page == StagePage::styles ? stylesPage.get()
                          : page == StagePage::file   ? filePage.get()
                          : page == StagePage::chords ? chordsPage.get()
                          : page == StagePage::effects ? effectsPage.get()
                          : page == StagePage::voices ? voicesPage.get()
                                                      : settingsPage.get();
    const auto frames = stagePageFrames();
    if (midiWindow && settingsPage && settingsPage->isVisible() && shown != settingsPage.get())
        midiWindow->pageHidden();   // another page replaces Settings: stop polling the devices
    const bool anyPageWasOpen = std::any_of (frames.begin(), frames.end(),
                                             [] (const StagePageFrame* frame) { return frame != nullptr && frame->isVisible(); });

    for (auto* frame : frames)
        if (frame != nullptr)
            frame->setVisible (frame == shown);

    // The GL note layer renders on top of normal painting: hide it while a page covers the note area.
    if (! anyPageWasOpen)
        noteLayerVisibleBeforePage = noteLayer && noteLayer->isVisible();
    if (noteLayer)
    {
        noteLayer->setHeldHidden (true);   // a note played while the page is open must not bring it back on top
        noteLayer->setVisible (false);
    }

    shown->toFront (false);
}

void MainComponent::closeStagePage()
{
    if (pageRouter.open() == StagePage::settings && midiWindow)
        midiWindow->pageHidden();
    pageRouter.close();
    if (display != nullptr)
        display->resetView();   // a track picker / notes page does not outlive its page
    bool anyPageWasOpen = false;
    for (auto* frame : stagePageFrames())
        if (frame != nullptr && frame->isVisible())
        {
            anyPageWasOpen = true;
            frame->setVisible (false);
        }

    if (noteLayer)
        noteLayer->setHeldHidden (false);
    if (anyPageWasOpen && noteLayer)
    {
        noteLayer->resetState();   // don't come back to frozen notes
        noteLayer->setVisible (noteLayerVisibleBeforePage);
    }

    if (anyPageWasOpen)   // focus back to the play surface, so the PC keyboard plays again (P.13)
        juce::MessageManager::callAsync ([safeThis = juce::Component::SafePointer<MainComponent> (this)]
        {
            if (safeThis != nullptr && safeThis->isVisible())
                safeThis->grabKeyboardFocus();
        });
}

//==============================================================================
void MainComponent::togglePartFromPanel (bool left)
{
    auto& toggle = left ? leftHandInstrumentToggle : rightHandInstrumentToggle;
    toggle.setToggleState (! toggle.getToggleState(), juce::dontSendNotification);
    refreshStage();
}

int MainComponent::selectedPartChannel() const
{
    if (rightHandInstrumentToggle.getToggleState())
        return HandChannels::rightHand;
    if (leftHandInstrumentToggle.getToggleState())
        return HandChannels::leftHand;
    return {};
}

void MainComponent::applyQuickKnob (StagePanel::QuickKnob knob, int value, bool save)
{
    if (selectedPartChannel() == int {})
    {
        showStageMessage (StageWiringText::choosePart);
        stagePanel->revertQuickKnobs();   // nothing changed, so the knob must not show a change
        return;
    }

    const auto controller = knob == StagePanel::QuickKnob::volume ? EffectControllers::volume : EffectControllers::reverb;
    if (leftHandInstrumentToggle.getToggleState())
        applyAndSaveEffect (controller, value, HandChannels::leftHand, save);
    if (rightHandInstrumentToggle.getToggleState())
        applyAndSaveEffect (controller, value, HandChannels::rightHand, save);

    // The Effects page shows the same stored values: keep it current while it is open.
    if (pageRouter.open() == StagePage::effects && effectsContent && effectsContent->onChannelChanged)
        effectsContent->onChannelChanged (effectsContent->getSelectedChannel());
}

void MainComponent::setParticleStyle (int styleId, bool save)
{
    // Spec B9: NoteLayer::setParticleStyle existed but nothing called it (always Dust). Saved as ParticleStyle.
    particleSparks.setLedOn (styleId == ParticleStyles::sparks);
    particleDust.setLedOn (styleId == ParticleStyles::dust);
    particleSmoke.setLedOn (styleId == ParticleStyles::smoke);
    particleStyleId = styleId;
    if (noteLayer)
        noteLayer->setParticleStyle (styleId);
    if (save && propertiesFile != nullptr)
    {
        propertiesFile->setValue (SettingsFileKeys::particleStyle, styleId);
        propertiesFile->saveIfNeeded();
    }
}
