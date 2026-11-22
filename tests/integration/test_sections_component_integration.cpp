#include <juce_core/juce_core.h>
#include "SectionsComponent.h"

namespace SectionsComponentIntegrationTestValues
{
    const juce::String testName     = "StyleSectionComponent - Integration";
    const juce::String testCategory = "Integration";

    const juce::String clickingAButtonInOneGroupDeactivates = "Clicking a button in one group deactivates the last button in the other group";
    const juce::String resizedLaysOutSectionGroupsCorrectly = "resized() lays out section groups correctly";
    const juce::String clickingAButtonActivatesIt = "Clicking a button activates it";
    const juce::String clickingTheSameButtonAgainDeactivatesIt = "Clicking the same button again deactivates it";
    const juce::String onlyOneButtonActiveAtATime = "Only one button active at a time";
    const juce::String callbacksAreExecutedCorrectly = "Callbacks are executed correctly";
    const juce::String deactivateLastClickedWorks = "deactivateLastClicked works";

    const juce::String mainGroup = "Main";
    const juce::String buttonA = "A";
    const juce::String buttonB = "B";
    const juce::String buttonC = "C";
    const juce::String buttonD = "D";
    const juce::String introsGroup = "Intros";
    const juce::String introOne = "Intro 1";
    const juce::String introTwo = "Intro 2";
    const juce::String variationsGroup = "Variations";
    const juce::String varOne = "Var 1";
    const juce::String varTwo = "Var 2";
    const juce::String firstGroupName = "Group1";
    const juce::String secondGroupName = "Group2";
    constexpr size_t firstGroup = 0;
    constexpr size_t secondGroup = 1;
    constexpr size_t firstButton = 0;
    constexpr size_t secondButton = 1;
    constexpr size_t twoGroups = 2;
    constexpr size_t twoButtons = 2;
    constexpr int layoutOrigin = 0;
    constexpr int layoutWidth = 400;
    constexpr int layoutHeight = 100;
    constexpr int groupCount = 2;
    constexpr int groupInsetTotal = 10;   // resized() uses reduced(5) on each side
}

using namespace SectionsComponentIntegrationTestValues;

class SectionsComponentIntegrationTest : public juce::UnitTest
{
public:
    SectionsComponentIntegrationTest()
        : juce::UnitTest (SectionsComponentIntegrationTestValues::testName, SectionsComponentIntegrationTestValues::testCategory) {}

    void runTest() override
    {
        testSingleButtonActivation();
        testButtonToggle();
        testOnlyOneActiveAtATime();
        testCrossGroupDeactivation();
        testCallbacksExecuted();
        testResizedLayout();
        testDeactivateLastClicked();
    }

private:
    std::unique_ptr<StyleSectionComponent> createComponent(
        bool& callbackA, bool& callbackB)
    {
        std::vector<juce::String> names = { mainGroup };
        std::vector<std::vector<juce::String>> groupNames = { {buttonA, buttonB} };

        std::unordered_map<juce::String, std::function<void()>> callbacks{
            {buttonA, [&]() { callbackA = true; }},
            {buttonB, [&]() { callbackB = true; }}
        };

        return std::make_unique<StyleSectionComponent>(names, groupNames, callbacks);
    }

    void testCrossGroupDeactivation()
    {
        beginTest(clickingAButtonInOneGroupDeactivates);

        bool intro1 = false, intro2 = false;
        bool var1 = false, var2 = false;

        // Create introsEndings component
        std::unordered_map<juce::String, std::function<void()>> callbacksIntros{
            {introOne, [&]() { intro1 = true; }},
            {introTwo, [&]() { intro2 = true; }}
        };

        auto introsEndings = std::make_unique<StyleSectionComponent>(
            std::vector<juce::String>{introsGroup},
            std::vector<std::vector<juce::String>>{ {introOne, introTwo}},
            callbacksIntros
            );

        // Create variationsFills component
        std::unordered_map<juce::String, std::function<void()>> callbacksVars{
            {varOne, [&]() { var1 = true; }},
            {varTwo, [&]() { var2 = true; }}
        };

        auto variationsFills = std::make_unique<StyleSectionComponent>(
            std::vector<juce::String>{variationsGroup},
            std::vector<std::vector<juce::String>>{ {varOne, varTwo}},
            callbacksVars
            );

        // Wire cross-deactivation
        introsEndings->undoLastClickedForOtherGroupSections = [&]()
        {
            variationsFills->deactivateLastClicked();
        };

        variationsFills->undoLastClickedForOtherGroupSections = [&]()
        {
            introsEndings->deactivateLastClicked();
        };

        auto& introButtons = introsEndings->getSectionGroups()[firstGroup]->getButtons();
        auto& varButtons = variationsFills->getSectionGroups()[firstGroup]->getButtons();

        // Step 1: Click Intro 1
        introButtons[firstButton]->onClick();
        expect(introsEndings->getLastUsedButton() == introButtons[firstButton].get());
        expect(variationsFills->getLastUsedButton() == nullptr);

        // Step 2: Click Var 2 (from another component)
        varButtons[secondButton]->onClick();

        // Intro 1 should now be deactivated
        expect(introsEndings->getLastUsedButton() == nullptr);
        expect(variationsFills->getLastUsedButton() == varButtons[secondButton].get());

        // Callbacks should all have executed
        expect(intro1 == true);
        expect(var2 == true);
    }

