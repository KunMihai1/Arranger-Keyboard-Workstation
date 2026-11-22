#include "displayGUI.h"

MyTabbedComponent::MyTabbedComponent(juce::TabbedButtonBar::Orientation orientation) : juce::TabbedComponent(orientation)
{
    getTabbedButtonBar().addMouseListener(this, true);

}

MyTabbedComponent::~MyTabbedComponent()
{
    getTabbedButtonBar().removeMouseListener(this);
}

void MyTabbedComponent::currentTabChanged(int newCurrentTabIndex, const juce::String& newTabName)
{
    if (onTabChanged)
        onTabChanged(newCurrentTabIndex, newTabName);
}

void MyTabbedComponent::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isRightButtonDown()) return;

    int numTabs = getNumTabs();
    if (numTabs <= 1) return;

    int tabWidth = getWidth() / numTabs;
    int clickedIndex = event.x / tabWidth;

    if (clickedIndex > 0 && clickedIndex < numTabs)
    {
        juce::PopupMenu menu;
        menu.addItem("Close Tab", [this, clickedIndex]() { removeTab(clickedIndex); });
        menu.showMenuAsync(juce::PopupMenu::Options());
    }
}

namespace
{
    constexpr int lastTabOffset = 1;   // getNumTabs() - lastTabOffset is the last tab's index
    constexpr int notFound      = -1;

    void removeTabNamed (juce::TabbedComponent& tabs, const juce::String& name)
    {
        const int index = tabs.getTabNames().indexOf (name);
        if (index != notFound)
            tabs.removeTab (index);
    }
}

void TemporaryTabs::playbackSettingsOpened() { playbackSettingsOpen = true; }
void TemporaryTabs::tracksOpened()           { tracksOpen = true; }

void TemporaryTabs::tabChanged (int newIndex, const juce::String& tabName)
{
    if (tracksOpen && tabName != StylePanelTabs::myTracks)
    {
        closeTracks();
        return;
    }

    if (playbackSettingsOpen && newIndex == StylePanelTabs::stylesIndex)
        closePlaybackSettings();
}

void TemporaryTabs::homePressed()
{
    if (tracksOpen)
        closeTracks();
    if (playbackSettingsOpen)
        closePlaybackSettings();
    tabs.setCurrentTabIndex (tabs.getNumTabs() - lastTabOffset);   // the style tab
}

// Each close clears its state BEFORE removing the tab: removing the selected tab selects another one,
// which calls tabChanged again, and that call must not see this tab as still open.
void TemporaryTabs::closeTracks()
{
    tracksOpen = false;
    removeTabNamed (tabs, StylePanelTabs::myTracks);
    if (onTracksClosed) onTracksClosed();
}

void TemporaryTabs::closePlaybackSettings()
{
    playbackSettingsOpen = false;
    removeTabNamed (tabs, StylePanelTabs::playbackSettings);
    if (onPlaybackSettingsClosed) onPlaybackSettingsClosed();
}
