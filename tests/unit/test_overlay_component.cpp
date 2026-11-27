#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "OverlayComponent.h"

namespace OverlayComponentTestValues
{
    const juce::String testName     = "OverlayComponent Unit";
    const juce::String testCategory = "Unit";

    const juce::String constructorStartsInvisible = "Constructor - starts invisible";
    const juce::String constructorIsNotOpaque = "Constructor - is not opaque";
    const juce::String constructorWantsKeyboardFocus = "Constructor - wants keyboard focus";
    const juce::String settingsButtonTextIsSettings = "Settings button - text is 'Settings'";
    const juce::String settingsButtonHasPointingHandCursor = "Settings button - has pointing hand cursor";
    const juce::String menuLayout = "Esc menu (owner: 'esc opens the old screen'): a centred page panel, heading, then Settings, Effects, Exit stacked";
    const juce::String menuScales = "Esc menu - the same look at any scale";
    const juce::String clickOutsideCloses = "Esc menu - a click outside the panel closes it, a click on it does not";
    const juce::String exitButtonTextIsExit = "Exit button - text is 'Exit'";
    const juce::String exitButtonHasPointingHandCursor = "Exit button - has pointing hand cursor";
    const juce::String showOverlaySetVisibleTrueMakesComponentVisible = "showOverlay - setVisible true makes component visible";
    const juce::String hideOverlayMakesComponentInvisible = "hideOverlay - makes component invisible";
    const juce::String showThenHideThenShowTogglesCorrectly = "show then hide then show - toggles correctly";
    const juce::String getExitButtonReturnsAValidReference = "getExitButton returns a valid reference";
    const juce::String getSettingsButtonReturnsAValidReference = "getSettingsButton returns a valid reference";
    const juce::String settingsButtonClickInvokesOnSettingsClickCallback = "Settings button click - invokes onSettingsClick callback";
    const juce::String settingsButtonClickNoCrashWhenOnSettingsClick = "Settings button click - no crash when onSettingsClick is null";
    const juce::String keyPressedEscapeKeyReturnsTrue = "keyPressed - escape key returns true";
    const juce::String keyPressedNonEscapeKeyReturnsFalse = "keyPressed - non-escape key returns false";
    const juce::String keyPressedEscapeCallsOnRequestCloseWhenSet = "keyPressed - escape calls onRequestClose when set";
    const juce::String keyPressedEscapeWithoutOnRequestCloseCallsSetWindowFlagAnd = "keyPressed - escape without onRequestClose calls setWindowFlag and hides";
    const juce::String keyPressedEscapeWithoutOnRequestCloseAndWithoutSetWindowFlag = "keyPressed - escape without onRequestClose and without setWindowFlag still hides";
    const juce::String bringSeparateWindowFrontCallableWhenSet = "bringSeparateWindowFront - callable when set";
    const juce::String bringSeparateWindowFrontNullCheckPreventsCrash = "bringSeparateWindowFront - null check prevents crash";
    const juce::String paintExecutesWithoutCrash = "paint - executes without crash";
    const juce::String resizedExecutesWithoutCrash = "resized - executes without crash";
    const juce::String resizedSettingsButtonGetsNonZeroBounds = "resized - settings button gets non-zero bounds";
    const juce::String resizedExitButtonGetsNonZeroBounds = "resized - exit button gets non-zero bounds";
    const juce::String callbacksAreNullByDefault = "Callbacks are null by default";

    const juce::String settingsText = "Settings";
    const juce::String exitText = "Exit";

    constexpr int letterKey = 'a';
    constexpr int noModifiers = 0;
    constexpr juce::juce_wchar noCharacter = 0;
    constexpr int boundsLeft = 0;
    constexpr int boundsTop = 0;
    constexpr int smallWidth = 400;
    constexpr int smallHeight = 300;
    constexpr int largeWidth = 800;
    constexpr int largeHeight = 600;
    constexpr int noSize = 0;
    constexpr int boardWidth = 1200;
    constexpr int boardHeight = 645;
    constexpr float boardScale = 1.0f;
    constexpr float doubleScale = 2.0f;
    constexpr int menuWidth = 260;           // board px
    constexpr int menuButtonCount = 3;
    constexpr int firstButton = 0;
    constexpr int secondButton = 1;
    constexpr int thirdButton = 2;
    constexpr int doubled = 2;
    constexpr int noCloses = 0;
    constexpr int oneClose = 1;
    const juce::Point<int> outsideThePanel { 10, 10 };
}

using namespace OverlayComponentTestValues;

class OverlayComponentUnitTest : public juce::UnitTest
{
public:
    OverlayComponentUnitTest() : juce::UnitTest (OverlayComponentTestValues::testName, OverlayComponentTestValues::testCategory) {}

