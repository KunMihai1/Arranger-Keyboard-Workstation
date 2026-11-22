#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerStyleIOHelper.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerEngine.h"

namespace ArrangerStyleIOTestValues
{
    const juce::String testName     = "ArrangerStyleIO";
    const juce::String testCategory = "Arranger";

    const juce::String saveThenLoad          = "save then load reproduces the style";
    const juce::String originalChordRoundTrip = "v4 round-trips the style-level original chord";
    const juce::String v3LoadsAsCMajor       = "a v3 file with no original chord loads as C major";
    const juce::String nttTypeRoundTrip      = "nttType round-trips and a missing field migrates by part";
    const juce::String missingFileFails      = "loading a missing file fails gracefully";
    const juce::String fileToEngine          = "file -> buildStyleFromFile -> engine renders the first section";

    const juce::String styleExtension   = ".style";
    const juce::String sampleStyleName  = "Sunset Groove";
    const juce::String drumsTrackId     = "t1";
    const juce::String drumsName        = "Drums";
    const juce::String variationId      = "var_1";
    const juce::String variationName    = "Variation 1";
    const juce::String nttStyleId       = "s";
    const juce::String nttStyleName     = "S";
    const juce::String bassTrackId      = "b";
    const juce::String bassName         = "Bass";
    const juce::String accTrackId       = "a";
    const juce::String pianoName        = "Piano";
    const juce::String smokeStyleName   = "Smoke";
    const juce::String smokeTrackId     = "t";
    const juce::String smokeSectionId   = "v";
    const juce::String missingFileName  = "arranger-style-does-not-exist-9d3f.style";

    // A minimal v3-style JSON written by hand (no originalRoot/originalQuality keys).
    const juce::String v3StyleJson =
        "{ \"schemaVersion\": 3, \"kind\": \"arrangerStyle\", \"id\": \"x\", \"name\": \"Old\","
        "  \"originalTempo\": 120.0, \"timeSigNum\": 4, \"timeSigDenom\": 4,"
        "  \"sourceTracks\": [], \"sections\": [] }";

    constexpr double      tempo          = 120.0;
    constexpr int         beatsPerBar    = 4;
    constexpr int         beatUnit       = 4;
    constexpr int         drumChannel    = 10;
    constexpr int         noInstrument   = -1;
    constexpr double      drumsVolume    = 100.0;
    constexpr int         kickNote       = 36;
    constexpr juce::uint8 noteVelocity   = 100;
    constexpr double      kickOnBeat     = 0.0;
    constexpr double      kickOffBeat    = 0.5;
    constexpr int         variationStart = 5;
    constexpr int         variationBars  = 8;

    constexpr int    currentSchemaVersion = 5;
    constexpr int    oneTrack       = 1;
    constexpr int    oneSection     = 1;
    constexpr int    kickOnAndOff   = 2;
    constexpr size_t firstTrack     = 0;
    constexpr size_t secondTrack    = 1;
    constexpr size_t thirdTrack     = 2;
    constexpr size_t firstSection   = 0;
    constexpr size_t firstEvent     = 0;
    constexpr size_t secondEvent    = 1;
    constexpr double tolerance      = 1e-9;

    constexpr int rootA      = 9;
    constexpr int rootC      = 0;
    constexpr int bassChannel = 2;
    constexpr int accChannel  = 3;
    constexpr int twoTracks   = 2;
    constexpr int threeTracks = 3;

    // a hand-written v4 JSON with NO nttType field -> part-derived defaults
    const juce::String legacyV4Json =
        "{ \"schemaVersion\":4, \"kind\":\"arrangerStyle\", \"sourceTracks\":["
        "  {\"id\":\"d\",\"name\":\"Drums\",\"partType\":\"Drum\",\"channel\":10},"
        "  {\"id\":\"b\",\"name\":\"Bass\",\"partType\":\"Bass\",\"channel\":2},"
        "  {\"id\":\"a\",\"name\":\"Pad\",\"partType\":\"Acc\",\"channel\":3} ], \"sections\":[] }";

