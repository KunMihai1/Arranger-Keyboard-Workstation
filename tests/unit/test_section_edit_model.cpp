#include <JuceHeader.h>
#include "UI/Sections/SectionEditModel.h"
#include "UI/Sections/SectionInspector.h"
#include "UI/Sections/SectionJumpPads.h"

/*  The Sections page's inspector (board PageSections): today a section could only be moved or resized by
    dragging; the inspector's arrows make the same edits, clamped the same way, and change the type and what
    happens when the section ends. */

namespace SectionEditModelTestValues
{
    const juce::String testName     = "SectionEditModel";
    const juce::String testCategory = "Unit";

    const juce::String typesAreTheAddButtons = "the types are the Add buttons' five, in order";
    const juce::String namesNumberPerType    = "a new name is the type and the next number of that type, gaps kept";
    const juce::String typeStepsAndRenames   = "a type step moves to the next type, wrapping, and takes that type's next number";
    const juce::String startClampsLikeDrag   = "start moves one bar, never before bar 1 or past the last bar line";
    const juce::String lengthClampsLikeDrag  = "length changes one bar, at least one bar, never past the last bar line";
    const juce::String afterCompleteSets     = "When it ends sets the window's after-complete; the same value is no change";
    const juce::String outOfRangeIgnored     = "an index with no window changes nothing";
    const juce::String inspectorTexts        = "the inspector shows Var for a variation, and 1 bar / n bars";
    const juce::String widgetsTestName       = "SectionPageWidgets";
    const juce::String arrowsReport          = "each inspector arrow sits in the LCD and reports itself when clicked";
    const juce::String nothingSelected       = "with nothing selected the arrows are gone and the buttons are off";
    const juce::String afterButtonsShow      = "the When it ends button of the section's value is lit, and a press reports its value";
    const juce::String padsReport            = "pads sit left to right after Jump; a click on one reports it, elsewhere nothing";
    const juce::String padLedFollows         = "the playing pad's LED: a section index, or none when out of range";
    const juce::String introPad              = "INTRO 1";
    const juce::String varPad                = "VAR 1";
    const juce::String fillPad               = "FILL 1";

    const juce::String intro1    = "Intro 1";
    const juce::String intro3    = "Intro 3";
    const juce::String intro4    = "Intro 4";
    const juce::String variation1 = "Variation 1";
    const juce::String break2    = "Break 2";
    const juce::String break3    = "Break 3";
    const juce::String fill1     = "Fill 1";
    const juce::String ending1   = "Ending 1";
    const juce::String varText   = "Var";
    const juce::String introText = "Intro";
    const juce::String oneBarText    = "1 bar";
    const juce::String eightBarsText = "8 bars";

    constexpr int totalBars      = 10;
    constexpr int startThree     = 3;
    constexpr int startFour      = 4;
    constexpr int startNine      = 9;
    constexpr int firstBar       = 1;
    constexpr int lengthTwo      = 2;
    constexpr int lengthOne      = 1;
    constexpr int eightBars      = 8;
    constexpr int first          = 0;
    constexpr int second         = 1;
    constexpr int third          = 2;
    constexpr int missing        = 7;
    constexpr int negative       = -1;
    constexpr size_t typeCount   = 5;
    constexpr int inspectorWidth = 206;
    constexpr int inspectorHeight = 200;
    constexpr int padsWidth      = 600;
    constexpr int padsHeight     = 44;
    constexpr int noPadClicked   = -1;

    SectionWindow window (ArrangerSectionType type, const juce::String& name, int startBar, int lengthBars)
    {
        SectionWindow made;
        made.type = type;
        made.name = name;
        made.startBar = startBar;
        made.lengthBars = lengthBars;
        return made;
    }
}

