#include "StageModels.h"
#include <cmath>

namespace
{
    // Tempo hold timing (board StageRevised: first repeat after 400 ms, then 130 ms minus 10 ms per step, floor 30 ms)
    constexpr int firstRepeatDelayMs = 400;
    constexpr int repeatStartMs      = 130;
    constexpr int repeatSpeedUpMs    = 10;
    constexpr int fastestRepeatMs    = 30;

    constexpr int fineWheelStep   = 1;
    constexpr int coarseWheelStep = 5;

    // More digits than any tempo has: the typed value is simply the fastest tempo (avoids integer overflow).
    constexpr int maxMeaningfulDigits = 3;
    const juce::String digits = "0123456789";
    const juce::String zero   = "0";

    // Chord names on the screen (board: "C maj").
    const juce::StringArray pitchClassNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const juce::String chordSeparator = " ";
    const juce::String bassSeparator  = "/";
    constexpr int firstBarNumber      = 1;

    double clampTempo (double tempo)
    {
        return juce::jlimit (TempoModel::slowest, TempoModel::fastest, static_cast<double> (juce::roundToInt (tempo)));
    }
}

TempoModel::TempoModel (double initialStyleTempo)
    : styleTempo (clampTempo (initialStyleTempo)),
      currentValue (styleTempo)
{
}

double TempoModel::value() const      { return currentValue; }
void   TempoModel::setLocked (bool shouldBeLocked) { locked = shouldBeLocked; }
bool   TempoModel::isLocked() const   { return locked; }
void   TempoModel::setStyleTempo (double newStyleTempo) { styleTempo = clampTempo (newStyleTempo); }

bool TempoModel::setValue (double newValue)
{
    if (locked)
        return false;

    const auto clamped = clampTempo (newValue);
    if (juce::exactlyEqual (clamped, currentValue))
        return false;

    currentValue = clamped;
    return true;
}

void TempoModel::showValue (double tempoFromElsewhere)
{
    currentValue = clampTempo (tempoFromElsewhere);
}

bool TempoModel::step (int direction)
{
    return setValue (currentValue + direction);
}

bool TempoModel::wheel (int notches, bool coarse)
{
    return setValue (currentValue + notches * (coarse ? coarseWheelStep : fineWheelStep));
}

bool TempoModel::applyTyped (const juce::String& typedText)
{
    const auto trimmed = typedText.trim();
    if (trimmed.isEmpty() || ! trimmed.containsOnly (digits))
        return false;

    auto significant = trimmed.trimCharactersAtStart (zero);
    if (significant.length() > maxMeaningfulDigits)
        return setValue (fastest);

    return setValue (significant.getIntValue());
}

bool TempoModel::resetToStyle()
{
    return setValue (styleTempo);
}

int TempoModel::holdDelayMs (int stepIndex)
{
    if (stepIndex <= int {})
        return firstRepeatDelayMs;

    return juce::jmax (fastestRepeatMs, repeatStartMs - stepIndex * repeatSpeedUpMs);
}

std::vector<LampState> BeatLampModel::lamps (int beatsPerBar, double elapsedBeats, bool playing, bool asleep)
{
    std::vector<LampState> lampStates (static_cast<size_t> (juce::jmax (int {}, beatsPerBar)), LampState::off);
    if (lampStates.empty() || ! playing || asleep)
        return lampStates;

    const auto wholeBeats  = static_cast<long long> (std::floor (elapsedBeats));
    const auto barLength   = static_cast<long long> (beatsPerBar);
    const auto beatInBar   = ((wholeBeats % barLength) + barLength) % barLength;   // also right for negative counts
    lampStates[static_cast<size_t> (beatInBar)] = beatInBar == decltype (beatInBar) {} ? LampState::downbeat : LampState::beat;
    return lampStates;
}

juce::String StageText::chordName (const ArrangerChord& chord)
{
    if (! chord.isValid() || ! juce::isPositiveAndBelow (chord.root, pitchClassNames.size()))
        return {};

    auto name = pitchClassNames[chord.root] + chordSeparator + toString (chord.quality).toLowerCase();
    if (juce::isPositiveAndBelow (chord.bassNote, pitchClassNames.size()) && chord.bassNote != chord.root)
        name += bassSeparator + pitchClassNames[chord.bassNote];
    return name;
}

int StageText::barNumber (double elapsedBeats, int beatsPerBar)
{
    if (beatsPerBar <= int {} || elapsedBeats <= double {})
        return firstBarNumber;

    return static_cast<int> (std::floor (elapsedBeats / beatsPerBar)) + firstBarNumber;
}

void ScreenMessages::post (const juce::String& message, double nowMs, double showForMs)
{
    text = message.toUpperCase();
    postedAtMs = nowMs;
    durationMs = juce::jmax (showTimeMs, showForMs);
}

double ScreenMessages::elapsedMs (double nowMs) const
{
    return nowMs - postedAtMs;
}

double StageText::marqueeDurationMs (float overflowPx, float pixelsPerMs)
{
    if (overflowPx <= float {} || pixelsPerMs <= float {})
        return ScreenMessages::showTimeMs;

    return juce::jmax (ScreenMessages::showTimeMs, marqueeHoldMs + static_cast<double> (overflowPx / pixelsPerMs) + marqueeHoldMs);
}

float StageText::marqueeOffset (float overflowPx, float pixelsPerMs, double elapsedMs)
{
    if (overflowPx <= float {} || elapsedMs <= marqueeHoldMs)
        return {};

    return juce::jmin (overflowPx, static_cast<float> ((elapsedMs - marqueeHoldMs) * pixelsPerMs));
}

juce::String ScreenMessages::current (double nowMs) const
{
    return nowMs - postedAtMs < durationMs ? text : juce::String();
}

StagePage PageRouter::toggle (StagePage page)
{
    openPage = openPage == page ? StagePage::none : page;
    return openPage;
}

StagePage PageRouter::close()
{
    openPage = StagePage::none;
    return openPage;
}

StagePage PageRouter::open() const
{
    return openPage;
}

StyleView StyleViewRouter::show (StyleView view)
{
    shown = view;
    return shown;
}

StyleView StyleViewRouter::back()
{
    shown = StyleView::style;
    return shown;
}

void StyleViewRouter::reset()
{
    shown = StyleView::style;
}

StyleView StyleViewRouter::current() const
{
    return shown;
}