    constexpr int    smokeChannel  = 2;
    constexpr int    smokeNote     = 60;
    constexpr double noteOnBeat    = 0.0;
    constexpr double noteOffBeat   = 1.0;
    constexpr int    firstBar      = 1;
    constexpr int    oneBar        = 1;
    constexpr double renderStart   = 0.0;
    constexpr double renderEnd     = 4.0;   // one 4/4 bar
    constexpr int    atLeastOneNote = 1;
}

using namespace ArrangerStyleIOTestValues;

static ArrangerStyleFile makeSample()
{
    ArrangerStyleFile style;
    style.name = sampleStyleName;
    style.originalTempo = tempo;
    style.timeSigNum = beatsPerBar; style.timeSigDenom = beatUnit;

    SourceTrackFile drums;
    drums.id = drumsTrackId; drums.name = drumsName;
    drums.partType = ArrangerPartType::Drum;
    drums.channel = drumChannel; drums.instrument = noInstrument; drums.volume = drumsVolume;
    drums.events.push_back ({ kickOnBeat,  juce::MidiMessage::noteOn  (drumChannel, kickNote, noteVelocity) });
    drums.events.push_back ({ kickOffBeat, juce::MidiMessage::noteOff (drumChannel, kickNote) });
    style.sourceTracks.push_back (drums);

    SectionWindow var;
    var.id = variationId; var.name = variationName;
    var.type = ArrangerSectionType::Variation;
    var.startBar = variationStart; var.lengthBars = variationBars;
    var.afterComplete = ArrangerAfterComplete::Loop;
    style.sections.push_back (var);
    return style;
}

class ArrangerStyleIOTest : public juce::UnitTest
{
public:
    ArrangerStyleIOTest() : juce::UnitTest (ArrangerStyleIOTestValues::testName, ArrangerStyleIOTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (saveThenLoad);
        {
            auto file = juce::File::createTempFile (styleExtension);
            auto original = makeSample();
            ArrangerStyleIOHelper::saveToFile (file, original);

            ArrangerStyleFile loaded; juce::String err;
            const bool ok = ArrangerStyleIOHelper::loadFromFile (file, loaded, err);
            expect (ok, err);

            expectEquals (loaded.schemaVersion, currentSchemaVersion);
            expectEquals (loaded.name, sampleStyleName);
            expectEquals (loaded.timeSigNum, beatsPerBar);
            expectEquals ((int) loaded.sourceTracks.size(), oneTrack);
            expectEquals ((int) loaded.sections.size(), oneSection);

            auto& tr = loaded.sourceTracks[firstTrack];
            expectEquals (tr.channel, drumChannel);
            expect (tr.partType == ArrangerPartType::Drum);
            expectEquals ((int) tr.events.size(), kickOnAndOff);
            expect (tr.events[firstEvent].message.isNoteOn());
            expectEquals (tr.events[firstEvent].message.getNoteNumber(), kickNote);
            expectWithinAbsoluteError (tr.events[secondEvent].beats, kickOffBeat, tolerance);

            auto& sec = loaded.sections[firstSection];
            expect (sec.type == ArrangerSectionType::Variation);
            expectEquals (sec.startBar, variationStart);
            expectEquals (sec.lengthBars, variationBars);
            expect (sec.afterComplete == ArrangerAfterComplete::Loop);

            file.deleteFile();
        }

        beginTest (originalChordRoundTrip);
        {
            auto file = juce::File::createTempFile (styleExtension);
            auto style = makeSample();
            style.originalRoot = rootA; style.originalQuality = ChordQuality::Min;   // A minor
            ArrangerStyleIOHelper::saveToFile (file, style);

            ArrangerStyleFile loaded; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (file, loaded, err), err);
            expectEquals (loaded.originalRoot, rootA);
            expect (loaded.originalQuality == ChordQuality::Min);

            // and it survives the conversion to the runtime style
            ArrangerStyle st = ArrangerPatternBuilder::buildStyleFromFile (loaded);
            expectEquals (st.originalRoot, rootA);
            expect (st.originalQuality == ChordQuality::Min);
            file.deleteFile();
        }