    void runTest() override
    {
        // ---- Constructor defaults ----

        beginTest(constructorStartsInvisible);
        {
            OverlayComponent overlay;
            expect(overlay.isVisible() == false);
        }

        beginTest(constructorIsNotOpaque);
        {
            OverlayComponent overlay;
            expect(overlay.isOpaque() == false);
        }

        beginTest(constructorWantsKeyboardFocus);
        {
            OverlayComponent overlay;
            expect(overlay.getWantsKeyboardFocus() == true);
        }

        // ---- Settings button ----

        beginTest(settingsButtonTextIsSettings);
        {
            OverlayComponent overlay;
            expect(overlay.getSettingsButton().getButtonText() == settingsText);
        }

        beginTest(settingsButtonHasPointingHandCursor);
        {
            OverlayComponent overlay;
            expect(overlay.getSettingsButton().getMouseCursor() == juce::MouseCursor::PointingHandCursor);
        }

        beginTest(menuLayout);
        {
            const juce::Rectangle<int> host (boardWidth, boardHeight);
            const auto rects = escMenuLayout (host, boardScale);
            expectEquals (rects.panel.getWidth(), menuWidth);
            expect (rects.panel.getCentre() == host.getCentre());
            expect (rects.panel.contains (rects.heading));
            expectEquals ((int) rects.buttons.size(), menuButtonCount);
            expect (rects.heading.getBottom() <= rects.buttons[firstButton].getY());
            for (int index = firstButton; index < menuButtonCount; ++index)
            {
                expect (rects.panel.contains (rects.buttons[(size_t) index]));
                expectEquals (rects.buttons[(size_t) index].getWidth(), rects.heading.getWidth());
                expectEquals (rects.buttons[(size_t) index].getHeight(), rects.buttons[firstButton].getHeight());
            }
            expect (rects.buttons[firstButton].getBottom() < rects.buttons[secondButton].getY());
            expect (rects.buttons[secondButton].getBottom() < rects.buttons[thirdButton].getY());

            OverlayComponent overlay;
            overlay.setBounds (host);
            expect (overlay.getSettingsButton().getBounds() == rects.buttons[firstButton]);
            expect (overlay.getEffectsButton().getBounds() == rects.buttons[secondButton]);
            expect (overlay.getExitButton().getBounds() == rects.buttons[thirdButton]);
        }

        beginTest(menuScales);
        {
            const juce::Rectangle<int> host (boardWidth * doubled, boardHeight * doubled);
            const auto atOne = escMenuLayout ({ boardWidth, boardHeight }, boardScale);
            const auto atTwo = escMenuLayout (host, doubleScale);
            expectEquals (atTwo.panel.getWidth(), atOne.panel.getWidth() * doubled);
            expectEquals (atTwo.panel.getHeight(), atOne.panel.getHeight() * doubled);
            expect (atTwo.panel.getCentre() == host.getCentre());
        }

        beginTest(clickOutsideCloses);
        {
            OverlayComponent overlay;
            overlay.setBounds (juce::Rectangle<int> (boardWidth, boardHeight));
            int closes {};
            overlay.onRequestClose = [&closes] { ++closes; };
            overlay.clickAt (escMenuLayout (overlay.getLocalBounds(), boardScale).panel.getCentre());
            expectEquals (closes, noCloses);
            overlay.clickAt (outsideThePanel);
            expectEquals (closes, oneClose);
        }

        // ---- Exit button ----

        beginTest(exitButtonTextIsExit);
        {
            OverlayComponent overlay;
            expect(overlay.getExitButton().getButtonText() == exitText);
        }

        beginTest(exitButtonHasPointingHandCursor);
        {
            OverlayComponent overlay;
            expect(overlay.getExitButton().getMouseCursor() == juce::MouseCursor::PointingHandCursor);
        }


        // ---- showOverlay / hideOverlay ----

        beginTest(showOverlaySetVisibleTrueMakesComponentVisible);
        {
            OverlayComponent overlay;
            expect(overlay.isVisible() == false);
            overlay.showOverlay();
            expect(overlay.isVisible() == true);
        }

        beginTest(hideOverlayMakesComponentInvisible);
        {
            OverlayComponent overlay;
            overlay.showOverlay();
            expect(overlay.isVisible() == true);
            overlay.hideOverlay();
            expect(overlay.isVisible() == false);
        }

        beginTest(showThenHideThenShowTogglesCorrectly);
        {
            OverlayComponent overlay;
            overlay.showOverlay();
            expect(overlay.isVisible() == true);
            overlay.hideOverlay();
            expect(overlay.isVisible() == false);
            overlay.showOverlay();
            expect(overlay.isVisible() == true);
        }

        // ---- getExitButton / getSettingsButton accessors ----

        beginTest(getExitButtonReturnsAValidReference);
        {
            OverlayComponent overlay;
            auto& btn = overlay.getExitButton();
            expect(btn.getButtonText() == exitText);
        }

        beginTest(getSettingsButtonReturnsAValidReference);
        {
            OverlayComponent overlay;
            auto& btn = overlay.getSettingsButton();
            expect(btn.getButtonText() == settingsText);
        }

        // ---- Settings button callback ----

        beginTest(settingsButtonClickInvokesOnSettingsClickCallback);
        {
            OverlayComponent overlay;
            bool called = false;
            overlay.onSettingsClick = [&called]() { called = true; };
            overlay.getSettingsButton().onClick();
            expect(called == true);
        }

        beginTest(settingsButtonClickNoCrashWhenOnSettingsClick);
        {
            OverlayComponent overlay;
            overlay.onSettingsClick = nullptr;
            overlay.getSettingsButton().onClick();
            expect(true); // no crash
        }

        // ---- keyPressed ----

        beginTest(keyPressedEscapeKeyReturnsTrue);
        {
            OverlayComponent overlay;
            bool result = overlay.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
            expect(result == true);
        }

        beginTest(keyPressedNonEscapeKeyReturnsFalse);
        {
            OverlayComponent overlay;
            bool result = overlay.keyPressed(juce::KeyPress(letterKey, noModifiers, noCharacter));
            expect(result == false);
        }

        beginTest(keyPressedEscapeCallsOnRequestCloseWhenSet);
        {
            OverlayComponent overlay;
            bool closeCalled = false;
            overlay.onRequestClose = [&closeCalled]() { closeCalled = true; };
            overlay.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
            expect(closeCalled == true);
        }

        beginTest(keyPressedEscapeWithoutOnRequestCloseCallsSetWindowFlagAnd);
        {
            OverlayComponent overlay;
            overlay.onRequestClose = nullptr;
            bool flagCalled = false;
            overlay.setWindowFlag = [&flagCalled]() { flagCalled = true; };

            overlay.showOverlay();
            overlay.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));

