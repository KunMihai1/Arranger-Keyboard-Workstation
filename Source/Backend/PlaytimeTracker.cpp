/*
  ==============================================================================

    PlaytimeTracker.cpp
    Created: 28 Feb 2026 3:15:21am
    Author:  Kisuke

  ==============================================================================
*/

#include "PlaytimeTracker.h"

namespace
{
    constexpr int millisecondsPerSecond = 1000;
}

PlaytimeTracker::PlaytimeTracker(std::function<void(int sec)> onTimeElapsed, int batchIntervalSec=defaultBatchIntervalSeconds): callbackFunction{onTimeElapsed}, batchIntervalSeconds{batchIntervalSec}
{
    lastTickTime = juce::Time::getCurrentTime();
    
    startTimer(startTimerInSec*millisecondsPerSecond);
    
}

int PlaytimeTracker::getAccumlatedSeconds()
{
    return accumulatedSeconds;
}

void PlaytimeTracker::timerCallback()
{
    auto now = juce::Time::getCurrentTime();
    int deltaSec = static_cast<int>((now.toMilliseconds() - lastTickTime.toMilliseconds()) / millisecondsPerSecond);
    lastTickTime = now;

    accumulatedSeconds += deltaSec;

    if (callbackFunction && accumulatedSeconds>=defaultBatchIntervalSeconds)
    {
        callbackFunction(accumulatedSeconds);
        accumulatedSeconds = {};
    }
}
