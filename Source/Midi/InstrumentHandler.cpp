/*
  ==============================================================================

    InstrumentHandler.cpp
    Created: 26 Mar 2025 9:45:50pm
    Author:  Kisuke

  ==============================================================================
*/

#include "InstrumentHandler.h"

InstrumentHandler::InstrumentHandler()
{
    initializeMAP();
}

InstrumentHandler::~InstrumentHandler()
{
}

const std::unordered_map<int, std::vector<std::pair<int, int>>>& InstrumentHandler::getInstrumentPresets()
{
    return instrumentPresetsMap;

}

const std::vector<std::pair<int, int>>& InstrumentHandler::getPreset(int programNumber)
{
    return instrumentPresetsMap[programNumber];
}

namespace InstrumentPresetValues
{
    // The controllers each preset sets, in the order they are listed.
    constexpr int reverbController     = 91;
    constexpr int chorusController     = 93;
    constexpr int cutoffController     = 74;
    constexpr int resonanceController  = 71;
    constexpr int expressionController = 11;
    constexpr int reverbNotSet         = -1;   // no preset sets a reverb value

    struct Preset
    {
        int program;                                    // General MIDI program number
        int chorus, cutoff, resonance, expression;      // controller values 0..127
    };

    const Preset presets[] =
    {
        { 0, 10, 100, 40, 127 },   // grand piano
        { 4, 70, 90, 30, 127 },   // electric piano
        { 32, 0, 60, 20, 100 },
        { 33, 10, 70, 35, 110 },
        { 34, 15, 80, 45, 115 },
        { 35, 10, 75, 50, 100 },
        { 36, 25, 95, 60, 127 },
        { 37, 30, 100, 55, 127 },
        { 38, 40, 127, 80, 127 },
        { 39, 50, 127, 85, 127 },
        { 24, 10, 70, 40, 100 },
        { 25, 15, 80, 50, 110 },
        { 26, 20, 90, 55, 110 },
        { 27, 25, 100, 60, 115 },
        { 41, 18, 95, 55, 122 },   // violin
        { 40, 18, 94, 55, 122 },   // viola
        { 42, 15, 85, 50, 120 },
        { 43, 10, 80, 45, 115 },
        { 56, 40, 100, 30, 127 },
        { 57, 30, 95, 40, 120 },
        { 59, 20, 100, 60, 127 },
        { 68, 10, 85, 35, 115 },
        { 70, 5, 80, 30, 110 },
        { 71, 15, 95, 40, 120 },
        { 73, 20, 110, 45, 127 },
        { 21, 20, 75, 60, 100 },
        { 22, 15, 70, 55, 105 },
        { 23, 10, 68, 52, 98 },
        { 16, 30, 80, 65, 110 },
        { 17, 25, 75, 60, 108 },
        { 18, 40, 85, 55, 115 },
        { 19, 60, 90, 70, 120 },
        { 20, 35, 70, 60, 105 },
    };
}

void InstrumentHandler::initializeMAP()
{
    using namespace InstrumentPresetValues;
    instrumentPresetsMap.clear();

    for (const auto& preset : presets)
        instrumentPresetsMap[preset.program] = { { reverbController, reverbNotSet },
                                                 { chorusController, preset.chorus },
                                                 { cutoffController, preset.cutoff },
                                                 { resonanceController, preset.resonance },
                                                 { expressionController, preset.expression } };
}