        beginTest (v3LoadsAsCMajor);
        {
            auto file = juce::File::createTempFile (styleExtension);
            file.replaceWithText (v3StyleJson);
            ArrangerStyleFile loaded; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (file, loaded, err), err);
            expectEquals (loaded.originalRoot, rootC);             // C
            expect (loaded.originalQuality == ChordQuality::Maj);  // major
            file.deleteFile();
        }

        beginTest (nttTypeRoundTrip);
        {
            ArrangerStyleFile in;
            in.id = nttStyleId; in.name = nttStyleName;
            SourceTrackFile bass; bass.id = bassTrackId; bass.name = bassName; bass.partType = ArrangerPartType::Bass; bass.channel = bassChannel;
            bass.nttType = NttType::Parallel;
            SourceTrackFile acc;  acc.id = accTrackId;  acc.name = pianoName; acc.partType = ArrangerPartType::Acc;  acc.channel = accChannel;
            acc.nttType  = NttType::Fixed;      // non-default, must survive the round-trip
            in.sourceTracks = { bass, acc };

            auto tmp = juce::File::createTempFile (styleExtension);
            ArrangerStyleIOHelper::saveToFile (tmp, in);
            ArrangerStyleFile out; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (tmp, out, err), err);
            expectEquals ((int) out.sourceTracks.size(), twoTracks);
            expect (out.sourceTracks[firstTrack].nttType == NttType::Parallel);
            expect (out.sourceTracks[secondTrack].nttType == NttType::Fixed);

            auto tmp2 = juce::File::createTempFile (styleExtension); tmp2.replaceWithText (legacyV4Json);
            ArrangerStyleFile mig; expect (ArrangerStyleIOHelper::loadFromFile (tmp2, mig, err), err);
            expectEquals ((int) mig.sourceTracks.size(), threeTracks);
            expect (mig.sourceTracks[firstTrack].nttType == NttType::NoTranspose);  // Drum
            expect (mig.sourceTracks[secondTrack].nttType == NttType::Parallel);    // Bass
            expect (mig.sourceTracks[thirdTrack].nttType == NttType::Chord);        // Acc
            tmp.deleteFile(); tmp2.deleteFile();
        }

        beginTest (missingFileFails);
        {
            // Use a non-existent ABSOLUTE path: juce::File asserts on relative paths.
            auto missing = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getChildFile (missingFileName);
            missing.deleteFile();

            ArrangerStyleFile loaded; juce::String err;
            const bool ok = ArrangerStyleIOHelper::loadFromFile (missing, loaded, err);
            expect (! ok);
            expect (err.isNotEmpty());
        }

        beginTest (fileToEngine);
        {
            // Build a 1-bar variation with a single note, save, reload, build, render one bar.
            ArrangerStyleFile styleFile;
            styleFile.name = smokeStyleName; styleFile.originalTempo = tempo; styleFile.timeSigNum = beatsPerBar; styleFile.timeSigDenom = beatUnit;
            SourceTrackFile track; track.id = smokeTrackId; track.partType = ArrangerPartType::Acc; track.channel = smokeChannel;
            track.events.push_back ({ noteOnBeat,  juce::MidiMessage::noteOn  (smokeChannel, smokeNote, noteVelocity) });
            track.events.push_back ({ noteOffBeat, juce::MidiMessage::noteOff (smokeChannel, smokeNote) });
            styleFile.sourceTracks.push_back (track);
            SectionWindow window; window.id = smokeSectionId; window.type = ArrangerSectionType::Variation;
            window.startBar = firstBar; window.lengthBars = oneBar; window.afterComplete = ArrangerAfterComplete::Loop;
            styleFile.sections.push_back (window);

            auto file = juce::File::createTempFile (styleExtension);
            ArrangerStyleIOHelper::saveToFile (file, styleFile);
            ArrangerStyleFile loaded; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (file, loaded, err), err);

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromFile (loaded);

            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            int noteOns {};
            engine.onMidiMessage = [&] (const juce::MidiMessage& message) { if (message.isNoteOn()) ++noteOns; };
            engine.setStyle (style);
            engine.renderRange (renderStart, renderEnd); // one 4/4 bar

            expect (noteOns >= atLeastOneNote);
            file.deleteFile();
        }
    }
};
static ArrangerStyleIOTest arrangerStyleIOTest;
