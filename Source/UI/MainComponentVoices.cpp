/*
  ==============================================================================

    MainComponentVoices.cpp

    The Voices page (board StageRevised openPage=voices, spec 2026-11-24 voices
    page): the instrument of the lit hand(s) (it was the instrument tree in a
    call-out), the open style's range / split / transpose (Display's fields,
    they were the style settings view) and the SFZ library (it was a panel on
    the Settings page). Split from MainComponentStage.cpp to keep both under
    the file cap (ADR-003).

  ==============================================================================
*/

#include "MainComponent.h"

namespace
{
    namespace VoicesWiringText
    {
        const juce::String title      = "Voices";
        const juce::String editing    = "Editing";
        const juce::String leftHand   = "Left hand";
        const juce::String rightHand  = "Right hand";
        const juce::String choosePart = "Choose a part: Left or Right";
    }
}

void MainComponent::voicesPageInit()
{
    voicesPage = std::make_unique<StagePageFrame> (VoicesWiringText::title);
    voicesPage->onExit = [this] { closeStagePage(); refreshStage(); };
    addChildComponent (*voicesPage);

    // Title row: Editing | Left hand | Right hand - the same two toggles as the panel's Part LEDs.
    voicesEditingLabel.setText (VoicesWiringText::editing);
    voicesLeftHand.setButtonText (VoicesWiringText::leftHand);
    voicesRightHand.setButtonText (VoicesWiringText::rightHand);
    voicesLeftHand.onClick  = [this] { togglePartFromPanel (true); };
    voicesRightHand.onClick = [this] { togglePartFromPanel (false); };
    voicesPage->setActions ({ &voicesEditingLabel, &voicesLeftHand, &voicesRightHand });

    voicesPanel = std::make_unique<VoicesInstrumentPanel>();
    voicesPanel->onInstrumentChosen = [this] (int program) { chooseHandInstrument (program); };
    voicesPage->addAndMakeVisible (*voicesPanel);

    voicesPage->addAndMakeVisible (display->getPlaybackSettings());
    display->onStyleSettings = [this] { openStagePage (StagePage::voices); };

    // The library is loaded later in the constructor; refresh() then shows it.
    sfzLibraryUI = std::make_unique<SFZLibraryUI> (sfzManager);
    sfzLibraryUI->getCurrentStyleId   = [this] { return display != nullptr ? display->getStyleID() : juce::String(); };
    sfzLibraryUI->getCurrentStyleName = [this] { return display != nullptr ? display->getStyleName() : StyleNames::defaultStyle; };
    sfzLibraryUI->onLibraryChanged = [this]
    {
        sfzManager.save (IOHelper::getFile (AppDataNames::sfzLibraryFile));
        if (audioHandler != nullptr && MIDIDevice.isOpenAudioOUT())
            loadSfzForCurrentStyle();
    };
    voicesPage->addAndMakeVisible (*sfzLibraryUI);

    voicesPage->onLayoutContent = [this] (juce::Rectangle<int> area, float scale)
    {
        const auto columns = voicesPageColumns (area, scale);
        voicesPanel->setScale (scale);
        voicesPanel->setBounds (columns.instrument);
        auto& range = display->getPlaybackSettings();
        range.setScale (scale);
        range.setBounds (columns.range);
        sfzLibraryUI->setScale (scale);
        sfzLibraryUI->setBounds (columns.sfz);
    };
    syncVoicesPage();
}

void MainComponent::voicesPageShown()
{
    syncVoicesPage();
    voicesPanel->showOpeningGroup();
    sfzLibraryUI->refresh();   // the open style's assignments
}

void MainComponent::syncVoicesPage()
{
    if (voicesPanel == nullptr)
        return;
    const bool left  = leftHandInstrumentToggle.getToggleState();
    const bool right = rightHandInstrumentToggle.getToggleState();
    voicesLeftHand.setLedOn (left);
    voicesRightHand.setLedOn (right);
    voicesPanel->setHands ({ left, right, midiHandler.getProgramNumberLeftHand(), midiHandler.getProgramNumberRightHand() });
}

void MainComponent::chooseHandInstrument (int programNumber)
{
    // What the instrument tree's click did, for every lit hand: the sound, the style's saved choice, its SFZ.
    const bool left  = leftHandInstrumentToggle.getToggleState();
    const bool right = rightHandInstrumentToggle.getToggleState();
    if (! left && ! right)
    {
        showStageMessage (VoicesWiringText::choosePart);
        return;
    }

    const auto styleId = display ? display->getStyleID() : StyleNames::defaultStyleId;
    auto apply = [this, &styleId, programNumber] (const juce::String& hand, const juce::String& suffix, int channel)
    {
        midiHandler.setProgramNumber (programNumber, hand);   // the recordPlayer follows by listener
        if (propertiesFile)
        {
            propertiesFile->setValue (styleId + suffix, programNumber);
            propertiesFile->saveIfNeeded();
        }
        if (audioHandler != nullptr)
            audioHandler->loadSfz (sfzManager.getSfzForStyleInstrument (styleId, programNumber), channel);
    };
    if (left)
        apply (HandChoiceNames::left, SettingsFileKeys::leftInstrumentSuffix, HandChannels::leftHand);
    if (right)
        apply (HandChoiceNames::right, SettingsFileKeys::rightInstrumentSuffix, HandChannels::rightHand);

    recordPlayer.setProgarmNumber (programNumber); //setting only the value
    syncVoicesPage();
}
