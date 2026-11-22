#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"

namespace InstrumentHandlerTestValues
{
    using Preset = std::vector<std::pair<int, int>>;

    const juce::String testName     = "InstrumentHandler";
    const juce::String testCategory = "Unit";

    const juce::String grandPianoPreset     = "getPreset - program 0 is the grand-piano CC preset";
    const juce::String knownProgramPreset   = "getPreset - a known program returns its 5-CC preset";
    const juce::String unknownProgramEmpty  = "getPreset - an unknown program is empty";
    const juce::String seededProgramsListed = "getInstrumentPresets - contains the seeded programs";

    // {CC, value}; -1 leaves that controller untouched.
    const Preset expectedGrandPiano { {91,-1}, {93,10}, {74,100}, {71,40}, {11,127} };
    constexpr int grandPiano       = 0;
    constexpr int electricPiano    = 4;
    constexpr int violin           = 41;
    constexpr int unknownProgram   = 9999;
    constexpr int ccsPerPreset     = 5;
    const std::pair<int, int> reverbSendFirst { 91, -1 };
    constexpr size_t fewestSeeded  = 30;
}

//======================================================================================
//  InstrumentHandler — predefined per-program CC presets (previously untested)
//======================================================================================
class InstrumentHandlerTest : public juce::UnitTest
{
public:
    InstrumentHandlerTest() : juce::UnitTest(InstrumentHandlerTestValues::testName, InstrumentHandlerTestValues::testCategory) {}

    void runTest() override
    {
        using namespace InstrumentHandlerTestValues;

        beginTest(grandPianoPreset);
        {
            InstrumentHandler handler;
            expect(handler.getPreset(grandPiano) == expectedGrandPiano);
        }

        beginTest(knownProgramPreset);
        {
            InstrumentHandler handler;
            const auto& preset = handler.getPreset(electricPiano);
            expectEquals((int) preset.size(), ccsPerPreset);
            expect(preset.front() == reverbSendFirst);   // reverb-send CC first
        }

        beginTest(unknownProgramEmpty);
        {
            InstrumentHandler handler;
            expect(handler.getPreset(unknownProgram).empty());
        }

        beginTest(seededProgramsListed);
        {
            InstrumentHandler handler;
            const auto& presets = handler.getInstrumentPresets();
            expect(presets.find(grandPiano) != presets.end());
            expect(presets.find(violin)     != presets.end());
            expect(presets.size() >= fewestSeeded);
        }
    }
};
static InstrumentHandlerTest instrumentHandlerTest;
