#include <JuceHeader.h>
#include "SoundEffectWindowComponent.h"
#include "MidiHandler.h"

/*  The Effects page (board StageRevised openPage=effects, scale 1, page content at (28, 180), 1144 wide):
    Tone knobs at x 28 / 178.67 / 329.33 / 480 (rings 12 further in), y 198; Envelope from x 624; the second row
    of groups (Space, Modulation) has its heading at y 311 and its knobs at 329; a cell is 96 x 99. */

namespace EffectsPageTestValues
{
    const juce::String testName     = "EffectsPage";
    const juce::String testCategory = "Unit";

    const juce::String layoutMatchesBoard = "the 16 knobs and 4 group headings sit where the board puts them";
    const juce::String secondInstrument   = "2nd selects the right hand's channel and reports it, 1st the left hand's";
    const juce::String titleRow           = "the title row has Instrument, 1st, 2nd, a divider and Sustain";

    constexpr float referenceScale = 1.0f;
    constexpr int contentX      = 28;
    constexpr int contentY      = 180;
    constexpr int contentWidth  = 1144;
    constexpr int contentHeight = 296;
    constexpr int cellWidth     = 96;
    constexpr int cellHeight    = 99;
    constexpr int firstRowKnobs = 198;
    constexpr int secondRowHeading = 311;
    constexpr int secondRowKnobs   = 329;
    constexpr int secondKnobX   = 179;   // 178.67
    constexpr int lastToneKnobX = 480;
    constexpr int envelopeX     = 624;
    constexpr int tolerance     = 1;
    constexpr int firstKnob     = 0;
    constexpr int secondKnob    = 1;
    constexpr int lastToneKnob  = 3;
    constexpr int firstEnvelope = 4;
    constexpr int firstSpace    = 8;
    constexpr int spaceHeading  = 2;
    constexpr int titleActions  = 5;
    constexpr int noChannel     = -1;
}

class EffectsPageTest : public juce::UnitTest
{
public:
    EffectsPageTest() : juce::UnitTest (EffectsPageTestValues::testName, EffectsPageTestValues::testCategory) {}

    void runTest() override
    {
        using namespace EffectsPageTestValues;

        beginTest (layoutMatchesBoard);
        {
            const auto rects = effectsPageLayout ({ contentX, contentY, contentWidth, contentHeight }, referenceScale);
            auto near = [this] (juce::Rectangle<int> actual, int expectedX, int expectedY)
            {
                expectWithinAbsoluteError (actual.getX(), expectedX, tolerance);
                expectWithinAbsoluteError (actual.getY(), expectedY, tolerance);
                expectWithinAbsoluteError (actual.getWidth(), cellWidth, tolerance);
                expectWithinAbsoluteError (actual.getHeight(), cellHeight, tolerance);
            };
            near (rects.knobs[firstKnob], contentX, firstRowKnobs);
            near (rects.knobs[secondKnob], secondKnobX, firstRowKnobs);
            near (rects.knobs[lastToneKnob], lastToneKnobX, firstRowKnobs);
            near (rects.knobs[firstEnvelope], envelopeX, firstRowKnobs);
            near (rects.knobs[firstSpace], contentX, secondRowKnobs);
            expectWithinAbsoluteError (rects.headings[spaceHeading].getY(), secondRowHeading, tolerance);
        }

        beginTest (secondInstrument);
        {
            SoundEffectContentComponent content;
            int reported = noChannel;
            content.onChannelChanged = [&reported] (int channel) { reported = channel; };
            expectEquals (content.getSelectedChannel(), HandChannels::leftHand);
            content.selectSecondInstrument (true);
            expectEquals (content.getSelectedChannel(), HandChannels::rightHand);
            expectEquals (reported, HandChannels::rightHand);
            content.selectSecondInstrument (false);
            expectEquals (reported, HandChannels::leftHand);
        }

        beginTest (titleRow);
        {
            SoundEffectContentComponent content;
            const auto actions = content.getTitleActions();
            expectEquals (static_cast<int> (actions.size()), titleActions);
            expect (dynamic_cast<SustainToggleComponent*> (actions.back()) == &content.getSustainToggle());
        }
    }
};
static EffectsPageTest effectsPageTest;