class SectionEditModelTest : public juce::UnitTest
{
public:
    SectionEditModelTest() : juce::UnitTest (SectionEditModelTestValues::testName, SectionEditModelTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SectionEditModelTestValues;
        using namespace SectionEditModel;

        beginTest (typesAreTheAddButtons);
        {
            const std::vector<ArrangerSectionType> expected { ArrangerSectionType::Intro, ArrangerSectionType::Variation, ArrangerSectionType::Fill,
                                                              ArrangerSectionType::Break, ArrangerSectionType::Ending };
            expectEquals (editableTypes().size(), typeCount);
            expect (editableTypes() == expected);
        }

        beginTest (namesNumberPerType);
        expectEquals (nextName ({}, ArrangerSectionType::Intro), intro1);
        expectEquals (nextName ({ window (ArrangerSectionType::Intro, intro1, firstBar, lengthTwo),
                                  window (ArrangerSectionType::Intro, intro3, startThree, lengthTwo) }, ArrangerSectionType::Intro), intro4);

        beginTest (typeStepsAndRenames);
        {
            std::vector<SectionWindow> windows { window (ArrangerSectionType::Variation, variation1, firstBar, eightBars),
                                                 window (ArrangerSectionType::Break, break2, startNine, lengthOne),
                                                 window (ArrangerSectionType::Fill, fill1, startNine, lengthOne) };
            expect (stepType (windows, third, later));
            expect (windows[(size_t) third].type == ArrangerSectionType::Break);
            expectEquals (windows[(size_t) third].name, break3);

            std::vector<SectionWindow> wrapping { window (ArrangerSectionType::Intro, intro1, firstBar, lengthTwo) };
            expect (stepType (wrapping, first, earlier));
            expect (wrapping[(size_t) first].type == ArrangerSectionType::Ending);
            expectEquals (wrapping[(size_t) first].name, ending1);
        }

        beginTest (startClampsLikeDrag);
        {
            std::vector<SectionWindow> windows { window (ArrangerSectionType::Variation, variation1, startThree, lengthTwo),
                                                 window (ArrangerSectionType::Fill, fill1, startNine, lengthTwo) };
            expect (stepStart (windows, first, later, totalBars));
            expectEquals (windows[(size_t) first].startBar, startFour);
            expect (! stepStart (windows, second, later, totalBars));   // bars 9-10 end on the last bar line
            expectEquals (windows[(size_t) second].startBar, startNine);
            windows[(size_t) first].startBar = firstBar;
            expect (! stepStart (windows, first, earlier, totalBars));
        }

        beginTest (lengthClampsLikeDrag);
        {
            std::vector<SectionWindow> windows { window (ArrangerSectionType::Fill, fill1, startNine, lengthTwo) };
            expect (! stepLength (windows, first, later, totalBars));
            expect (stepLength (windows, first, earlier, totalBars));
            expectEquals (windows[(size_t) first].lengthBars, lengthOne);
            expect (! stepLength (windows, first, earlier, totalBars));
        }

        beginTest (afterCompleteSets);
        {
            std::vector<SectionWindow> windows { window (ArrangerSectionType::Variation, variation1, firstBar, eightBars) };
            windows[(size_t) first].afterComplete = ArrangerAfterComplete::Loop;
            expect (setAfterComplete (windows, first, ArrangerAfterComplete::Stop));
            expect (windows[(size_t) first].afterComplete == ArrangerAfterComplete::Stop);
            expect (! setAfterComplete (windows, first, ArrangerAfterComplete::Stop));
        }

        beginTest (outOfRangeIgnored);
        {
            std::vector<SectionWindow> windows { window (ArrangerSectionType::Variation, variation1, firstBar, eightBars) };
            expect (! stepType (windows, missing, later));
            expect (! stepStart (windows, negative, later, totalBars));
            expect (! stepLength (windows, missing, later, totalBars));
            expect (! setAfterComplete (windows, negative, ArrangerAfterComplete::Stop));
        }

        beginTest (inspectorTexts);
        expectEquals (typeText (ArrangerSectionType::Variation), varText);
        expectEquals (typeText (ArrangerSectionType::Intro), introText);
        expectEquals (lengthText (lengthOne), oneBarText);
        expectEquals (lengthText (eightBars), eightBarsText);
    }
};
static SectionEditModelTest sectionEditModelTest;

