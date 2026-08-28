#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerStyleIOHelper.h"
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerEngine.h"

static ArrangerStyleFile makeSample()
{
    ArrangerStyleFile s;
    s.name = "Sunset Groove";
    s.originalTempo = 120.0;
    s.timeSigNum = 4; s.timeSigDenom = 4;

    SourceTrackFile drums;
    drums.id = "t1"; drums.name = "Drums";
    drums.partType = ArrangerPartType::Drum;
    drums.channel = 10; drums.instrument = -1; drums.volume = 100.0;
    drums.events.push_back ({ 0.0, juce::MidiMessage::noteOn  (10, 36, (juce::uint8) 100) });
    drums.events.push_back ({ 0.5, juce::MidiMessage::noteOff (10, 36) });
    s.sourceTracks.push_back (drums);

    SectionWindow var;
    var.id = "var_1"; var.name = "Variation 1";
    var.type = ArrangerSectionType::Variation;
    var.startBar = 5; var.lengthBars = 8;
    var.afterComplete = ArrangerAfterComplete::Loop;
    s.sections.push_back (var);
    return s;
}

class ArrangerStyleIOTest : public juce::UnitTest
{
public:
    ArrangerStyleIOTest() : juce::UnitTest ("ArrangerStyleIO", "Arranger") {}

    void runTest() override
    {
        beginTest ("save then load reproduces the style");
        {
            auto file = juce::File::createTempFile (".style");
            auto original = makeSample();
            ArrangerStyleIOHelper::saveToFile (file, original);

            ArrangerStyleFile loaded; juce::String err;
            const bool ok = ArrangerStyleIOHelper::loadFromFile (file, loaded, err);
            expect (ok, err);

            expectEquals (loaded.schemaVersion, 5);
            expectEquals (loaded.name, juce::String ("Sunset Groove"));
            expectEquals (loaded.timeSigNum, 4);
            expectEquals ((int) loaded.sourceTracks.size(), 1);
            expectEquals ((int) loaded.sections.size(), 1);

            auto& tr = loaded.sourceTracks[0];
            expectEquals (tr.channel, 10);
            expect (tr.partType == ArrangerPartType::Drum);
            expectEquals ((int) tr.events.size(), 2);
            expect (tr.events[0].message.isNoteOn());
            expectEquals (tr.events[0].message.getNoteNumber(), 36);
            expectWithinAbsoluteError (tr.events[1].beats, 0.5, 1e-9);

            auto& sec = loaded.sections[0];
            expect (sec.type == ArrangerSectionType::Variation);
            expectEquals (sec.startBar, 5);
            expectEquals (sec.lengthBars, 8);
            expect (sec.afterComplete == ArrangerAfterComplete::Loop);

            file.deleteFile();
        }

        beginTest ("v4 round-trips the style-level original chord");
        {
            auto file = juce::File::createTempFile (".style");
            auto s = makeSample();
            s.originalRoot = 9; s.originalQuality = ChordQuality::Min;   // A minor
            ArrangerStyleIOHelper::saveToFile (file, s);

            ArrangerStyleFile loaded; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (file, loaded, err), err);
            expectEquals (loaded.originalRoot, 9);
            expect (loaded.originalQuality == ChordQuality::Min);

            // and it survives the conversion to the runtime style
            ArrangerStyle st = ArrangerPatternBuilder::buildStyleFromFile (loaded);
            expectEquals (st.originalRoot, 9);
            expect (st.originalQuality == ChordQuality::Min);
            file.deleteFile();
        }

