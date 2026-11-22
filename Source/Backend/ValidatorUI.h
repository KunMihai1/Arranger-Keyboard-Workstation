/*
  ==============================================================================

    ValidatorUI.h
    Created: 21 Jul 2025 7:23:52pm
    Author:  Kisuke

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

/**
 * @class Validator
 * @brief Utility class for validating MIDI values and strings.
 *
 * Provides static functions to:
 *   - Validate integer and double MIDI values
 *   - Validate MIDI velocity values
 *   - Validate timestamp-related double values
 *   - Validate string representations of MIDI integers and doubles
 */
class Validator
{
public:
    static constexpr double noTimeStamp = -1.0;   ///< Timestamp argument not given.

    /**
     * @brief Checks if an integer is a valid MIDI value (0–127).
     * @param value The integer value to check.
     * @return true if valid, false otherwise.
     */
    static bool isValidMidiIntValue(int value);

    /**
     * @brief Checks if a string represents a valid MIDI integer.
     * @param s The string to validate.
     * @return true if the string represents a valid MIDI integer, false otherwise.
     */
    static bool isValidMidiIntegerString(const juce::String& text);

    /**
     * @brief Checks if a double value is a valid MIDI velocity.
     * @param value The velocity value to validate.
     * @return true if valid, false otherwise.
     */
    static bool isValidMidiIntegerVelocities(double value);

    /**
     * @brief Checks if a double value is a valid generic MIDI value.
     * @param value The double value to validate.
     * @return true if valid, false otherwise.
     */
    static bool isValidMidiDoubleValue(double value);

    /**
     * @brief Checks if a double value is a valid MIDI timestamp.
     * 
     * Ensures the value is positive and optionally compares it with
     * current and previous timestamps.
     *
     * @param value The timestamp value to validate.
     * @param currentTimeStamp Optional current timestamp (default -1).
     * @param previousTimeStamp Optional previous timestamp (default -1).
     * @return true if valid, false otherwise.
     */
    static bool isValidMidiDoubleValueTimeStamps(double value, double currentTimeStamp=noTimeStamp, double previousTimeStamp=noTimeStamp, double nextTimeStamp=noTimeStamp);

    /**
     * @brief Checks if a string represents a valid MIDI double value.
     * @param s The string to validate.
     * @return true if valid, false otherwise.
     */
    static bool isValidMidiDoubleString(const juce::String& text);

private:
    // Private to prevent instantiation
    Validator() = default;
};
