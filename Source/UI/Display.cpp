#include "displayGUI.h"
#include "CustomTableContainer.h"
#include "PlaybackRange.h"

namespace
{
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

Display::Display(std::weak_ptr<juce::MidiOutput> outputDev, juce::PropertiesFile* props, int widthForList) : outputDevice{outputDev}, propertiesFile{props},
    minNote{lowestPianoNote}, maxNote{highestPianoNote}
{
    playBackSettings = std::make_unique<PlayBackSettingsComponent>(minNote, maxNote, this->settings);
    playBackSettings->getStyleID = [this]() { return this->getStyleID(); };
    playBackSettings->onChangingSettings = [this](PlayBackSettings)
    {
        // The field shows the note as it applies now (the PC keyboard may be shifted): save it moved home.
        savedSettings = PlaybackRange::savedAfterEdit(savedSettings, this->settings, keyboardShift());
        PlaybackSettingsIOHelper::savePlaybackSettings(propertiesFile, savedSettings, this->getStyleID());
        showRangeSettings();
        displayListeners.call(&DisplayListener::playBackSettingsChanged, this->settings);
    };
    playBackSettings->onChangingTranspose = [this](int transposeValue)
    {
        displayListeners.call(&DisplayListener::playBackSettingsTransposeChanged, transposeValue);
    };
    playBackSettings->setStyleOpen(false);

    availableTracksFromFolder = std::make_shared<std::deque<TrackEntry>>();
    groupedTracks = std::make_shared<std::unordered_map<juce::String, std::deque<TrackEntry>>>();
    groupedTrackKeys = std::make_shared<std::vector<juce::String>>();
    sectionsPerStyleMap = std::make_shared<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>>();

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
        if (currentStyleComponent && currentStyleComponent->getName() == oldName)
        {
            currentStyleComponent->updateName(newName);
            viewChanged();   // the screen shows the open style's name
        }

        updateStyleNameInJson(oldName, newName);
    };

    list->onInfo = [this](const juce::String& text) { if (onInfo) onInfo(text); };
    list->onStyleAdd = [this](const juce::String& newName)
    {
        appendNewStyleInJson(newName);
    };

    list->onStyleRemove = [this](const juce::String& name)
    {
        // Removing the open style closes it. Sub-pages hold callbacks into it: they go first.
        if (currentStyleComponent && currentStyleComponent->getName() == name)
        {
            destroySubView();
            currentStyleComponent.reset();
            playBackSettings->setStyleOpen(false);
            layoutViews();
            refreshTitleActions();   // nothing open: All tracks / Solo / Sections... / Style settings... off
            viewChanged();
        }
        removeStyleInJson(name);
    };

    addAndMakeVisible(list.get());
    stylesListComponent = std::move(list);

    // The Style page's title row (board: PLAY · All tracks · Solo | Sections... · Style settings...).
    allTracksButton.onClick     = [this] { if (currentStyleComponent) { currentStyleComponent->setPlayModeSolo(false); refreshTitleActions(); } };
    soloButton.onClick          = [this] { if (currentStyleComponent) { currentStyleComponent->setPlayModeSolo(true);  refreshTitleActions(); } };
    sectionsButton.onClick      = [this] { if (currentStyleComponent) currentStyleComponent->openSectionConfigurations(); };
    styleSettingsButton.onClick = [this] { if (onStyleSettings) onStyleSettings(); };
    refreshTitleActions();
}

