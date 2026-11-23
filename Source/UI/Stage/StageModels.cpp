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

void ScreenMessages::post (const juce::String& message, double nowMs)
{
    text = message.toUpperCase();
    postedAtMs = nowMs;
}

juce::String ScreenMessages::current (double nowMs) const
{
    return nowMs - postedAtMs < showTimeMs ? text : juce::String();
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
