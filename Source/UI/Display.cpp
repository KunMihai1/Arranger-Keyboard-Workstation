#include "displayGUI.h"

namespace
{
    constexpr int    firstTabIndex       = 0;
    constexpr int    nextTab             = 1;    // the style tab sits right after the Styles list
    constexpr int    lastTabOffset       = 1;    // getNumTabs() - 1 is the last tab
    constexpr int    tabBeforeLastOffset = 2;
    constexpr int    tabNotFound         = -1;
    constexpr int    allTracksIndex      = 0;    // CurrentStyleComponent::comboBoxChangeIndex
    constexpr int    lowestPianoNote     = 21;   // A0, the lowest key of an 88-key piano
    constexpr int    highestPianoNote    = 108;  // C8
    constexpr double firstSectionStartSeconds = 4.0;
    constexpr double sectionSeconds      = 10.0;
    constexpr int    firstSectionNumber  = 1;
    const juce::String sectionIdSeparator = "_";    // "intro_1"
    const juce::String midiFilePattern    = "*.mid";
    const juce::String saveLogged         = "save happens";
}

Display::Display(std::weak_ptr<juce::MidiOutput> outputDev, juce::PropertiesFile* props, int widthForList) : outputDevice{outputDev}, propertiesFile{props}
{
    availableTracksFromFolder = std::make_shared<std::deque<TrackEntry>>();
    groupedTracks = std::make_shared<std::unordered_map<juce::String, std::deque<TrackEntry>>>();
    groupedTrackKeys = std::make_shared<std::vector<juce::String>>();
    sectionsPerStyleMap = std::make_shared<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>>();

    tabComp = std::make_unique<MyTabbedComponent>(juce::TabbedButtonBar::TabsAtLeft);
    temporaryTabs = std::make_unique<TemporaryTabs>(*tabComp);
    temporaryTabs->onTracksClosed = [this]()
    {
        if (trackListComp)
            trackListComp->removeListener(this);
        trackListComp.reset();
    };
    temporaryTabs->onPlaybackSettingsClosed = [this]() { playBackSettings.reset(); };


    addAndMakeVisible(tabComp.get());

    initializeAllStyles();
    loadAllStyles();

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);


    if (!jsonFile.exists())
    {
        juce::var emptyArray = juce::Array<juce::var>{};
        juce::String jsonString = juce::JSON::toString(emptyArray);

        jsonFile.replaceWithText(jsonString);
    }

    auto jsonFileSections = IOHelper::getFile(AppDataNames::sectionsFile);

    if (!jsonFileSections.exists())
    {
        auto* emptyObj = new juce::DynamicObject();
        juce::var jsonVar(emptyObj);

        juce::String jsonString = juce::JSON::toString(jsonVar);
        jsonFileSections.replaceWithText(jsonString);
    }


    SectionIOHelper::loadFromFile(jsonFileSections, *sectionsPerStyleMap);

    TrackIOHelper::loadFromFile(jsonFile, *groupedTracks, *groupedTrackKeys);


    mapUuidToTrack = buildTrackUuidMap();

    std::vector<juce::String> stylesNames = getAllStylesFromJson();

    auto list = std::make_unique<StylesListComponent>(stylesNames,
        [this](const juce::String& name) {
            showCurrentStyleTab(name);
        },
        widthForList
    );

    list->onStyleRename = [this](const juce::String& oldName, const juce::String& newName)
    {
        int index = tabComp->getCurrentTabIndex();
        if (index>=firstTabIndex && index + nextTab < tabComp->getNumTabs())
        {
            tabComp->setTabName(index + nextTab, newName);

            currentStyleComponent->updateName(newName);
        }

        updateStyleNameInJson(oldName, newName);
    };

    list->onStyleAdd = [this](const juce::String& newName)
    {
        appendNewStyleInJson(newName);
    };

    list->onStyleRemove = [this](const juce::String& name)
    {
        if (tabComp)
        {
            int currentTabIndex = tabComp->getCurrentTabIndex();
            if (currentTabIndex + nextTab <= tabComp->getNumTabs())
            {
                juce::String tabName = tabComp->getTabNames()[currentTabIndex + nextTab];
                if (tabName == name)
                {
                    tabComp->removeTab(currentTabIndex + nextTab);
                    currentStyleComponent.reset();
                    created = false;
                }
            }
        }
        removeStyleInJson(name);
    };

    //auto* scrollableView = new juce::Viewport();
    //scrollableView->setScrollBarsShown(true, false); // only vertical
    //scrollableView->setViewedComponent(list, true);
    tabComp->addTab(StylePanelTabs::styles, AppColours::stylePanelTab, list.get(), false);

    stylesListComponent = std::move(list);
    //list->resized();
    //scrollableView->getViewedComponent()->resized();

    tabComp->onTabChanged = [this](int newIndex, juce::String tabName)
    {
        temporaryTabs->tabChanged(newIndex, tabName);
    };
}

