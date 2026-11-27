#pragma once

#include <JuceHeader.h>
#include "StageMetrics.h"
#include "StagePageWidgets.h"

/** One of Home's big keys (board PageHome .big; Play is .big-lit: bigger, warm, with its own text size). */
class StageBigKey : public juce::Button
{
public:
    StageBigKey (const juce::String& text, bool isPlayKey);

    void setScale (float newScale);
    void paintButton (juce::Graphics& g, bool isHighlighted, bool isDown) override;

private:
    bool isPlay;
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StageBigKey)
};

/*  Home on the instrument (boards StageHome / PageHome, owner decision 2026-11-23): the page area shows three big keys,
    Settings · Play (lit, "Start playing") · Exit, and at the top right "Signed in as <email>" with Log out.
    Replaces the forest picture and its Play text. */
class StageHomePage : public juce::Component
{
public:
    StageHomePage();

    void setScale (float newScale);

    /** The account shown at the top right, left of Log out; empty shows Log out alone. */
    void setSignedInAs (const juce::String& email);

    std::function<void()> onSettings, onPlay, onExit, onLogOut;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    StageBigKey settingsKey, playKey, exitKey;
    SegButton logOutButton;
    juce::String signedInEmail;
    float scale { StageMetrics::referenceScale };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StageHomePage)
};