        beginTest ("a v3 file with no original chord loads as C major");
        {
            // Write a minimal v3-style JSON by hand (no originalRoot/originalQuality keys).
            auto file = juce::File::createTempFile (".style");
            file.replaceWithText (
                "{ \"schemaVersion\": 3, \"kind\": \"arrangerStyle\", \"id\": \"x\", \"name\": \"Old\","
                "  \"originalTempo\": 120.0, \"timeSigNum\": 4, \"timeSigDenom\": 4,"
                "  \"sourceTracks\": [], \"sections\": [] }");
            ArrangerStyleFile loaded; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (file, loaded, err), err);
            expectEquals (loaded.originalRoot, 0);                 // C
            expect (loaded.originalQuality == ChordQuality::Maj);  // major
            file.deleteFile();
        }

        beginTest ("nttType round-trips and a missing field migrates by part");
        {
            ArrangerStyleFile in;
            in.id = "s"; in.name = "S";
            SourceTrackFile bass; bass.id = "b"; bass.name = "Bass"; bass.partType = ArrangerPartType::Bass; bass.channel = 2;
            bass.nttType = NttType::Parallel;
            SourceTrackFile acc;  acc.id = "a";  acc.name = "Piano"; acc.partType = ArrangerPartType::Acc;  acc.channel = 3;
            acc.nttType  = NttType::Fixed;      // non-default, must survive the round-trip
            in.sourceTracks = { bass, acc };

            auto tmp = juce::File::createTempFile (".style");
            ArrangerStyleIOHelper::saveToFile (tmp, in);
            ArrangerStyleFile out; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (tmp, out, err), err);
            expectEquals ((int) out.sourceTracks.size(), 2);
            expect (out.sourceTracks[0].nttType == NttType::Parallel);
            expect (out.sourceTracks[1].nttType == NttType::Fixed);

            // a hand-written v4 JSON with NO nttType field -> part-derived defaults
            const juce::String legacy =
                "{ \"schemaVersion\":4, \"kind\":\"arrangerStyle\", \"sourceTracks\":["
                "  {\"id\":\"d\",\"name\":\"Drums\",\"partType\":\"Drum\",\"channel\":10},"
                "  {\"id\":\"b\",\"name\":\"Bass\",\"partType\":\"Bass\",\"channel\":2},"
                "  {\"id\":\"a\",\"name\":\"Pad\",\"partType\":\"Acc\",\"channel\":3} ], \"sections\":[] }";
            auto tmp2 = juce::File::createTempFile (".style"); tmp2.replaceWithText (legacy);
            ArrangerStyleFile mig; expect (ArrangerStyleIOHelper::loadFromFile (tmp2, mig, err), err);
            expectEquals ((int) mig.sourceTracks.size(), 3);
            expect (mig.sourceTracks[0].nttType == NttType::NoTranspose);  // Drum
            expect (mig.sourceTracks[1].nttType == NttType::Parallel);     // Bass
            expect (mig.sourceTracks[2].nttType == NttType::Chord);        // Acc
            tmp.deleteFile(); tmp2.deleteFile();
        }

        beginTest ("loading a missing file fails gracefully");
        {
            // Use a non-existent ABSOLUTE path: juce::File asserts on relative paths.
            auto missing = juce::File::getSpecialLocation (juce::File::tempDirectory)
                               .getChildFile ("arranger-style-does-not-exist-9d3f.style");
            missing.deleteFile();

            ArrangerStyleFile loaded; juce::String err;
            const bool ok = ArrangerStyleIOHelper::loadFromFile (missing, loaded, err);
            expect (! ok);
            expect (err.isNotEmpty());
        }

        beginTest ("file -> buildStyleFromFile -> engine renders the first section");
        {
            // Build a 1-bar variation with a single note, save, reload, build, render one bar.
            ArrangerStyleFile f;
            f.name = "Smoke"; f.originalTempo = 120.0; f.timeSigNum = 4; f.timeSigDenom = 4;
            SourceTrackFile t; t.id = "t"; t.partType = ArrangerPartType::Acc; t.channel = 2;
            t.events.push_back ({ 0.0, juce::MidiMessage::noteOn  (2, 60, (juce::uint8) 100) });
            t.events.push_back ({ 1.0, juce::MidiMessage::noteOff (2, 60) });
            f.sourceTracks.push_back (t);
            SectionWindow w; w.id = "v"; w.type = ArrangerSectionType::Variation;
            w.startBar = 1; w.lengthBars = 1; w.afterComplete = ArrangerAfterComplete::Loop;
            f.sections.push_back (w);

            auto file = juce::File::createTempFile (".style");
            ArrangerStyleIOHelper::saveToFile (file, f);
            ArrangerStyleFile loaded; juce::String err;
            expect (ArrangerStyleIOHelper::loadFromFile (file, loaded, err), err);

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromFile (loaded);

            ArrangerEngine engine (std::weak_ptr<juce::MidiOutput>{});
            int noteOns = 0;
            engine.onMidiMessage = [&] (const juce::MidiMessage& m) { if (m.isNoteOn()) ++noteOns; };
            engine.setStyle (style);
            engine.renderRange (0.0, 4.0); // one 4/4 bar

            expect (noteOns >= 1);
            file.deleteFile();
        }
    }
};
static ArrangerStyleIOTest arrangerStyleIOTest;
