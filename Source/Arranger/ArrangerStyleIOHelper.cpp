#include "ArrangerStyleIOHelper.h"
#include "ArrangerEnums.h"
#include "ArrangerMidiHex.h"

using juce::var;

/** Keys of the .style JSON format. Persisted: the text must never change. */
namespace StyleJsonKeys
{
    const juce::Identifier id              = "id";
    const juce::Identifier name            = "name";
    const juce::Identifier partType        = "partType";
    const juce::Identifier channel         = "channel";
    const juce::Identifier instrument      = "instrument";
    const juce::Identifier volume          = "volume";
    const juce::Identifier nttType         = "nttType";
    const juce::Identifier effects         = "effects";
    const juce::Identifier events          = "events";
    const juce::Identifier eventBeats      = "b";
    const juce::Identifier eventMessage    = "m";
    const juce::Identifier type            = "type";
    const juce::Identifier startBar        = "startBar";
    const juce::Identifier lengthBars      = "lengthBars";
    const juce::Identifier afterComplete   = "afterComplete";
    const juce::Identifier schemaVersion   = "schemaVersion";
    const juce::Identifier kind            = "kind";
    const juce::Identifier originalTempo   = "originalTempo";
    const juce::Identifier timeSigNum      = "timeSigNum";
    const juce::Identifier timeSigDenom    = "timeSigDenom";
    const juce::Identifier originalRoot    = "originalRoot";
    const juce::Identifier originalQuality = "originalQuality";
    const juce::Identifier sourceTracks    = "sourceTracks";
    const juce::Identifier sections        = "sections";

    const juce::String     arrangerStyleKind = "arrangerStyle";
}

namespace StyleFileLoading
{
    constexpr int      legacySchemaVersion = 3;      // files written before the version was stored
    constexpr bool     prettyPrint         = true;
    const juce::String fileMissing  = "File does not exist: ";
    const juce::String invalidJson  = "Invalid JSON: ";
}

static var trackToVar (const SourceTrackFile& track)
{
    using namespace StyleJsonKeys;
    auto* trackObject = new juce::DynamicObject();
    trackObject->setProperty (id, track.id);
    trackObject->setProperty (name, track.name);
    trackObject->setProperty (partType, ArrangerEnums::toString (track.partType));
    trackObject->setProperty (channel, track.channel);
    trackObject->setProperty (instrument, track.instrument);
    trackObject->setProperty (volume, track.volume);
    trackObject->setProperty (nttType, ArrangerEnums::toString (track.nttType));   // Phase 7a
    trackObject->setProperty (effects, var()); // reserved: null

    juce::Array<var> eventList;
    for (const auto& event : track.events)
    {
        auto* eventObject = new juce::DynamicObject();
        eventObject->setProperty (eventBeats, event.beats);
        eventObject->setProperty (eventMessage, ArrangerMidiHex::toHex (event.message));
        eventList.add (var (eventObject));
    }
    trackObject->setProperty (events, eventList);
    return var (trackObject);
}

static var sectionToVar (const SectionWindow& section)
{
    using namespace StyleJsonKeys;
    auto* sectionObject = new juce::DynamicObject();
    sectionObject->setProperty (id, section.id);
    sectionObject->setProperty (name, section.name);
    sectionObject->setProperty (type, ArrangerEnums::toString (section.type));
    sectionObject->setProperty (startBar, section.startBar);
    sectionObject->setProperty (lengthBars, section.lengthBars);
    sectionObject->setProperty (afterComplete, ArrangerEnums::toString (section.afterComplete));
    return var (sectionObject);
}

void ArrangerStyleIOHelper::saveToFile (const juce::File& file, const ArrangerStyleFile& style)
{
    using namespace StyleJsonKeys;
    auto* root = new juce::DynamicObject();
    root->setProperty (schemaVersion, style.schemaVersion);
    root->setProperty (kind, arrangerStyleKind);
    root->setProperty (id, style.id);
    root->setProperty (name, style.name);
    root->setProperty (originalTempo, style.originalTempo);
    root->setProperty (timeSigNum, style.timeSigNum);
    root->setProperty (timeSigDenom, style.timeSigDenom);
    root->setProperty (originalRoot, style.originalRoot);
    root->setProperty (originalQuality, toString (style.originalQuality));

    juce::Array<var> trackList;
    for (const auto& track : style.sourceTracks) trackList.add (trackToVar (track));
    root->setProperty (sourceTracks, trackList);

    juce::Array<var> sectionList;
    for (const auto& section : style.sections) sectionList.add (sectionToVar (section));
    root->setProperty (sections, sectionList);

    file.getParentDirectory().createDirectory();   // getFolder() doesn't create it
    file.replaceWithText (juce::JSON::toString (var (root), StyleFileLoading::prettyPrint));
}