Display::~Display()
{
    destroySubView();   // the track picker / notes page listen to the style component: they go first
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

void Display::showCurrentStyleTab(const juce::String& name)
{
    if (currentStyleComponent == nullptr)
    {
        std::weak_ptr<std::unordered_map<juce::String, std::unordered_map<juce::String,StyleSection>>> weakSectionsMap = sectionsPerStyleMap;

        currentStyleComponent = std::make_unique<CurrentStyleComponent>(name, mapUuidToTrack, outputDevice,weakSectionsMap);
        currentStyleComponent->onRequestTrackSelectionFromTrack = [this](TrackChosenCallback trackChosenCallback)
        {
            showListOfTracksToSelectFrom(trackChosenCallback);
        };

        currentStyleComponent->updateTrackFile = [this]()
        {
            auto appDataFolder = IOHelper::getAppDataFolder();

            auto jsonFile = IOHelper::getFile(AppDataNames::tracksFile);
            TrackIOHelper::saveToFile(jsonFile, *groupedTracks);
        };

        currentStyleComponent->keybindTabStarting = [this]() { if (onStyleSettings) onStyleSettings(); };

        addAndMakeVisible(currentStyleComponent.get());
        currentStyleComponent->onInfo = [this](const juce::String& text) { if (onInfo) onInfo(text); };
        currentStyleComponent->onTrackInstrumentChosen = [this](int channel, int program)
        {
            if (onTrackInstrumentChosen)
                onTrackInstrumentChosen(channel, program);
        };
        currentStyleComponent->onShowNotes = [this](std::unique_ptr<TableContainer> notes, const juce::String& title, const juce::String& subtitle)
        {
            showNotes(std::move(notes), title, subtitle);
        };

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

        destroySubView();   // a sub-page of the previous style must not outlive the switch
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
    stylesListComponent->setOpenStyle(name);
    views.reset();
    layoutViews();
    refreshTitleActions();
    viewChanged();

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

    readPlaybackSettingsFromProperties();   // the Voices page's range fields show the style's settings
    playBackSettings->setStyleOpen(true);

    currentStyleComponent->applyBPMchangeBeforePlayback(bpmToUse);
    currentStyleComponent->applyChangesForAllTracksCurrentStyle();
}

void Display::showListOfTracksToSelectFrom(TrackChosenCallback onTrackSelected)
{
    destroySubView();   // one sub-page at a time
    trackListComp = std::make_unique<TrackListComponent>(availableTracksFromFolder, groupedTracks, groupedTrackKeys,
        [this, onTrackSelected](int index)
        {
            auto& track = (*availableTracksFromFolder)[index];
            juce::String typeString = TrackTypeConversion::toString(track.type);

            onTrackSelected({ track.getDisplayName(), track.getUniqueID(), typeString, track.instrumentAssociated });

            // No map update here: `track` lives in the picker's copy of the open folder, which the
            // next folder open overwrites. The map already points at the library entry
            // (buildTrackUuidMap at startup, addToMapOnAdding for new tracks).

            if (currentStyleComponent)
            {
                //currentStyleComponent->applyBPMchangeForOne(currentStyleComponent->getTempo(), track.getUniqueID());
                currentStyleComponent->applyBPMchangeBeforePlayback(currentStyleComponent->getTempo(),true);
            }

            if (currentStyleComponent)
            {
                DBG(saveLogged);
                updateStyleInJson(currentStyleComponent->getName());
            }

            // Back to the style. Not now: this runs inside the picker's own click, which must not
            // delete the picker under itself.
            const juce::Component::SafePointer<Display> safeThis (this);
            juce::MessageManager::callAsync([safeThis]
            {
                if (safeThis != nullptr && safeThis->getView() == StyleView::tracks)
                    safeThis->closeSubView();
            });
        });

    trackListComp->addListener(this);

    // Removing entries shifts or destroys the library's deque elements, so the UUID -> entry map is
    // rebuilt (assigned in place: CurrentStyleComponent holds a reference to this same map).
    trackListComp->onInfo = [this](const juce::String& text) { if (onInfo) onInfo(text); };
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

    trackListComp->setSlotNumber(currentStyleComponent ? currentStyleComponent->getRequestingSlot() : int {});
    trackListComp->setScale(scale);
    // Owner: open the folder used last instead of the folder list each time (this session; a gone folder is forgotten).
    trackListComp->onFolderOpened = [this](const juce::String& folder) { lastTrackFolder = folder; };
    if (lastTrackFolder.isNotEmpty() && ! trackListComp->openFolderNamed(lastTrackFolder))
        lastTrackFolder = {};
    addAndMakeVisible(trackListComp.get());
    showView(StyleView::tracks);
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

void Display::set_min_max(int min, int max, bool pcKeyboard)
{
    pcKeyboardInput = pcKeyboard;
    if (min < lowestPianoNote)
        this->minNote = lowestPianoNote;
    else this->minNote = min;

    if (max > highestPianoNote)
        this->maxNote = highestPianoNote;
    else this->maxNote = max;
    settings = PlaybackRange::shown(savedSettings, keyboardShift());   // the range follows the keyboard's octave
    playBackSettings->setLowestHighest(minNote, maxNote);
}

int Display::keyboardShift() const
{
    return PlaybackRange::octaveShift(minNote, pcKeyboardInput);
}

void Display::showRangeSettings()
{
    settings = PlaybackRange::shown(savedSettings, keyboardShift());
    playBackSettings->showSettings();
}

void Display::set_VID_PID(const juce::String& VID, const juce::String& PID)
{
    this->settings.VID = VID;
    this->settings.PID = PID;
    this->savedSettings.VID = VID;
    this->savedSettings.PID = PID;
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
    savedSettings = PlaybackSettingsIOHelper::loadPlaybackSettings(propertiesFile, this->settings.VID, this->settings.PID, this->getStyleID());
    showRangeSettings();   // shown at the keyboard's current octave
}

void Display::homeButtonInteraction()
{
    // Home with a sub-page open (formerly: a temporary tab): back to the style, play mode All tracks.
    closeSubView();
    if (currentStyleComponent)
        currentStyleComponent->comboBoxChangeIndex(allTracksIndex);
    refreshTitleActions();
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

void Display::keyboardShifted()
{
    showRangeSettings();
    displayListeners.call(&DisplayListener::playBackSettingsChanged, this->settings);
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
    layoutViews();
}
