#include <juce_core/juce_core.h>
#include "SectionGroupComponent.h"

namespace SectionGroupComponentTestValues
{
    const juce::String testName     = "SectionGroupComponent - Unit";
    const juce::String testCategory = "Unit";

    const juce::String buttonsCreated       = "Buttons are created correctly";
    const juce::String activationMapStarts  = "Activation map initialized correctly";
    const juce::String toggleAndTooltip     = "Toggle button and tooltip setup";
    const juce::String tooltipOnHover       = "Tooltip visibility on hover";

    const juce::String groupTitle  = "TestGroup";
    const juce::String fillsTitle  = "Fills";
    const juce::String buttonA     = "A";
    const juce::String buttonB     = "B";
    const juce::String buttonC     = "C";
    const juce::String fillOne     = "Fill 1";
    const juce::String fillTwo     = "Fill 2";

    constexpr size_t threeButtons = 3;
    constexpr size_t twoButtons   = 2;
    constexpr size_t firstButton  = 0;
    constexpr size_t secondButton = 1;
    constexpr size_t thirdButton  = 2;

    const juce::Point<float> origin { 0.0f, 0.0f };
    constexpr float noPressure    = 0.0f;
    constexpr float noOrientation = 0.0f;
    constexpr float noRotation    = 0.0f;
    constexpr float noTilt        = 0.0f;
    constexpr int   singleClick   = 1;
}

class SectionGroupComponentUnitTest : public juce::UnitTest
{
public:
    SectionGroupComponentUnitTest()
        : juce::UnitTest(SectionGroupComponentTestValues::testName, SectionGroupComponentTestValues::testCategory) {}

    void runTest() override
    {
        testButtonCreation();
        testActivationMapInitialization();
        testToggleButtonAndTooltip();
        testMouseEnterExitTooltip();
    }

private:
    void testButtonCreation()
    {
        using namespace SectionGroupComponentTestValues;
        beginTest(buttonsCreated);

        std::vector<juce::String> names = { buttonA, buttonB, buttonC };
        SectionGroupComponent group(groupTitle, names, false);

        auto& buttons = group.getButtons();
        expect(buttons.size() == threeButtons);
        expect(buttons[firstButton]->getButtonText() == buttonA);
        expect(buttons[secondButton]->getButtonText() == buttonB);
        expect(buttons[thirdButton]->getButtonText() == buttonC);
    }

    void testActivationMapInitialization()
    {
        using namespace SectionGroupComponentTestValues;
        beginTest(activationMapStarts);

        std::vector<juce::String> names = { buttonA, buttonB };
        SectionGroupComponent group(groupTitle, names, false);

        auto& map = group.getActivationMap();
        expect(map.size() == twoButtons);
        expect(map[buttonA] == false);
        expect(map[buttonB] == false);
    }

    void testToggleButtonAndTooltip()
    {
        using namespace SectionGroupComponentTestValues;
        beginTest(toggleAndTooltip);

        std::vector<juce::String> names = { fillOne, fillTwo };
        SectionGroupComponent group(fillsTitle, names, true);

        // Toggle button should exist
        expect(group.getToggleButton() != nullptr);

        // Tooltip should exist
        expect(group.getToggleToolTip() != nullptr);
        expect(!group.getToggleToolTip()->isVisible());
    }

    void testMouseEnterExitTooltip()
    {
        using namespace SectionGroupComponentTestValues;
        beginTest(tooltipOnHover);

        std::vector<juce::String> names = { fillOne };
        SectionGroupComponent group(fillsTitle, names, true);

        auto* toggle = group.getToggleButton();
        auto* tooltip = group.getToggleToolTip();

        expect(toggle != nullptr);
        expect(tooltip != nullptr);

        // Grab a real mouse input source from Desktop
        auto& source = juce::Desktop::getInstance().getMainMouseSource();

        // Create a MouseEvent with required parameters
        juce::MouseEvent hover(
            source,                            // real mouse input source
            origin,                            // local position
            juce::ModifierKeys::noModifiers,  // no modifiers
            noPressure,
            noOrientation,
            noRotation,
            noTilt, // tiltX
            noTilt, // tiltY
            toggle,   // eventComponent
            toggle,   // originator
            juce::Time::getCurrentTime(),     // event time
            origin,                            // mouse down pos
            juce::Time::getCurrentTime(),     // mouse down time
            singleClick,                       // number of clicks
            false                             // mouse was dragged
        );

        // Simulate mouse enter
        group.mouseEnter(hover);
        expect(tooltip->isVisible());

        // Simulate mouse exit
        group.mouseExit(hover);
        expect(!tooltip->isVisible());
    }
};

static SectionGroupComponentUnitTest sectionGroupComponentUnitTest;
