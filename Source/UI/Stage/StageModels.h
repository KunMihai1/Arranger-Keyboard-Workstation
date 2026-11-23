/*
  ==============================================================================

    StageModels.h

    Pure models behind the Stage panel (spec 2026-11-23 §5), no JUCE components:
      - TempoModel:    the screen tempo (range, typing, wheel, hold timing, lock)
      - BeatLampModel: which beat lamp is lit
      - ScreenMessages: the short message in the screen's top-left corner
      - PageRouter:    which page is open over the note area

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>

/** Tempo shown on the screen. Every mutator returns true only when the value
    changed; while locked (arranger playing) every mutator refuses. */
class TempoModel
{
public:
    static constexpr double slowest = 60.0;
    static constexpr double fastest = 200.0;

    explicit TempoModel (double styleTempo);

    double value() const;
    void   setLocked (bool shouldBeLocked);
    bool   isLocked() const;
    void   setStyleTempo (double newStyleTempo);

    bool step (int direction);
    bool applyTyped (const juce::String& typedText);
    bool wheel (int notches, bool coarse);
    bool resetToStyle();

    /** Delay before the next repeat while a tempo button is held, after step stepIndex
        (0 = the step made on press). */
    static int holdDelayMs (int stepIndex);

private:
    bool setValue (double newValue);

    double styleTempo {};
    double currentValue {};
    bool locked {};
};

enum class LampState { off, downbeat, beat };

struct BeatLampModel
{
    /** One lamp per beat of the bar; the current beat's lamp is lit (downbeat red,
        others yellow). All off while stopped or asleep. */
    static std::vector<LampState> lamps (int beatsPerBar, double elapsedBeats, bool playing, bool asleep);
};

/** The screen's message line: upper case, shown for showTimeMs, a newer post replaces the older. */
class ScreenMessages
{
public:
    static constexpr double showTimeMs = 2500.0;

    void post (const juce::String& message, double nowMs);
    juce::String current (double nowMs) const;

private:
    juce::String text;
    double postedAtMs {};
};

enum class StagePage { none, effects, settings, styles, voices, chords, file, home };

/** At most one page is open; pressing its button again closes it. */
class PageRouter
{
public:
    StagePage toggle (StagePage page);
    StagePage close();
    StagePage open() const;

private:
    StagePage openPage { StagePage::none };
};
