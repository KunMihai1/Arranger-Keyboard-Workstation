#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Arranger/ArrangerPatternBuilder.h"
#include "Arranger/ArrangerStyleFile.h"
#include "TrackEntry.h"

namespace ArrangerPatternBuilderTestValues
{
    const juce::String testName     = "ArrangerPatternBuilder";
    const juce::String testCategory = "Arranger";

    const juce::String secondsToBeats          = "seconds convert to beats at the reference BPM";
    const juce::String nonNoteEventsSkipped    = "non-note events are skipped";
    const juce::String singleSectionStyle      = "single-section style: channel mapping and loop length";
    const juce::String zeroBpmFallsBack        = "originalBPM <= 0 falls back to 120";
    const juce::String demoMultiSectionStyle   = "demo multi-section style: four sections with the right transport rules";
    const juce::String fillSlicesLastBar       = "fill slices the loop's last bar and rebases it to beat 0";
    const juce::String windowsSliceAndRebase   = "buildStyleFromWindows slices real windows and rebases to beat 0";
    const juce::String windowsKeepTrackSettings = "buildStyleFromWindows preserves per-track channel and afterComplete";
    const juce::String fileCopiesMetadata      = "buildStyleFromFile copies metadata and builds sections";

    const juce::String keysTrackId     = "t1";
    const juce::String keysName        = "Keys";
    const juce::String introId         = "intro_1";
    const juce::String introName       = "Intro 1";
    const juce::String variationId     = "var_1";
    const juce::String variationName   = "Variation 1";
    const juce::String drumsTrackId    = "d";
    const juce::String endingId        = "end";
    const juce::String fileStyleName   = "X";

    constexpr int         recordedChannel = 1;
    constexpr int         melodicChannel  = 2;
    constexpr int         drumChannel     = 10;
    constexpr int         overrideChannel = 5;
    constexpr int         middleC         = 60;
    constexpr int         noteG4          = 67;
    constexpr int         noteC5          = 72;
    constexpr int         kickNote        = 36;
    constexpr juce::uint8 velocity        = 100;
    constexpr int         volumeController = 7;
    constexpr int         controllerValue  = 100;
    constexpr int         grandPiano      = 0;
    constexpr double      trackVolume     = 100.0;

    constexpr double tempo        = 120.0;
    constexpr double fileTempo    = 100.0;
    constexpr double noTempo      = 0.0;    // falls back to 120
    constexpr int    beatsPerBar  = 4;
    constexpr int    beatUnit     = 4;
    constexpr double tolerance    = 1e-9;

    // Times in seconds (at 120 BPM one beat lasts half a second).
    constexpr double atStart        = 0.0;
    constexpr double oneSecond      = 1.0;    // 2 beats
    constexpr double twoSeconds     = 2.0;    // 4 beats: one 4/4 bar
    constexpr double threeSeconds   = 3.0;    // 6 beats: into bar 2
    constexpr double threeAndAHalf  = 3.5;
    constexpr double fiveSeconds    = 5.0;    // 10 beats: bar 3
    constexpr double fiveAndAHalf   = 5.5;    // 11 beats

    // Times in beats.
    constexpr double beatZero      = 0.0;
    constexpr double halfBeat      = 0.5;
    constexpr double twoBeats      = 2.0;
    constexpr double beatEight     = 8.0;    // the start of bar 3
    constexpr double beatEightAndAHalf = 8.5;

    constexpr int    oneBar         = 1;
    constexpr int    twoBars        = 2;
    constexpr int    threeBars      = 3;
    constexpr int    firstBar       = 1;
    constexpr int    thirdBar       = 3;
    constexpr int    oneEvent       = 1;
    constexpr int    twoEvents      = 2;
    constexpr int    oneSection     = 1;
    constexpr int    twoSections    = 2;
    constexpr int    fourSections   = 4;
    constexpr int    oneTrack       = 1;
    constexpr int    twoTracks      = 2;
    constexpr int    demoSchemaVersion = 2;

    constexpr size_t firstEvent     = 0;
    constexpr size_t secondEvent    = 1;
    constexpr size_t firstTrack     = 0;
    constexpr size_t secondTrack    = 1;
    constexpr size_t introSection   = 0;   // demo order: Intro, Variation, Fill, Ending
    constexpr size_t variationSection = 1;
    constexpr size_t fillSection    = 2;
    constexpr size_t endingSection  = 3;
    constexpr size_t firstSection   = 0;
    constexpr size_t secondSection  = 1;
}

