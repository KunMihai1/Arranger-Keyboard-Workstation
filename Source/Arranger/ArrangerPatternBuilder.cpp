#include "ArrangerPatternBuilder.h"
#include "ArrangerStyleFile.h"
#include "ArrangerTime.h"
#include "ArrangerChannels.h"
#include <algorithm>

namespace ArrangerPatternBuilder
{
    constexpr int    oneBar            = 1;
    constexpr int    demoEndingBars    = 4;     // the demo style's short wind-down
    constexpr double noTempo           = 0.0;

    std::vector<TimedBeatEvent> buildBeatEvents (const juce::MidiMessageSequence& sequence,
                                                 double referenceBpm, int channel)
    {
        std::vector<TimedBeatEvent> out;
        for (int eventIndex {}; eventIndex < sequence.getNumEvents(); ++eventIndex)
        {
            const auto& sourceMessage = sequence.getEventPointer (eventIndex)->message;
            if (! (sourceMessage.isNoteOn() || sourceMessage.isNoteOff()))
                continue;

            juce::MidiMessage onChannel = sourceMessage;
            onChannel.setChannel (channel);
            out.push_back ({ ArrangerTime::secondsToBeats (sourceMessage.getTimeStamp(), referenceBpm), onChannel });
        }
        std::stable_sort (out.begin(), out.end(),
                          [] (const TimedBeatEvent& earlier, const TimedBeatEvent& later) { return earlier.beats < later.beats; });
        return out;
    }

    ArrangerStyle buildSingleSectionStyle (const std::vector<TrackEntry>& tracks,
                                           int timeSigNum, int timeSigDenom,
                                           double referenceBpm)
    {
        if (referenceBpm <= noTempo)
            referenceBpm = ArrangerStyleDefaults::tempoBpm;

        ArrangerStyle style;
        style.timeSigNum = timeSigNum;
        style.timeSigDenom = timeSigDenom;
        style.originalTempo = referenceBpm;

        const double beatsPerBarValue = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);

        ArrangerSection section;
        section.type = ArrangerSectionType::Variation;
        section.name = DefaultSectionNames::variation;
        section.id = DefaultSectionNames::variationId;

        int melodicChannel = ArrangerChannels::kFirstMelodicChannel;
        int maxBars {};

        for (const auto& trackEntry : tracks)
        {
            const bool isPerc = (trackEntry.type == TrackType::Percussion);
            const int channel = isPerc ? ArrangerChannels::kDrumChannel : melodicChannel++;
            const bool isBass = (! isPerc) && trackEntry.getDisplayName().containsIgnoreCase (TrackRoles::bassNameMarker);

            ArrangerTrack track;
            track.id = trackEntry.getUniqueID();
            track.name = trackEntry.getDisplayName();
            track.partType = isPerc ? ArrangerPartType::Drum
                        : isBass ? ArrangerPartType::Bass
                                 : ArrangerPartType::Acc;
            track.instrument = trackEntry.instrumentAssociated;
            track.channel = channel;
            track.volume = trackEntry.volumeAssociated;
            // The entry's sequence is already scaled to referenceBpm's timebase, so convert with referenceBpm.
            track.pattern = buildBeatEvents (trackEntry.sequence, referenceBpm, channel);

            double lastBeat = track.pattern.empty() ? ArrangerStyleDefaults::sectionStartBeat : track.pattern.back().beats;
            maxBars = std::max (maxBars, ArrangerTime::barsForBeats (lastBeat, beatsPerBarValue));

            section.tracks.push_back (std::move (track));
        }

