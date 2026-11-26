#include "MainComponent.h"
#include "AppInfo.h"
#include "addDeviceWindow.h"
#include "UI/Stage/StageMetrics.h"
#include "Common/AppColours.h"

namespace
{
    namespace Text
    {
        const juce::String errorTitle         = "ERROR";
        const juce::String okButton           = "OK";
        const juce::String midiOutputFailed   = "Failed to open MIDI output device.";
        const juce::String audioOutputFailed  = "Failed to open audio output device.";
    }

    constexpr int firstSectionNumber = 1;   // "Var" with no number means Var 1
}


void MainComponent::setCallBacksForOverlayWindow()
{
    overlayWindow->onRequestClose = [this]()
    {
        overlayShouldBeVisible = false;
        setOverlayMenuVisible(false);
    };

    overlayWindow->setWindowFlag = [this]()
    {
        overlayShouldBeVisible = false;
    };

    overlayWindow->bringSeparateWindowFront = [this]()
    {
        // Bring the child windows above the overlay without stealing
        // focus.  toFront(true) + grabKeyboardFocus from inside the
        // overlay's mouseDown causes a focus fight: after the handler
        // returns the OS restores activation to the overlay (the
        // clicked native window), leaving both DocumentWindows grey.

    };

    overlayWindow->onSettingsClick = [this]()
    {
        // The Esc menu's Settings opens the Settings page (it used to be the MIDI Settings window).
        setOverlayMenuVisible(false);
        openStagePage(StagePage::settings);
    };

    overlayWindow->onEffectsClick = [this]()
    {
        // The Esc menu's Effects opens the Effects page (it used to be a separate window).
        setOverlayMenuVisible(false);
        openStagePage(StagePage::effects);
    };
}

void MainComponent::initializeOverlay()
{
    // In-app child (not a top-level/OS window): it dies with the app, so a crash can't leave a
    // stuck window behind. It's hidden until ESC; setOverlayMenuVisible() hides the GL note layer
    // while it's up so it isn't covered. Bounds are set in resized().
    overlayWindow = std::make_unique<OverlayComponent>();
    addChildComponent(*overlayWindow);
    overlayWindow->setAlwaysOnTop(true);
    overlayWindow->setOpaque(false);

    overlayShouldBeVisible = false;

    setCallBacksForOverlayWindow();

    loadEffectsFromFile();
}

void MainComponent::setLoadingOverlayVisible(bool show, const juce::String& text)
{
    if (show)
    {
        if (text.isNotEmpty())
            openingAudioLabel.setText(text, juce::dontSendNotification);

        // The GL note layer renders on top of normal painting, so it would cover the centered
        // label. Remember its visibility once, then hide it for the duration of the load.
        if (! loadingOverlayActive)
        {
            noteLayerVisibleBeforeLabel = (noteLayer && noteLayer->isVisible());
            loadingOverlayActive = true;
        }
        if (noteLayer) noteLayer->setVisible(false);
        openingAudioLabel.setVisible(true);
    }
    else
    {
        openingAudioLabel.setVisible(false);
        if (loadingOverlayActive && noteLayer)
        {
            // The animation timer was stopped when the layer was hidden (visibilityChanged),
            // so any notes that were mid-flight would otherwise stay frozen on screen until the
            // next note-on restarts the timer. Clear them, matching the arranger-overlay/menu paths.
            noteLayer->resetState();
            noteLayer->setVisible(noteLayerVisibleBeforeLabel);
        }
        loadingOverlayActive = false;
    }
    repaint();
}

void MainComponent::setArrangerOverlaySceneHidden(bool hidden)
{
    if (hidden == sceneHiddenForArrangerOverlay)
        return;                                  // guard re-entrancy (browser->editor presents twice)
    sceneHiddenForArrangerOverlay = hidden;

    if (hidden)
    {
        noteLayerVisibleBeforeArranger = (noteLayer && noteLayer->isVisible());
        if (noteLayer) noteLayer->setVisible(false);   // would render through the full-screen overlay
    }
    else if (noteLayer)
    {
        noteLayer->setVisible(noteLayerVisibleBeforeArranger);
        noteLayer->resetState();                 // don't return to stale frozen notes
    }
}

