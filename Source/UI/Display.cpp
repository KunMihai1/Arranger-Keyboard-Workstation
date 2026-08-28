#include "displayGUI.h"

Display::Display(std::weak_ptr<juce::MidiOutput> outputDev, juce::PropertiesFile* props, int widthForList) : outputDevice{outputDev}, propertiesFile{props}
{
    availableTracksFromFolder = std::make_shared<std::deque<TrackEntry>>();
    groupedTracks = std::make_shared<std::unordered_map<juce::String, std::deque<TrackEntry>>>();
    groupedTrackKeys = std::make_shared<std::vector<juce::String>>();
    sectionsPerStyleMap = std::make_shared<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>>();

    tabComp = std::make_unique<MyTabbedComponent>(juce::TabbedButtonBar::TabsAtLeft);


    addAndMakeVisible(tabComp.get());

    initializeAllStyles();
    loadAllStyles();

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile("myTracks.json");


    if (!jsonFile.exists())
    {
        juce::var emptyArray = juce::Array<juce::var>{};
        juce::String jsonString = juce::JSON::toString(emptyArray);

        jsonFile.replaceWithText(jsonString);
    }

    auto jsonFileSections = IOHelper::getFile("mySections.json");

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
        if (index>=0 && index + 1 < tabComp->getNumTabs())
        {
            tabComp->setTabName(index + 1, newName);

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
            if (currentTabIndex + 1 <= tabComp->getNumTabs())
            {
                juce::String tabName = tabComp->getTabNames()[currentTabIndex + 1];
                if (tabName == name)
                {
                    tabComp->removeTab(currentTabIndex + 1);
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
    tabComp->addTab("Styles", juce::Colour::fromRGB(10, 15, 10), list.get(), false);

    stylesListComponent = std::move(list);
    //list->resized();
    //scrollableView->getViewedComponent()->resized();

    tabComp->onTabChanged = [this](int newIndex, juce::String tabName)
    {
        if (createdTracksTab && tabName != "My tracks")
        {
            if (trackListComp)
            {
                trackListComp->removeListener(this);
            }
            int myTracksIndex = tabComp->getNumTabs() - 1;
            tabComp->removeTab(myTracksIndex);

            trackListComp.reset();
            createdTracksTab = false;
            return;
        }

        if (createdPlaybackSettingsTab && newIndex==0)
        {
            int mySettingsIndex = tabComp->getNumTabs() - 1;
            tabComp->removeTab(mySettingsIndex);

            playBackSettings.reset();
            createdPlaybackSettingsTab = false;
            return;
        }


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
        int tracksTabIndex = getTabIndexByName("My tracks");

        if (tracksTabIndex >= 0 && currentIndex != tracksTabIndex)
        {
            tabComp->removeTab(tracksTabIndex);
        }
    }
}

juce::String Display::getStyleID() const
{
    if (currentStyleComponent)
        return this->currentStyleComponent->getStyleID();
    else return "style_default";
}

juce::String Display::getStyleName() const
{
    if (currentStyleComponent)
        return this->currentStyleComponent->getName();
    else return "DEFAULT Style";
}

int Display::getTabIndexByName(const juce::String& name)
{
    auto& tabBar = tabComp->getTabbedButtonBar();

    for (int i = 0; i < tabComp->getNumTabs(); ++i)
    {
        if (tabBar.getTabButton(i)->getName() == name)
            return i;
    }
    return -1;
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

            auto jsonFile = IOHelper::getFile("myTracks.json");
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
        };

        currentStyleComponent->keybindTabStarting = [this]()
        {

            int existingIndex = getTabIndexByName("Playback Settings");

            if (existingIndex >= 0)
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

            createdPlaybackSettingsTab = true;
            tabComp->addTab("Playback Settings", juce::Colour::fromRGB(10, 15, 10), playBackSettings.get(), false);

            existingIndex = tabComp->getNumTabs() - 1;


            tabComp->setCurrentTabIndex(existingIndex);
        };

        tabComp->addTab(name, juce::Colour::fromRGB(10, 15, 10), currentStyleComponent.get(), false);
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

        int index = tabComp->getNumTabs() - 1;
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

    currentStyleComponent->tabExsitsCallback = [this](const juce::String& name)
    {
        return this->existsTab(name);
    };

    if (allStylesJsonVar.isObject())
    {
        auto stylesArray = allStylesJsonVar["styles"];
        if (stylesArray.isArray())
        {
            for (const auto& style : *stylesArray.getArray())
            {
                if (auto* obj = style.getDynamicObject())
                {
                    if (obj->getProperty("name").toString() == name)
                    {
                        currentStyleComponent->loadJson(style);
                        break;
                    }
                }
            }
        }
    }
    tabComp->setCurrentTabIndex(tabComp->getNumTabs() - 1);

    juce::String styleID = currentStyleComponent->getStyleID();

    if ((*sectionsPerStyleMap)[styleID].empty())
    {
        initializeSectionsForStyle((*sectionsPerStyleMap)[styleID]);
        SectionIOHelper::saveToFile(IOHelper::getFile("mySections.json"), *sectionsPerStyleMap);
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

            mapUuidToTrack[track.getUniqueID()] = &track;

            if (currentStyleComponent)
            {
                //currentStyleComponent->applyBPMchangeForOne(currentStyleComponent->getTempo(), track.getUniqueID());
                currentStyleComponent->applyBPMchangeBeforePlayback(currentStyleComponent->getTempo(),true);
            }

            if (trackListComp)
                trackListComp->removeListener(this);
            int in = tabComp->getNumTabs() - 2;
            if (in >= 0)
            {
                DBG("save happens");
                updateStyleInJson(currentStyleComponent->getName());
                tabComp->setCurrentTabIndex(in);
            }
        });

    trackListComp->addListener(this);

    trackListComp->onRemoveTrack = [this](const juce::Uuid& uuid)
    {
        removeTrackFromAllStyles(uuid);
    };

    trackListComp->onRemoveMultipleTracks = [this](const std::vector<juce::Uuid>& uuids)
    {
        removeTracksFromAllStyles(uuids);
    };

    trackListComp->onRenameTrackFromList = [this](const juce::Uuid& uuid, const juce::String& newName)
    {
        updateTrackNameFromAllStyles(uuid, newName);
    };


    trackListComp->addToMapOnAdding= [this](TrackEntry* newEntry)
    {
        mapUuidToTrack[newEntry->getUniqueID()] = newEntry;
    };

    tabComp->addTab("My tracks", juce::Colour::fromRGB(10, 15, 10), trackListComp.get(), false);
    tabComp->setCurrentTabIndex(tabComp->getNumTabs() - 1);

    createdTracksTab = true;
}

std::vector<TrackEntry> Display::getAvailableTracksFromFolder(const juce::File& folder)
{
    std::vector<TrackEntry> tracks;

    if (!folder.exists() || !folder.isDirectory())
        return tracks;

    juce::DirectoryIterator iter(folder, false, "*.mid", juce::File::findFiles);

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
    if (min < 21)
        this->minNote = 21;
    else this->minNote = min;

    if (max > 108)
        this->maxNote = 108;
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
    tabComp->removeTab(tabComp->getNumTabs() - 1);
    tabComp->setCurrentTabIndex(tabComp->getNumTabs() - 1);
    this->currentStyleComponent->comboBoxChangeIndex(0);
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
    if(settings.startNote!=-1)
        settings.startNote += value;

    if(settings.endNote!=-1)
        settings.endNote += value;

    if(settings.leftHandBound!=-1)
        settings.leftHandBound += value;

    if(settings.rightHandBound!=-1)
        settings.rightHandBound += value;

    if (settings.startNote < 21 && settings.startNote!=-1)
        settings.startNote = 21;

    if (settings.endNote > 108 && settings.endNote!=-1)
        settings.endNote = 108;

    if (settings.leftHandBound < 21)
        settings.leftHandBound = 21;

    if (settings.rightHandBound > 108)
        settings.rightHandBound = 108;

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

    double startTime = 4.0;
    double duration = 10.0;

    std::vector<std::vector<juce::String>> names = {
        { "Intro 1", "Intro 2", "Intro 3" },
        { "Var 1", "Var 2", "Var 3", "Var 4" },
        { "Fill 1", "Fill 2", "Fill 3", "Fill 4" },
        { "Ending 1", "Ending 2", "Ending 3" },
        { "Break" }
    };

    std::vector<juce::String> categories = { "Intro", "Variation", "Fill", "Ending", "Break" };

    int c= 0;

    for (int i = 0; i < names.size(); i++)
    {
        for (int j = 0; j < names[i].size(); j++)
        {
            StyleSection s;
            s.id = categories[i].toLowerCase() + "_" + juce::String(j + 1);
            s.name = names[i][j];
            s.startTimeSeconds = startTime + c * duration;
            s.endTimeSeconds = s.startTimeSeconds + duration;
            s.startBar = -1;
            s.endBar = -1;

            sections[s.name] = s;
            c++;
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

void Display::removeTrackFromAllStyles(const juce::Uuid& uuid)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar["styles"];

    if (!stylesArray.isArray())
        return;

    for (auto& styleVariable : *stylesArray.getArray())
    {
        auto* styleObj = styleVariable.getDynamicObject();

        if (styleObj == nullptr)
            continue;

        juce::var trackVariable = styleObj->getProperty("tracks");

        if (!trackVariable.isArray())
            continue;

        auto* trackArray = trackVariable.getArray();

        for (auto& trackVar : *trackArray)
        {
            auto* trackObj = trackVar.getDynamicObject();

            if (trackObj == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty("uuid").toString();
            if (uuidString == uuid.toString())
            {
                trackObj->setProperty("uuid", "noID");
                trackObj->setProperty("name", "None");
                trackObj->setProperty("instrumentNumber", -1);
                trackObj->setProperty("type", "None");
            }
        }
    }

    if (currentStyleComponent)
    {
        currentStyleComponent->removingTrack(uuid);
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile("allStyles.json");

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::removeTracksFromAllStyles(const std::vector<juce::Uuid>& uuids)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar["styles"];

    if (!stylesArray.isArray())
        return;

    std::unordered_set<juce::String> uuidSet;
    for (const auto& uuid : uuids)
        uuidSet.insert(uuid.toString());

    for (auto& styleVariable : *stylesArray.getArray())
    {
        auto* styleObj = styleVariable.getDynamicObject();
        if (styleObj == nullptr)
            continue;

        juce::var trackVariable = styleObj->getProperty("tracks");

        if (!trackVariable.isArray())
            continue;

        for (auto& trackVar : *trackVariable.getArray())
        {
            auto* trackObj = trackVar.getDynamicObject();
            if (trackObj == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty("uuid").toString();
            if (uuidSet.count(uuidString))
            {
                trackObj->setProperty("uuid", "noID");
                trackObj->setProperty("name", "None");
                trackObj->setProperty("instrumentNumber", -1);
                trackObj->setProperty("type", "None");
            }
        }
    }

    if (currentStyleComponent)
    {
        currentStyleComponent->removingTracks(uuids);
    }

    juce::File appDataFolder = IOHelper::getAppDataFolder();

    juce::File jsonFile = appDataFolder.getChildFile("allStyles.json");

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::updateTrackNameFromAllStyles(const juce::Uuid& uuid, const juce::String& newName)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar["styles"];

    if (!stylesArray.isArray())
        return;

    for (auto& styleVar : *stylesArray.getArray())
    {
        auto* styleObj = styleVar.getDynamicObject();

        if (styleObj == nullptr)
            continue;

        juce::var trackVariable = styleObj->getProperty("tracks");

        if (!trackVariable.isArray())
            continue;

        for (auto& trackVar : *trackVariable.getArray())
        {
            auto* trackObj = trackVar.getDynamicObject();
            if (trackObj == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty("uuid").toString();
            if (uuidString == uuid.toString())
            {
                trackObj->setProperty("name", newName);
            }
        }
    }

    if (currentStyleComponent)
    {
        currentStyleComponent->renamingTrack(uuid, newName);
    }

    juce::File appDataFolder = IOHelper::getAppDataFolder();

    juce::File jsonFile = appDataFolder.getChildFile("allStyles.json");

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::initializeAllStyles()
{
    juce::File appDataFolder = IOHelper::getAppDataFolder();

    if (!appDataFolder.exists())
        appDataFolder.createDirectory();

    auto file = appDataFolder.getChildFile("allStyles.json");

    if (file.exists())
        return;

    juce::Array<juce::var> stylesArray;

    auto* styleObj = new juce::DynamicObject{};
    juce::String styleName = "DEFAULT Style";
    styleObj->setProperty("name", styleName);
    styleObj->setProperty("BPM", 120.0f);
    styleObj->setProperty("StyleID", "style_default");
    juce::Array<juce::var> tracksArray;
    for (int t = 0; t < 8; ++t)
    {
        auto* trackObj = new juce::DynamicObject{};
        trackObj->setProperty("name", "None");
        trackObj->setProperty("type", "None");
        trackObj->setProperty("volume", 50.0f);
        trackObj->setProperty("instrumentNumber", -1);
        trackObj->setProperty("uuid", "noID");
        tracksArray.add(trackObj);
    }
    styleObj->setProperty("tracks", tracksArray);
    stylesArray.add(styleObj);

    for (int i = 0; i < 9; ++i)
    {
        auto* styleObj = new juce::DynamicObject{};
        juce::String styleName = "Style " + juce::String(i + 1);
        styleObj->setProperty("name", styleName);
        styleObj->setProperty("BPM", 120.0f);
        styleObj->setProperty("StyleID", "style_" + juce::String(i+1));

        juce::Array<juce::var> tracksArray;
        for (int t = 0; t < 8; ++t)
        {
            auto* trackObj = new juce::DynamicObject{};
            trackObj->setProperty("name", "None");
            trackObj->setProperty("type", "None");
            trackObj->setProperty("volume", 50.0f);
            trackObj->setProperty("instrumentNumber", -1);
            trackObj->setProperty("uuid", "noID");
            tracksArray.add(trackObj);
        }
        styleObj->setProperty("tracks", tracksArray);
        stylesArray.add(styleObj);
    }

    auto* rootObj = new juce::DynamicObject{};
    rootObj->setProperty("styles", stylesArray);

    juce::var jsonVar(rootObj);  // var now owns rootObj pointer

    juce::String jsonString = juce::JSON::toString(jsonVar);

    file.replaceWithText(jsonString);
}

std::vector<juce::String> Display::getAllStylesFromJson()
{
    std::vector<juce::String> allStylesNames;

    if (!allStylesJsonVar.isObject())
        return allStylesNames;

    auto stylesArray = allStylesJsonVar["styles"];
    if (!stylesArray.isArray())
        return allStylesNames;

    for (auto& styleVar : *stylesArray.getArray())
    {
        auto* obj = styleVar.getDynamicObject();
        if (obj == nullptr)
            continue;

        juce::String name = obj->getProperty("name").toString();
        if (name.isNotEmpty())
            allStylesNames.push_back(name);
    }

    return allStylesNames;
}

void Display::loadAllStyles()
{
    auto appDataFolder = IOHelper::getAppDataFolder();

    auto file= IOHelper::getFile("allStyles.json");

    if (!file.exists())
    {
        allStylesJsonVar = juce::var();
        return;
    }

    juce::String jsonString = file.loadFileAsString();
    allStylesJsonVar = juce::JSON::parse(jsonString);
}

void Display::updateStyleInJson(const juce::String& name)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar["styles"];

    if (!stylesArray.isArray())
        return;

    for (int i = 0; i < stylesArray.getArray()->size(); ++i)
    {
        auto& styleVar = stylesArray.getArray()->getReference(i);
        if (auto* obj = styleVar.getDynamicObject())
        {
            if (obj->getProperty("name").toString() == name)
            {
                styleVar = juce::var(currentStyleComponent->getJson());
                break;
            }
        }
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto file = IOHelper::getFile("allStyles.json");

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    file.replaceWithText(jsonString);
}

void Display::updateStyleNameInJson(const juce::String& oldName, const juce::String& newName)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar["styles"];

    if (!stylesArray.isArray())
        return;

    for (int i = 0; i < stylesArray.getArray()->size(); i++)
    {
        auto& styleVar = stylesArray.getArray()->getReference(i);
        auto* obj = styleVar.getDynamicObject();
        if (obj == nullptr)
            continue;

        if (obj->getProperty("name").toString() == oldName)
        {
            obj->setProperty("name", newName);
            break;
        }
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile("allStyles.json");

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::appendNewStyleInJson(const juce::String& newName)
{
    if (!allStylesJsonVar.isObject())
        return;


    auto* obj = allStylesJsonVar.getDynamicObject();
    if (obj == nullptr)
        return;


    juce::var stylesVar = obj->getProperty("styles");

    if (!stylesVar.isArray())
        return;


    auto* stylesArray = stylesVar.getArray();

    if (stylesArray == nullptr)
        return;

    const int numExistingStyles = stylesArray->size();
    const juce::String styleID = "style_" + juce::String(numExistingStyles);


    auto* newStyle = new juce::DynamicObject();
    newStyle->setProperty("name", newName);
    newStyle->setProperty("BPM", 120.0f);
    newStyle->setProperty("StyleID", styleID);
    juce::Array<juce::var> tracksArray;
    for (int i = 0; i < 8; ++i)
    {
        auto* track = new juce::DynamicObject();
        track->setProperty("name", "None");
        track->setProperty("type", "None");
        track->setProperty("volume", 50.0f);
        track->setProperty("instrumentNumber", -1);
        track->setProperty("uuid", "noID");
        tracksArray.add(track);
    }
    newStyle->setProperty("tracks", tracksArray);
    stylesArray->add(newStyle);

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile("allStyles.json");

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::removeStyleInJson(const juce::String& name)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto* obj = allStylesJsonVar.getDynamicObject();
    if (obj == nullptr)
        return;

    juce::var styles = obj->getProperty("styles");
    if (!styles.isArray())
        return;

    auto* stylesArray = styles.getArray();

    for (int i = 0; i < stylesArray->size(); i++)
    {
        auto& style = stylesArray->getReference(i);
        auto* styleObj = style.getDynamicObject();
        if (styleObj == nullptr)
            continue;

        juce::String styleName = styleObj->getProperty("name").toString();
        if (styleName == name)
        {
            stylesArray->remove(i);
            break;
        }
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile("allStyles.json");

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

std::unordered_map<juce::Uuid, TrackEntry*> Display::buildTrackUuidMap()
{
    std::unordered_map<juce::Uuid, TrackEntry*> map;

    for (auto& [folderName, tracks] : *groupedTracks)
    {
        for (auto& tr : tracks)
        {
            map[tr.getUniqueID()] = &tr;
        }
    }

    return map;
}

void Display::resized()
{
    tabComp->setBounds(getLocalBounds());

}

const juce::var& Display::getJsonVar()
{
    return allStylesJsonVar;
}
