#include "displayGUI.h"

namespace
{
    // A fresh allStyles.json: DEFAULT Style plus "Style 1".."Style 9", each with empty track slots.
    constexpr float defaultStyleBpm    = 120.0f;
    constexpr float defaultTrackVolume = 50.0f;
    constexpr int   numberedStyles     = 9;
    constexpr int   firstStyleNumber   = 1;
}


void Display::removeTrackFromAllStyles(const juce::Uuid& uuid)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar[juce::Identifier { AllStylesJsonKeys::styles }];

    if (!stylesArray.isArray())
        return;

    for (auto& styleVariable : *stylesArray.getArray())
    {
        auto* styleObj = styleVariable.getDynamicObject();

        if (styleObj == nullptr)
            continue;

        juce::var trackVariable = styleObj->getProperty(AllStylesJsonKeys::tracks);

        if (!trackVariable.isArray())
            continue;

        auto* trackArray = trackVariable.getArray();

        for (auto& trackVar : *trackArray)
        {
            auto* trackObj = trackVar.getDynamicObject();

            if (trackObj == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty(StyleTrackJsonKeys::uuid).toString();
            if (uuidString == uuid.toString())
            {
                trackObj->setProperty(StyleTrackJsonKeys::uuid, StyleTrackJsonKeys::noTrackId);
                trackObj->setProperty(StyleTrackJsonKeys::name, StyleTrackJsonKeys::emptySlot);
                trackObj->setProperty(StyleTrackJsonKeys::instrumentNumber, TrackEntryDefaults::notSet);
                trackObj->setProperty(StyleTrackJsonKeys::type, StyleTrackJsonKeys::emptySlot);
            }
        }
    }

    if (currentStyleComponent)
    {
        currentStyleComponent->removingTrack(uuid);
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile(AppDataNames::stylesFile);

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::removeTracksFromAllStyles(const std::vector<juce::Uuid>& uuids)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar[juce::Identifier { AllStylesJsonKeys::styles }];

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

        juce::var trackVariable = styleObj->getProperty(AllStylesJsonKeys::tracks);

        if (!trackVariable.isArray())
            continue;

        for (auto& trackVar : *trackVariable.getArray())
        {
            auto* trackObj = trackVar.getDynamicObject();
            if (trackObj == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty(StyleTrackJsonKeys::uuid).toString();
            if (uuidSet.count(uuidString))
            {
                trackObj->setProperty(StyleTrackJsonKeys::uuid, StyleTrackJsonKeys::noTrackId);
                trackObj->setProperty(StyleTrackJsonKeys::name, StyleTrackJsonKeys::emptySlot);
                trackObj->setProperty(StyleTrackJsonKeys::instrumentNumber, TrackEntryDefaults::notSet);
                trackObj->setProperty(StyleTrackJsonKeys::type, StyleTrackJsonKeys::emptySlot);
            }
        }
    }

    if (currentStyleComponent)
    {
        currentStyleComponent->removingTracks(uuids);
    }

    juce::File appDataFolder = IOHelper::getAppDataFolder();

    juce::File jsonFile = appDataFolder.getChildFile(AppDataNames::stylesFile);

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::updateTrackNameFromAllStyles(const juce::Uuid& uuid, const juce::String& newName)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar[juce::Identifier { AllStylesJsonKeys::styles }];

    if (!stylesArray.isArray())
        return;

    for (auto& styleVar : *stylesArray.getArray())
    {
        auto* styleObj = styleVar.getDynamicObject();

        if (styleObj == nullptr)
            continue;

        juce::var trackVariable = styleObj->getProperty(AllStylesJsonKeys::tracks);

        if (!trackVariable.isArray())
            continue;

        for (auto& trackVar : *trackVariable.getArray())
        {
            auto* trackObj = trackVar.getDynamicObject();
            if (trackObj == nullptr)
                continue;

            juce::String uuidString = trackObj->getProperty(StyleTrackJsonKeys::uuid).toString();
            if (uuidString == uuid.toString())
            {
                trackObj->setProperty(StyleTrackJsonKeys::name, newName);
            }
        }
    }

    if (currentStyleComponent)
    {
        currentStyleComponent->renamingTrack(uuid, newName);
    }

    juce::File appDataFolder = IOHelper::getAppDataFolder();

    juce::File jsonFile = appDataFolder.getChildFile(AppDataNames::stylesFile);

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    jsonFile.replaceWithText(jsonString);
}

void Display::initializeAllStyles()
{
    juce::File appDataFolder = IOHelper::getAppDataFolder();

    if (!appDataFolder.exists())
        appDataFolder.createDirectory();

    auto file = appDataFolder.getChildFile(AppDataNames::stylesFile);

    if (file.exists())
        return;

    juce::Array<juce::var> stylesArray;

    auto* styleObj = new juce::DynamicObject{};
    juce::String styleName = StyleNames::defaultStyle;
    styleObj->setProperty(AllStylesJsonKeys::name, styleName);
    styleObj->setProperty(AllStylesJsonKeys::bpm, defaultStyleBpm);
    styleObj->setProperty(AllStylesJsonKeys::styleId, StyleNames::defaultStyleId);
    juce::Array<juce::var> tracksArray;
    for (int slotIndex {}; slotIndex < CurrentStyleComponent::trackSlots; ++slotIndex)
    {
        auto* trackObj = new juce::DynamicObject{};
        trackObj->setProperty(StyleTrackJsonKeys::name, StyleTrackJsonKeys::emptySlot);
        trackObj->setProperty(StyleTrackJsonKeys::type, StyleTrackJsonKeys::emptySlot);
        trackObj->setProperty(StyleTrackJsonKeys::volume, defaultTrackVolume);
        trackObj->setProperty(StyleTrackJsonKeys::instrumentNumber, TrackEntryDefaults::notSet);
        trackObj->setProperty(StyleTrackJsonKeys::uuid, StyleTrackJsonKeys::noTrackId);
        tracksArray.add(trackObj);
    }
    styleObj->setProperty(AllStylesJsonKeys::tracks, tracksArray);
    stylesArray.add(styleObj);

    for (int styleIndex {}; styleIndex < numberedStyles; ++styleIndex)
    {
        auto* styleObj = new juce::DynamicObject{};
        juce::String styleName = StyleNames::newStylePrefix + juce::String(styleIndex + firstStyleNumber);
        styleObj->setProperty(AllStylesJsonKeys::name, styleName);
        styleObj->setProperty(AllStylesJsonKeys::bpm, defaultStyleBpm);
        styleObj->setProperty(AllStylesJsonKeys::styleId, StyleNames::styleIdPrefix + juce::String(styleIndex+firstStyleNumber));

        juce::Array<juce::var> tracksArray;
        for (int slotIndex {}; slotIndex < CurrentStyleComponent::trackSlots; ++slotIndex)
        {
            auto* trackObj = new juce::DynamicObject{};
            trackObj->setProperty(StyleTrackJsonKeys::name, StyleTrackJsonKeys::emptySlot);
            trackObj->setProperty(StyleTrackJsonKeys::type, StyleTrackJsonKeys::emptySlot);
            trackObj->setProperty(StyleTrackJsonKeys::volume, defaultTrackVolume);
            trackObj->setProperty(StyleTrackJsonKeys::instrumentNumber, TrackEntryDefaults::notSet);
            trackObj->setProperty(StyleTrackJsonKeys::uuid, StyleTrackJsonKeys::noTrackId);
            tracksArray.add(trackObj);
        }
        styleObj->setProperty(AllStylesJsonKeys::tracks, tracksArray);
        stylesArray.add(styleObj);
    }

    auto* rootObj = new juce::DynamicObject{};
    rootObj->setProperty(AllStylesJsonKeys::styles, stylesArray);

    juce::var jsonVar(rootObj);  // var now owns rootObj pointer

    juce::String jsonString = juce::JSON::toString(jsonVar);

    file.replaceWithText(jsonString);
}

std::vector<juce::String> Display::getAllStylesFromJson()
{
    std::vector<juce::String> allStylesNames;

    if (!allStylesJsonVar.isObject())
        return allStylesNames;

    auto stylesArray = allStylesJsonVar[juce::Identifier { AllStylesJsonKeys::styles }];
    if (!stylesArray.isArray())
        return allStylesNames;

    for (auto& styleVar : *stylesArray.getArray())
    {
        auto* obj = styleVar.getDynamicObject();
        if (obj == nullptr)
            continue;

        juce::String name = obj->getProperty(AllStylesJsonKeys::name).toString();
        if (name.isNotEmpty())
            allStylesNames.push_back(name);
    }

    return allStylesNames;
}

void Display::loadAllStyles()
{
    auto appDataFolder = IOHelper::getAppDataFolder();

    auto file= IOHelper::getFile(AppDataNames::stylesFile);

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

    auto stylesArray = allStylesJsonVar[juce::Identifier { AllStylesJsonKeys::styles }];

    if (!stylesArray.isArray())
        return;

    for (int styleIndex {}; styleIndex < stylesArray.getArray()->size(); ++styleIndex)
    {
        auto& styleVar = stylesArray.getArray()->getReference(styleIndex);
        if (auto* obj = styleVar.getDynamicObject())
        {
            if (obj->getProperty(AllStylesJsonKeys::name).toString() == name)
            {
                styleVar = juce::var(currentStyleComponent->getJson());
                break;
            }
        }
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto file = IOHelper::getFile(AppDataNames::stylesFile);

    juce::String jsonString = juce::JSON::toString(allStylesJsonVar);
    file.replaceWithText(jsonString);
}

void Display::updateStyleNameInJson(const juce::String& oldName, const juce::String& newName)
{
    if (!allStylesJsonVar.isObject())
        return;

    auto stylesArray = allStylesJsonVar[juce::Identifier { AllStylesJsonKeys::styles }];

    if (!stylesArray.isArray())
        return;

    for (int styleIndex {}; styleIndex < stylesArray.getArray()->size(); styleIndex++)
    {
        auto& styleVar = stylesArray.getArray()->getReference(styleIndex);
        auto* obj = styleVar.getDynamicObject();
        if (obj == nullptr)
            continue;

        if (obj->getProperty(AllStylesJsonKeys::name).toString() == oldName)
        {
            obj->setProperty(AllStylesJsonKeys::name, newName);
            break;
        }
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile(AppDataNames::stylesFile);

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


    juce::var stylesVar = obj->getProperty(AllStylesJsonKeys::styles);

    if (!stylesVar.isArray())
        return;


    auto* stylesArray = stylesVar.getArray();

    if (stylesArray == nullptr)
        return;

    const int numExistingStyles = stylesArray->size();
    const juce::String styleID = StyleNames::styleIdPrefix + juce::String(numExistingStyles);


    auto* newStyle = new juce::DynamicObject();
    newStyle->setProperty(AllStylesJsonKeys::name, newName);
    newStyle->setProperty(AllStylesJsonKeys::bpm, defaultStyleBpm);
    newStyle->setProperty(AllStylesJsonKeys::styleId, styleID);
    juce::Array<juce::var> tracksArray;
    for (int slotIndex {}; slotIndex < CurrentStyleComponent::trackSlots; ++slotIndex)
    {
        auto* track = new juce::DynamicObject();
        track->setProperty(StyleTrackJsonKeys::name, StyleTrackJsonKeys::emptySlot);
        track->setProperty(StyleTrackJsonKeys::type, StyleTrackJsonKeys::emptySlot);
        track->setProperty(StyleTrackJsonKeys::volume, defaultTrackVolume);
        track->setProperty(StyleTrackJsonKeys::instrumentNumber, TrackEntryDefaults::notSet);
        track->setProperty(StyleTrackJsonKeys::uuid, StyleTrackJsonKeys::noTrackId);
        tracksArray.add(track);
    }
    newStyle->setProperty(AllStylesJsonKeys::tracks, tracksArray);
    stylesArray->add(newStyle);

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile(AppDataNames::stylesFile);

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

    juce::var styles = obj->getProperty(AllStylesJsonKeys::styles);
    if (!styles.isArray())
        return;

    auto* stylesArray = styles.getArray();

    for (int styleIndex {}; styleIndex < stylesArray->size(); styleIndex++)
    {
        auto& style = stylesArray->getReference(styleIndex);
        auto* styleObj = style.getDynamicObject();
        if (styleObj == nullptr)
            continue;

        juce::String styleName = styleObj->getProperty(AllStylesJsonKeys::name).toString();
        if (styleName == name)
        {
            stylesArray->remove(styleIndex);
            break;
        }
    }

    auto appDataFolder = IOHelper::getAppDataFolder();

    auto jsonFile = IOHelper::getFile(AppDataNames::stylesFile);

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

const juce::var& Display::getJsonVar()
{
    return allStylesJsonVar;
}