void MainComponent::highlightArrangerSection(int sectionIndex, ArrangerSectionType type, const juce::String& name)
{
    const juce::ScopeGuard mirrorOnPanel { [this] { syncSectionLedsFromGroups(); } };   // panel LEDs follow, on every path

    if (!variationsFills || !introsEndings)
        return;

    if (sectionIndex < SectionHighlight::byTypeAndName)   // nothing active (stopped before first play): clear both groups
    {
        variationsFills->setHighlightedButton({});
        introsEndings->setHighlightedButton({});
        return;
    }

    // Map the engine section (e.g. "Variation 2"/"Ending 1") to its live button label by type + number.
    const int    sectionNumber = juce::jmax(firstSectionNumber, name.getTrailingIntValue());
    juce::String buttonName;
    bool         inVariationsFills = true;
    switch (type)
    {
        case ArrangerSectionType::Variation: buttonName = SectionGroupNames::variationPrefix + juce::String(sectionNumber); break;
        case ArrangerSectionType::Fill:      buttonName = SectionGroupNames::fillPrefix      + juce::String(sectionNumber); break;
        case ArrangerSectionType::Break:     buttonName = SectionGroupNames::breakButton;                                  break;
        case ArrangerSectionType::Intro:     buttonName = SectionGroupNames::introPrefix     + juce::String(sectionNumber); inVariationsFills = false; break;
        case ArrangerSectionType::Ending:    buttonName = SectionGroupNames::endingPrefix    + juce::String(sectionNumber); inVariationsFills = false; break;
        case ArrangerSectionType::CountIn:   variationsFills->setHighlightedButton({});
                                             introsEndings->setHighlightedButton({});
                                             return;
    }

    // Highlight in the owning group; clear the other so only one button is ever lit.
    if (inVariationsFills)
    {
        variationsFills->setHighlightedButton(buttonName);
        introsEndings->setHighlightedButton({});
    }
    else
    {
        introsEndings->setHighlightedButton(buttonName);
        variationsFills->setHighlightedButton({});
    }
}

void MainComponent::setSectionButtonsEngineDriven(bool engineDriven)
{
    const juce::ScopeGuard mirrorOnPanel { [this] { syncSectionLedsFromGroups(); } };

    if (variationsFills)
    {
        variationsFills->setEngineDrivenHighlight(engineDriven);
        if (!engineDriven) variationsFills->setHighlightedButton({});
    }
    if (introsEndings)
    {
        introsEndings->setEngineDrivenHighlight(engineDriven);
        if (!engineDriven) introsEndings->setHighlightedButton({});
    }
}

void MainComponent::setOverlayMenuVisible(bool show)
{
    if (overlayWindow == nullptr)
        return;

    if (show)
    {
        overlayWindow->setBounds(getLocalBounds());
        // Hide the GL note layer (would cover the overlay) and the keyboard, remembering both.
        noteLayerVisibleBeforeMenu = (noteLayer && noteLayer->isVisible());
        keyboardVisibleBeforeMenu  = keyboard.isVisible();
        if (noteLayer) noteLayer->setVisible(false);
        keyboard.setVisible(false);

        overlayWindow->setVisible(true);
        overlayWindow->toFront(true);
        overlayWindow->grabKeyboardFocus();
    }
    else
    {
        overlayWindow->setVisible(false);
        if (noteLayer)
        {
            noteLayer->resetState();   // clear leftover falling/active notes so we return to a clean screen
            noteLayer->setVisible(noteLayerVisibleBeforeMenu);
        }
        keyboard.setVisible(keyboardVisibleBeforeMenu);
        grabKeyboardFocus();   // so ESC/keys keep working after the menu closes
    }
}

