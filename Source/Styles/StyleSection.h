/*
  ==============================================================================

    StyleSection.h
    Created: 28 Nov 2025 12:40:51am
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

struct StyleSection {
    juce::String id;
    juce::String name;
    double startTimeSeconds;
    double endTimeSeconds;
    static constexpr int barNotSet = -1;
    int startBar = barNotSet;
    int endBar = barNotSet;
};
