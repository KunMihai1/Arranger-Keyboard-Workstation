#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "MidiHandler.h"
#include "InstrumentHandler.h"
#include "MidiRecordPlayer.h"



//======================================================================================
//  InstrumentHandler — predefined per-program CC presets (previously untested)
//======================================================================================
class InstrumentHandlerTest : public juce::UnitTest
{
public:
    InstrumentHandlerTest() : juce::UnitTest("InstrumentHandler", "Unit") {}

    void runTest() override
    {
        using Preset = std::vector<std::pair<int, int>>;

        beginTest("getPreset - program 0 is the grand-piano CC preset");
        {
            InstrumentHandler h;
            const Preset expected { {91,-1}, {93,10}, {74,100}, {71,40}, {11,127} };
            expect(h.getPreset(0) == expected);
        }

        beginTest("getPreset - a known program returns its 5-CC preset");
        {
            InstrumentHandler h;
            const auto& p = h.getPreset(4);   // electric piano
            expectEquals((int) p.size(), 5);
            expect(p.front() == std::make_pair(91, -1));   // reverb-send CC first
        }

        beginTest("getPreset - an unknown program is empty");
        {
            InstrumentHandler h;
            expect(h.getPreset(9999).empty());
        }

        beginTest("getInstrumentPresets - contains the seeded programs");
        {
            InstrumentHandler h;
            const auto& m = h.getInstrumentPresets();
            expect(m.find(0)  != m.end());    // grand piano
            expect(m.find(41) != m.end());    // violin
            expect(m.size() >= 30);
        }
    }
};
static InstrumentHandlerTest instrumentHandlerTest;