Display::~Display()
{

}

void Display::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    auto* tabbedCast = dynamic_cast<juce::TabbedComponent*>(source);
    if (tabbedCast == tabComp.get())
    {
        int currentIndex = tabComp->getCurrentTabIndex();
        int tracksTabIndex = getTabIndexByName(StylePanelTabs::myTracks);

        if (tracksTabIndex >= firstTabIndex && currentIndex != tracksTabIndex)
        {
            tabComp->removeTab(tracksTabIndex);
        }
    }
}

juce::String Display::getStyleID() const
{
    if (currentStyleComponent)
        return this->currentStyleComponent->getStyleID();
    else return StyleNames::defaultStyleId;
}

juce::String Display::getStyleName() const
{
    if (currentStyleComponent)
        return this->currentStyleComponent->getName();
    else return StyleNames::defaultStyle;
}

int Display::getTabIndexByName(const juce::String& name)
{
    auto& tabBar = tabComp->getTabbedButtonBar();

    for (int tabIndex {}; tabIndex < tabComp->getNumTabs(); ++tabIndex)
    {
        if (tabBar.getTabButton(tabIndex)->getName() == name)
            return tabIndex;
    }
    return tabNotFound;
}

void Display::showCurrentStyleTab(const juce::String& name)
{
    if (!created)
    {
        std::weak_ptr<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>> weakSectionsMap = sectionsPerStyleMap;

        currentStyleComponent = std::make_unique<CurrentStyleComponent>(name, mapUuidToTrack, outputDevice,weakSectionsMap);
        currentStyleComponent->onRequestTrackSelectionFromTrack = [this](std::function<void(const juce::String&, const juce::Uuid& uuid, const juce::String& type)> trackChosenCallback)
        {
            showListOfTracksToSelectFrom(trackChosenCallback);
        };

        currentStyleComponent->updateTrackFile = [this]()
        {
            auto appDataFolder = IOHelper::getAppDataFolder();

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
        };

        currentStyleComponent->keybindTabStarting = [this]()
        {

            int existingIndex = getTabIndexByName(StylePanelTabs::playbackSettings);

            if (existingIndex >= firstTabIndex)
            {
                tabComp->setCurrentTabIndex(existingIndex);
                return;
            }


            playBackSettings = std::make_unique<PlayBackSettingsComponent>(minNote, maxNote, this->settings);
            playBackSettings->getStyleID = [this]() { return this->getStyleID(); };
            playBackSettings->setBounds(getLocalBounds());

            playBackSettings->onChangingSettings = [this](PlayBackSettings newSettings)
            {
                // Save settings using PropertiesFile
                PlaybackSettingsIOHelper::savePlaybackSettings(propertiesFile, this->settings, minNote, maxNote, this->getStyleID());
                displayListeners.call(&DisplayListener::playBackSettingsChanged, this->settings);
            };

            playBackSettings->onChangingTranspose = [this](int transposeValue)
            {
                displayListeners.call(&DisplayListener::playBackSettingsTransposeChanged, transposeValue);
            };

            temporaryTabs->playbackSettingsOpened();
            tabComp->addTab(StylePanelTabs::playbackSettings, AppColours::stylePanelTab, playBackSettings.get(), false);

            existingIndex = tabComp->getNumTabs() - lastTabOffset;


            tabComp->setCurrentTabIndex(existingIndex);
        };

        tabComp->addTab(name, AppColours::stylePanelTab, currentStyleComponent.get(), false);
        created = true;

        if (pendingMidiInjectCallback)
            currentStyleComponent->setMidiInjectCallback(pendingMidiInjectCallback);

        currentStyleComponent->setArrangerModeEnabled(arrangerModeEnabled);
        currentStyleComponent->setArrangerAutoFillEnabled(arrangerAutoFillEnabled);
        currentStyleComponent->setArrangerBassInversion(arrangerBassInversion);
        currentStyleComponent->setArrangerNttEnabled(arrangerNttEnabled);
        currentStyleComponent->setArrangerMinorScale(arrangerMinorScale);
        currentStyleComponent->setSynchroStartEnabled(arrangerSynchroStart);
        currentStyleComponent->setCountInEnabled(arrangerCountIn);
    }
    else
    {
        for (auto& track : currentStyleComponent->getAllTracks())
            track->removeListener(currentStyleComponent->getTrackPlayer());

        int index = tabComp->getNumTabs() - lastTabOffset;
        tabComp->getTabbedButtonBar().setTabName(index, name);
        currentStyleComponent->updateName(name);
    }

    currentStyleComponent->anyTrackChanged = [this, name]()
    {
        updateStyleInJson(name);
    };

    currentStyleComponent->onAuthoringOverlayVisible = [this](bool visible)
    {
        if (onArrangerOverlayVisible)
            onArrangerOverlayVisible(visible);
    };

    currentStyleComponent->onRequestPlayFocus = [this]()
    {
        if (onRequestPlayFocus)
            onRequestPlayFocus();
    };

    currentStyleComponent->getHeldChord = [this]() -> ArrangerChord
    {
        return getHeldChord ? getHeldChord() : ArrangerChord{};
    };

    currentStyleComponent->onBusy = [this](bool show, const juce::String& text)
    {
        if (onArrangerBusy)
            onArrangerBusy(show, text);
    };

    currentStyleComponent->onArrangerSectionChanged = [this](int idx, ArrangerSectionType type, juce::String name)
    {
        if (onArrangerSectionChanged)
            onArrangerSectionChanged(idx, type, name);
    };

    currentStyleComponent->onBeatPosition = [this](double elapsedBeats, int beatsPerBar)
    {
        if (onBeatPosition)
            onBeatPosition(elapsedBeats, beatsPerBar);
    };

    currentStyleComponent->tabExsitsCallback = [this](const juce::String& name)
    {
        return this->existsTab(name);
    };

    if (allStylesJsonVar.isObject())
    {
        auto stylesArray = allStylesJsonVar[juce::Identifier { AllStylesJsonKeys::styles }];
        if (stylesArray.isArray())
        {
            for (const auto& style : *stylesArray.getArray())
            {
                if (auto* obj = style.getDynamicObject())
                {
                    if (obj->getProperty(AllStylesJsonKeys::name).toString() == name)
                    {
                        currentStyleComponent->loadJson(style);
                        break;
                    }
                }
            }
        }
    }
    tabComp->setCurrentTabIndex(tabComp->getNumTabs() - lastTabOffset);

    juce::String styleID = currentStyleComponent->getStyleID();

    if ((*sectionsPerStyleMap)[styleID].empty())
    {
        initializeSectionsForStyle((*sectionsPerStyleMap)[styleID]);
        SectionIOHelper::saveToFile(IOHelper::getFile(AppDataNames::sectionsFile), *sectionsPerStyleMap);
    }

    double bpmToUse = currentStyleComponent->getTempo();

    //aici dam load la efecte si tot ce tine de acest stil

    if (loadSettingsOnStyleChange)
        loadSettingsOnStyleChange(styleID);

    readPlaybackSettingsFromProperties();
    if (playBackSettings)
    {
        playBackSettings->setNewSettings(this->settings);
    }

    currentStyleComponent->applyBPMchangeBeforePlayback(bpmToUse);
    currentStyleComponent->applyChangesForAllTracksCurrentStyle();
}

