#pragma once

#include <atomic>

/**
    Classic playback's position, in its sequences' own seconds.

    The track player gets sequences already scaled for the tempo set when it starts. Each tick moves the position on
    by the wall-clock time x live tempo / that tempo, so a tempo change while playing changes the speed from that
    moment on and never moves the musical position (the arranger's beat playhead works the same way).

    setTempo may be called from any thread. restart, advance and the readers belong to the player: restart before its
    timer starts, advance on the timer thread.
*/
class TempoClock
{
public:
    static constexpr double defaultTempoBpm  = 120.0;
    static constexpr double secondsPerMinute = 60.0;

    void   setTempo (double bpm) { if (bpm > noTempo) liveTempo.store (bpm); }
    double getTempo() const      { return liveTempo.load(); }

    /** Back to the start, for sequences scaled for the tempo set now. */
    void restart()
    {
        sequenceTempo = liveTempo.load();
        position = {};
    }

    /** Moves on by this much wall-clock time; returns the new position in sequence seconds. */
    double advance (double wallSeconds)
    {
        position += wallSeconds * liveTempo.load() / sequenceTempo;
        return position;
    }

    double positionSeconds() const { return position; }

    /** Beats played so far, counted in the sequences' own beats. */
    double beats() const { return position * sequenceTempo / secondsPerMinute; }

private:
    static constexpr double noTempo = 0.0;

    std::atomic<double> liveTempo { defaultTempoBpm };
    double sequenceTempo = defaultTempoBpm;
    double position {};
};
