#include <JuceHeader.h>
#include "UI/Stage/StagePageFrame.h"
#include "UI/Stage/StageMetrics.h"
#include "UI/Stage/StagePageWidgets.h"

namespace StagePageFrameTestValues
{
    const juce::String testName     = "StagePageFrame";
    const juce::String testCategory = "Unit";

    const juce::String contentBelowTitle = "the content area sits inside the page padding, below the title row";
    const juce::String layoutOnResize    = "resizing lays the content out at the current scale";
    const juce::String pageTitle         = "Recorder";

    constexpr float ownerScale  = 1.6f;
    constexpr int   pageWidth   = 1920;
    constexpr int   pageHeight  = 585;
    constexpr int   noCalls     = 0;
    constexpr int   oneCall     = 1;
    constexpr float tolerance   = 0.0001f;

    // Board StageRevised openPage=styles at scale 1: page at y 128, Exit 1127.89..1172 x 144..166,
    // "Style settings..." ends 1113.89 (gap 14 to Exit), content row at y 180.
    const juce::String exitCloses        = "the frame's Exit button calls onExit";
    const juce::String actionsRightAlign = "title actions sit right-aligned before Exit, the board's gap apart";
    const juce::String subtitleAfter     = "the subtitle starts one gap after the title (notes page)";
    const juce::String contentLikeBoard  = "the content starts below the 22 px title row plus the row gap (board: y 180)";
    const juce::String styleTitle        = "Style";
    const juce::String notesTitle        = "Notes";
    const juce::String notesSubtitle     = "Track 2";
    const juce::String firstAction       = "Sections";
    constexpr float referenceScale  = 1.0f;
    constexpr int   boardWidth      = 1200;
    constexpr int   boardPageHeight = 365;
    constexpr int   boardExitLeft   = 1128;
    constexpr int   boardExitRight  = 1172;
    constexpr int   boardRowTop     = 16;     // 144 - 128
    constexpr int   boardRowHeight  = 22;
    constexpr int   boardContentTop = 52;     // 180 - 128
    constexpr int   styleRowGap     = 14;
    constexpr int   notesRowGap     = 12;
    constexpr int   positionTolerance = 1;
}

class StagePageFrameTest : public juce::UnitTest
{
public:
    StagePageFrameTest() : juce::UnitTest (StagePageFrameTestValues::testName, StagePageFrameTestValues::testCategory) {}

    void runTest() override
    {
        using namespace StagePageFrameTestValues;

        beginTest (contentBelowTitle);
        {
            StagePageFrame frame { pageTitle };
            frame.setScale (ownerScale);
            frame.setSize (pageWidth, pageHeight);
            const auto content = frame.getContentBounds();
            expectEquals (frame.getTitle(), pageTitle);
            expectEquals (content.getX(), juce::roundToInt (StageMetrics::pagePaddingSide * ownerScale));
            expectGreaterThan (content.getY(), juce::roundToInt (StageMetrics::pagePaddingTop * ownerScale));
            expect (frame.getLocalBounds().contains (content));
            expectEquals (content.getRight(), pageWidth - juce::roundToInt (StageMetrics::pagePaddingSide * ownerScale));
        }

        beginTest (layoutOnResize);
        {
            StagePageFrame frame { pageTitle };
            int calls {};
            float scaleSeen {};
            juce::Rectangle<int> areaSeen;
            frame.onLayoutContent = [&] (juce::Rectangle<int> area, float scale) { ++calls; areaSeen = area; scaleSeen = scale; };
            expectEquals (calls, noCalls);
            frame.setScale (ownerScale);
            frame.setSize (pageWidth, pageHeight);
            expectGreaterOrEqual (calls, oneCall);
            expect (areaSeen == frame.getContentBounds());
            expectWithinAbsoluteError (scaleSeen, ownerScale, tolerance);
        }

        beginTest (exitCloses);
        {
            StagePageFrame frame { styleTitle };
            frame.setScale (referenceScale);
            frame.setSize (boardWidth, boardPageHeight);
            int exits {};
            frame.onExit = [&exits] { ++exits; };
            frame.getExitButton().onClick();
            expectEquals (exits, oneCall);
            expectWithinAbsoluteError (frame.getExitButton().getX(), boardExitLeft, positionTolerance);
            expectWithinAbsoluteError (frame.getExitButton().getRight(), boardExitRight, positionTolerance);
            expectEquals (frame.getExitButton().getY(), boardRowTop);
            expectEquals (frame.getExitButton().getHeight(), boardRowHeight);
        }

        beginTest (actionsRightAlign);
        {
            StagePageFrame frame { styleTitle };
            SegButton sections { firstAction, SegButton::Style::seg };
            frame.setActions ({ &sections });
            frame.setScale (referenceScale);
            frame.setSize (boardWidth, boardPageHeight);
            expect (sections.getParentComponent() == &frame);
            expectEquals (sections.getRight() + styleRowGap, frame.getExitButton().getX());
            expectEquals (sections.getY(), boardRowTop);
        }

        beginTest (subtitleAfter);
        {
            StagePageFrame frame { notesTitle };
            frame.setRowGap (notesRowGap);
            frame.setSubtitle (notesSubtitle);
            frame.setScale (referenceScale);
            frame.setSize (boardWidth, boardPageHeight);
            const auto subtitle = frame.getSubtitleBounds();
            expectWithinAbsoluteError (subtitle.getX(), frame.getTitleBounds().getRight() + notesRowGap, static_cast<float> (positionTolerance));
        }

        beginTest (contentLikeBoard);
        {
            StagePageFrame frame { styleTitle };
            frame.setScale (referenceScale);
            frame.setSize (boardWidth, boardPageHeight);
            expectEquals (frame.getContentBounds().getY(), boardContentTop);
        }
    }
};
static StagePageFrameTest stagePageFrameTest;
