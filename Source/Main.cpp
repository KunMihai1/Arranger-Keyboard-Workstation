/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"
#include "AppInfo.h"

//==============================================================================
class ProjectSynthApplication : public juce::JUCEApplication
{
public:
    //==============================================================================
    ProjectSynthApplication() {}

    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override
    {
        // Tests live in their own headless target (ci/Tests.jucer -> Project Tests.exe), so the
        // production app carries no test code and no --run-tests path.
        juce::ignoreUnused (commandLine);
        mainWindow.reset(new MainWindow(AppInfo::appName));
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..

        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                juce::Desktop::getInstance().getDefaultLookAndFeel()
                .findColour(juce::ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

            /*
        #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
        #else
            setResizable(true, false);
            centreWithSize(getWidth(), getHeight());
        #endif

        */
            // "Restore down" returns to the bounds the window had just before it went full screen.
            // Those used to be the content's size (whole screen width x height) plus the frame, so the
            // restored window ran off the screen and under the taskbar. Create the native window hidden
            // (that makes its frame size known), size it to the screen's work area, then go full screen.
            // Narrower than the full width the header controls overlap, so it stays work-area sized.
            addToDesktop(getDesktopWindowStyleFlags());
            if (auto* peer = getPeer())
                if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
                    if (const auto frame = peer->getFrameSizeIfPresent())
                        setBounds(frame->subtractedFrom(display->userArea));

            setFullScreen(true);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(ProjectSynthApplication)
