#include "displayGUI.h"


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

const juce::var& Display::getJsonVar()
{
    return allStylesJsonVar;
}
