/*
  ==============================================================================

    PlaytimeTracker.h
    Created: 28 Feb 2026 3:15:21am
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class PlaytimeTracker : private juce::Timer
{
public:
    PlaytimeTracker(std::function<void(int sec)> onTimeElapsed, int batchInterval);

    int getAccumlatedSeconds();

    static constexpr int defaultBatchIntervalSeconds = 300;   // playtime is sent in 5-minute batches

private:
    void timerCallback() override;

    int accumulatedSeconds {};

    static constexpr int tickIntervalSeconds = 60;
    int startTimerInSec = tickIntervalSeconds;

    int batchIntervalSeconds {};

    juce::Time lastTickTime;

    std::function<void(int sec)> callbackFunction;
};