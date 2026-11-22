/*
  ==============================================================================

    CustomBeatBar.cpp
    Created: 15 Jul 2025 5:59:41pm
    Author:  Kisuke

  ==============================================================================
*/

#include "CustomBeatBar.h"

namespace
{
    constexpr int   defaultBeatsPerBar   = 4;     // 4/4 until a style says otherwise
    constexpr int   defaultBeatUnit      = 4;
    constexpr float quarterNoteUnit      = 4.0f;  // sub-beats per beat = beat unit / 4
    constexpr float cornerRatio          = 0.3f;
    constexpr int   downbeat             = 0;
    constexpr float activeAlpha          = 0.85f;
    constexpr float outlineAlpha         = 0.3f;
    constexpr float outlineThickness     = 1.0f;
    constexpr double barStart            = 0.0;
}

BeatBar::BeatBar(): numerator{defaultBeatsPerBar}, denominator{defaultBeatUnit}
{

}

void BeatBar::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::black);

    float height = area.getHeight();

    int totalSubdivisions = getTotalSubdivisions();
    int currentSubdivision = getCurrentSubdivision();

    float subBeatWidth = area.getWidth() / totalSubdivisions;
    float cornerSize = juce::jmin(subBeatWidth, height) * cornerRatio;

    for (int subdivision {}; subdivision < totalSubdivisions; ++subdivision)
    {
        juce::Rectangle<float> beatRect(
            area.getX() + subdivision * subBeatWidth,
            area.getY(),
            subBeatWidth,
            height
        );

        bool isCurrent = (subdivision == currentSubdivision);

        if (isCurrent)
        {
            if (subdivision == downbeat && isPlayingCheck && isPlayingCheck())
            {
                g.setColour(juce::Colours::red.withAlpha(activeAlpha));
            }
            else
            {
                g.setColour(juce::Colours::yellow.withAlpha(activeAlpha));
            }
        }
        else
        {
            g.setColour(juce::Colours::dimgrey);
        }

        g.fillRoundedRectangle(beatRect, cornerSize);

        g.setColour(juce::Colours::blanchedalmond.withAlpha(outlineAlpha));
        g.drawRoundedRectangle(beatRect, cornerSize, outlineThickness);
    }
}

void BeatBar::resized()
{

}

int BeatBar::getNumerator() const
{
    return this->numerator;
}

int BeatBar::getDenominator() const 
{
    return this->denominator;
}

void BeatBar::setNumerator(int newNumerator)
{
    this->numerator = newNumerator;
}

void BeatBar::setDenominator(int newDenominator)
{
    this->denominator = newDenominator;
}

void BeatBar::setCurrentBeatsElapsed(double beatsElapsed)
{
    this->currentBeatsElapsed = beatsElapsed;
    repaint();
}

int BeatBar::getTotalSubdivisions()
{
    float beatsPerBar = static_cast<float>(numerator);
    return static_cast<int>(beatsPerBar * getSubBeatsPerBeat());
}

int BeatBar::getCurrentSubdivision()
{
    float beatsPerBar = static_cast<float>(numerator);
    float subBeatsPerBeat = getSubBeatsPerBeat();

    double beatInBar = std::fmod(currentBeatsElapsed, beatsPerBar);

    if (beatInBar < barStart)
        beatInBar += beatsPerBar;

    return static_cast<int>(std::floor(beatInBar * subBeatsPerBeat));
}

float BeatBar::getSubBeatsPerBeat()
{
    return static_cast<float>(denominator) / quarterNoteUnit;
}
