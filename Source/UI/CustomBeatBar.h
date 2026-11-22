/*
  ==============================================================================

    CustomBeatBar.h
    Created: 15 Jul 2025 5:59:41pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * @class BeatBar
 * @brief A visual component that displays beat subdivisions within a musical bar.
 *
 * The BeatBar highlights the current subdivision of the beat based on elapsed beats.
 * It is configurable via numerator and denominator to represent a musical time signature.
 */
class BeatBar: public juce::Component
{

public:
    std::function<bool()> isPlayingCheck;

    /**
     * @brief Constructs a BeatBar with default time signature 4/4.
     */
    BeatBar();

    /**
     * @brief Renders the BeatBar UI.
     *
     * Draws beat subdivisions and highlights the current active subdivision.
     *
     * @param g The Graphics context used for rendering.
     */
    void paint(juce::Graphics& g) override;

    /**
     * @brief Called when the component is resized.
     *
     * Useful for recalculating layout. Currently no implementation.
     */
    void resized() override;

    /**
     * @brief Retrieves the time signature numerator.
     *
     * @return The numerator value (beats per bar).
     */
    int getNumerator() const;

    /**
     * @brief Retrieves the time signature denominator.
     *
     * @return The denominator value (beat subdivision).
     */
    int getDenominator() const;

    /**
     * @brief Sets the time signature numerator.
     *
     * @param newNumerator The new number of beats per bar.
     */
    void setNumerator(int newNumerator);

    /**
     * @brief Sets the time signature denominator.
     *
     * @param newDenominator The new denominator value (defines subdivisions).
     */
    void setDenominator(int newDenominator);

    /**
     * @brief Updates the current elapsed beat position and repaints the display.
     *
     * @param elapsedBeats Elapsed beats relative to musical playback.
     */
    void setCurrentBeatsElapsed(double elapsedBeats);

    int getTotalSubdivisions();

    int getCurrentSubdivision();

    float getSubBeatsPerBeat();

private:
    /** @brief Number of beats per bar (top number of time signature). */
    int numerator;

    /** @brief Beat subdivision base (bottom number of time signature). */
    int denominator;

    /** @brief Tracks how many beats have elapsed for highlighting. */
    double currentBeatsElapsed {};
};