    void testResizedLayout()
    {
        beginTest(resizedLaysOutSectionGroupsCorrectly);

        bool clickedA = false, clickedB = false;
        auto comp = createComponent(clickedA, clickedB); // creates 1 group with 2 buttons

        // Add a second group manually to test multiple groups layout
        std::vector<juce::String> names = { firstGroupName, secondGroupName };
        std::vector<std::vector<juce::String>> buttonsPerGroup = { {buttonA, buttonB}, {buttonC, buttonD} };
        std::unordered_map<juce::String, std::function<void()>> callbacks = {
            {buttonA, [&]() { clickedA = true; }}, {buttonB, [&]() { clickedB = true; }},
            {buttonC, [&]() {}}, {buttonD, [&]() {}}
        };
        auto compMulti = std::make_unique<StyleSectionComponent>(names, buttonsPerGroup, callbacks);

        // Trigger layout
        compMulti->setBounds(layoutOrigin, layoutOrigin, layoutWidth, layoutHeight);
        compMulti->resized();

        auto& groups = compMulti->getSectionGroups();
        expect(groups.size() == twoGroups);

        int groupWidth = layoutWidth / groupCount;
        int reducedWidth = groupWidth - groupInsetTotal;

        expect(groups[firstGroup]->getWidth() == reducedWidth);
        expect(groups[secondGroup]->getWidth() == reducedWidth);

        // Check that buttons exist inside the groups
        expect(groups[firstGroup]->getButtons().size() == twoButtons);
        expect(groups[secondGroup]->getButtons().size() == twoButtons);
    }

    void testSingleButtonActivation()
    {
        beginTest(clickingAButtonActivatesIt);

        bool clickedA = false, clickedB = false;
        auto comp = createComponent(clickedA, clickedB);
        auto& buttons = comp->getSectionGroups()[firstGroup]->getButtons();

        buttons[firstButton]->onClick();

        expect(comp->getLastUsedButton() == buttons[firstButton].get());
        expect(clickedA == true);
        expect(clickedB == false);
    }

    void testButtonToggle()
    {
        beginTest(clickingTheSameButtonAgainDeactivatesIt);

        bool clickedA = false, clickedB = false;
        auto comp = createComponent(clickedA, clickedB);
        auto& buttons = comp->getSectionGroups()[firstGroup]->getButtons();

        buttons[firstButton]->onClick(); // activate
        buttons[firstButton]->onClick(); // deactivate

        expect(comp->getLastUsedButton() == nullptr);
        expect(clickedA == true); // callback still executed
    }

    void testOnlyOneActiveAtATime()
    {
        beginTest(onlyOneButtonActiveAtATime);

        bool clickedA = false, clickedB = false;
        auto comp = createComponent(clickedA, clickedB);
        auto& buttons = comp->getSectionGroups()[firstGroup]->getButtons();

        buttons[firstButton]->onClick();
        buttons[secondButton]->onClick();

        expect(comp->getLastUsedButton() == buttons[secondButton].get());
        expect(clickedA == true);
        expect(clickedB == true);

        // First button should be deactivated
        // Check its colour state indirectly (optional)
    }

    void testCallbacksExecuted()
    {
        beginTest(callbacksAreExecutedCorrectly);

        bool clickedA = false, clickedB = false;
        auto comp = createComponent(clickedA, clickedB);
        auto& buttons = comp->getSectionGroups()[firstGroup]->getButtons();

        buttons[firstButton]->onClick();
        buttons[secondButton]->onClick();

        expect(clickedA == true);
        expect(clickedB == true);
    }

    void testDeactivateLastClicked()
    {
        beginTest(deactivateLastClickedWorks);

        bool clickedA = false, clickedB = false;
        auto comp = createComponent(clickedA, clickedB);
        auto& buttons = comp->getSectionGroups()[firstGroup]->getButtons();

        buttons[firstButton]->onClick();
        comp->deactivateLastClicked();

        expect(comp->getLastUsedButton() == nullptr);

        // Click again to ensure toggle works after deactivate
        buttons[secondButton]->onClick();
        expect(comp->getLastUsedButton() == buttons[secondButton].get());
    }
};

static SectionsComponentIntegrationTest sectionsComponentIntegrationTest;