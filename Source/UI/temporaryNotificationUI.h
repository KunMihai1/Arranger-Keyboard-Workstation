/*
  ==============================================================================

    temporaryNotificationUI.h
    Created: 25 May 2025 2:56:51am
    Author:  Kisuke

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/**
 * @class TemporaryMessage
 * @brief Displays a temporary notification message that fades out over time.
 *
 * Inherits from juce::Component and uses a juce::Timer to gradually reduce its alpha
 * until it disappears. Supports a callback when the message has finished fading.
 */
class TemporaryMessage : public juce::Component, private juce::Timer
{
public:
    /** 
     * @brief Type for the callback called when the message has finished displaying.
     */
    using FinishedCallback = std::function<void()>;

    /**
     * @brief Constructor.
     * @param message Optional initial message to display.
     */
    TemporaryMessage(const juce::String& message = "");

    /**
     * @brief Paints the component with a fading background.
     * @param g Graphics context to paint on.
     */
    void paint(juce::Graphics& g) override;

    /**
     * @brief Called when the component is resized. Updates label bounds.
     */
    void resized() override;

    /**
     * @brief Sets the callback function to call when the message finishes.
     * @param callBack Function to call when finished.
     */
    void setFinishedCallBack(FinishedCallback callBack);

    /**
     * @brief Restarts the fade timer and resets alpha to full opacity.
     */
    void restartTimer();

    /**
     * @brief Updates the displayed text.
     * @param newText The new message text to show.
     */
    void updateText(const juce::String& newText);

    /**
     * @brief Directly sets the current alpha of the background.
     * @param alpha New alpha value (0.0f to 1.0f).
     */
    void setCurrentAlpha(float alpha);

private:
    /**
     * @brief Timer callback that gradually reduces alpha and triggers the finished callback.
     */
    void timerCallback() override;

    // Member variables
    static constexpr float fullyOpaque = 1.0f;
    float currentAlpha = fullyOpaque;           /**< Current alpha value for fading effect */
    juce::Label labelToShow;             /**< Label showing the notification text */
    FinishedCallback finishedCallback;    /**< Callback called when message finishes */
};