bool ArrangerStyleIOHelper::loadFromFile (const juce::File& file, ArrangerStyleFile& out, juce::String& error)
{
    using namespace StyleJsonKeys;
    using namespace ArrangerStyleDefaults;

    if (! file.existsAsFile()) { error = StyleFileLoading::fileMissing + file.getFullPathName(); return false; }

    var root;
    auto result = juce::JSON::parse (file.loadFileAsString(), root);
    if (result.failed() || ! root.isObject()) { error = StyleFileLoading::invalidJson + result.getErrorMessage(); return false; }

    out = ArrangerStyleFile();
    out.schemaVersion = (int)    root.getProperty (schemaVersion, StyleFileLoading::legacySchemaVersion);
    out.id            =          root.getProperty (id, "").toString();
    out.name          =          root.getProperty (name, "").toString();
    out.originalTempo = (double) root.getProperty (originalTempo, tempoBpm);
    out.timeSigNum    = (int)    root.getProperty (timeSigNum, ArrangerStyleDefaults::beatsPerBar);
    out.timeSigDenom  = (int)    root.getProperty (timeSigDenom, beatUnit);
    out.originalRoot  = (int)    root.getProperty (originalRoot, PitchClasses::lowest);   // v3 files: default C
    out.originalQuality = chordQualityFromString (root.getProperty (originalQuality, ChordQualityNames::majorChord).toString());
    if (out.originalQuality == ChordQuality::None) out.originalQuality = ChordQuality::Maj;

    if (auto* trackArray = root.getProperty (sourceTracks, var()).getArray())
        for (const auto& trackValue : *trackArray)
        {
            SourceTrackFile track;
            track.id         =          trackValue.getProperty (id, "").toString();
            track.name       =          trackValue.getProperty (name, "").toString();
            track.partType   = ArrangerEnums::partFromString (trackValue.getProperty (partType, ArrangerEnums::PartTypeNames::accompaniment).toString());
            track.channel    = (int)    trackValue.getProperty (channel, ArrangerChannels::kFirstMelodicChannel);
            track.instrument = (int)    trackValue.getProperty (instrument, noInstrument);
            track.volume     = (double) trackValue.getProperty (volume, fullVolume);
            const auto nttName = trackValue.getProperty (nttType, juce::var()).toString();   // Phase 7a: migrate by part
            track.nttType    = nttName.isEmpty() ? ArrangerEnums::nttDefaultForPart (track.partType)
                                                 : ArrangerEnums::nttFromString (nttName);
            if (auto* eventArray = trackValue.getProperty (events, var()).getArray())
                for (const auto& eventValue : *eventArray)
                    track.events.push_back ({ (double) eventValue.getProperty (eventBeats, sectionStartBeat),
                                              ArrangerMidiHex::fromHex (eventValue.getProperty (eventMessage, "").toString()) });
            out.sourceTracks.push_back (std::move (track));
        }

    if (auto* sectionArray = root.getProperty (sections, var()).getArray())
        for (const auto& sectionValue : *sectionArray)
        {
            SectionWindow section;
            section.id            =          sectionValue.getProperty (id, "").toString();
            section.name          =          sectionValue.getProperty (name, "").toString();
            section.type          = ArrangerEnums::sectionTypeFromString (sectionValue.getProperty (type, ArrangerEnums::SectionTypeNames::variation).toString());
            section.startBar      = (int)    sectionValue.getProperty (startBar, firstBar);
            section.lengthBars    = (int)    sectionValue.getProperty (lengthBars, minimumLengthBars);
            section.afterComplete = ArrangerEnums::afterFromString (sectionValue.getProperty (afterComplete, ArrangerEnums::AfterCompleteNames::loop).toString());
            out.sections.push_back (std::move (section));
        }

    return true;
}
