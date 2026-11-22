#include "ArrangerTime.h"
#include <cmath>
#include "ArrangerModel.h"

namespace ArrangerTime
{
    constexpr double noBeats = 0.0;
    constexpr int    noBars  = 0;

    double secondsToBeats (double seconds, double bpm)
    {
        if (bpm <= noBeats) bpm = ArrangerStyleDefaults::tempoBpm;
        return seconds * (bpm / ArrangerStyleDefaults::secondsPerMinute);
    }

    double beatsToSeconds (double beats, double bpm)
    {
        if (bpm <= noBeats) bpm = ArrangerStyleDefaults::tempoBpm;
        return beats * (ArrangerStyleDefaults::secondsPerMinute / bpm);
    }

    double beatsPerBar (int timeSigNum, int timeSigDenom)
    {
        if (timeSigNum <= int {})   timeSigNum = ArrangerStyleDefaults::beatsPerBar;
        if (timeSigDenom <= int {}) timeSigDenom = ArrangerStyleDefaults::beatUnit;
        return (double) timeSigNum * (ArrangerStyleDefaults::quarterNotesPerWholeNote / (double) timeSigDenom);
    }

    int barsForBeats (double beats, double beatsPerBarValue)
    {
        if (beatsPerBarValue <= noBeats || beats <= noBeats) return noBars;
        const double bars = beats / beatsPerBarValue;
        return (int) std::ceil (bars - BeatTolerance::wholeBars);
    }
}