void Display::showListOfTracksToSelectFrom(std::function<void(const juce::String&, const juce::Uuid& uuid, const juce::String& type)> onTrackSelected)
{
    trackListComp = std::make_unique<TrackListComponent>(availableTracksFromFolder, groupedTracks, groupedTrackKeys,
        [this, onTrackSelected](int index)
        {
            auto& track = (*availableTracksFromFolder)[index];
            juce::String typeString = TrackTypeConversion::toString(track.type);

            onTrackSelected(track.getDisplayName(), track.getUniqueID(), typeString);

            // No map update here: `track` lives in the picker's copy of the open folder, which the
            // next folder open overwrites. The map already points at the library entry
            // (buildTrackUuidMap at startup, addToMapOnAdding for new tracks).

            if (currentStyleComponent)
            {
                //currentStyleComponent->applyBPMchangeForOne(currentStyleComponent->getTempo(), track.getUniqueID());
                currentStyleComponent->applyBPMchangeBeforePlayback(currentStyleComponent->getTempo(),true);
            }

            if (trackListComp)
                trackListComp->removeListener(this);
            int styleTabIndex = tabComp->getNumTabs() - tabBeforeLastOffset;
            if (styleTabIndex >= firstTabIndex)
            {
                DBG(saveLogged);
                updateStyleInJson(currentStyleComponent->getName());
                tabComp->setCurrentTabIndex(styleTabIndex);
            }
        });

    trackListComp->addListener(this);

    // Removing entries shifts or destroys the library's deque elements, so the UUID -> entry map is
    // rebuilt (assigned in place: CurrentStyleComponent holds a reference to this same map).
    trackListComp->onRemoveTrack = [this](const juce::Uuid& uuid)
    {
        removeTrackFromAllStyles(uuid);
        mapUuidToTrack = buildTrackUuidMap();
    };

    trackListComp->onRemoveMultipleTracks = [this](const std::vector<juce::Uuid>& uuids)
    {
        removeTracksFromAllStyles(uuids);
        mapUuidToTrack = buildTrackUuidMap();
    };

    trackListComp->onRenameTrackFromList = [this](const juce::Uuid& uuid, const juce::String& newName)
    {
        updateTrackNameFromAllStyles(uuid, newName);
    };


    trackListComp->addToMapOnAdding= [this](TrackEntry* newEntry)
    {
        mapUuidToTrack[newEntry->getUniqueID()] = newEntry;
    };

    tabComp->addTab(StylePanelTabs::myTracks, AppColours::stylePanelTab, trackListComp.get(), false);
    tabComp->setCurrentTabIndex(tabComp->getNumTabs() - lastTabOffset);

    temporaryTabs->tracksOpened();
}