using namespace ArrangerPatternBuilderTestValues;

class ArrangerPatternBuilderTest : public juce::UnitTest
{
public:
    ArrangerPatternBuilderTest() : juce::UnitTest (ArrangerPatternBuilderTestValues::testName, ArrangerPatternBuilderTestValues::testCategory) {}

    void runTest() override
    {
        beginTest (secondsToBeats);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent (juce::MidiMessage::noteOn (recordedChannel, middleC, velocity), atStart);
            seq.addEvent (juce::MidiMessage::noteOff (recordedChannel, middleC), oneSecond); // 1.0s -> 2 beats @120

            auto evts = ArrangerPatternBuilder::buildBeatEvents (seq, tempo, overrideChannel);
            expectEquals ((int) evts.size(), twoEvents);
            expectWithinAbsoluteError (evts[firstEvent].beats, beatZero, tolerance);
            expectWithinAbsoluteError (evts[secondEvent].beats, twoBeats, tolerance);
            // channel is overridden to the supplied value
            expectEquals (evts[firstEvent].message.getChannel(), overrideChannel);
            expectEquals (evts[secondEvent].message.getChannel(), overrideChannel);
        }

        beginTest (nonNoteEventsSkipped);
        {
            juce::MidiMessageSequence seq;
            seq.addEvent (juce::MidiMessage::controllerEvent (recordedChannel, volumeController, controllerValue), atStart);
            seq.addEvent (juce::MidiMessage::noteOn (recordedChannel, middleC, velocity), atStart);
            auto evts = ArrangerPatternBuilder::buildBeatEvents (seq, tempo, melodicChannel);
            expectEquals ((int) evts.size(), oneEvent);
            expect (evts[firstEvent].message.isNoteOn());
        }

