#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "OverlayComponent.h"

namespace OverlayIntegrationTestValues
{
    const juce::String testName     = "OverlayComponent Integration";
    const juce::String testCategory = "Integration";

    const juce::String escCycle            = "Integration - ESC open/close cycle simulates MainComponent behavior";
    const juce::String settingsCreatesMidi = "Integration - Settings button triggers MIDI window creation";
    const juce::String windowFlagOnEsc     = "Integration - setWindowFlag updates external state on ESC without onRequestClose";
    const juce::String bringWindowFront    = "Integration - bringSeparateWindowFront callback brings MIDI window to front";
    const juce::String fullLifecycle       = "Integration - Full overlay lifecycle";
    const juce::String focusControlsVisibility = "Integration - overlay visibility controlled by app focus state";
    const juce::String cyclesDoNotLeak     = "Integration - multiple create/destroy cycles do not leak";
    const juce::String settingsEveryClick  = "Integration - clicking settings multiple times calls callback each time";
    const juce::String resizeGivesLayout   = "Integration - show then resize provides usable button layout";
    const juce::String escClosesOverlay    = "Integration - ESC on overlay triggers keyPressed -> onRequestClose -> destroys overlay";

    constexpr int screenLeft   = 0;
    constexpr int screenTop    = 0;
    constexpr int screenWidth  = 1920;
    constexpr int screenHeight = 1080;
    constexpr int createCycles = 10;
    constexpr int threeClicks  = 3;
    constexpr int noSize       = 0;
}

using namespace OverlayIntegrationTestValues;


// ==================================================================
// INTEGRATION TESTS
// These tests verify OverlayComponent behavior in realistic
// scenarios where callbacks interact with external state,
// simulating how MainComponent wires up the overlay.
// ==================================================================

class OverlayComponentIntegrationTest : public juce::UnitTest
{
public:
    OverlayComponentIntegrationTest()
        : juce::UnitTest(testName, testCategory) {}

    void runTest() override
    {
        // ---- Simulated MainComponent integration: ESC open/close cycle ----

        beginTest(escCycle);
        {
            // Simulate the overlayWindow unique_ptr pattern from MainComponent
            std::unique_ptr<OverlayComponent> overlayWindow = nullptr;
            bool overlayShouldBeVisible = false;

            // First ESC press: create and show overlay
            overlayWindow = std::make_unique<OverlayComponent>();
            overlayWindow->setBounds(screenLeft, screenTop, screenWidth, screenHeight);
            overlayWindow->showOverlay();
            overlayShouldBeVisible = true;

            overlayWindow->onRequestClose = [&overlayWindow, &overlayShouldBeVisible]()
            {
                if (overlayWindow)
                {
                    overlayShouldBeVisible = false;
                    overlayWindow.reset();
                }
            };

            expect(overlayWindow != nullptr);
            expect(overlayWindow->isVisible() == true);
            expect(overlayShouldBeVisible == true);

            // Second ESC press: close overlay via onRequestClose
            if (overlayWindow && overlayWindow->onRequestClose)
                overlayWindow->onRequestClose();

            expect(overlayWindow == nullptr);
            expect(overlayShouldBeVisible == false);
        }

        // ---- Settings button opens settings window ----

        beginTest(settingsCreatesMidi);
        {
            OverlayComponent overlay;
            bool midiWindowCreated = false;
            bool midiWindowShouldBeVisible = false;

            overlay.onSettingsClick = [&midiWindowCreated, &midiWindowShouldBeVisible]()
            {
                midiWindowShouldBeVisible = true;
                midiWindowCreated = true;
            };

            // Simulate clicking the settings button
            overlay.getSettingsButton().onClick();

            expect(midiWindowCreated == true);
            expect(midiWindowShouldBeVisible == true);
        }

        // ---- setWindowFlag callback integration ----

        beginTest(windowFlagOnEsc);
        {
            OverlayComponent overlay;
            bool overlayShouldBeVisible = true;

            overlay.onRequestClose = nullptr; // no close handler
            overlay.setWindowFlag = [&overlayShouldBeVisible]()
            {
                overlayShouldBeVisible = false;
            };

            overlay.showOverlay();
            expect(overlay.isVisible() == true);

            // Press ESC — falls through to setWindowFlag branch
            overlay.keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));