std::vector<TrackEntry> Display::getAvailableTracksFromFolder(const juce::File& folder)
{
    std::vector<TrackEntry> tracks;

    if (!folder.exists() || !folder.isDirectory())
        return tracks;

    juce::DirectoryIterator iter(folder, false, midiFilePattern, juce::File::findFiles);

    while (iter.next())
    {
        juce::File trackFile = iter.getFile();
        TrackEntry tr;
        tr.file = trackFile;
        tracks.push_back(tr);
    }

    return tracks;
}

void Display::setDeviceOutput(std::weak_ptr<juce::MidiOutput> devOutput)
{
    this->outputDevice = devOutput;
    if (currentStyleComponent)
        currentStyleComponent->setDeviceOutputCurrentStyle(devOutput);
}

void Display::setMidiInjectCallback(std::function<void(const juce::MidiMessage&)> cb)
{
    pendingMidiInjectCallback = cb;
    if (currentStyleComponent)
        currentStyleComponent->setMidiInjectCallback(std::move(cb));
}

void Display::setArrangerModeEnabled(bool shouldEnable)
{
    arrangerModeEnabled = shouldEnable;
    if (currentStyleComponent)
        currentStyleComponent->setArrangerModeEnabled(shouldEnable);
}

void Display::setArrangerAutoFillEnabled(bool enabled)
{
    arrangerAutoFillEnabled = enabled;
    if (currentStyleComponent)
        currentStyleComponent->setArrangerAutoFillEnabled(enabled);
}

void Display::setArrangerLiveChord(const ArrangerChord& chord)
{
    if (currentStyleComponent)
        currentStyleComponent->setLiveChord(chord);
}

