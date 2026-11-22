/*
  ==============================================================================

    temporaryNotificationUI.cpp
    Created: 25 May 2025 2:56:51am
    Author:  Kisuke

  ==============================================================================
*/

#include "temporaryNotificationUI.h"

namespace
{
    constexpr float labelAlpha      = 0.7f;
    constexpr int   messageWidth    = 300;
    constexpr int   messageHeight   = 50;
    constexpr int   fadeTickMs      = 60;
    constexpr float cornerRadius    = 10.0f;
    constexpr float fadeStep        = 0.03f;   // alpha lost per tick
    constexpr float transparent     = 0.0f;
}

TemporaryMessage::TemporaryMessage(const juce::String& message)
{
    labelToShow.setText(message, juce::dontSendNotification);
    labelToShow.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(labelAlpha));
    labelToShow.setColour(juce::Label::textColourId, juce::Colours::white);
    labelToShow.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(labelToShow);

    setSize(messageWidth, messageHeight);
    startTimer(fadeTickMs);
}

void TemporaryMessage::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black.withAlpha(currentAlpha));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerRadius);
}

void TemporaryMessage::resized()
{
    labelToShow.setBounds(getLocalBounds());
}

void TemporaryMessage::setFinishedCallBack(FinishedCallback callBack)
{
    finishedCallback = std::move(callBack);
}

void TemporaryMessage::restartTimer()
{
    currentAlpha = fullyOpaque;
    startTimer(fadeTickMs);
}

void TemporaryMessage::updateText(const juce::String& newText)
{
    labelToShow.setText(newText, juce::dontSendNotification);
}

void TemporaryMessage::setCurrentAlpha(float alpha)
{
    currentAlpha = alpha;
}

void TemporaryMessage::timerCallback()
{
    currentAlpha -= fadeStep;

    if (currentAlpha <= transparent)
    {
        currentAlpha = transparent;
        stopTimer();
        if (finishedCallback)
            finishedCallback();
        return;
    }
    labelToShow.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(currentAlpha));
    repaint();
}