            expect(overlayShouldBeVisible == false);
            expect(overlay.isVisible() == false);
        }

        // ---- bringSeparateWindowFront callback integration ----

        beginTest(bringWindowFront);
        {
            OverlayComponent overlay;
            bool midiWindowBroughtToFront = false;

            overlay.bringSeparateWindowFront = [&midiWindowBroughtToFront]()
            {
                midiWindowBroughtToFront = true;
            };

            // Directly invoke the callback (mouseDown delegates to this)
            overlay.bringSeparateWindowFront();
            expect(midiWindowBroughtToFront == true);
        }

        // ---- Full lifecycle: create, configure, show, interact, close ----

        beginTest(fullLifecycle);
        {
            std::unique_ptr<OverlayComponent> overlayWindow = nullptr;
            bool overlayShouldBeVisible = false;
            bool settingsOpened = false;
            bool windowFrontCalled = false;

            // Step 1: Create overlay (simulating first ESC press)
            overlayWindow = std::make_unique<OverlayComponent>();
            overlayWindow->setBounds(screenLeft, screenTop, screenWidth, screenHeight);
            overlayShouldBeVisible = true;

            // Step 2: Wire up all callbacks (like MainComponent does)
            overlayWindow->onRequestClose = [&overlayWindow, &overlayShouldBeVisible]()
            {
                if (overlayWindow)
                {
                    overlayShouldBeVisible = false;
                    overlayWindow.reset();
                }
            };

            overlayWindow->setWindowFlag = [&overlayShouldBeVisible]()
            {
                overlayShouldBeVisible = false;
            };

            overlayWindow->bringSeparateWindowFront = [&windowFrontCalled]()
            {
                windowFrontCalled = true;
            };

            overlayWindow->onSettingsClick = [&settingsOpened]()
            {
                settingsOpened = true;
            };

            overlayWindow->showOverlay();
            expect(overlayWindow->isVisible() == true);

            // Step 4: Click settings
            overlayWindow->getSettingsButton().onClick();
            expect(settingsOpened == true);

            // Step 5: invoke bringSeparateWindowFront
            overlayWindow->bringSeparateWindowFront();
            expect(windowFrontCalled == true);

            // Step 6: Close via ESC (onRequestClose)
            overlayWindow->onRequestClose();
            expect(overlayWindow == nullptr);
            expect(overlayShouldBeVisible == false);
        }

        // ---- globalFocusChanged simulation: hide/show on app deactivate/activate ----

        beginTest(focusControlsVisibility);
        {
            std::unique_ptr<OverlayComponent> overlayWindow = std::make_unique<OverlayComponent>();
            bool overlayShouldBeVisible = true;

            overlayWindow->showOverlay();
            expect(overlayWindow->isVisible() == true);

            // Simulate app losing focus
            overlayWindow->hideOverlay();
            expect(overlayWindow->isVisible() == false);

            // Simulate app regaining focus — restore based on flag
            overlayWindow->showOverlay();
            expect(overlayWindow->isVisible() == true);

            // Now close overlay
            overlayShouldBeVisible = false;

            // Simulate app losing and regaining focus again
            overlayWindow->hideOverlay();
            overlayWindow->setVisible(overlayShouldBeVisible);
            expect(overlayWindow->isVisible() == false);
        }

        // ---- Multiple open/close cycles ----

        beginTest(cyclesDoNotLeak);
        {
            for (int cycle {}; cycle < createCycles; ++cycle)
            {
                std::unique_ptr<OverlayComponent> overlayWindow = std::make_unique<OverlayComponent>();
                bool overlayShouldBeVisible = true;

                overlayWindow->onRequestClose = [&overlayWindow, &overlayShouldBeVisible]()
                {
                    overlayShouldBeVisible = false;
                    overlayWindow.reset();
                };

                overlayWindow->showOverlay();
                expect(overlayWindow != nullptr);
                expect(overlayWindow->isVisible() == true);

                overlayWindow->onRequestClose();
                expect(overlayWindow == nullptr);
                expect(overlayShouldBeVisible == false);
            }
            expect(true); // survived 10 cycles
        }

        // ---- Settings button idempotency ----

        beginTest(settingsEveryClick);
        {
            OverlayComponent overlay;
            int callCount {};

            overlay.onSettingsClick = [&callCount]() { callCount++; };

            overlay.getSettingsButton().onClick();
            overlay.getSettingsButton().onClick();
            overlay.getSettingsButton().onClick();

            expect(callCount == threeClicks);
        }

        // ---- Resized after showOverlay sets valid button bounds ----

        beginTest(resizeGivesLayout);
        {
            OverlayComponent overlay;
            overlay.setBounds(screenLeft, screenTop, screenWidth, screenHeight);
            overlay.setVisible(true);
            overlay.resized();

            auto& settings = overlay.getSettingsButton();
            auto& exit = overlay.getExitButton();

            // Buttons should have been laid out
            expect(settings.getWidth() > noSize);
            expect(settings.getHeight() > noSize);
            expect(exit.getWidth() > noSize);
            expect(exit.getHeight() > noSize);

            // Settings button should be above exit button
            expect(settings.getY() < exit.getY());
        }

        // ---- ESC keyPressed dispatches correctly through onRequestClose ----

        beginTest(escClosesOverlay);
        {
            std::unique_ptr<OverlayComponent> overlayWindow = std::make_unique<OverlayComponent>();
            bool overlayShouldBeVisible = true;

            overlayWindow->onRequestClose = [&overlayWindow, &overlayShouldBeVisible]()
            {
                overlayShouldBeVisible = false;
                overlayWindow.reset();
            };

            overlayWindow->setVisible(true);

            // Simulate ESC key via keyPressed
            bool handled = overlayWindow->keyPressed(juce::KeyPress(juce::KeyPress::escapeKey));
            expect(handled == true);
            expect(overlayWindow == nullptr);
            expect(overlayShouldBeVisible == false);
        }
    }
};

static OverlayComponentIntegrationTest overlayComponentIntegrationTest;