            expect(flagCalled == true);
            expect(overlay.isVisible() == false);
        }

        beginTest(keyPressedEscapeWithoutOnRequestCloseAndWithoutSetWindowFlag);
        {
            OverlayComponent overlay;
            overlay.onRequestClose = nullptr;
            overlay.setWindowFlag = nullptr;

            overlay.showOverlay();
            overlay.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));

            expect(overlay.isVisible() == false);
        }

        // ---- bringSeparateWindowFront callback (tested via direct callback) ----

        beginTest(bringSeparateWindowFrontCallableWhenSet);
        {
            OverlayComponent overlay;
            bool called = false;
            overlay.bringSeparateWindowFront = [&called]() { called = true; };
            overlay.bringSeparateWindowFront();
            expect(called == true);
        }

        beginTest(bringSeparateWindowFrontNullCheckPreventsCrash);
        {
            OverlayComponent overlay;
            overlay.bringSeparateWindowFront = nullptr;
            // mouseDown checks for null before calling, so no crash
            expect(!overlay.bringSeparateWindowFront);
        }

        // ---- paint ----

        beginTest(paintExecutesWithoutCrash);
        {
            OverlayComponent overlay;
            overlay.setBounds(boundsLeft, boundsTop, smallWidth, smallHeight);

            juce::Image img(juce::Image::ARGB, smallWidth, smallHeight, true);
            juce::Graphics g(img);

            overlay.paint(g);
            expect(true); // no crash
        }

        // ---- resized ----

        beginTest(resizedExecutesWithoutCrash);
        {
            OverlayComponent overlay;
            overlay.setBounds(boundsLeft, boundsTop, largeWidth, largeHeight);
            overlay.resized();
            expect(true); // no crash
        }

        beginTest(resizedSettingsButtonGetsNonZeroBounds);
        {
            OverlayComponent overlay;
            overlay.setBounds(boundsLeft, boundsTop, largeWidth, largeHeight);
            overlay.resized();
            auto& btn = overlay.getSettingsButton();
            expect(btn.getWidth() > noSize);
            expect(btn.getHeight() > noSize);
        }

        beginTest(resizedExitButtonGetsNonZeroBounds);
        {
            OverlayComponent overlay;
            overlay.setBounds(boundsLeft, boundsTop, largeWidth, largeHeight);
            overlay.resized();
            auto& btn = overlay.getExitButton();
            expect(btn.getWidth() > noSize);
            expect(btn.getHeight() > noSize);
        }

        // ---- Callback defaults ----

        beginTest(callbacksAreNullByDefault);
        {
            OverlayComponent overlay;
            expect(!overlay.onSettingsClick);
            expect(!overlay.windowExists);
            expect(!overlay.closeWindow);
            expect(!overlay.bringSeparateWindowFront);
            expect(!overlay.setWindowFlag);
            expect(!overlay.onRequestClose);
        }
    }
};

static OverlayComponentUnitTest overlayComponentUnitTest;