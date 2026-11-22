
#include <juce_core/juce_core.h>
#include "ValidatorUI.h"

namespace ValidatorUiTestValues
{
    const juce::String testName     = "Validator";
    const juce::String testCategory = "Unit";

    const juce::String isValidMidiIntValueValidRange0127 = "isValidMidiIntValue - valid range 0-127";
    const juce::String isValidMidiIntValueOutOfRange = "isValidMidiIntValue - out of range";
    const juce::String isValidMidiIntegerVelocitiesValidVelocities = "isValidMidiIntegerVelocities - valid velocities";
    const juce::String isValidMidiIntegerVelocitiesInvalidVelocities = "isValidMidiIntegerVelocities - invalid velocities";
    const juce::String isValidMidiIntegerStringValidStrings = "isValidMidiIntegerString - valid strings";
    const juce::String isValidMidiIntegerStringInvalidLetters = "isValidMidiIntegerString - invalid: letters";
    const juce::String isValidMidiIntegerStringInvalidMinusPlusNotAtStart = "isValidMidiIntegerString - invalid: minus/plus not at start";
    const juce::String isValidMidiIntegerStringInvalidDoubleSigns = "isValidMidiIntegerString - invalid: double signs";
    const juce::String isValidMidiIntegerStringInvalidSignOnly = "isValidMidiIntegerString - invalid: sign only";
    const juce::String isValidMidiDoubleStringValidStrings = "isValidMidiDoubleString - valid strings";
    const juce::String isValidMidiDoubleStringInvalidLetters = "isValidMidiDoubleString - invalid: letters";
    const juce::String isValidMidiDoubleStringInvalidMinusPlusNotAtStart = "isValidMidiDoubleString - invalid: minus/plus not at start";
    const juce::String isValidMidiDoubleStringInvalidDoubleDotsOrSigns = "isValidMidiDoubleString - invalid: double dots or signs";
    const juce::String isValidMidiDoubleStringInvalidSignWithDotOnly = "isValidMidiDoubleString - invalid: sign with dot only";
    const juce::String isValidMidiDoubleValueTimeStampsValidWithinBounds = "isValidMidiDoubleValueTimeStamps - valid within bounds";
    const juce::String isValidMidiDoubleValueTimeStampsNewTimeGoesNegative = "isValidMidiDoubleValueTimeStamps - newTime goes negative";
    const juce::String isValidMidiDoubleValueTimeStampsNewTimeBeforePrevious = "isValidMidiDoubleValueTimeStamps - newTime before previous";
    const juce::String isValidMidiDoubleValueTimeStampsNewTimeAfterNext = "isValidMidiDoubleValueTimeStamps - newTime after next";

    const int isValidMidiIntValueValidRange0127Inputs[] { 0, 64, 127 };
    const int isValidMidiIntValueOutOfRangeInputs[] { -1, 128, -100, 999 };
    const double isValidMidiIntegerVelocitiesValidVelocitiesInputs[] { 0.0, 64.0, 127.0 };
    const double isValidMidiIntegerVelocitiesInvalidVelocitiesInputs[] { -1.0, 128.0 };
    const juce::StringArray isValidMidiIntegerStringValidStringsInputs { "0", "127", "-5", "+10", "42" };
    const juce::StringArray isValidMidiIntegerStringInvalidLettersInputs { "abc", "12a" };
    const juce::StringArray isValidMidiIntegerStringInvalidMinusPlusNotAtStartInputs { "5-3", "5+3" };
    const juce::StringArray isValidMidiIntegerStringInvalidDoubleSignsInputs { "++5", "--5" };
    const juce::StringArray isValidMidiIntegerStringInvalidSignOnlyInputs { "+", "-" };
    const juce::StringArray isValidMidiDoubleStringValidStringsInputs { "0.5", "123", "-1.5", "+2.0", "0" };
    const juce::StringArray isValidMidiDoubleStringInvalidLettersInputs { "1.5a", "abc" };
    const juce::StringArray isValidMidiDoubleStringInvalidMinusPlusNotAtStartInputs { "5-3", "5+3" };
    const juce::StringArray isValidMidiDoubleStringInvalidDoubleDotsOrSignsInputs { "1..5", "++5", "--5" };
    const juce::StringArray isValidMidiDoubleStringInvalidSignWithDotOnlyInputs { "+", "-", "+.", "-." };
    const double isValidMidiDoubleValueTimeStampsValidWithinBoundsCase[] { 0.5, 5.0, 4.0, 6.0 };   // value, current, previous, next timestamp
    const double isValidMidiDoubleValueTimeStampsNewTimeGoesNegativeCase[] { -5.0, 1.0, 0.0, 2.0 };   // value, current, previous, next timestamp
    const double isValidMidiDoubleValueTimeStampsNewTimeBeforePreviousCase[] { -2.0, 5.0, 4.0, 6.0 };   // value, current, previous, next timestamp
    const double isValidMidiDoubleValueTimeStampsNewTimeAfterNextCase[] { 3.0, 5.0, 4.0, 6.0 };   // value, current, previous, next timestamp
    constexpr size_t valueSlot = 0;
    constexpr size_t currentSlot = 1;
    constexpr size_t previousSlot = 2;
    constexpr size_t nextSlot = 3;
}

using namespace ValidatorUiTestValues;

class ValidatorTest : public juce::UnitTest
{
public:
    ValidatorTest() : juce::UnitTest (ValidatorUiTestValues::testName, ValidatorUiTestValues::testCategory) {}