        section.lengthBars = std::max (ArrangerStyleDefaults::minimumLengthBars, maxBars);
        style.sections.push_back (std::move (section));
        return style;
    }

    ArrangerStyle buildDemoMultiSectionStyle (const std::vector<TrackEntry>& tracks,
                                              int timeSigNum, int timeSigDenom,
                                              double referenceBpm)
    {
        ArrangerStyle base = buildSingleSectionStyle (tracks, timeSigNum, timeSigDenom, referenceBpm);

        ArrangerStyle style;
        style.timeSigNum    = timeSigNum;
        style.timeSigDenom  = timeSigDenom;
        style.originalTempo = base.originalTempo;

        if (base.sections.empty())
            return style;

        const double beatsPerBarValue     = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);
        ArrangerSection var  = base.sections.front();     // the full loop, correct channels/length
        const int loopBars   = juce::jmax (ArrangerStyleDefaults::minimumLengthBars, var.lengthBars);

        var.id = DefaultSectionNames::variationId; var.name = DefaultSectionNames::variation;
        var.type = ArrangerSectionType::Variation;
        var.afterComplete = ArrangerAfterComplete::Loop;

        // Slice a range of whole bars out of a cloned section and rebase to beat 0 (shared
        // sliceSection). Keeps the throwaway demo Intro/Fill/Ending short so the transport
        // (intro-once, fill-once, ending-once-then-stop) is observable instead of replaying the
        // whole-song Variation loop. Lengths are a compromise until Phase 3 supplies real phrases.
        const int    endingBars   = juce::jmin (demoEndingBars, loopBars);     // a short wind-down
        const double lastBarStart = (double) (loopBars - oneBar)     * beatsPerBarValue;
        const double endingStart  = (double) (loopBars - endingBars) * beatsPerBarValue;

        ArrangerSection intro = sliceSection (var, ArrangerStyleDefaults::sectionStartBeat, oneBar, beatsPerBarValue);   // first bar -> lead-in
        intro.id = DefaultSectionNames::introId; intro.name = DefaultSectionNames::intro;
        intro.type = ArrangerSectionType::Intro;
        intro.afterComplete = ArrangerAfterComplete::FallThrough;

        ArrangerSection fill = sliceSection (var, lastBarStart, oneBar, beatsPerBarValue); // last bar -> fill accent
        fill.id = DefaultSectionNames::fillId; fill.name = DefaultSectionNames::fill;
        fill.type = ArrangerSectionType::Fill;
        fill.afterComplete = ArrangerAfterComplete::FallThrough;

        ArrangerSection ending = sliceSection (var, endingStart, endingBars, beatsPerBarValue); // last few bars -> wind-down, then stop
        ending.id = DefaultSectionNames::endingId; ending.name = DefaultSectionNames::ending;
        ending.type = ArrangerSectionType::Ending;
        ending.afterComplete = ArrangerAfterComplete::Stop;

        style.sections.push_back (std::move (intro));
        style.sections.push_back (std::move (var));
        style.sections.push_back (std::move (fill));
        style.sections.push_back (std::move (ending));
        return style;
    }

    ArrangerSection sliceSection (const ArrangerSection& src,
                                  double startBeats, int numBars, double beatsPerBar)
    {
        ArrangerSection sliced = src;
        const int    bars     = juce::jmax (ArrangerStyleDefaults::minimumLengthBars, numBars);
        const double endBeats = startBeats + (double) bars * beatsPerBar;
        for (auto& track : sliced.tracks)
        {
            std::vector<TimedBeatEvent> inWindow;
            for (const auto& event : track.pattern)
                if (event.beats >= startBeats - BeatTolerance::sameBeat && event.beats < endBeats - BeatTolerance::sameBeat)
                    inWindow.push_back ({ event.beats - startBeats, event.message });
            track.pattern = std::move (inWindow);
        }
        sliced.lengthBars = bars;
        return sliced;
    }

    ArrangerStyle buildStyleFromWindows (const std::vector<SourceTrackFile>& sourceTracks,
                                         const std::vector<SectionWindow>& windows,
                                         int timeSigNum, int timeSigDenom,
                                         double referenceBpm)
    {
        if (referenceBpm <= noTempo) referenceBpm = ArrangerStyleDefaults::tempoBpm;

        ArrangerStyle style;
        style.timeSigNum    = timeSigNum;
        style.timeSigDenom  = timeSigDenom;
        style.originalTempo = referenceBpm;

        const double beatsPerBarValue = ArrangerTime::beatsPerBar (timeSigNum, timeSigDenom);

        // Master section = every source track as one full-length ArrangerTrack.
        ArrangerSection master;
        for (const auto& sourceTrack : sourceTracks)
        {
            ArrangerTrack track;
            track.id = sourceTrack.id; track.name = sourceTrack.name; track.partType = sourceTrack.partType;
            track.instrument = sourceTrack.instrument; track.channel = sourceTrack.channel; track.volume = sourceTrack.volume;
            track.nttType = sourceTrack.nttType;   // Phase 7a: file -> model
            track.pattern = sourceTrack.events;
            master.tracks.push_back (std::move (track));
        }

        for (const auto& window : windows)
        {
            const double startBeats = (double) (juce::jmax (ArrangerStyleDefaults::firstBar, window.startBar) - ArrangerStyleDefaults::firstBar) * beatsPerBarValue;
            ArrangerSection section = sliceSection (master, startBeats, juce::jmax (ArrangerStyleDefaults::minimumLengthBars, window.lengthBars), beatsPerBarValue);
            section.id            = window.id;
            section.name          = window.name;
            section.type          = window.type;
            section.afterComplete = window.afterComplete;
            style.sections.push_back (std::move (section));
        }

        return style;
    }

    ArrangerStyle buildStyleFromFile (const ArrangerStyleFile& file)
    {
        ArrangerStyle style = buildStyleFromWindows (file.sourceTracks, file.sections,
                                                     file.timeSigNum, file.timeSigDenom, file.originalTempo);
        style.id   = file.id;
        style.name = file.name;
        style.originalRoot    = file.originalRoot;      // Phase 4: carry the recorded chord through
        style.originalQuality = file.originalQuality;
        return style;
    }
}