class SectionPageWidgetsTest : public juce::UnitTest
{
public:
    SectionPageWidgetsTest() : juce::UnitTest (SectionEditModelTestValues::widgetsTestName, SectionEditModelTestValues::testCategory) {}

    void runTest() override
    {
        using namespace SectionEditModelTestValues;

        beginTest (arrowsReport);
        {
            SectionInspector inspector;
            inspector.setBounds (int {}, int {}, inspectorWidth, inspectorHeight);
            inspector.setSection (window (ArrangerSectionType::Variation, variation1, startThree, eightBars));
            std::vector<InspectorArrow> reported;
            inspector.onArrow = [&reported] (InspectorArrow arrow) { reported.push_back (arrow); };
            const std::vector<InspectorArrow> all { InspectorArrow::typeEarlier, InspectorArrow::typeLater, InspectorArrow::startEarlier,
                                                    InspectorArrow::startLater, InspectorArrow::lengthShorter, InspectorArrow::lengthLonger };
            for (auto arrow : all)
            {
                const auto box = inspector.arrowBounds (arrow);
                expect (! box.isEmpty());
                expect (inspector.getLocalBounds().contains (box));
                inspector.clickAt (box.getCentre());
            }
            expect (reported == all);
        }

        beginTest (nothingSelected);
        {
            SectionInspector inspector;
            inspector.setBounds (int {}, int {}, inspectorWidth, inspectorHeight);
            inspector.setSection (std::nullopt);
            expect (inspector.arrowBounds (InspectorArrow::typeLater).isEmpty());
            expect (! inspector.getPlayThisButton().isEnabled());
            expect (! inspector.getRemoveButton().isEnabled());
            expect (! inspector.getAfterButton (ArrangerAfterComplete::Loop).isEnabled());
        }

        beginTest (afterButtonsShow);
        {
            SectionInspector inspector;
            auto section = window (ArrangerSectionType::Variation, variation1, startThree, eightBars);
            section.afterComplete = ArrangerAfterComplete::FallThrough;
            inspector.setSection (section);
            expect (inspector.getAfterButton (ArrangerAfterComplete::FallThrough).isLedOn());
            expect (! inspector.getAfterButton (ArrangerAfterComplete::Loop).isLedOn());
            std::optional<ArrangerAfterComplete> pressed;
            inspector.onAfterComplete = [&pressed] (ArrangerAfterComplete value) { pressed = value; };
            auto& stopButton = inspector.getAfterButton (ArrangerAfterComplete::Stop);
            if (stopButton.onClick != nullptr)
                stopButton.onClick();
            expect (pressed.has_value() && *pressed == ArrangerAfterComplete::Stop);
        }

        beginTest (padsReport);
        {
            SectionJumpPads pads;
            pads.setBounds (int {}, int {}, padsWidth, padsHeight);
            pads.setPads ({ introPad, varPad, fillPad });
            expect (pads.padBounds (first).getRight() < pads.padBounds (second).getX());
            expectEquals (pads.padBounds (first).getWidth(), pads.padBounds (third).getWidth());
            int clicked = noPadClicked;
            pads.onPadClicked = [&clicked] (int index) { clicked = index; };
            pads.clickAt (pads.padBounds (second).getCentre());
            expectEquals (clicked, second);
            clicked = noPadClicked;
            pads.clickAt ({ int {}, padsHeight / lengthTwo });   // the "Jump" label
            expectEquals (clicked, noPadClicked);
        }

        beginTest (padLedFollows);
        {
            SectionJumpPads pads;
            pads.setPads ({ introPad, varPad });
            pads.setPlayingIndex (second);
            expectEquals (pads.getPlayingIndex(), second);
            pads.setPlayingIndex (missing);
            expectEquals (pads.getPlayingIndex(), SectionJumpPadValues::noPad);
        }
    }
};
static SectionPageWidgetsTest sectionPageWidgetsTest;