void Display::setArrangerBassInversion(bool shouldInvert)
{
    arrangerBassInversion = shouldInvert;   // remembered, re-applied when currentStyleComponent is rebuilt
    if (currentStyleComponent)
        currentStyleComponent->setArrangerBassInversion(shouldInvert);
}

void Display::setArrangerNttEnabled(bool enabled)
{
    arrangerNttEnabled = enabled;   // remembered, re-applied when currentStyleComponent is rebuilt
    if (currentStyleComponent)
        currentStyleComponent->setArrangerNttEnabled(enabled);
}

void Display::setArrangerMinorScale(MinorScaleChoice choice)
{
    arrangerMinorScale = choice;   // remembered, re-applied when currentStyleComponent is rebuilt
    if (currentStyleComponent)
        currentStyleComponent->setArrangerMinorScale(choice);
}

void Display::setArrangerSynchroStart(bool enabled)
{
    arrangerSynchroStart = enabled;   // remembered, re-applied when currentStyleComponent is rebuilt
    if (currentStyleComponent)
        currentStyleComponent->setSynchroStartEnabled(enabled);
}

void Display::setArrangerCountIn(bool enabled)
{
    arrangerCountIn = enabled;   // remembered, re-applied when currentStyleComponent is rebuilt
    if (currentStyleComponent)
        currentStyleComponent->setCountInEnabled(enabled);
}

MultipleTrackPlayer* Display::getTrackPlayer()
{
    if (currentStyleComponent)
        return currentStyleComponent->getTrackPlayer();
    return nullptr;
}

std::vector<CurrentStyleComponent::TrackChannelInstrument> Display::getTrackChannelInstruments() const
{
    if (currentStyleComponent)
        return currentStyleComponent->getTrackChannelInstruments();
    return {};
}

void Display::stoppingPlayer()
{
    if (currentStyleComponent)
        this->currentStyleComponent->triggerStopClick();
}

void Display::startingPlayer()
{
    if (currentStyleComponent)
        this->currentStyleComponent->triggerStartClick();
}

void Display::updateUIbeforeAnyLoadingCase()
{
    //if (this->mapNameToTrack.empty())
    //    mapNameToTrack = trackListComp->buildTrackNameMap();
}

void Display::set_min_max(int min, int max)
{
    if (min < lowestPianoNote)
        this->minNote = lowestPianoNote;
    else this->minNote = min;

    if (max > highestPianoNote)
        this->maxNote = highestPianoNote;
    else this->maxNote = max;
    if (playBackSettings)
        this->playBackSettings->setLowestHighest(minNote, maxNote);
}

void Display::set_VID_PID(const juce::String& VID, const juce::String& PID)
{
    this->settings.VID = VID;
    this->settings.PID = PID;
}

juce::String Display::getVID()
{
    return this->settings.VID;
}

juce::String Display::getPID()
{
    return this->settings.PID;
}

void Display::readPlaybackSettingsFromProperties()
{
    PlayBackSettings settingsLoaded = PlaybackSettingsIOHelper::loadPlaybackSettings(propertiesFile, this->settings.VID, this->settings.PID, this->getStyleID());
    this->settings = settingsLoaded;
}

void Display::homeButtonInteraction()
{
    temporaryTabs->homePressed();
    this->currentStyleComponent->comboBoxChangeIndex(allTracksIndex);
    this->playBackSettings.reset();
}

int Display::getStartNote()
{
    return this->settings.startNote;
}

int Display::getEndNote()
{
    return this->settings.endNote;
}

int Display::getLeftBound()
{
    return this->settings.leftHandBound;
}

int Display::getRightBound()
{
    return this->settings.rightHandBound;
}

void Display::addListener(DisplayListener* listener)
{
    displayListeners.add(listener);
}

void Display::removeListener(DisplayListener* listener)
{
    displayListeners.remove(listener);
}

int Display::getNumTabs()
{
    return this->tabComp->getNumTabs();
}