    void runTest() override
    {
        beginTest(isValidMidiIntValueValidRange0127);
        {
            for (const auto& input : isValidMidiIntValueValidRange0127Inputs)
                expect(Validator::isValidMidiIntValue(input) == true);
        }

        beginTest(isValidMidiIntValueOutOfRange);
        {
            for (const auto& input : isValidMidiIntValueOutOfRangeInputs)
                expect(Validator::isValidMidiIntValue(input) == false);
        }

        beginTest(isValidMidiIntegerVelocitiesValidVelocities);
        {
            for (const auto& input : isValidMidiIntegerVelocitiesValidVelocitiesInputs)
                expect(Validator::isValidMidiIntegerVelocities(input) == true);
        }

        beginTest(isValidMidiIntegerVelocitiesInvalidVelocities);
        {
            for (const auto& input : isValidMidiIntegerVelocitiesInvalidVelocitiesInputs)
                expect(Validator::isValidMidiIntegerVelocities(input) == false);
        }

        beginTest(isValidMidiIntegerStringValidStrings);
        {
            for (const auto& input : isValidMidiIntegerStringValidStringsInputs)
                expect(Validator::isValidMidiIntegerString(input) == true);
        }

        beginTest(isValidMidiIntegerStringInvalidLetters);
        {
            for (const auto& input : isValidMidiIntegerStringInvalidLettersInputs)
                expect(Validator::isValidMidiIntegerString(input) == false);
        }

        beginTest(isValidMidiIntegerStringInvalidMinusPlusNotAtStart);
        {
            for (const auto& input : isValidMidiIntegerStringInvalidMinusPlusNotAtStartInputs)
                expect(Validator::isValidMidiIntegerString(input) == false);
        }

        beginTest(isValidMidiIntegerStringInvalidDoubleSigns);
        {
            for (const auto& input : isValidMidiIntegerStringInvalidDoubleSignsInputs)
                expect(Validator::isValidMidiIntegerString(input) == false);
        }

        beginTest(isValidMidiIntegerStringInvalidSignOnly);
        {
            for (const auto& input : isValidMidiIntegerStringInvalidSignOnlyInputs)
                expect(Validator::isValidMidiIntegerString(input) == false);
        }

        beginTest(isValidMidiDoubleStringValidStrings);
        {
            for (const auto& input : isValidMidiDoubleStringValidStringsInputs)
                expect(Validator::isValidMidiDoubleString(input) == true);
        }

        beginTest(isValidMidiDoubleStringInvalidLetters);
        {
            for (const auto& input : isValidMidiDoubleStringInvalidLettersInputs)
                expect(Validator::isValidMidiDoubleString(input) == false);
        }

        beginTest(isValidMidiDoubleStringInvalidMinusPlusNotAtStart);
        {
            for (const auto& input : isValidMidiDoubleStringInvalidMinusPlusNotAtStartInputs)
                expect(Validator::isValidMidiDoubleString(input) == false);
        }

        beginTest(isValidMidiDoubleStringInvalidDoubleDotsOrSigns);
        {
            for (const auto& input : isValidMidiDoubleStringInvalidDoubleDotsOrSignsInputs)
                expect(Validator::isValidMidiDoubleString(input) == false);
        }

        beginTest(isValidMidiDoubleStringInvalidSignWithDotOnly);
        {
            for (const auto& input : isValidMidiDoubleStringInvalidSignWithDotOnlyInputs)
                expect(Validator::isValidMidiDoubleString(input) == false);
        }

        beginTest(isValidMidiDoubleValueTimeStampsValidWithinBounds);
        {
            expect(Validator::isValidMidiDoubleValueTimeStamps(isValidMidiDoubleValueTimeStampsValidWithinBoundsCase[valueSlot], isValidMidiDoubleValueTimeStampsValidWithinBoundsCase[currentSlot], isValidMidiDoubleValueTimeStampsValidWithinBoundsCase[previousSlot], isValidMidiDoubleValueTimeStampsValidWithinBoundsCase[nextSlot]) == true);
        }

        beginTest(isValidMidiDoubleValueTimeStampsNewTimeGoesNegative);
        {
            expect(Validator::isValidMidiDoubleValueTimeStamps(isValidMidiDoubleValueTimeStampsNewTimeGoesNegativeCase[valueSlot], isValidMidiDoubleValueTimeStampsNewTimeGoesNegativeCase[currentSlot], isValidMidiDoubleValueTimeStampsNewTimeGoesNegativeCase[previousSlot], isValidMidiDoubleValueTimeStampsNewTimeGoesNegativeCase[nextSlot]) == false);
        }

        beginTest(isValidMidiDoubleValueTimeStampsNewTimeBeforePrevious);
        {
            expect(Validator::isValidMidiDoubleValueTimeStamps(isValidMidiDoubleValueTimeStampsNewTimeBeforePreviousCase[valueSlot], isValidMidiDoubleValueTimeStampsNewTimeBeforePreviousCase[currentSlot], isValidMidiDoubleValueTimeStampsNewTimeBeforePreviousCase[previousSlot], isValidMidiDoubleValueTimeStampsNewTimeBeforePreviousCase[nextSlot]) == false);
        }

        beginTest(isValidMidiDoubleValueTimeStampsNewTimeAfterNext);
        {
            expect(Validator::isValidMidiDoubleValueTimeStamps(isValidMidiDoubleValueTimeStampsNewTimeAfterNextCase[valueSlot], isValidMidiDoubleValueTimeStampsNewTimeAfterNextCase[currentSlot], isValidMidiDoubleValueTimeStampsNewTimeAfterNextCase[previousSlot], isValidMidiDoubleValueTimeStampsNewTimeAfterNextCase[nextSlot]) == false);
        }
    }
};

static ValidatorTest validatorTest;
