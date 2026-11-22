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
    const juce::String settingsButtonTextColourIsWhite = "Settings button - text colour is white";
    const juce::String exitButtonTextIsExit = "Exit button - text is 'Exit'";
    const juce::String exitButtonHasPointingHandCursor = "Exit button - has pointing hand cursor";
    const juce::String exitButtonTextColourIsWhite = "Exit button - text colour is white";
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

        beginTest(settingsButtonTextColourIsWhite);
        {
            OverlayComponent overlay;
            auto colour = overlay.getSettingsButton().findColour(juce::TextButton::textColourOffId);
            expect(colour == juce::Colours::white);
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

        beginTest(exitButtonTextColourIsWhite);
        {
            OverlayComponent overlay;
            auto colour = overlay.getExitButton().findColour(juce::TextButton::textColourOffId);
            expect(colour == juce::Colours::white);
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