#include <JuceHeader.h>
#include "SettingsWindow.h"
#include "NoteLayer.h"

/*  The Settings page layout (board StageRevised openPage=settings, scale 1, content at (28, 180), 1144 wide):
    columns 262 wide at x 28 / 322 / 616 / 910; Output engine field at y 220, MIDI in at 272, Keyboard at 426;
    Arranger rows at 198 + 28 n; Chord mode row at 220, Minor scale at 270; Display rows at 198 + 28 n,
    "Particle style" at 288 and its buttons at 306. */

namespace SettingsPageTestValues
{
    const juce::String testName     = "SettingsPage";
    const juce::String testCategory = "Unit";

    const juce::String layoutMatchesBoard = "the Settings page's columns, fields and rows sit where the board puts them";
    const juce::String keyboardSizesKeepStored = "the Keyboard field lists the standard sizes and a stored size outside them";
    const juce::String storedParticleStyleChecked = "a stored particle style outside Sparks / Dust / Wisps reads as Dust";

    constexpr int keys25 = 25;
    constexpr int keys49 = 49;
    constexpr int keys61 = 61;
    constexpr int keys76 = 76;
    constexpr int keys88 = 88;
    constexpr int noStoredCount = 0;
    constexpr int unknownStyle = 7;
    constexpr int negativeStyle = -1;

    constexpr float referenceScale = 1.0f;
    constexpr int contentX      = 28;
    constexpr int contentY      = 180;
    constexpr int contentWidth  = 1144;
    constexpr int contentHeight = 296;
    constexpr int columnWidth   = 262;
    constexpr int arrangerX     = 322;
    constexpr int chordsX       = 616;
    constexpr int displayX      = 910;
    constexpr int engineFieldY  = 220;
    constexpr int midiInFieldY  = 272;
    constexpr int keyboardY     = 426;
    constexpr int firstRowY     = 198;
    constexpr int lastArrangerY = 366;
    constexpr int chordModeY    = 220;
    constexpr int minorScaleY   = 270;
    constexpr int styleLabelY   = 288;
    constexpr int styleRowY     = 306;
    constexpr int fieldHeight   = 24;
    constexpr int tolerance     = 1;
    constexpr size_t engineField = 0;
    constexpr size_t midiInField = 1;
    constexpr size_t firstRow    = 0;
    constexpr size_t lastArrangerRow = 6;
    constexpr size_t arrangerColumn  = 1;
}

class SettingsPageTest : public juce::UnitTest
{
public:
    SettingsPageTest() : juce::UnitTest (SettingsPageTestValues::testName, SettingsPageTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SettingsPageTestValues;

        beginTest (layoutMatchesBoard);
        {
            const auto rects = settingsPageLayout ({ contentX, contentY, contentWidth, contentHeight }, referenceScale);
            auto near = [this] (int actual, int expected) { expectWithinAbsoluteError (actual, expected, tolerance); };

            near (rects.columns[arrangerColumn].getX(), arrangerX);
            near (rects.columns[arrangerColumn].getWidth(), columnWidth);
            near (rects.fields[engineField].getX(), contentX);
            near (rects.fields[engineField].getY(), engineFieldY);
            near (rects.fields[engineField].getHeight(), fieldHeight);
            near (rects.fields[midiInField].getY(), midiInFieldY);
            near (rects.keyboardRow.getY(), keyboardY);
            near (rects.arrangerToggles[firstRow].getX(), arrangerX);
            near (rects.arrangerToggles[firstRow].getY(), firstRowY);
            near (rects.arrangerToggles[lastArrangerRow].getY(), lastArrangerY);
            near (rects.chordModeRow.getX(), chordsX);
            near (rects.chordModeRow.getY(), chordModeY);
            near (rects.minorScaleRow.getY(), minorScaleY);
            near (rects.displayToggles[firstRow].getX(), displayX);
            near (rects.displayToggles[firstRow].getY(), firstRowY);
            near (rects.particleStyleLabel.getY(), styleLabelY);
            near (rects.particleStyleRow.getY(), styleRowY);
        }

        beginTest (keyboardSizesKeepStored);
        {
            const std::vector<int> standard { keys49, keys61, keys76, keys88 };
            expect (keyboardFieldSizes (keys61) == standard);
            expect (keyboardFieldSizes (noStoredCount) == standard);
            const std::vector<int> withStored { keys25, keys49, keys61, keys76, keys88 };
            expect (keyboardFieldSizes (keys25) == withStored);   // a device added with 25 keys keeps its count shown
        }

        beginTest (storedParticleStyleChecked);
        {
            expectEquals (ParticleStyles::fromStored (ParticleStyles::sparks), ParticleStyles::sparks);
            expectEquals (ParticleStyles::fromStored (ParticleStyles::wisps), ParticleStyles::wisps);
            expectEquals (ParticleStyles::fromStored (unknownStyle), ParticleStyles::dust);
            expectEquals (ParticleStyles::fromStored (negativeStyle), ParticleStyles::dust);
        }
    }
};
static SettingsPageTest settingsPageTest;
