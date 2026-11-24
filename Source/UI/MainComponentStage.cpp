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
      pages            -> Styles / Recorder / Settings / Chords host today's
                          controls on a page frame; Effects, Voices and Home open
                          what their old buttons opened
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
        const juce::String effectsTitle     = "Effects";
        const juce::String tracksMode       = "TRACKS";
        const juce::String notesMode        = "NOTES";
        const juce::String noStyleTitle     = "No style";
        const juce::String noStyleOpen      = "Open a style first (Styles)";
        const juce::String choosePart       = "Choose a part: Left or Right";
        const juce::String signatureDivider = "/";
        const juce::String stylesTitle      = "Styles";
        const juce::String recorderTitle    = "Recorder";
        const juce::String settingsTitle    = "Settings";
        const juce::String chordsTitle      = "Chords";
        const juce::String midiSettings     = "MIDI settings...";
    }

    constexpr int beatUnit = 4;   // styles carry no other time-signature denominator today (BeatBar's default)

    // Legacy page controls, reference px (scaled with the Stage).
    constexpr int pageButtonWidth  = 180;
    constexpr int pageButtonHeight = 30;
    constexpr int pageButtonGap    = 12;
    constexpr int pageIconSize     = 30;
    constexpr int pageToggleWidth  = 200;
    constexpr int pageToggleHeight = 26;
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

    // Settings: today's MIDI Settings window, plus the display toggles that lived in the header.
    settingsPage = makePage (StageWiringText::settingsTitle);
    midiSettingsPageButton.setButtonText (StageWiringText::midiSettings);
    midiSettingsPageButton.setLookAndFeel (&playScreenLAF);
    midiSettingsPageButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    midiSettingsPageButton.setColour (juce::TextButton::buttonColourId, AppColours::accent2);
    midiSettingsPageButton.onClick = [this] { if (overlayWindow && overlayWindow->onSettingsClick) overlayWindow->onSettingsClick(); };
    settingsPage->addAndMakeVisible (midiSettingsPageButton);
    for (juce::Component* control : { static_cast<juce::Component*> (&particleToggle), static_cast<juce::Component*> (&noteNumbersAnnotation),
                                       static_cast<juce::Component*> (&colourSelectorButton) })
        settingsPage->addChildComponent (control);
    settingsPage->onLayoutContent = [this] (juce::Rectangle<int> area, float scale)
    {
        const auto gap = juce::roundToInt (pageButtonGap * scale);
        auto column = area.withWidth (juce::roundToInt (pageToggleWidth * scale));
        midiSettingsPageButton.setBounds (column.removeFromTop (juce::roundToInt (pageButtonHeight * scale)));
        column.removeFromTop (gap);
        particleToggle.setBounds (column.removeFromTop (juce::roundToInt (pageToggleHeight * scale)));
        noteNumbersAnnotation.setBounds (column.removeFromTop (juce::roundToInt (pageToggleHeight * scale)));
        column.removeFromTop (gap);
        colourSelectorButton.setBounds (column.removeFromTop (juce::roundToInt (pageButtonHeight * scale)));
    };

    // Chords: today's chord helper (search, list, preview; a click plays the chord). It used to open in
    // a call-out box that covered the panel and kept Esc to its search box, so it lives on a page now.
    chordsPage = makePage (StageWiringText::chordsTitle);
    chordsBrowser = std::make_unique<ChordBrowserComponent> (myChordLibrary);
    chordsBrowser->onChordChosen = [this] (const Chord& chord) { playChordOnClick (chord); };
    chordsBrowser->onEscape = [this] { closeStagePage(); refreshStage(); };
    chordsPage->addAndMakeVisible (*chordsBrowser);
    chordsPage->onLayoutContent = [this] (juce::Rectangle<int> area, float) { chordsBrowser->setBounds (area); };

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
    return { stylesPage.get(), filePage.get(), settingsPage.get(), chordsPage.get(), effectsPage.get() };
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
                                                     : StageWiringText::stylesMode;
        }
            break;
        case StagePage::file:     content.mode = StageWiringText::recorderMode; break;
        case StagePage::settings: content.mode = StageWiringText::settingsMode; break;
        case StagePage::chords:   content.mode = StageWiringText::chordsMode;   break;
        case StagePage::effects:  content.mode = StageWiringText::effectsMode;  break;
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

        case StagePage::voices:
            closeStagePage();
            showInstrumentSelector();
            break;

        case StagePage::effects:
            if (pageRouter.toggle (page) == StagePage::none)
                closeStagePage();
            else
            {
                if (midiWindow)
                    midiWindow->closeButtonPressed();   // as before: Effects and the MIDI Settings window replace each other
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
            if (pageRouter.toggle (page) == StagePage::none)
                closeStagePage();
            else
                showStagePage (page);
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
                                                      : settingsPage.get();
    const auto frames = stagePageFrames();
    const bool anyPageWasOpen = std::any_of (frames.begin(), frames.end(),
                                             [] (const StagePageFrame* frame) { return frame != nullptr && frame->isVisible(); });

    for (auto* frame : frames)
        if (frame != nullptr)
            frame->setVisible (frame == shown);

    // The GL note layer renders on top of normal painting: hide it while a page covers the note area.
    if (! anyPageWasOpen)
        noteLayerVisibleBeforePage = noteLayer && noteLayer->isVisible();
    if (noteLayer)
        noteLayer->setVisible (false);

    shown->toFront (false);
}

void MainComponent::closeStagePage()
{
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
