#include "ArrangerStyleIOHelper.h"
#include "ArrangerEnums.h"
#include "ArrangerMidiHex.h"

using juce::var;

static var trackToVar (const SourceTrackFile& t)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("id", t.id);
    o->setProperty ("name", t.name);
    o->setProperty ("partType", ArrangerEnums::toString (t.partType));
    o->setProperty ("channel", t.channel);
    o->setProperty ("instrument", t.instrument);
    o->setProperty ("volume", t.volume);
    o->setProperty ("nttType", ArrangerEnums::toString (t.nttType));   // Phase 7a
    o->setProperty ("effects", var()); // reserved: null

    juce::Array<var> events;
    for (const auto& e : t.events)
    {
        auto* eo = new juce::DynamicObject();
        eo->setProperty ("b", e.beats);
        eo->setProperty ("m", ArrangerMidiHex::toHex (e.message));
        events.add (var (eo));
    }
    o->setProperty ("events", events);
    return var (o);
}

static var sectionToVar (const SectionWindow& s)
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("id", s.id);
    o->setProperty ("name", s.name);
    o->setProperty ("type", ArrangerEnums::toString (s.type));
    o->setProperty ("startBar", s.startBar);
    o->setProperty ("lengthBars", s.lengthBars);
    o->setProperty ("afterComplete", ArrangerEnums::toString (s.afterComplete));
    return var (o);
}

void ArrangerStyleIOHelper::saveToFile (const juce::File& file, const ArrangerStyleFile& style)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("schemaVersion", style.schemaVersion);
    root->setProperty ("kind", "arrangerStyle");
    root->setProperty ("id", style.id);
    root->setProperty ("name", style.name);
    root->setProperty ("originalTempo", style.originalTempo);
    root->setProperty ("timeSigNum", style.timeSigNum);
    root->setProperty ("timeSigDenom", style.timeSigDenom);
    root->setProperty ("originalRoot", style.originalRoot);
    root->setProperty ("originalQuality", toString (style.originalQuality));

    juce::Array<var> tracks;
    for (const auto& t : style.sourceTracks) tracks.add (trackToVar (t));
    root->setProperty ("sourceTracks", tracks);

    juce::Array<var> sections;
    for (const auto& s : style.sections) sections.add (sectionToVar (s));
    root->setProperty ("sections", sections);

    file.getParentDirectory().createDirectory();   // getFolder() doesn't create it
    file.replaceWithText (juce::JSON::toString (var (root), true));
}

bool ArrangerStyleIOHelper::loadFromFile (const juce::File& file, ArrangerStyleFile& out, juce::String& error)
{
    if (! file.existsAsFile()) { error = "File does not exist: " + file.getFullPathName(); return false; }

    var root;
    auto result = juce::JSON::parse (file.loadFileAsString(), root);
    if (result.failed() || ! root.isObject()) { error = "Invalid JSON: " + result.getErrorMessage(); return false; }

    out = ArrangerStyleFile();
    out.schemaVersion = (int)    root.getProperty ("schemaVersion", 3);
    out.id            =          root.getProperty ("id", "").toString();
    out.name          =          root.getProperty ("name", "").toString();
    out.originalTempo = (double) root.getProperty ("originalTempo", 120.0);
    out.timeSigNum    = (int)    root.getProperty ("timeSigNum", 4);
    out.timeSigDenom  = (int)    root.getProperty ("timeSigDenom", 4);
    out.originalRoot  = (int)    root.getProperty ("originalRoot", 0);   // v3 files: default C
    out.originalQuality = chordQualityFromString (root.getProperty ("originalQuality", "Maj").toString());
    if (out.originalQuality == ChordQuality::None) out.originalQuality = ChordQuality::Maj;

    if (auto* tracks = root.getProperty ("sourceTracks", var()).getArray())
        for (const auto& tv : *tracks)
        {
            SourceTrackFile t;
            t.id         =          tv.getProperty ("id", "").toString();
            t.name       =          tv.getProperty ("name", "").toString();
            t.partType   = ArrangerEnums::partFromString (tv.getProperty ("partType", "Acc").toString());
            t.channel    = (int)    tv.getProperty ("channel", 2);
            t.instrument = (int)    tv.getProperty ("instrument", -1);
            t.volume     = (double) tv.getProperty ("volume", 100.0);
            const auto nttStr = tv.getProperty ("nttType", juce::var()).toString();   // Phase 7a: migrate by part
            t.nttType    = nttStr.isEmpty() ? ArrangerEnums::nttDefaultForPart (t.partType)
                                            : ArrangerEnums::nttFromString (nttStr);
            if (auto* evs = tv.getProperty ("events", var()).getArray())
                for (const auto& ev : *evs)
                    t.events.push_back ({ (double) ev.getProperty ("b", 0.0),
                                          ArrangerMidiHex::fromHex (ev.getProperty ("m", "").toString()) });
            out.sourceTracks.push_back (std::move (t));
        }

    if (auto* sections = root.getProperty ("sections", var()).getArray())
        for (const auto& sv : *sections)
        {
            SectionWindow s;
            s.id            =          sv.getProperty ("id", "").toString();
            s.name          =          sv.getProperty ("name", "").toString();
            s.type          = ArrangerEnums::sectionTypeFromString (sv.getProperty ("type", "Variation").toString());
            s.startBar      = (int)    sv.getProperty ("startBar", 1);
            s.lengthBars    = (int)    sv.getProperty ("lengthBars", 1);
            s.afterComplete = ArrangerEnums::afterFromString (sv.getProperty ("afterComplete", "Loop").toString());
            out.sections.push_back (std::move (s));
        }

    return true;
}
