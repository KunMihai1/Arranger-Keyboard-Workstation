#pragma once
#include <JuceHeader.h>

/**
 * @class MyTabbedComponent
 * @brief Custom tabbed component that allows a callback when the current tab changes.
 *
 * Extends `juce::TabbedComponent` and adds a std::function callback
 * `onTabChanged` which is called whenever the current tab is changed.
 */
class MyTabbedComponent : public juce::TabbedComponent, private juce::MouseListener
{
public:

    /**
     * @brief Constructs a MyTabbedComponent with a specified tab orientation.
     * @param orientation Orientation of the tab bar (e.g., TabsAtTop, TabsAtLeft)
     */
    MyTabbedComponent(juce::TabbedButtonBar::Orientation orientation);

    ~MyTabbedComponent();

    /**
     * @brief Called by JUCE when the current tab changes.
     * @param newCurrentTabIndex Index of the newly selected tab
     * @param newTabName Name of the newly selected tab
     */
    void currentTabChanged(int newCurrentTabIndex, const juce::String& newTabName) override;

    /** @brief Callback function triggered whenever the current tab changes */
    std::function<void(int, juce::String)> onTabChanged;

    void mouseDown(const juce::MouseEvent& event) override;

    //JUCE_LEAK_DETECTOR(MyTabbedComponent)
};

/** Names of the style panel's tabs. */
namespace StylePanelTabs
{
    inline const juce::String styles           = "Styles";
    inline const juce::String playbackSettings = "Playback Settings";
    inline const juce::String myTracks         = "My tracks";
    constexpr int stylesIndex = 0;
}

/**
 * The tabs that open on demand after the style tab ("Playback Settings", "My tracks") and close again
 * when the user leaves them. Holds the rules for closing them, so Display only reacts to the result.
 * Spec: docs/superpowers/specs/2026-11-22-temporary-tabs-after-home-design.md
 */
class TemporaryTabs
{
public:
    explicit TemporaryTabs (juce::TabbedComponent& tabsToManage) : tabs (tabsToManage) {}

    std::function<void()> onPlaybackSettingsClosed;   // the tab is gone: free its component
    std::function<void()> onTracksClosed;

    void playbackSettingsOpened();   // call right after adding the tab
    void tracksOpened();

    /** The selected tab changed: "My tracks" closes when another tab is selected, "Playback Settings"
        when "Styles" is selected. */
    void tabChanged (int newIndex, const juce::String& tabName);

    /** Home pressed: close every temporary tab and select the style tab. */
    void homePressed();

private:
    void closeTracks();
    void closePlaybackSettings();

    juce::TabbedComponent& tabs;
    bool playbackSettingsOpen = false;
    bool tracksOpen = false;
};