        beginTest (singleSectionStyle);
        {
            // melodic track: a note ending at 2.0s -> 4 beats @120 -> exactly 1 bar (4/4).
            TrackEntry melodic;
            melodic.type = TrackType::Melodic;
            melodic.originalBPM = tempo;
            melodic.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, middleC, velocity), atStart);
            melodic.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, middleC), twoSeconds);

            // percussion track: a hit ending at 3.0s -> 6 beats -> rounds up to 2 bars.
            TrackEntry perc;
            perc.type = TrackType::Percussion;
            perc.originalBPM = tempo;
            perc.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, kickNote, velocity), atStart);
            perc.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, kickNote), threeSeconds);

            std::vector<TrackEntry> tracks { melodic, perc };
            ArrangerStyle style = ArrangerPatternBuilder::buildSingleSectionStyle (tracks, beatsPerBar, beatUnit, tempo);

            expectEquals ((int) style.sections.size(), oneSection);
            auto& sec = style.sections[firstSection];
            expectEquals ((int) sec.tracks.size(), twoTracks);

            // melodic -> channel 2; percussion -> channel 10
            expectEquals (sec.tracks[firstTrack].channel, melodicChannel);
            expect (sec.tracks[firstTrack].partType != ArrangerPartType::Drum);
            expectEquals (sec.tracks[secondTrack].channel, drumChannel);
            expect (sec.tracks[secondTrack].partType == ArrangerPartType::Drum);

            // loop length = max(1 bar, 2 bars) = 2 bars
            expectEquals (sec.lengthBars, twoBars);
        }

        beginTest (zeroBpmFallsBack);
        {
            TrackEntry track;
            track.type = TrackType::Melodic;
            track.originalBPM = noTempo;
            track.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, middleC, velocity), atStart);
            track.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, middleC), twoSeconds); // 2s -> 4 beats @120
            std::vector<TrackEntry> tracks { track };
            ArrangerStyle style = ArrangerPatternBuilder::buildSingleSectionStyle (tracks, beatsPerBar, beatUnit, noTempo);
            expectEquals (style.sections[firstSection].lengthBars, oneBar);
        }

        beginTest (demoMultiSectionStyle);
        {
            // melodic note ends at 2.0s -> 4 beats @120 -> 1 bar; perc hit later -> 2 bars.
            TrackEntry melodic;
            melodic.type = TrackType::Melodic;
            melodic.originalBPM = tempo;
            melodic.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, middleC, velocity), atStart);
            melodic.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, middleC), twoSeconds);

            TrackEntry perc;
            perc.type = TrackType::Percussion;
            perc.originalBPM = tempo;
            perc.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, kickNote, velocity), fiveSeconds); // 10 beats -> bar 3 area
            perc.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, kickNote), fiveAndAHalf);

            std::vector<TrackEntry> tracks { melodic, perc };
            ArrangerStyle style = ArrangerPatternBuilder::buildDemoMultiSectionStyle (tracks, beatsPerBar, beatUnit, tempo);

            expectEquals (style.schemaVersion, demoSchemaVersion);
            expectEquals ((int) style.sections.size(), fourSections);

            // order: Intro, Variation, Fill, Ending
            expect (style.sections[introSection].type == ArrangerSectionType::Intro);
            expect (style.sections[variationSection].type == ArrangerSectionType::Variation);
            expect (style.sections[fillSection].type == ArrangerSectionType::Fill);
            expect (style.sections[endingSection].type == ArrangerSectionType::Ending);

            expect (style.sections[introSection].afterComplete == ArrangerAfterComplete::FallThrough); // intro
            expect (style.sections[variationSection].afterComplete == ArrangerAfterComplete::Loop);    // variation
            expect (style.sections[fillSection].afterComplete == ArrangerAfterComplete::FallThrough);  // fill
            expect (style.sections[endingSection].afterComplete == ArrangerAfterComplete::Stop);       // ending

            // Intro and Fill are one-bar slices; Ending is the last min(4, loopBars) bars; Variation is the full loop.
            // perc note-off at 5.5s -> 11 beats @120 -> 3 bars, so the Variation (and the ending window) span 3 bars.
            expectEquals (style.sections[variationSection].lengthBars, threeBars); // variation = full loop
            expectEquals (style.sections[introSection].lengthBars, oneBar);        // intro is one bar
            expectEquals (style.sections[fillSection].lengthBars, oneBar);         // fill is one bar
            expectEquals (style.sections[endingSection].lengthBars, threeBars);    // ending = last min(4, loopBars) = 3 bars

            // Variation keeps the Phase-1 channel mapping: melodic -> 2, perc -> 10.
            expectEquals (style.sections[variationSection].tracks[firstTrack].channel, melodicChannel);
            expectEquals (style.sections[variationSection].tracks[secondTrack].channel, drumChannel);
        }

        beginTest (fillSlicesLastBar);
        {
            // single 2-bar loop: a melodic hit in the last bar (beat ~6) must reappear in the fill near beat 2.
            TrackEntry track;
            track.type = TrackType::Melodic;
            track.originalBPM = tempo;
            track.sequence.addEvent (juce::MidiMessage::noteOn  (recordedChannel, noteC5, velocity), threeSeconds); // 6 beats @120 -> bar 2
            track.sequence.addEvent (juce::MidiMessage::noteOff (recordedChannel, noteC5), threeAndAHalf);

            std::vector<TrackEntry> tracks { track };
            ArrangerStyle style = ArrangerPatternBuilder::buildDemoMultiSectionStyle (tracks, beatsPerBar, beatUnit, tempo);

            expectEquals (style.sections[variationSection].lengthBars, twoBars);          // variation is 2 bars
            auto& fillTrack = style.sections[fillSection].tracks[firstTrack];
            expect (! fillTrack.pattern.empty());
            // event at beat 6 in a 2-bar (8-beat) loop -> fill (last bar starts at beat 4) -> beat 2.
            expectWithinAbsoluteError (fillTrack.pattern.front().beats, twoBeats, tolerance);
        }

        beginTest (windowsSliceAndRebase);
        {
            // One melodic source track. 4/4 -> 1 bar = 4 beats.
            // Put a note at beat 0 (bar 1) and a note at beat 8 (bar 3).
            SourceTrackFile track;
            track.id = keysTrackId; track.name = keysName; track.partType = ArrangerPartType::Acc;
            track.channel = melodicChannel; track.instrument = grandPiano; track.volume = trackVolume;
            track.events.push_back ({ beatZero,          juce::MidiMessage::noteOn  (melodicChannel, middleC, velocity) });
            track.events.push_back ({ halfBeat,          juce::MidiMessage::noteOff (melodicChannel, middleC) });
            track.events.push_back ({ beatEight,         juce::MidiMessage::noteOn  (melodicChannel, noteG4, velocity) }); // bar 3
            track.events.push_back ({ beatEightAndAHalf, juce::MidiMessage::noteOff (melodicChannel, noteG4) });

            std::vector<SourceTrackFile> tracks { track };

            SectionWindow intro;
            intro.id = introId; intro.name = introName; intro.type = ArrangerSectionType::Intro;
            intro.startBar = firstBar; intro.lengthBars = oneBar; intro.afterComplete = ArrangerAfterComplete::FallThrough;

            SectionWindow var;
            var.id = variationId; var.name = variationName; var.type = ArrangerSectionType::Variation;
            var.startBar = thirdBar; var.lengthBars = oneBar; var.afterComplete = ArrangerAfterComplete::Loop;

            std::vector<SectionWindow> windows { intro, var };

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromWindows (tracks, windows, beatsPerBar, beatUnit, tempo);

            expectEquals ((int) style.sections.size(), twoSections);
            expect (style.sections[firstSection].type == ArrangerSectionType::Intro);
            expect (style.sections[secondSection].type == ArrangerSectionType::Variation);

            // Intro = bar 1: the beat-0 note stays at beat 0.
            expectEquals ((int) style.sections[firstSection].tracks.size(), oneTrack);
            expect (! style.sections[firstSection].tracks[firstTrack].pattern.empty());
            expectWithinAbsoluteError (style.sections[firstSection].tracks[firstTrack].pattern.front().beats, beatZero, tolerance);
            expectEquals (style.sections[firstSection].lengthBars, oneBar);

            // Variation = bar 3 (starts at beat 8): the beat-8 note rebases to beat 0.
            expect (! style.sections[secondSection].tracks[firstTrack].pattern.empty());
            expectWithinAbsoluteError (style.sections[secondSection].tracks[firstTrack].pattern.front().beats, beatZero, tolerance);
            expectEquals (style.sections[secondSection].tracks[firstTrack].pattern.front().message.getNoteNumber(), noteG4);
        }

        beginTest (windowsKeepTrackSettings);
        {
            SourceTrackFile drums;
            drums.id = drumsTrackId; drums.partType = ArrangerPartType::Drum; drums.channel = drumChannel;
            drums.events.push_back ({ beatZero, juce::MidiMessage::noteOn  (drumChannel, kickNote, velocity) });
            drums.events.push_back ({ halfBeat, juce::MidiMessage::noteOff (drumChannel, kickNote) });

            SectionWindow ending;
            ending.id = endingId; ending.type = ArrangerSectionType::Ending;
            ending.startBar = firstBar; ending.lengthBars = oneBar; ending.afterComplete = ArrangerAfterComplete::Stop;

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromWindows ({ drums }, { ending }, beatsPerBar, beatUnit, tempo);
            expectEquals (style.sections[firstSection].tracks[firstTrack].channel, drumChannel);
            expect (style.sections[firstSection].afterComplete == ArrangerAfterComplete::Stop);
        }

        beginTest (fileCopiesMetadata);
        {
            ArrangerStyleFile styleFile;
            styleFile.name = fileStyleName; styleFile.originalTempo = fileTempo; styleFile.timeSigNum = beatsPerBar; styleFile.timeSigDenom = beatUnit;
            SourceTrackFile track; track.channel = melodicChannel;
            track.events.push_back ({ beatZero, juce::MidiMessage::noteOn  (melodicChannel, middleC, velocity) });
            track.events.push_back ({ halfBeat, juce::MidiMessage::noteOff (melodicChannel, middleC) });
            styleFile.sourceTracks.push_back (track);
            SectionWindow window; window.type = ArrangerSectionType::Variation; window.startBar = firstBar; window.lengthBars = oneBar;
            styleFile.sections.push_back (window);

            ArrangerStyle style = ArrangerPatternBuilder::buildStyleFromFile (styleFile);
            expectWithinAbsoluteError (style.originalTempo, fileTempo, tolerance);
            expectEquals ((int) style.sections.size(), oneSection);
        }
    }
};

static ArrangerPatternBuilderTest arrangerPatternBuilderTest;
