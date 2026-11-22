#include <JuceHeader.h>
#include "MyTabbedComponent.h"

/*
    The style panel's temporary tabs ("Playback Settings", "My tracks") must never take the style tab with
    them. Found live: Settings tab open -> Home -> Play -> select "Styles" removed the style tab.
    Spec: docs/superpowers/specs/2026-11-22-temporary-tabs-after-home-design.md
*/
namespace TemporaryTabsTestValues
{
    const juce::String testName     = "Temporary tabs";
    const juce::String testCategory = "Unit";

    const juce::String homeThenStylesKeepsStyleAfterSettings = "Home after Playback Settings, then Styles: the style tab stays";
    const juce::String homeThenStylesKeepsStyleAfterTracks   = "Home after My tracks, then Styles: the style tab stays";
    const juce::String leavingTracksClosesIt                 = "selecting another tab closes My tracks";
    const juce::String stylesClosesPlaybackSettings          = "selecting Styles closes Playback Settings";
    const juce::String styleTabKeepsPlaybackSettings         = "selecting the style tab keeps Playback Settings open";

    const juce::String styleTab = "Style 1";
    const juce::Colour tabColour = juce::Colours::black;
    constexpr int stylesIndex      = 0;
    constexpr int styleIndex       = 1;
    constexpr int temporaryIndex   = 2;
    constexpr int twoTabs          = 2;
    constexpr int threeTabs        = 3;
}

class TemporaryTabsTest : public juce::UnitTest
{
public:
    TemporaryTabsTest() : juce::UnitTest (TemporaryTabsTestValues::testName, TemporaryTabsTestValues::testCategory) {}

    void runTest() override
    {
        using namespace TemporaryTabsTestValues;

        beginTest (homeThenStylesKeepsStyleAfterSettings);
        {
            Panel panel;
            panel.openTemporary (StylePanelTabs::playbackSettings);
            panel.temporary.homePressed();
            expectEquals (panel.tabs.getNumTabs(), twoTabs);

            panel.select (stylesIndex);   // back on the play screen, the user picks "Styles"
            expectEquals (panel.tabs.getNumTabs(), twoTabs);
            expectEquals (panel.tabs.getTabNames()[styleIndex], styleTab);
        }

        beginTest (homeThenStylesKeepsStyleAfterTracks);
        {
            Panel panel;
            panel.openTemporary (StylePanelTabs::myTracks);
            panel.temporary.homePressed();
            expectEquals (panel.tabs.getNumTabs(), twoTabs);

            panel.select (stylesIndex);
            expectEquals (panel.tabs.getNumTabs(), twoTabs);
            expectEquals (panel.tabs.getTabNames()[styleIndex], styleTab);
        }

        beginTest (leavingTracksClosesIt);
        {
            Panel panel;
            panel.openTemporary (StylePanelTabs::myTracks);
            panel.select (styleIndex);
            expectEquals (panel.tabs.getNumTabs(), twoTabs);
            expect (panel.tracksClosed);
        }

        beginTest (stylesClosesPlaybackSettings);
        {
            Panel panel;
            panel.openTemporary (StylePanelTabs::playbackSettings);
            panel.select (stylesIndex);
            expectEquals (panel.tabs.getNumTabs(), twoTabs);
            expect (panel.settingsClosed);
        }

        beginTest (styleTabKeepsPlaybackSettings);
        {
            Panel panel;
            panel.openTemporary (StylePanelTabs::playbackSettings);
            panel.select (styleIndex);
            expectEquals (panel.tabs.getNumTabs(), threeTabs);
            expect (! panel.settingsClosed);
        }
    }

private:
    /** The style panel as Display builds it: "Styles", the style tab, then any temporary tab. */
    struct Panel
    {
        Panel()
        {
            using namespace TemporaryTabsTestValues;
            tabs.addTab (StylePanelTabs::styles, tabColour, &stylesContent, false);
            tabs.addTab (styleTab, tabColour, &styleContent, false);
            tabs.onTabChanged = [this] (int newIndex, juce::String tabName) { temporary.tabChanged (newIndex, tabName); };
            temporary.onPlaybackSettingsClosed = [this] { settingsClosed = true; };
            temporary.onTracksClosed           = [this] { tracksClosed = true; };
        }

        /** Open a temporary tab the way Display does: register it, add it, select it. */
        void openTemporary (const juce::String& name)
        {
            if (name == StylePanelTabs::playbackSettings) temporary.playbackSettingsOpened();
            tabs.addTab (name, TemporaryTabsTestValues::tabColour, &temporaryContent, false);
            select (TemporaryTabsTestValues::temporaryIndex);
            if (name == StylePanelTabs::myTracks) temporary.tracksOpened();
        }

        void select (int index) { tabs.setCurrentTabIndex (index); }

        juce::Component stylesContent, styleContent, temporaryContent;
        MyTabbedComponent tabs { juce::TabbedButtonBar::TabsAtLeft };
        TemporaryTabs temporary { tabs };
        bool settingsClosed = false;
        bool tracksClosed   = false;
    };
};

static TemporaryTabsTest temporaryTabsTest;