void MainComponent::createSettingsContent()
{
    // The Settings page's content (it was the MIDI Settings window): created once, with every callback the window got.
    midiWindow = std::make_unique<MIDIWindow>(this->MIDIDevice, devicesIN, devicesOUT, devicesAudioOUT, propertiesFile);

        midiWindow->isMidiDeviceOpen = [this]()
        {
            return !playButton.isVisible();
        };

        midiWindow->onArrangerModeChanged = [this](bool enabled)
        {
            if (display != nullptr)
                display->setArrangerModeEnabled(enabled);
            setSectionButtonsEngineDriven(enabled);
            midiHandler.setArrangerEngaged(enabled);   // gates chord-zone muting (off => normal playing)
        };

        // Phase 4 chord-mode toggles.
        midiWindow->onChordBassInversionChanged = [this](bool on)
        {
            if (display != nullptr)
                display->setArrangerBassInversion(on);
        };
        midiWindow->onChordModeChanged = [this](ChordMode mode)
        {
            midiHandler.setChordMode(mode);
        };
        midiWindow->onChordMemoryChanged = [this](bool on)
        {
            midiHandler.setChordMemory(on);
        };
        midiWindow->onSynchroStartChanged = [this](bool on)
        {
            if (display != nullptr)
                display->setArrangerSynchroStart(on);
        };
        midiWindow->onCountInChanged = [this](bool on)
        {
            if (display != nullptr)
                display->setArrangerCountIn(on);
        };
        midiWindow->onChordZoneMuteChanged = [this](bool on)
        {
            midiHandler.setChordZoneMute(on);   // Korg-style: silence the chord keys when the arranger is engaged
        };

        // Phase 7a NTT controls.
        midiWindow->onNttEnabledChanged = [this](bool on)
        {
            if (display != nullptr)
                display->setArrangerNttEnabled(on);
        };
        midiWindow->onNttMinorScaleChanged = [this](int idx)
        {
            const auto scale = idx == MinorScaleButtons::aeolian ? MinorScaleChoice::Aeolian
                             : idx == MinorScaleButtons::harmonic ? MinorScaleChoice::HarmonicMinor
                                                                  : MinorScaleChoice::Dorian;
            if (display != nullptr)
                display->setArrangerMinorScale(scale);
        };

        midiWindow->onOutputEngineChanged = [this](int engineOption)
        {
            disconnectCurrentOutput();

            if (engineOption == OutputEngineIds::externalMidi)
            {
                if (!MIDIDevice.deviceOpenOUT(MIDIDevice.getDeviceIndexOUT()))
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::errorTitle, Text::midiOutputFailed, Text::okButton);
                    return;
                }
                deviceOpenedOUT = MIDIDevice.getDeviceOUT();

                if (display != nullptr)
                {
                    // Route style/arranger playback to the MIDI device and stop sending to the
                    // (now closed) SFZ sampler. Mirrors the Play-button wiring path; without this,
                    // switching engine mid-session leaves the players with a stale output -> silence.
                    display->setMidiInjectCallback(nullptr);
                    display->setDeviceOutput(MIDIDevice.getDeviceOUT());
                }
            }
            else if (engineOption == OutputEngineIds::internalSfz)
            {
                if (!MIDIDevice.deviceOpenAudioOUT(MIDIDevice.getDeviceIndexAudioOUT()))
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, Text::errorTitle, Text::audioOutputFailed, Text::okButton);
                    return;
                }
                ensureAudioHandlerReady();   // re-wires the SFZ inject callback into the players
                loadSfzForCurrentStyle();

                if (display != nullptr)
                    display->setDeviceOutput({});   // stop sending to the (now closed) MIDI device
            }

            applyCurrentStyleToOutput();
        };

    // Keyboard: the key count stored for the MIDI-in device (0 = PC keyboard / unknown), as "Update key count" did.
    auto identifierOf = [this](const juce::String& deviceName) -> juce::String
    {
        for (const auto& [name, identifier] : MIDIDevice.getAvailableInputDevicesNameIdentifier())
            if (name == deviceName)
                return identifier;
        return {};
    };
    midiWindow->getKeyCount = [this, identifierOf](const juce::String& deviceName)
    {
        const auto identifier = identifierOf(deviceName);
        const auto vid = MIDIDevice.extractVID(identifier);
        const auto pid = MIDIDevice.extractPID(identifier);
        return identifier.isNotEmpty() && dataBase.deviceExists(vid, pid) ? dataBase.getNrKeysPidVid(vid, pid) : int {};
    };
    midiWindow->setKeyCount = [this, identifierOf](const juce::String& deviceName, int keys)
    {
        const auto identifier = identifierOf(deviceName);
        if (identifier.isNotEmpty())
            dataBase.updateDeviceJson(MIDIDevice.extractVID(identifier), MIDIDevice.extractPID(identifier), {}, keys);
    };
    midiWindow->onAddDevice = [this, identifierOf](const juce::String& deviceName)
    {
        const auto identifier = identifierOf(deviceName);
        if (identifier.isEmpty())
            return;   // the PC keyboard: nothing to add
        const auto vid = MIDIDevice.extractVID(identifier);
        const auto pid = MIDIDevice.extractPID(identifier);
        addDeviceWindow = std::make_unique<AddDeviceWindow>(vid, pid);   // today's Add MIDI Device window
        addDeviceWindow->onAddDevice = [this, vid, pid](const juce::String& name, int keys)
        {
            dataBase.addDeviceJson(vid, pid, name, keys);
            if (midiWindow)
                midiWindow->refreshKeyboardField();
        };
        addDeviceWindow->setVisible(true);
    };
}