void Display::setNewSettingsHelperFunction(int value)
{
    if(settings.startNote!=PlaybackSettingsKeys::notSet)
        settings.startNote += value;

    if(settings.endNote!=PlaybackSettingsKeys::notSet)
        settings.endNote += value;

    if(settings.leftHandBound!=PlaybackSettingsKeys::notSet)
        settings.leftHandBound += value;

    if(settings.rightHandBound!=PlaybackSettingsKeys::notSet)
        settings.rightHandBound += value;

    if (settings.startNote < lowestPianoNote && settings.startNote!=PlaybackSettingsKeys::notSet)
        settings.startNote = lowestPianoNote;

    if (settings.endNote > highestPianoNote && settings.endNote!=PlaybackSettingsKeys::notSet)
        settings.endNote = highestPianoNote;

    if (settings.leftHandBound < lowestPianoNote)
        settings.leftHandBound = lowestPianoNote;

    if (settings.rightHandBound > highestPianoNote)
        settings.rightHandBound = highestPianoNote;

    if(playBackSettings)
        this->playBackSettings->setNewSettings(settings);
    else displayListeners.call(&DisplayListener::playBackSettingsChanged, this->settings);
}

bool Display::existsTab(const juce::String& name)
{
    for (auto& tabName : tabComp->getTabNames())
    {
        if (tabName == name)
            return true;
    }
    return false;
}

void Display::initializeSectionsForStyle(std::unordered_map<juce::String,StyleSection>& sections)
{
    sections.clear();

    // Saved to mySections.json: the names and ids must not change.
    static const std::vector<std::vector<juce::String>> names = {
        { "Intro 1", "Intro 2", "Intro 3" },
        { "Var 1", "Var 2", "Var 3", "Var 4" },
        { "Fill 1", "Fill 2", "Fill 3", "Fill 4" },
        { "Ending 1", "Ending 2", "Ending 3" },
        { "Break" }
    };

    static const std::vector<juce::String> categories = { "Intro", "Variation", "Fill", "Ending", "Break" };

    int sectionCount {};

    for (int groupIndex {}; groupIndex < names.size(); groupIndex++)
    {
        for (int sectionIndex {}; sectionIndex < names[groupIndex].size(); sectionIndex++)
        {
            StyleSection section;
            section.id = categories[groupIndex].toLowerCase() + sectionIdSeparator + juce::String(sectionIndex + firstSectionNumber);
            section.name = names[groupIndex][sectionIndex];
            section.startTimeSeconds = firstSectionStartSeconds + sectionCount * sectionSeconds;
            section.endTimeSeconds = section.startTimeSeconds + sectionSeconds;
            section.startBar = StyleSection::barNotSet;
            section.endBar = StyleSection::barNotSet;

            sections[section.name] = section;
            sectionCount++;
        }
    }
}

void Display::handleIntroDisplay(const juce::String& name)
{
    if (currentStyleComponent)
    {
        juce::String styleID = currentStyleComponent->getStyleID();
        currentStyleComponent->handleIntroCurrentStyle(name,(* sectionsPerStyleMap)[styleID]);
    }
}

void Display::handleEndingDisplay(const juce::String& name)
{
    if (currentStyleComponent)
    {
        juce::String styleID = currentStyleComponent->getStyleID();
        currentStyleComponent->handleEndingCurrentStyle(name, (*sectionsPerStyleMap)[styleID]);
    }
}

void Display::handleVarDisplay(const juce::String& name)
{
    if (currentStyleComponent)
    {
        juce::String styleID = currentStyleComponent->getStyleID();
        currentStyleComponent->handleVarCurrentStyle(name, (*sectionsPerStyleMap)[styleID]);
    }
}

void Display::handleFillDisplay(const juce::String& name)
{
    if (currentStyleComponent)
    {
        juce::String styleID = currentStyleComponent->getStyleID();
        currentStyleComponent->handleFillCurrentStyle(name, (*sectionsPerStyleMap)[styleID]);
    }
}

void Display::handleBreakDisplay(const juce::String& name)
{
    if (currentStyleComponent)
    {
        juce::String styleID = currentStyleComponent->getStyleID();
        currentStyleComponent->handleBreakCurrentStyle(name, (*sectionsPerStyleMap)[styleID]);
    }
}

void Display::callingListeners()
{
    displayListeners.call(&DisplayListener::playBackSettingsChanged, this->settings);
}

void Display::resized()
{
    tabComp->setBounds(getLocalBounds());

}
