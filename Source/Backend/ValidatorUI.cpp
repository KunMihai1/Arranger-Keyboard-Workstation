    /*
  ==============================================================================

    ValidatorUI.cpp
    Created: 21 Jul 2025 7:23:52pm
    Author:  Kisuke

  ==============================================================================
*/

#include "ValidatorUI.h"

namespace
{
    constexpr int    lowestMidiValue  = 0;
    constexpr int    highestMidiValue = 127;
    constexpr double zeroSeconds = 0.0;
    constexpr int    firstCharacter   = 0;

    namespace Text
    {
        const juce::String integerCharacters = "0123456789-+";
        const juce::String decimalCharacters = "0123456789.+-";
        const juce::String minus             = "-";
        const juce::String plus              = "+";
        const juce::String doublePlus        = "++";
        const juce::String doubleMinus       = "--";
        const juce::String doubleDot         = "..";
        const juce::String plusDot           = "+.";
        const juce::String minusDot          = "-.";
    }

    constexpr juce::juce_wchar minusSign = '-';
    constexpr juce::juce_wchar plusSign  = '+';
}

bool Validator::isValidMidiIntValue(int value)
{
    if (value < lowestMidiValue || value > highestMidiValue)
        return false;


    return true;
}

bool Validator::isValidMidiDoubleValue(double value)
{
    if (std::isnan(value) || std::isinf(value))
        return false;

    return true;
}

bool Validator::isValidMidiDoubleValueTimeStamps(double value, double currentTimeStamp, double previousTimeStamp, double nextTimeStamp)
{
    if (!isValidMidiDoubleValue(value))
        return false;

    double newTime = currentTimeStamp + value;
    if (newTime < zeroSeconds)
        return false;

    if (newTime - previousTimeStamp < zeroSeconds)
        return false;

    if (newTime - nextTimeStamp > zeroSeconds)
        return false;

    return true;
}

bool Validator::isValidMidiIntegerString(const juce::String& text)
{
    if(!text.containsOnly(Text::integerCharacters))
        return false;

    if (text.contains(Text::minus) && text[firstCharacter] != minusSign)
        return false;

    if (text.contains(Text::plus) && text[firstCharacter] != plusSign)
        return false;

    if (text.contains(Text::doublePlus) || text.contains(Text::doubleMinus))
        return false;

    if (text == Text::plus || text == Text::minus)
        return false;

    return true;
}

bool Validator::isValidMidiIntegerVelocities(double value)
{
    if (!isValidMidiIntValue(value))
        return false;

    return true;
}

bool Validator::isValidMidiDoubleString(const juce::String& text)
{
    if (!text.containsOnly(Text::decimalCharacters))
        return false;

    if (text.contains(Text::minus) && text[firstCharacter] != minusSign)
        return false;

    if (text.contains(Text::plus) && text[firstCharacter] != plusSign)
        return false;

    if (text.contains(Text::doublePlus) || text.contains(Text::doubleMinus) || text.contains(Text::doubleDot))
        return false;

    if (text == Text::plus || text == Text::minus || text==Text::plusDot || text==Text::minusDot)
        return false;

    return true;
}
