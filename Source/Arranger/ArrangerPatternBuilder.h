#pragma once
#include <JuceHeader.h>
#include "ArrangerModel.h"
#include "ArrangerStyleFile.h"
#include "TrackEntry.h"
#include <vector>

/** Builds Phase-1 arranger content (beat-based patterns, single-section style) from TrackEntry data. */
namespace ArrangerPatternBuilder
{
    /** Convert a seconds-timestamped sequence to note-on/off beat events on `channel`. */
    std::vector<TimedBeatEvent> buildBeatEvents (const juce::MidiMessageSequence& sequence,
                                                 double referenceBpm, int channel);

    /** Wrap the selected tracks as one looped section. Channels: perc->10, melodic->2,3,4,…
        Loop length = longest track rounded up to whole bars.
        `referenceBpm` MUST be the tempo `TrackEntry::sequence` is currently scaled to
        (i.e. the live playback tempo) so beat positions line up with the bar grid. */
    ArrangerStyle buildSingleSectionStyle (const std::vector<TrackEntry>& tracks,
                                           int timeSigNum, int timeSigDenom,
                                           double referenceBpm);

    /** Phase-2 demo: derive Intro/Variation/Fill/Ending from the single loop produced by
        buildSingleSectionStyle. Transport rules are real (one-shot vs loop); distinct
        per-section audio is Phase 3. Fill = the loop's last bar. Order: Intro, Variation,
        Fill, Ending (the engine defaults to the first Variation). */
    ArrangerStyle buildDemoMultiSectionStyle (const std::vector<TrackEntry>& tracks,
                                              int timeSigNum, int timeSigDenom,
                                              double referenceBpm);

    /** Slice whole bars out of a section and rebase events to beat 0. Public so both the
        demo builder and the window builder share one implementation. */
    ArrangerSection sliceSection (const ArrangerSection& src, double startBeats, int numBars, double beatsPerBar);

    /** Build a runtime ArrangerStyle from source tracks + explicit bar-window sections. */
    ArrangerStyle buildStyleFromWindows (const std::vector<SourceTrackFile>& sourceTracks,
                                         const std::vector<SectionWindow>& windows,
                                         int timeSigNum, int timeSigDenom,
                                         double referenceBpm);

    /** Convenience: build a runtime ArrangerStyle straight from a loaded style file. */
    ArrangerStyle buildStyleFromFile (const ArrangerStyleFile& file);
}
