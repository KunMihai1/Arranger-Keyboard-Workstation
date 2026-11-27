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
#include "../../Arranger/Chord.h"

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

    /** Shows a tempo set elsewhere (the style's own slider, a style being opened): clamped, applied even
        while locked, and not a user change. */
    void   showValue (double tempoFromElsewhere);

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

    void post (const juce::String& message, double nowMs, double showForMs = showTimeMs);
    juce::String current (double nowMs) const;
    double elapsedMs (double nowMs) const;   ///< since the current message was posted

private:
    juce::String text;
    double postedAtMs {};
    double durationMs { showTimeMs };
};

/** Text shown on the screen. */
namespace StageText
{
    /** "C maj", "F# min7", "C maj/E"; empty for no chord. */
    juce::String chordName (const ArrangerChord& chord);

    /** Bar number from 1 for the elapsed beats (1 before the start or without a time signature). */
    int barNumber (double elapsedBeats, int beatsPerBar);

    /** A message wider than the screen row scrolls like an LCD: it holds, scrolls left until its end is
        visible, then holds again. */
    inline constexpr double marqueeHoldMs = 800.0;

    /** How long a message with this many px of overflow stays: never less than ScreenMessages::showTimeMs. */
    double marqueeDurationMs (float overflowPx, float pixelsPerMs);

    /** How far (px) the message has scrolled left after elapsedMs. */
    float marqueeOffset (float overflowPx, float pixelsPerMs, double elapsedMs);
}

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

/** What the Style page shows: the style itself, or one of the pages opened from it (track picker, notes,
    instrument picker). Exit / Esc from those goes back to the style; closing the Styles page forgets them. */
enum class StyleView { style, tracks, notes, instruments, sections };

class StyleViewRouter
{
public:
    StyleView show (StyleView view);
    StyleView back();
    void reset();
    StyleView current() const;

private:
    StyleView shown { StyleView::style };
};
